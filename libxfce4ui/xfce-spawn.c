/*
 * Copyright (c) 2009 Brian Tarricone <brian@terricone.org>
 * Copyright (C) 1999 Olivier Fourdan <fourdan@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_CRT_EXTERNS_H
#include <crt_externs.h> /* for _NSGetEnviron */
#endif

#include <gdk/gdk.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#else
/* no x11, no sn */
#undef HAVE_LIBSTARTUP_NOTIFICATION
#endif

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
#include <libsn/sn.h>
#endif

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#include <libnotify/notify-enum-types.h>
#endif

#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/xfce-dialogs.h>
#include <libxfce4ui/xfce-spawn.h>
#include <libxfce4ui/xfce-workspace.h>
#include <libxfce4ui/xfce-gdk-extensions.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>

#ifdef HAVE__NSGETENVIRON
/* for support under apple/darwin */
#define environ (*_NSGetEnviron())
#elif !HAVE_DECL_ENVIRON
/* try extern if environ is not defined in unistd.h */
extern gchar **environ;
#endif

/* the maximum time (seconds) for an application to startup */
#define XFCE_SPAWN_STARTUP_TIMEOUT (30)



typedef struct
{
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  /* startup notification data */
  SnLauncherContext *sn_launcher;
  guint              timeout_id;
#endif

  /* child watch data */
  guint              watch_id;
  GPid               pid;
  GClosure          *closure;
} XfceSpawnData;

typedef struct {
  GdkScreen           *screen;
  gchar               *working_directory;
  gchar              **argv;
  gchar              **envp;
  GSpawnFlags         flags;
  gboolean            startup_notify;
  guint32             startup_timestamp;
  gchar              *startup_icon_name;
  GClosure           *child_watch_closure;
  gchar              *monitor_path;
  gchar              *ws_name;
#ifdef HAVE_LIBNOTIFY
  NotifyNotification *notification;
#endif
  gboolean            launched;
  gboolean            succeed;
  GMainLoop          *loop;
  pthread_mutex_t     mutex;
} XfceSpawnWatchData;

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
static gboolean
xfce_spawn_startup_timeout (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GTimeVal       now;
  gdouble        elapsed;
  glong          tv_sec;
  glong          tv_usec;

  g_return_val_if_fail (spawn_data->sn_launcher != NULL, FALSE);

  /* determine the amount of elapsed time */
  g_get_current_time (&now);
  sn_launcher_context_get_last_active_time (spawn_data->sn_launcher, &tv_sec, &tv_usec);
  elapsed = now.tv_sec - tv_sec + ((gdouble) (now.tv_usec - tv_usec) / G_USEC_PER_SEC);

  return elapsed < XFCE_SPAWN_STARTUP_TIMEOUT;
}



static void
xfce_spawn_startup_timeout_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GPid           pid;

  spawn_data->timeout_id = 0;

  if (G_LIKELY (spawn_data->sn_launcher != NULL))
   {
     /* abort the startup notification */
     sn_launcher_context_complete (spawn_data->sn_launcher);
     sn_launcher_context_unref (spawn_data->sn_launcher);
     spawn_data->sn_launcher = NULL;
   }

  /* if there is no closure to watch the child, also stop
   * the child watch */
  if (G_LIKELY (spawn_data->closure == NULL
                && spawn_data->watch_id != 0))
    {
      pid = spawn_data->pid;
      g_source_remove (spawn_data->watch_id);
      g_child_watch_add (pid, (GChildWatchFunc) g_spawn_close_pid, NULL);
    }
}
#endif



static void
xfce_spawn_startup_watch (GPid     pid,
                          gint     status,
                          gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GValue         instance_and_params[2] = { { 0, }, { 0, } };

  g_return_if_fail (spawn_data->pid == pid);

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      /* xfce spawn has no instance */
      g_value_init (&instance_and_params[0], G_TYPE_POINTER);
      g_value_set_pointer (&instance_and_params[0], NULL);

      g_value_init (&instance_and_params[1], G_TYPE_INT);
      g_value_set_int (&instance_and_params[1], status);

      g_closure_set_marshal (spawn_data->closure, g_cclosure_marshal_VOID__INT);

      g_closure_invoke (spawn_data->closure, NULL,
                        2, instance_and_params, NULL);
    }

  /* don't leave zombies */
  g_spawn_close_pid (pid);
}



static void
xfce_spawn_startup_watch_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;

  spawn_data->watch_id = 0;

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  if (spawn_data->timeout_id != 0)
    g_source_remove (spawn_data->timeout_id);
