/*
 * Copyright (c) 2016 Steve Dodier-Lazaro <sidi@xfce.org>
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

#include <glib/gstdio.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/xfce-workspace.h>
#include <libxfce4ui/xfce-gdk-extensions.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>
#include <xfconf/xfconf.h>
#include <firejail/common.h>

#ifdef HAVE__NSGETENVIRON
/* for support under apple/darwin */
#define environ (*_NSGetEnviron())
#elif !HAVE_DECL_ENVIRON
/* try extern if environ is not defined in unistd.h */
extern gchar **environ;
#endif

//TODO atexit xfconf_shutdown

static XfconfChannel *
xfce_workspace_xfconf_init (void)
{
  static gboolean ini = FALSE;
  static XfconfChannel *channel = NULL;

  if (!ini)
    {
      GError *error = NULL;
      xfconf_init (&error);

      // we'll try again, just live with it for now
      if (error)
        {
          g_warning ("Warning: libxfce4ui failed to initialise xfconf: %s\n", error->message);
          g_error_free (error);
        }
      else
        ini = TRUE;
    }

  if (!ini)
    return NULL;

  if (!channel)
    channel = xfconf_channel_get ("xfwm4");

  return channel;
}

gboolean
xfce_workspace_has_locked_clients (gint ws)
{
  gboolean has_clients = FALSE;
  gchar *ws_name = xfce_workspace_get_workspace_name (ws);
  pid_t result;

  if (!ws_name)
    return FALSE;

  has_clients = name2pid (ws_name, &result) == 0;
  g_free (ws_name);

  return has_clients;
}



gboolean
xfce_workspace_let_unsandboxed_in (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       lockout;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/unsandboxed_lock_out", ws);
  lockout = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);

  return !lockout;
}




