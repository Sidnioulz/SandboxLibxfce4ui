/*
 * Copyright (c) 2016 Steve Dodier-Lzaro <sidi@xfce.org>.
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

#ifndef __XFCE_FIREJAIL_WIDGET_H__
#define __XFCE_FIREJAIL_WIDGET_H__

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

typedef struct _XfceFirejailWidgetClass   XfceFirejailWidgetClass;
typedef struct _XfceFirejailWidget        XfceFirejailWidget;

#define XFCE_TYPE_FIREJAIL_WIDGET             (_xfce_firejail_widget_get_type ())
#define XFCE_FIREJAIL_WIDGET(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFCE_TYPE_FIREJAIL_WIDGET, XfceFirejailWidget))
#define XFCE_FIREJAIL_WIDGET_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), XFCE_TYPE_FIREJAIL_WIDGET, XfceFirejailWidgetClass))
#define XFCE_IS_FIREJAIL_WIDGET(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFCE_TYPE_FIREJAIL_WIDGET))
#define XFCE_IS_FIREJAIL_WIDGET_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), XFCE_TYPE_FIREJAIL_WIDGET))
#define XFCE_FIREJAIL_WIDGET_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), XFCE_TYPE_FIREJAIL_WIDGET, XfceFirejailWidgetClass))

GType               _xfce_firejail_widget_get_type              (void) G_GNUC_CONST;

#ifndef XFCE_FIREJAIL_WIDGET_CONSTANTS
#define XFCE_FIREJAIL_WIDGET_CONSTANTS
typedef enum {
  FS_PRIV_NONE = 0,
  FS_PRIV_FULL = 1,
  FS_PRIV_READ_ONLY = 2,
  FS_PRIV_PRIVATE = 4
} FsPrivMode;

#define XFCE_FIREJAIL_RUN_IN_SANDBOX_KEY     "X-XfceSandboxWithFirejail"
#define XFCE_FIREJAIL_PROFILE_KEY            "X-XfceFirejailProfile"
#define XFCE_FIREJAIL_ENABLE_NETWORK_KEY     "X-XfceFirejailEnableNetwork"
#define XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_KEY "X-XfceFirejailBandwidthDownload"
#define XFCE_FIREJAIL_BANDWIDTH_UPLOAD_KEY   "X-XfceFirejailBandwidthUpload"
#define XFCE_FIREJAIL_FS_MODE_KEY            "X-XfceFirejailFsMode"
#define XFCE_FIREJAIL_DISPOSABLE_KEY         "X-XfceFirejailOverlayDisposable"
#define XFCE_FIREJAIL_FS_SYNC_FOLDERS_KEY    "X-XfceFirejailSyncFolders"

#define XFCE_FIREJAIL_RUN_IN_SANDBOX_DEFAULT     FALSE
#define XFCE_FIREJAIL_ENABLE_NETWORK_DEFAULT     TRUE
#define XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_DEFAULT 200000
#define XFCE_FIREJAIL_BANDWIDTH_UPLOAD_DEFAULT   200000
#define XFCE_FIREJAIL_DISPOSABLE_DEFAULT         FALSE
#define XFCE_FIREJAIL_FS_MODE_DEFAULT            FS_PRIV_FULL
#endif

XfceFirejailWidget* xfce_firejail_widget_new                          (GKeyFile *file);
XfceFirejailWidget* xfce_firejail_widget_new_from_app_info            (GDesktopAppInfo *info);
GtkWidget*          xfce_firejail_widget_get_widget                   (XfceFirejailWidget *self);
GtkWidget*          xfce_firejail_widget_get_run_checkbox             (XfceFirejailWidget *self);
GtkWidget*          xfce_firejail_widget_get_advanced_dialog          (XfceFirejailWidget *self);
GtkWidget*          xfce_firejail_widget_get_advanced_button          (XfceFirejailWidget *self);
gboolean            xfce_firejail_widget_get_advanced_button_visible  (XfceFirejailWidget *self);
void                xfce_firejail_widget_set_advanced_button_visible  (XfceFirejailWidget *self,
                                                                       gboolean            visible);
void                xfce_firejail_widget_load_from_key_file           (XfceFirejailWidget *self,
                                                                       GKeyFile *file);
GKeyFile*           xfce_firejail_widget_get_key_file                 (XfceFirejailWidget *self);

G_END_DECLS

#endif /* !__XFCE_FIREJAIL_WIDGET_H__ */