#endif

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      g_closure_invalidate (spawn_data->closure);
      g_closure_unref (spawn_data->closure);
    }

  g_slice_free (XfceSpawnData, spawn_data);
}





static void
xfce_spawn_on_secure_workspace_ready (GFileMonitor     *monitor,
                                      GFile            *file,
                                      GFile            *other_file,
                                      GFileMonitorEvent event_type,
                                      gpointer          user_data)
{
  XfceSpawnWatchData *data    = (XfceSpawnWatchData *) user_data;
  gboolean            succeed = FALSE;
  GError             *error   = NULL;

  g_return_if_fail (data != NULL);
  TRACE ("entering xfce_spawn_on_secure_workspace_ready");

  if (event_type == G_FILE_MONITOR_EVENT_CREATED || event_type == G_FILE_MONITOR_EVENT_CHANGED)
    {
      g_signal_handlers_disconnect_by_func (monitor, xfce_spawn_on_secure_workspace_ready, data);
      pthread_mutex_lock (&data->mutex);
      if (data->launched == FALSE)
        {
          TRACE ("Workspace %s seems ready, attempting to spawn original target %s", data->ws_name, data->argv[0]);
          succeed = xfce_spawn_on_screen_with_child_watch (data->screen,
                                                           data->working_directory,
                                                           data->argv,
                                                           data->envp,
                                                           data->flags,
                                                           data->startup_notify,
                                                           data->startup_timestamp,
                                                           data->startup_icon_name,
                                                           data->child_watch_closure,
                                                           &error);

          if (!succeed)
            {
              xfce_dialog_show_error (NULL, error, _("Failed to spawn %s in secure workspace %s"), data->argv[0], data->ws_name);
              g_error_free (error);
            }
          else
            {
    #ifdef HAVE_LIBNOTIFY
              if (data->notification)
                {
                  GError *notify_error = NULL;
                  gchar *text = g_strdup_printf (_("Workspace Ready. Launching %s"), data->argv[0]);
                  notify_notification_update (data->notification, text, NULL, "firejail-workspaces-ready");
                  g_free (text);

                  if (!notify_notification_show (data->notification, &notify_error))
                    {
	                    g_warning ("%s: failed to notify that secure workspace %s is initialized: %s\n", G_LOG_DOMAIN, data->ws_name, notify_error->message);
	                    g_error_free (notify_error);
                    }
                }
    #endif
            }

          data->launched = TRUE;
          data->succeed = succeed;
        }
      pthread_mutex_unlock (&data->mutex);
    }
}



static gboolean
xfce_spawn_shutdown_polling_loop (gpointer user_data)
{
  XfceSpawnWatchData *data = (XfceSpawnWatchData *) user_data;

  g_return_val_if_fail (data != NULL, FALSE);

  if (g_main_loop_is_running (data->loop))
    g_main_loop_quit (data->loop);

  return FALSE;
}



