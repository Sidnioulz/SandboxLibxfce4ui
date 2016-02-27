/*
 * Copyright (C) 2015 Xfce Development Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <errno.h>
#include <sys/types.h>

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

static GMainLoop    *loop        = NULL;
static gboolean      opt_version = FALSE;
static GOptionEntry  opt_entries[] =
{
  { "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &opt_version, N_("Version information"), NULL },
  { NULL }
};

static void
exit_handler (int s)
{
	// Remove unused parameter warning
	(void)s;

  if (loop && g_main_loop_is_running (loop))
    g_main_loop_quit (loop);
}



static gboolean
notify_spawner (gpointer user_data)
{
  gchar *path     = (gchar *) user_data;
  gchar *last_dir;
  FILE  *stream;

  if (!path)
    {
      xfce_dialog_show_error_manual (NULL,
                                     _("The Xfce secure workspace daemon failed to notify Xfce it is running properly (error 1)."),
                                     _("Failed to initialize secure workspace"));
     
      return FALSE;
    }

  last_dir = strrchr (path, '/');
  if (last_dir)
    {
      *last_dir = '\0';
      if (g_mkdir_with_parents (path, 0755) == -1)
        {
          gchar *txt = g_strdup_printf (_("The Xfce secure workspace daemon failed to notify Xfce it is running properly (error %d: %s)."), 2, strerror(errno));
          xfce_dialog_show_error_manual (NULL, txt, _("Failed to initialize secure workspace"));
          g_free (txt);
          return FALSE;
        }
      *last_dir = '/';
    }

	stream = fopen (path, "w");
  if (!stream)
    {
      gchar *txt = g_strdup_printf (_("The Xfce secure workspace daemon failed to notify Xfce it is running properly (error %d: %s)."), 3, strerror(errno));
      xfce_dialog_show_error_manual (NULL, txt, _("Failed to initialize secure workspace"));
      g_free (txt);
      return FALSE;
    }

	if (fprintf(stream, "%u\n", getpid()) == -1)
	  {
      gchar *txt = g_strdup_printf (_("The Xfce secure workspace daemon failed to notify Xfce it is running properly (error %d: %s)."), 4, strerror(errno));
      xfce_dialog_show_error_manual (NULL, txt, _("Failed to initialize secure workspace"));
      g_free (txt);
      return FALSE;
	  }

	fflush(stream);
	fclose(stream);

  return FALSE;
}

gint
main (gint    argc,
      gchar **argv)
{
  GError         *error = NULL;

  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  // parse options, initialize what needs initializing
  if (G_UNLIKELY (!gtk_init_with_args (&argc, &argv, NULL, opt_entries, PACKAGE, &error)))
    {
      if (G_LIKELY (error != NULL))
        {
          g_print ("%s: Failed to initialize Xfce Secure Workspace: %s.\n", G_LOG_DOMAIN, error->message);
          g_print ("\n");
          g_error_free (error);
        }
      else
        g_error (_("Failed to initialize Xfce Secure Workspace."));

      return EXIT_FAILURE;
    }

  if (G_UNLIKELY (opt_version))
    {
      g_print ("%s %s (Xfce %s)\n\n", G_LOG_DOMAIN, PACKAGE_VERSION, xfce_version_string ());
      g_print ("%s\n", "Copyright (c) 2015-2016");
      g_print ("\t%s\n\n", _("The Xfce development team. All rights reserved."));
      g_print (_("Please report bugs to <%s>."), PACKAGE_BUGREPORT);
      g_print ("\n");

      return EXIT_SUCCESS;
    }

  if (argc < 2)
    {
      xfce_dialog_show_error_manual (NULL, _("The Xfce secure workspace daemon was invoked improperly."), _("Failed to initialize secure workspace"));
      g_error (_("%s %s (Xfce %s)\n\nUsage: %s \"SANDBOX NAME\" \"PATH TO NOTIFY FILE\"\n"), G_LOG_DOMAIN, PACKAGE_VERSION, xfce_version_string (), G_LOG_DOMAIN);
      return EXIT_FAILURE;
    }

  // install a signal handler to quit the daemon
	signal (SIGINT, exit_handler);
	signal (SIGTERM, exit_handler);

  // get the loop going
  loop = g_main_loop_new (NULL, FALSE);
  if (!loop)
    {
      g_error (_("Unable to create an event loop. Your system might be out of memory."));
      return EXIT_FAILURE;
    }

  // create an idle function that will be called once the loop runs
  g_idle_add_full (G_PRIORITY_HIGH, notify_spawner, argv[1], NULL);

  // run the loop, we're now a happy 'daemon'!
  g_main_loop_run (loop);

  return EXIT_SUCCESS;
}
