/*
 * Copyright (c) 2007 The Xfce Development Team
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

#if !defined (LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined (LIBXFCE4UI_COMPILATION)
#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."
#endif

#ifndef __XFCE_WORKSPACE_H__
#define __XFCE_WORKSPACE_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS

typedef enum {
  XFCE_WORKSPACE_DONT_ENTER = 0,
  XFCE_WORKSPACE_ENTER_UNSANDBOXED = 1,
  XFCE_WORKSPACE_ENTER_REPLACE = 2
} XfceWorkspaceInBehavior;

typedef enum {
  XFCE_WORKSPACE_DONT_LEAVE = 0,
  XFCE_WORKSPACE_LEAVE_SANDBOXED = 1
} XfceWorkspaceOutBehavior;


gboolean                  xfce_workspace_has_locked_clients           (gint ws);
gboolean                  xfce_workspace_let_unsandboxed_in           (gint ws);
gboolean                  xfce_workspace_let_sandboxed_out            (gint ws);
gboolean                  xfce_workspace_enable_network               (gint ws);
gboolean                  xfce_workspace_fine_tuned_network           (gint ws);
gboolean                  xfce_workspace_isolate_dbus                 (gint ws);
gboolean                  xfce_workspace_enable_overlay               (gint ws);
gboolean                  xfce_workspace_enable_private_home          (gint ws);
gboolean                  xfce_workspace_disable_sound                (gint ws);
XfceWorkspaceInBehavior   xfce_workspace_unsandboxed_in_behavior      (gint ws);
gint                      xfce_workspace_download_speed               (gint ws);
gint                      xfce_workspace_upload_speed                 (gint ws);
gchar*                    xfce_workspace_get_workspace_name_escaped   (gint ws);
gchar*                    xfce_workspace_get_workspace_name           (gint ws);
gint                      xfce_workspace_get_workspace_id_from_name   (const gchar *);
gchar*                    xfce_workspace_get_workspace_security_label (gint ws);
gboolean                  xfce_workspace_is_secure                    (gint ws);
gint                      xfce_workspace_get_active_workspace_number  (GdkScreen *screen);
gboolean                  xfce_workspace_is_active_secure             (GdkScreen *screen);


G_END_DECLS

#endif /* !__XFCE_WORKSPACE_H__ */