static gboolean
xfce_spawn_secure_workspace_daemon (GdkScreen    *screen,
                                    const gchar  *working_directory,
                                    gchar       **spawn_argv,
                                    gchar       **spawn_envp,
                                    GSpawnFlags   spawn_flags,
                                    gboolean      startup_notify,
                                    guint32       startup_timestamp,
                                    const gchar  *startup_icon_name,
                                    GClosure     *child_watch_closure,
                                    guint         workspace_number,
                                    const gchar  *ws_name,
                                    gchar       **envp,
                                    GError      **error)
{
  gboolean            succeed = FALSE;
  gchar             **argv;
  GPid                pid;
  gchar             **cenvp;
  guint               n;
  guint               n_cenvp;
  gchar              *new_path;
  gsize               index;
  gboolean            overlaying = FALSE;
  GFileMonitor       *monitor;
  gchar              *monitor_path;
  XfceSpawnWatchData *data;
  GFile              *file;

#ifdef HAVE_LIBNOTIFY
#if NOTIFY_CHECK_VERSION (0, 7, 0)
  NotifyNotification *notification = notify_notification_new ("Xfce4 UI Utilities", NULL, NULL);
#else
  NotifyNotification *notification = notify_notification_new ("Xfce4 UI Utilities", NULL, NULL, NULL);
#endif
  gchar              *body;
  GError             *notify_error = NULL;
#endif

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  TRACE ("Spawning Xfce's secure workspace daemon in workspace %d, which will keep the Firejail domain alive", workspace_number);

#ifdef HAVE_LIBNOTIFY
  if (!notify_is_initted ())
      notify_init("Xfce4 UI Utilities");

  body = g_strdup_printf (_("Initializing Secure Workspace '%s'"), ws_name);
  notify_notification_update (notification, body, NULL, "firejail-workspaces-initializing");
  g_free (body);

  if (!notify_notification_show (notification, &notify_error))
    {
	    g_warning ("%s: failed to notify that secure workspace %s is being initialized: %s\n", G_LOG_DOMAIN, ws_name, notify_error->message);
	    g_error_free (notify_error);
	    g_object_unref (notification);
	    notification = NULL;
    }
#endif

  /* clean up path in the environment so we're confident the sandbox properly set up */
  if (G_LIKELY (envp == NULL))
    envp = (gchar **) environ;
  for (n = 0; envp[n] != NULL; ++n);
  cenvp = g_new0 (gchar *, n + 2);
  for (n_cenvp = n = 0; envp[n] != NULL; ++n)
    {
      if (strncmp (envp[n], "DESKTOP_STARTUP_ID", 18) != 0
          && strncmp (envp[n], "PATH", 4) != 0)
        cenvp[n_cenvp++] = envp[n];
    }

  /* add a safe path */
  cenvp[n_cenvp++] = new_path = g_strdup ("PATH=/usr/local/sbin:/usr/local/bin:/usr/bin:/usr/sbin:/sbin");

  /* new argv, starting with Firejail's locked workspace mode */
  argv = g_malloc(sizeof (gchar *) * (100));
  index = 0;

  argv[index++] = g_strdup ("firejail");
  argv[index++] = g_strdup ("--lock-workspace");
  argv[index++] = g_strdup_printf ("--name=%s", ws_name);

  /* network options */
  if (!xfce_workspace_enable_network (workspace_number))
      argv[index++] = g_strdup ("--net=none");
  else
    {
      if (xfce_workspace_fine_tuned_network (workspace_number))
          argv[index++] = g_strdup ("--net=auto");

      if (!xfce_workspace_isolate_dbus (workspace_number))
          argv[index++] = g_strdup ("--dbus=full");
    }

  /* overlay options */
  if (xfce_workspace_enable_overlay (workspace_number))
    {
      overlaying = TRUE;
      if (xfce_workspace_enable_private_home (workspace_number))
        argv[index++] = g_strdup ("--overlay-private-home");
      else
        argv[index++] = g_strdup ("--overlay");
    }

  /* audio options */
  if (xfce_workspace_disable_sound (workspace_number))
    {
      argv[index++] = g_strdup ("--nosound");
    }

  /* finally, adding the name of the daemon */
  argv[index++] = g_strdup ("xfce4-secure-workspaced");

  /* and the path to the file where it must report on its status */
  if (overlaying)
    {
      monitor_path = g_strdup_printf ("%s/Sandboxes/%s/Users/%s/.cache/xfce4/secure-workspace.notify",
                                      g_get_home_dir (),
                                      ws_name,
                                      g_get_user_name ());
      argv[index++] = g_strdup_printf ("/home/%s/.cache/xfce4/secure-workspace.notify",
                                       g_get_user_name ());
    }
  else
    {
      monitor_path = g_strdup_printf ("%s/xfce4/secure-workspace-%d.notify",
                                      g_get_user_cache_dir (),
                                      workspace_number);
      argv[index++] = monitor_path;
    }
  
  /* assuming that this would fail only because the file doesn't exist; if it
   * fails for other reasons, we can't recover anyway */
  g_unlink (monitor_path);
  argv[index] = NULL;

  /* start monitoring changes to the file */
  file = g_file_new_for_path (monitor_path);
  if (!file)
    {
      g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                   _("Failed to setup a monitor for the file path where we await a notification from the secure workspace daemon"));
      return FALSE;
    }

  monitor = g_file_monitor_file (file, G_FILE_MONITOR_NONE, NULL, error);
  if (error && *error)
    {
      return FALSE;
    }

  data = g_slice_new0 (XfceSpawnWatchData);

  data->screen                = g_object_ref (screen);
  data->working_directory     = g_strdup (working_directory);
  data->argv                  = g_strdupv (spawn_argv);
  data->envp                  = g_strdupv (spawn_envp);
  data->flags                 = spawn_flags;
  data->startup_notify        = startup_notify;
  data->startup_timestamp     = startup_timestamp;
  data->startup_icon_name     = g_strdup (startup_icon_name);
  data->child_watch_closure   = child_watch_closure;
  data->monitor_path          = monitor_path;
  data->ws_name               = g_strdup (ws_name);
