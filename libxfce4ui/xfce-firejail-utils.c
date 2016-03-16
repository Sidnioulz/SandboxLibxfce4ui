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
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <libxfce4ui/xfce-firejail-utils.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>




GList *
xfce_get_firejail_profile_names (gboolean insert_generic)
{
  GError           *error = NULL;
  GList            *profiles = NULL;
  GDir             *dir;
  const gchar      *name;
  gchar            *copy;
  gchar            *home_path;

  /* read the home profile directory */
  home_path = g_strdup_printf ("%s/firejail", g_get_user_config_dir ());
  dir = g_dir_open (home_path, 0, &error);
  if (error)
    {
      g_error_free (error);
    }
  else
    {
      while ((name = g_dir_read_name (dir)) != NULL)
        {
          if (g_str_has_suffix (name, ".profile") &&
              g_strcmp0 (EXECHELP_DEFAULT_USER_PROFILE ".profile", name) &&
              g_strcmp0 (EXECHELP_DEFAULT_ROOT_PROFILE ".profile", name))
            {
              copy = g_strndup (name, strstr (name, ".profile") - name);
              profiles = g_list_prepend (profiles, copy);
            }
        }
      g_dir_close (dir);
    }
  g_free (home_path);

  /* read the /etc profile directory */
  dir = g_dir_open ("/etc/firejail", 0, &error);
  if (error)
    {
      g_error_free (error);
    }
  else
    {
      while ((name = g_dir_read_name (dir)) != NULL)
        {
          if (g_str_has_suffix (name, ".profile") &&
              g_strcmp0 (EXECHELP_DEFAULT_USER_PROFILE ".profile", name) &&
              g_strcmp0 (EXECHELP_DEFAULT_ROOT_PROFILE ".profile", name))
            {
              copy = g_strndup (name, strstr (name, ".profile") - name);
              if (!g_list_find_custom (profiles, copy, (GCompareFunc) g_strcmp0))
                profiles = g_list_prepend (profiles, copy);
              else
                g_free (copy);
            }
        }
      g_dir_close (dir);
    }

  /* we did not know the reading order in the directory */
  profiles = g_list_sort (profiles, (GCompareFunc) g_strcmp0);

  /* prepend the default options */
  if (insert_generic)
    {
      profiles = g_list_prepend (profiles, g_strdup (EXECHELP_DEFAULT_ROOT_PROFILE));
      profiles = g_list_prepend (profiles, g_strdup (EXECHELP_DEFAULT_USER_PROFILE));
    }

  return profiles;
}


gchar *
xfce_get_firejail_profile_for_name (const gchar *name)
{
  gchar *profiletxt, *pathtxt;
  struct stat s;

  g_return_val_if_fail (name != NULL, NULL);

  pathtxt = g_strdup_printf ("%s/firejail/%s.profile", g_get_user_config_dir (), name);

  if (stat (pathtxt, &s) == 0)
    {
      profiletxt = g_strdup_printf ("--profile=%s", pathtxt);
      g_free (pathtxt);
      return profiletxt;
    }

  g_free (pathtxt);
  pathtxt = g_strdup_printf ("/etc/firejail/%s.profile", name);

  if (stat (pathtxt, &s) == 0)
    {
      profiletxt = g_strdup_printf ("--profile=%s", pathtxt);
      g_free (pathtxt);
      return profiletxt;
    }

  g_free (pathtxt);
  return NULL;
}

#define __XFCE_FIREJAIL_UTILS_C__
#include <libxfce4ui/libxfce4ui-aliasdef.c>
