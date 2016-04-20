/* $Id$ */
/*-
 * Copyright (c) 2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2016 Steve Dodier-Lazaro <sidi@xfce.org>F
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 */

#if !defined (LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined (LIBXFCE4UI_COMPILATION)
#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."
#endif

#ifndef __XFCE_SECURITY_DIALOG_H__
#define __XFCE_SECURITY_DIALOG_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS;

typedef struct _XfceSecurityDialogClass XfceSecurityDialogClass;
typedef struct _XfceSecurityDialog      XfceSecurityDialog;

#define XFCE_TYPE_SECURITY_DIALOG            (xfce_security_dialog_get_type ())
#define XFCE_SECURITY_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_SECURITY_DIALOG, XfceSecurityDialog))
#define XFCE_SECURITY_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_SECURITY_DIALOG, XfceSecurityDialogClass))
#define XFCE_IS_SECURITY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_SECURITY_DIALOG))
#define XFCE_IS_SECURITY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_SECURITY_DIALOG))
#define XFCE_SECURITY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_SECURITY_DIALOG, XfceSecurityDialogClass))

GType      xfce_security_dialog_get_type          (void) G_GNUC_CONST;

GtkWidget *xfce_security_dialog_add_button        (XfceSecurityDialog *dialog,
                                                   const gchar        *title,
                                                   const gchar        *icon_name,
                                                   const gchar        *icon_name_fallback,
                                                   GtkResponseType     type,
                                                   gboolean            new_row,
                                                   gboolean            grab_focus);
gint       xfce_security_dialog_run               (GtkDialog          *dialog);
void       xfce_security_dialog_set_title         (XfceSecurityDialog *dialog,
                                                   const gchar        *title);
void       xfce_security_dialog_set_top_widget    (XfceSecurityDialog *dialog,
                                                   GtkWidget          *widget);
void       xfce_security_dialog_set_bottom_widget (XfceSecurityDialog *dialog,
                                                   GtkWidget          *widget);
GtkWidget *xfce_security_dialog_new               (GdkScreen        *screen,
                                                   GtkWindow        *parent,
                                                   const gchar      *title);

G_END_DECLS;


#endif /* !__XFCE_SECURITY_DIALOG_H__ */