#ifdef HAVE_LIBNOTIFY
  data->notification          = notification;
#endif

  /* make sure a main loop exists to handle the file monitor's signals */
  data->loop                  = g_main_loop_new (NULL, FALSE);
  pthread_mutex_init (&data->mutex, NULL);

  g_signal_connect (G_OBJECT (monitor), "changed", G_CALLBACK (xfce_spawn_on_secure_workspace_ready), data);

  /* try to spawn the new process */
  succeed = g_spawn_async (NULL, argv, cenvp, G_SPAWN_SEARCH_PATH_FROM_ENVP, NULL,
                           NULL, &pid, error);

  if (G_LIKELY (succeed))
    {
      /* run the loop for up to five seconds */
      g_timeout_add (5000, xfce_spawn_shutdown_polling_loop, data);
      g_main_loop_run (data->loop);

      /* check if the intended client was launched within the given five seconds */
      succeed = (data->launched && data->succeed);
      if (!succeed)
        g_set_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                     _("Failed to receive notification from the secure workspace daemon that the sandbox is initialized"));
    }

  /* clean up local memory */    
  g_strfreev (argv);
  g_free (cenvp);
  g_free (new_path);

  /* clean-up now the loop is gone */
  g_object_unref (monitor);

  if (g_main_loop_is_running (data->loop))
    g_main_loop_unref (data->loop);

  /* clean up the data object */
  g_unlink (data->monitor_path);

  g_free (data->ws_name);
  g_free (data->working_directory);
  g_free (data->startup_icon_name);
  g_free (data->monitor_path);
  g_strfreev (data->argv);
  g_strfreev (data->envp);
  g_object_unref (data->screen);
#ifdef HAVE_LIBNOTIFY
  if (data->notification)
    g_object_unref (data->notification);
#endif
  g_slice_free (XfceSpawnWatchData, data);

  return succeed;
}



static gboolean
xfce_client_is_xfce (gchar *client)
{
    gchar *name = client;

    if (!name)
        return FALSE;

    if (name[0] == '/')
        name = strrchr (name, '/');

    return g_strcmp0 (name, "xfce4-panel") == 0               || g_strcmp0 (name, "xfdesktop") == 0 ||
           g_strcmp0 (name, "notify-osd") == 0                || g_strcmp0 (name, "xfce4-notifyd") == 0 ||
           g_strcmp0 (name, "xfce4-appfinder") == 0           || g_strcmp0 (name, "xfdesktop-settings") == 0 ||
           g_strcmp0 (name, "xfce4-mouse-settings") == 0      || g_strcmp0 (name, "xfce4-power-manager-settings") == 0 ||
           g_strcmp0 (name, "xfce4-session-settings") == 0    || g_strcmp0 (name, "xfce4-accessibility-settings") == 0 ||
           g_strcmp0 (name, "xfce4-display-settings") == 0    || g_strcmp0 (name, "xfce4-appearance-settings") == 0 ||
           g_strcmp0 (name, "xfce4-keyboard-settings") == 0   || g_strcmp0 (name, "xfce4-notes-settings") == 0 ||
           g_strcmp0 (name, "xfce4-screenshooter") == 0       || g_strcmp0 (name, "xfce4-taskmanager") == 0 ||
           g_strcmp0 (name, "xfce4-notifyd-config") == 0      || g_strcmp0 (name, "xfce4-settings-manager") == 0 ||
           g_strcmp0 (name, "xfce4-mime-settings") == 0       || g_strcmp0 (name, "xfce4-session-logout") == 0 ||
           g_strcmp0 (name, "xfce4-clipman-settings") == 0    || g_strcmp0 (name, "Mugshot") == 0 ||
           g_strcmp0 (name, "xfce4-clipman") == 0             || g_strcmp0 (name, "menulibre") == 0 ||
           g_strcmp0 (name, "xfce4-mixer") == 0               || g_strcmp0 (name, "xfce4-about") == 0 ||
           g_strcmp0 (name, "pavucontrol") == 0               || g_strcmp0 (name, "xfce4-popup-applicationsmenu") == 0 ||
           g_strcmp0 (name, "xfce4-pm-helper") == 0           || g_strcmp0 (name, "xfce4-popup-clipman") == 0 ||
           g_strcmp0 (name, "xfce4-popup-directorymenu") == 0 || g_strcmp0 (name, "xfce4-popup-notes") == 0 ||
           g_strcmp0 (name, "xfce4-popup-notes") == 0         || g_strcmp0 (name, "xfce4-notes") == 0 ||
           g_strcmp0 (name, "xfce4-popup-whiskermenu") == 0   || g_strcmp0 (name, "xfce4-popup-windowmenu") == 0 ||
           g_strcmp0 (name, "xfce4-power-manager") == 0       || g_strcmp0 (name, "xfce4-sensors") == 0 ||
           g_strcmp0 (name, "xfce4-session") == 0             || g_strcmp0 (name, "xfce4-settings-editor") == 0 ||
           g_strcmp0 (name, "xfce4-volumed") == 0             || g_strcmp0 (name, "xfce4-volumed-pulse") == 0 ||
           g_strcmp0 (name, "xfwm4-workspace-settings") == 0  || g_strcmp0 (name, "xfwm4-tweak-settings") == 0 ||
           g_strcmp0 (name, "xfwm4-settings") == 0;
}