gboolean
xfce_workspace_let_sandboxed_out (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/sandbox_escape", ws);
  let = xfconf_channel_get_bool (channel, prop, FALSE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_enable_network (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/enable_network", ws);
  let = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_fine_tuned_network (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/net_auto", ws);
  let = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_isolate_dbus (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/isolate_dbus", ws);
  let = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_enable_overlay (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/overlay_fs", ws);
  let = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_enable_private_home (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/overlay_fs_private_home", ws);
  let = xfconf_channel_get_bool (channel, prop, FALSE);
  g_free (prop);

  return let;
}



gboolean
xfce_workspace_disable_sound (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       let;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/disable_sound", ws);
  let = xfconf_channel_get_bool (channel, prop, FALSE);
  g_free (prop);

  return let;
}



XfceWorkspaceInBehavior
xfce_workspace_unsandboxed_in_behavior (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gboolean       replace, lockout, unsandboxed;

  xfce_workspace_xfconf_init();
  if (!xfce_workspace_let_unsandboxed_in (ws))
    return XFCE_WORKSPACE_DONT_ENTER;

  channel = xfconf_channel_get ("xfwm4");

  prop = g_strdup_printf ("/security/workspace_%d/unsandboxed_lock_out", ws);
  lockout = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);
  if (lockout)
    return XFCE_WORKSPACE_DONT_ENTER;

  prop = g_strdup_printf ("/security/workspace_%d/unsandboxed_enter", ws);
  unsandboxed = xfconf_channel_get_bool (channel, prop, TRUE);
  g_free (prop);
  if (unsandboxed)
    return XFCE_WORKSPACE_ENTER_UNSANDBOXED;

  prop = g_strdup_printf ("/security/workspace_%d/unsandboxed_replace", ws);
  replace = xfconf_channel_get_bool (channel, prop, FALSE);
  g_free (prop);
  if (replace)
    return XFCE_WORKSPACE_ENTER_REPLACE;

  return XFCE_WORKSPACE_DONT_ENTER;
}



gint
xfce_workspace_download_speed (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gint           speed;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/bandwidth_download", ws);
  speed = xfconf_channel_get_int (channel, prop, 1000000);
  g_free (prop);

  return speed;
}



gboolean
xfce_workspace_upload_speed (gint ws)
{
  XfconfChannel *channel;
  gchar         *prop;
  gint           speed;

  channel = xfce_workspace_xfconf_init ();
  prop = g_strdup_printf ("/security/workspace_%d/bandwidth_upload", ws);
  speed = xfconf_channel_get_int (channel, prop, 1000000);
  g_free (prop);

  return speed;
}



gchar *
xfce_workspace_get_path_to_home (gint ws)
{
  gchar *ws_name;
  gchar *home;

  if (!xfce_workspace_enable_overlay (ws))
    return g_strdup (g_get_home_dir ());

  ws_name = xfce_workspace_get_workspace_name (ws);
  home = g_strdup_printf ("%s/Sandboxes/%s/Users/%s", g_get_home_dir (), ws_name, g_get_user_name ());
  g_free (ws_name);

  return home;
}



/* Taken from GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2006-2007 Red Hat, Inc.
 *
 * Under GPL license version 2 or later
 */
static const char *
match_prefix (const char *path,
              const char *prefix)
{
  int prefix_len;

  if (!prefix || !path)
    return NULL;

  prefix_len = strlen (prefix);
  if (strncmp (path, prefix, prefix_len) != 0)
    return NULL;

  /* Handle the case where prefix is the root, so that
   * the IS_DIR_SEPRARATOR check below works */
  if (prefix_len > 0 &&
      G_IS_DIR_SEPARATOR (prefix[prefix_len-1]))
    prefix_len--;

  return path + prefix_len;
}



gchar *
xfce_workspace_get_home_file_path_on_host (gint         ws,
                                           const gchar *inside_file)
{
  const gchar *host_home;
  gchar       *inside_home;
  const gchar *remainder;
  gchar       *host_file = NULL;

  if (!inside_file)
    return NULL;

  host_home = g_get_home_dir ();
  inside_home = xfce_workspace_get_path_to_home (ws);

  remainder = match_prefix (inside_file, inside_home);

  if (remainder != NULL && G_IS_DIR_SEPARATOR (*remainder))
      host_file = g_strdup_printf ("%s/%s", host_home, remainder + 1);

  g_free (inside_home);

  return host_file;
}



gchar *
xfce_workspace_get_home_file_path_in_sandbox (gint         ws,
                                              const gchar *host_file)
{
  const gchar *host_home;
  gchar       *inside_home;
  const gchar *remainder;
  gchar       *inside_file = NULL;

  if (!host_file)
    return NULL;

  host_home = g_get_home_dir ();
  inside_home = xfce_workspace_get_path_to_home (ws);

  remainder = match_prefix (host_file, host_home);

  if (remainder != NULL && G_IS_DIR_SEPARATOR (*remainder))
      inside_file = g_strdup_printf ("%s/%s", inside_home, remainder + 1);

  g_free (inside_home);

  return inside_file;
}



gboolean
xfce_workspace_has_private_home (gint ws)
{
  return xfce_workspace_enable_overlay (ws) && xfce_workspace_enable_private_home (ws);
}



static gchar *
xfce_workspace_name_escape (const gchar *source)
{
  const guchar *p;
  gchar *dest;
  gchar *q;

  g_return_val_if_fail (source != NULL, NULL);

  p = (guchar *) source;
  /* Each source byte needs maximally four destination chars (\777) */
  q = dest = g_malloc (strlen (source) * 4 + 1);

  while (*p)
    {
        {
          switch (*p)
            {
            case ' ':
              *q++ = '\\';
              *q++ = ' ';
              break;
            case '\b':
              *q++ = '\\';
              *q++ = 'b';
              break;
            case '\f':
              *q++ = '\\';
              *q++ = 'f';
              break;
            case '\n':
              *q++ = '\\';
              *q++ = 'n';
              break;
            case '\r':
              *q++ = '\\';
              *q++ = 'r';
              break;
            case '\t':
              *q++ = '\\';
              *q++ = 't';
              break;
            case '\v':
              *q++ = '\\';
              *q++ = 'v';
              break;
            case '\\':
              *q++ = '\\';
              *q++ = '\\';
              break;
            case '"':
              *q++ = '\\';
              *q++ = '"';
              break;
            default:
              if ((*p < ' ') || (*p >= 0177))
                {
                  *q++ = '\\';
                  *q++ = '0' + (((*p) >> 6) & 07);
                  *q++ = '0' + (((*p) >> 3) & 07);
                  *q++ = '0' + ((*p) & 07);
                }
              else
                *q++ = *p;
              break;
            }
        }
      p++;
    }
  *q = 0;
  return dest;
}



static gchar *
_xfce_workspace_get_workspace_name_escaped (gint ws,
                                            gchar * (*fun)(const gchar *))
{
  XfconfChannel *channel;
  gchar        **labels;
  gint           i;
  gchar         *ret;

  channel = xfce_workspace_xfconf_init ();
  labels = xfconf_channel_get_string_list (channel, "/general/workspace_names");
  if (!labels)
    return NULL;

  for (i = 0; i < ws && labels[i] != NULL; i++);

  if (i == ws)
    ret = fun (labels[i]);
  else
    ret = NULL;

  g_strfreev (labels);
  return ret;
}



gchar *
xfce_workspace_get_workspace_name_escaped (gint ws)
{
  return _xfce_workspace_get_workspace_name_escaped(ws, xfce_workspace_name_escape);
}



gchar *
xfce_workspace_get_workspace_name (gint ws)
{
  return _xfce_workspace_get_workspace_name_escaped(ws, g_strdup);
}



gint
xfce_workspace_get_workspace_id_from_name (const gchar *name)
{
  XfconfChannel *channel;
  gchar        **labels;
  gint           i;

  channel = xfce_workspace_xfconf_init ();

  labels = xfconf_channel_get_string_list (channel, "/general/workspace_names");
  if (!labels)
    return -1;

  for (i = 0; labels[i] != NULL; i++)
    {
      if (g_strcmp0 (labels[i], name) == 0)
        {
          g_strfreev (labels);
          return i;
        }
    }

  g_strfreev (labels);
  return -1;
}



gchar *
xfce_workspace_get_workspace_security_label (gint ws)
{
  XfconfChannel *channel;
  gchar        **labels;
  gint           i;
  gchar         *ret;

  /* we don't sandbox workspace 1 as it is where daemons spawn on startup */
  if (ws==0)
    return g_strdup ("");

  channel = xfce_workspace_xfconf_init ();
  labels = xfconf_channel_get_string_list (channel, "/security/workspace_security_labels");
  if (!labels)
    return g_strdup ("");

  for (i = 0; i < ws && labels[i] != NULL; i++);

  if (i == ws)
    ret = g_strdup (labels[i]);
  else
    ret = g_strdup ("");

  g_strfreev (labels);
  return ret;
}



gboolean
xfce_workspace_is_secure (gint ws)
{
  XfconfChannel *channel;
  gchar        **labels;
  gint           i;
  gboolean       ret;

  /* we don't sandbox workspace 1 as it is where daemons spawn on startup */
  if (ws==0)
    return FALSE;

  channel = xfce_workspace_xfconf_init ();
  labels = xfconf_channel_get_string_list (channel, "/security/workspace_security_labels");
  if (!labels) {
    return FALSE;
  }

  for (i = 0; i < ws && labels[i] != NULL; i++);

  if (i == ws)
    ret = labels[i] && g_strcmp0 (labels[i], "")? TRUE:FALSE;
  else
    ret = FALSE;

  g_strfreev (labels);
  return ret;
}



gint
xfce_workspace_get_active_workspace_number (GdkScreen *screen)
{
  GdkWindow *root;
  gulong     bytes_after_ret = 0;
  gulong     nitems_ret = 0;
  guint     *prop_ret = NULL;
  Atom       _NET_CURRENT_DESKTOP;
  Atom       _WIN_WORKSPACE;
  Atom       type_ret = None;
  gint       format_ret;
  gint       ws_num = 0;

  gdk_error_trap_push ();

  root = gdk_screen_get_root_window (screen);

  /* determine the X atom values */
  _NET_CURRENT_DESKTOP = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_NET_CURRENT_DESKTOP", False);
  _WIN_WORKSPACE = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_WIN_WORKSPACE", False);

  if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                          gdk_x11_get_default_root_xwindow(),
                          _NET_CURRENT_DESKTOP, 0, 32, False, XA_CARDINAL,
                          &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                          (gpointer) &prop_ret) != Success)
    {
      if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                              gdk_x11_get_default_root_xwindow(),
                              _WIN_WORKSPACE, 0, 32, False, XA_CARDINAL,
                              &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                              (gpointer) &prop_ret) != Success)
        {
          if (G_UNLIKELY (prop_ret != NULL))
            {
              XFree (prop_ret);
              prop_ret = NULL;
            }
        }
    }

  if (G_LIKELY (prop_ret != NULL))
    {
      if (G_LIKELY (type_ret != None && format_ret != 0))
        ws_num = *prop_ret;
      XFree (prop_ret);
    }

#if GTK_CHECK_VERSION (3, 0, 0)
  gdk_error_trap_pop_ignored ();
#else
  if (gdk_error_trap_pop () != 0)
    return 0;
#endif

  return ws_num;
}



gboolean
xfce_workspace_is_active_secure (GdkScreen *screen)
{
  gint ws = xfce_workspace_get_active_workspace_number(screen);
  return xfce_workspace_is_secure (ws);
}



#define __XFCE_WORKSPACE_C__
#include <libxfce4ui/libxfce4ui-aliasdef.c>