/**
 * xfce_spawn_on_screen_with_closure:
 * @screen              : a #GdkScreen or %NULL to use the active screen,
 *                        see xfce_gdk_screen_get_active().
 * @working_directory   : child's current working directory or %NULL to
 *                        inherit parent's.
 * @argv                : child's argument vector.
 * @envp                : child's environment vector or %NULL to inherit
 *                        parent's.
 * @flags               : flags from #GSpawnFlags. #G_SPAWN_DO_NOT_REAP_CHILD
 *                        is not allowed, you should use the
 *                        @child_watch_closure for this.
 * @startup_notify      : whether to use startup notification.
 * @startup_timestamp   : the timestamp to pass to startup notification, use
 *                        the event time here if possible to make focus
 *                        stealing prevention work property. If you don't
 *                        have direct access to the event time you could use
 *                        gtk_get_current_event_time() or if nothing is
 *                        available 0 is valid too.
 * @startup_icon_name   : application icon or %NULL.
 * @child_watch_closure : closure that is triggered when the child exists
 *                        or %NULL.
 * @error               : return location for errors or %NULL.
 *
 * Like xfce_spawn_on_screen(), but allows to attach a closure to watch the
 * child's exit status. This because only one g_child_watch_add() is allowed on
 * Unix (per PID) and this is already internally needed for a proper
 * startup notification implementation.
 *
 * <example>
 * <title>Spawning with a child watch</title>
 * <programlisting>
 * static void
 * child_watch_callback (GObject *object,
 *                       gint     status)
 * {
 *   g_message ("Child exit status is %d", status);
 * }
 *
 * static void
 * spawn_something (void)
 * {
 *   GClosure *child_watch;
 *
 *   child_watch = g_cclosure_new_swap (G_CALLBACK (child_watch_callback),
 *                                      object, NULL);
 *   xfce_spawn_on_screen_with_child_watch (...,
 *                                          child_watch,
 *                                          ...);
 * }
 * </programlisting>
 * </example>
 *
 * Return value: %TRUE on success, %FALSE if @error is set.
 **/
gboolean
xfce_spawn_on_screen_with_child_watch (GdkScreen    *screen,
                                       const gchar  *working_directory,
                                       gchar       **argv,
                                       gchar       **envp,
                                       GSpawnFlags   flags,
                                       gboolean      startup_notify,
                                       guint32       startup_timestamp,
                                       const gchar  *startup_icon_name,
                                       GClosure     *child_watch_closure,
                                       GError      **error)
{
  gboolean            succeed;
  gboolean            reallocated_argv = FALSE;
  gchar             **cenvp;
  guint               n;
  guint               n_cenvp;
  gchar              *display_name;
  gchar              *new_display = NULL;
  GPid                pid;
  XfceSpawnData      *spawn_data;
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  SnLauncherContext  *sn_launcher = NULL;
  SnDisplay          *sn_display = NULL;
  gint                sn_workspace;
  const gchar        *startup_id;
  const gchar        *prgname;
  gchar              *new_startup_id = NULL;
#endif
  gsize               argc;
  gchar             **new_argv;
  gsize               index;
  gchar              *ws_name;
  const gchar        *app_name = argv? argv[0]:NULL;
  gboolean            is_firejail = FALSE;
  gboolean            sandboxing_in_ws  = FALSE;
  gboolean            display_launch_notification  = FALSE;

  g_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail ((flags & G_SPAWN_DO_NOT_REAP_CHILD) == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  printf ("DBG: pid %d sandbox %s argv:", getpid(), getenv("FIREJAIL_SANDBOX_NAME"));
  {
    gint i=0;
    for (i=0; argv && argv[i]; i++)
      printf ("\t\targv[%d] -> %s\n", i, argv[i]);
    printf ("\n");
  }

  if (argv[0] && (strcmp (argv[0], "firejail") == 0 ||
    strncmp (argv[0], "firejail ", 9) == 0 ||
    strcmp (argv[0], "/usr/bin/firejail") == 0 ||
    strncmp (argv[0], "/usr/bin/firejail ", 18) == 0 ||
    strcmp (argv[0], "/usr/local/bin/firejail") == 0 ||
    strncmp (argv[0], "/usr/local/bin/firejail ", 24) == 0))
    {
      is_firejail = TRUE;
      display_launch_notification = TRUE;
      for (index = 0; argv && argv[index]; ++index)
        {
          if (g_str_has_prefix (argv[index], "--name="))
            {
              app_name = argv[index] + strlen ("--name=");
              break;
            }
        }
    }

  /* lookup the screen with the pointer */
  if (screen == NULL)
    screen = xfce_gdk_screen_get_active (NULL);
  sn_workspace = xfce_workspace_get_active_workspace_number (screen);

  for (argc = 0; argv[argc]; argc++);

  /* setup the child environment (stripping $DESKTOP_STARTUP_ID and $DISPLAY) */
  if (G_LIKELY (envp == NULL))
    envp = (gchar **) environ;
  for (n = 0; envp[n] != NULL; ++n);
  cenvp = g_new0 (gchar *, n + 3);
  for (n_cenvp = n = 0; envp[n] != NULL; ++n)
    {
      if (strncmp (envp[n], "DESKTOP_STARTUP_ID", 18) != 0
          && strncmp (envp[n], "DISPLAY", 7) != 0)
        cenvp[n_cenvp++] = envp[n];
    }

  /* add the real display name for the screen */
  display_name = gdk_screen_make_display_name (screen);
  cenvp[n_cenvp++] = new_display = g_strconcat ("DISPLAY=", display_name, NULL);
  g_free (display_name);

  /* secure workspace support */
  sandboxing_in_ws = !is_firejail && xfce_workspace_is_secure (sn_workspace) && !xfce_client_is_xfce (argv[0]);
  if (sandboxing_in_ws)
    {
      ws_name = xfce_workspace_get_workspace_name (sn_workspace);

      TRACE ("Spawning %s in secure workspace %s (#%d), wrapping with Firejail", argv[0], ws_name, sn_workspace);

      /* if there are no clients yet, ensure the secure workspace daemon runs, to make the sandbox persistent */
      if (!xfce_workspace_has_locked_clients (sn_workspace))
        {
          succeed = xfce_spawn_secure_workspace_daemon (screen, working_directory, argv, envp, flags,
                                                        startup_notify, startup_timestamp, startup_icon_name,
                                                        child_watch_closure, sn_workspace, ws_name, cenvp, error);
          if (succeed)
            {
              TRACE ("Successfully spawned the secure workspace daemon for %s", ws_name);
            }
          else
            {
              xfce_dialog_show_error (NULL, *error, _("Failed to spawn the secure workspace daemon for %s"), ws_name);
              TRACE ("Failed to spawn the secure workspace daemon for %s", ws_name);
            }

          return succeed;
        }
      else
        {
          TRACE ("Joining the existing Firejail domain '%s'", ws_name);

          /* new argv, starting with Firejail's locked workspace mode */
          new_argv = g_malloc(sizeof (gchar *) * (argc + 100));
          index = 0;

          new_argv[index++] = g_strdup ("firejail");
          new_argv[index++] = g_strdup ("--lock-workspace");
          new_argv[index++] = g_strdup_printf ("--join=%s", ws_name);

          /* now, inject the argv parameters and set argv to point to our own pointer */
          for (n = 0; argv[n]; n++)
            new_argv[index++] = g_strdup (argv[n]);
          new_argv[index] = NULL;

          argv = new_argv;
          reallocated_argv = TRUE;
          display_launch_notification = TRUE;
        }
      
      g_free (ws_name);
    }


#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  /* initialize the sn launcher context */
  if (G_LIKELY (startup_notify))
    {
      sn_display = sn_display_new (GDK_SCREEN_XDISPLAY (screen),
                                   (SnDisplayErrorTrapPush) gdk_error_trap_push,
                                   (SnDisplayErrorTrapPop) gdk_error_trap_pop);

      if (G_LIKELY (sn_display != NULL))
        {
          sn_launcher = sn_launcher_context_new (sn_display, GDK_SCREEN_XNUMBER (screen));
          if (G_LIKELY (sn_launcher != NULL))
            {
              /* initiate the sn launcher context */
              sn_launcher_context_set_workspace (sn_launcher, sn_workspace);
              sn_launcher_context_set_binary_name (sn_launcher, argv[0]);
              sn_launcher_context_set_icon_name (sn_launcher, startup_icon_name != NULL ?
                                                 startup_icon_name : "applications-other");

              if (G_LIKELY (!sn_launcher_context_get_initiated (sn_launcher)))
                {
                  prgname = g_get_prgname ();
                  sn_launcher_context_initiate (sn_launcher, prgname != NULL ?
                                                prgname : "unknown", argv[0],
                                                startup_timestamp);
                }

              /* add the real startup id to the child environment */
              startup_id = sn_launcher_context_get_startup_id (sn_launcher);
              if (G_LIKELY (startup_id != NULL))
                cenvp[n_cenvp++] = new_startup_id = g_strconcat ("DESKTOP_STARTUP_ID=", startup_id, NULL);
            }
        }
    }
#endif

  /* watch the child process */
  flags |= G_SPAWN_DO_NOT_REAP_CHILD;

  /* test if the working directory exists */
  if (working_directory == NULL || *working_directory == '\0')
    {
      /* not worth a warning */
      working_directory = NULL;
    }
  else if (!g_file_test (working_directory, G_FILE_TEST_IS_DIR))
    {
      /* print warning for user */
      g_printerr (_("Working directory \"%s\" does not exist. It won't be used "
                    "when spawning \"%s\"."), working_directory, *argv);
      working_directory = NULL;
    }

  /* try to spawn the new process */
  succeed = g_spawn_async (working_directory, argv, cenvp, flags, NULL,
                           NULL, &pid, error);

  if (display_launch_notification)
    {
#ifdef HAVE_LIBNOTIFY
      NotifyNotification *notification;
      gchar              *body;
      GError             *notify_error = NULL;

      if (!notify_is_initted ())
          notify_init("Xfce4 UI Utilities");

      #if NOTIFY_CHECK_VERSION (0, 7, 0)
      notification = notify_notification_new ("Xfce4 UI Utilities", NULL, NULL);
      #else
      notification = notify_notification_new ("Xfce4 UI Utilities", NULL, NULL, NULL);
      #endif

      if (sandboxing_in_ws)
        {
          ws_name = xfce_workspace_get_workspace_name (sn_workspace);
          body = g_strdup_printf (_("Launching '%s' in '%s'"), app_name, ws_name);
          g_free (ws_name);
          notify_notification_update (notification, body, _("It may take a few minutes to appear in the secure workspace..."), "firejail-link");
          g_free (body);
        }
      else if (is_firejail)
        {
          body = g_strdup_printf (_("Launching Secure App '%s'"), app_name);
          notify_notification_update (notification, body, _("It may take a few minutes to be ready..."), "firejail-protect");
          g_free (body);
        }

      if (!notify_notification_show (notification, &notify_error))
        {
          g_warning ("%s: failed to notify of sandboxed application %s being launched: %s\n", G_LOG_DOMAIN, app_name, notify_error->message);
          g_error_free (notify_error);
        }
      g_object_unref (notification);
#endif
    }

  if (reallocated_argv)
    g_strfreev(argv);
  g_free (cenvp);
  g_free (new_display);
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  g_free (new_startup_id);
#endif

  if (G_LIKELY (succeed))
    {
      /* setup data to watch the child */
      spawn_data = g_slice_new0 (XfceSpawnData);
      spawn_data->pid = pid;
      if (child_watch_closure != NULL)
        {
          spawn_data->closure = g_closure_ref (child_watch_closure);
          g_closure_sink (spawn_data->closure);
        }

      spawn_data->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                                     xfce_spawn_startup_watch,
                                                     spawn_data,
                                                     xfce_spawn_startup_watch_destroy);

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
      if (G_LIKELY (sn_launcher != NULL))
        {
          /* start a timeout to stop the startup notification sequence after
           * a certain about of time, to handle applications that do not
           * properly implement startup notify */
          spawn_data->sn_launcher = sn_launcher;
          spawn_data->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_LOW,
                                                               XFCE_SPAWN_STARTUP_TIMEOUT,
                                                               xfce_spawn_startup_timeout,
                                                               spawn_data,
                                                               xfce_spawn_startup_timeout_destroy);
        }
#endif
    }
  else
    {
#ifdef HAVE_LIBSTARTUP_NOTIFICATION
      if (G_LIKELY (sn_launcher != NULL))
        {
          /* abort the startup notification sequence */
          sn_launcher_context_complete (sn_launcher);
          sn_launcher_context_unref (sn_launcher);
        }
#endif
    }

#ifdef HAVE_LIBSTARTUP_NOTIFICATION
  /* release the sn display */
  if (G_LIKELY (sn_display != NULL))
    sn_display_unref (sn_display);
#endif

  return succeed;
}



/**
 * xfce_spawn_on_screen:
 * @screen            : a #GdkScreen or %NULL to use the active screen,
 *                      see xfce_gdk_screen_get_active().
 * @working_directory : child's current working directory or %NULL to
 *                      inherit parent's.
 * @argv              : child's argument vector.
 * @envp              : child's environment vector or %NULL to inherit
 *                      parent's.
 * @flags             : flags from #GSpawnFlags. #G_SPAWN_DO_NOT_REAP_CHILD
 *                      is not allowed, use xfce_spawn_on_screen_with_child_watch()
 *                      if you want a child watch.
 * @startup_notify    : whether to use startup notification.
 * @startup_timestamp : the timestamp to pass to startup notification, use
 *                      the event time here if possible to make focus
 *                      stealing prevention work property. If you don't
 *                      have direct access to the event time you could use
 *                      gtk_get_current_event_time() or if nothing is
 *                      available 0 is valid too.
 * @startup_icon_name : application icon or %NULL.
 * @error             : return location for errors or %NULL.
 *
 * Like gdk_spawn_on_screen(), but also supports startup notification
 * (if Libxfce4ui was built with startup notification support).
 *
 * Return value: %TRUE on success, %FALSE if @error is set.
 **/
gboolean
xfce_spawn_on_screen (GdkScreen    *screen,
                      const gchar  *working_directory,
                      gchar       **argv,
                      gchar       **envp,
                      GSpawnFlags   flags,
                      gboolean      startup_notify,
                      guint32       startup_timestamp,
                      const gchar  *startup_icon_name,
                      GError      **error)
{
  return xfce_spawn_on_screen_with_child_watch (screen, working_directory, argv,
                                                envp, flags, startup_notify,
                                                startup_timestamp, startup_icon_name,
                                                NULL, error);
}



/**
 * xfce_spawn_command_line_on_screen:
 * @screen            : a #GdkScreen or %NULL to use the active screen, see xfce_gdk_screen_get_active().
 * @command_line      : command line to run.
 * @in_terminal       : whether to run @command_line in a terminal.
 * @startup_notify    : whether to use startup notification.
 * @error             : location for a #GError or %NULL.
 *
 * Executes the given @command_line and returns %TRUE if the
 * command terminated successfully. Else, the @error is set
 * to the standard error output.
 *
 * Returns: %TRUE if the @command_line was executed
 *          successfully, %FALSE if @error is set.
 */
gboolean
xfce_spawn_command_line_on_screen (GdkScreen    *screen,
                                   const gchar  *command_line,
                                   gboolean      in_terminal,
                                   gboolean      startup_notify,
                                   GError      **error)
{
  gchar    **argv;
  gboolean   succeed;

  g_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (command_line != NULL, FALSE);

  if (in_terminal == FALSE)
    {
      /* parse the command, retrun false with error when this fails */
      if (G_UNLIKELY (!g_shell_parse_argv (command_line, NULL, &argv, error)))
        return FALSE;
    }
  else
    {
      /* create an argv to run the command in a terminal */
      argv = g_new0 (gchar *, 5);
      argv[0] = g_strdup ("exo-open");
      argv[1] = g_strdup ("--launch");
      argv[2] = g_strdup ("TerminalEmulator");
      argv[3] = g_strdup (command_line);
      argv[4] = NULL;

      /* FIXME: startup notification does not work when
       * launching with exo-open */
      startup_notify = FALSE;
    }

  succeed = xfce_spawn_on_screen_with_child_watch (screen, NULL, argv, NULL,
                                                   G_SPAWN_SEARCH_PATH, startup_notify,
                                                   gtk_get_current_event_time (),
                                                   NULL, NULL, error);

  g_strfreev (argv);

  return succeed;
}



#define __XFCE_SPAWN_C__
#include <libxfce4ui/libxfce4ui-aliasdef.c>
