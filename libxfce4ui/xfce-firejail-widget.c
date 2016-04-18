/*
 * Copyright (c) 2016 Steve Dodier-Lazaro <sidi@xfce.org>.
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

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>

#include <glib.h>
#include <gio/gdesktopappinfo.h>

#include <libxfce4ui/xfce-firejail-widget.h>
#include <libxfce4ui/xfce-firejail-widget-ui.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>

static void         _xfce_firejail_widget_constructed          (GObject          *object);
static void         _xfce_firejail_widget_finalize             (GObject          *object);
static void         _xfce_firejail_widget_set_property         (GObject          *object,
                                                                guint             property_id,
                                                                const GValue     *value,
                                                                GParamSpec       *pspec);
static void         _xfce_firejail_widget_get_property         (GObject          *object,
                                                                guint             property_id,
                                                                GValue           *value,
                                                                GParamSpec       *pspec);

enum
{
  PROP_KEY_FILE = 1,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

typedef guint FsPrivHoverFlags;

#define XFCE_FIREJAIL_WIDGET_PROFILE_LIST_SEPARATOR "///"
#define INVALID_PATH_ICON      "gtk-dialog-error"
#define INVALID_PATH_TOOLTIP   _("This path is invalid. Firejail may fail to launch the application.")



struct _XfceFirejailWidgetClass
{
  /*< private >*/
  GObjectClass __parent__;
};

struct _XfceFirejailWidget
{
  /*< private >*/
  GObject         __parent__;

  /* widget stuff */
  GtkBuilder      *builder;

  /* app identity and app's profile location */
  GKeyFile        *key_file;

  /* sandboxing options */
  gboolean         run_in_sandbox;
  gboolean         enable_network;
  gint             bandwidth_download;
  gint             bandwidth_upload;
  gchar           *base_profile;
  GList           *sync_folders;
  FsPrivMode       fs_mode;
  gboolean         disposable;

  /* gui bits */
  FsPrivHoverFlags hovered_row;
};



G_DEFINE_TYPE (XfceFirejailWidget, _xfce_firejail_widget, G_TYPE_OBJECT)



static void
_xfce_firejail_widget_class_init (XfceFirejailWidgetClass *klass)
{
  GObjectClass   *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = _xfce_firejail_widget_constructed;
  gobject_class->finalize = _xfce_firejail_widget_finalize;
  gobject_class->set_property = _xfce_firejail_widget_set_property;
  gobject_class->get_property = _xfce_firejail_widget_get_property;

  obj_properties[PROP_KEY_FILE] =
    g_param_spec_pointer ("key-file",
                         "GKeyFile object this widget provides settings for",
                         "Set the application this widget is about",
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}



static void
_xfce_firejail_widget_set_run_in_sandbox (XfceFirejailWidget *self,
                                          gboolean value)
{
  self->run_in_sandbox = value;

  if (self->key_file)
    g_key_file_set_boolean (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_RUN_IN_SANDBOX_KEY, value);
}



static void
_xfce_firejail_widget_set_enable_network (XfceFirejailWidget *self,
                                          gboolean value)
{
  self->enable_network = value;

  if (self->key_file)
    g_key_file_set_boolean (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_ENABLE_NETWORK_KEY, value);
}



static void
_xfce_firejail_widget_set_bandwidth_download (XfceFirejailWidget *self,
                                              gint value)
{
  self->bandwidth_download = value;

  if (self->key_file)
    g_key_file_set_integer (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_KEY, value);
}



static void
_xfce_firejail_widget_set_bandwidth_upload (XfceFirejailWidget *self,
                                              gint value)
{
  self->bandwidth_upload = value;

  if (self->key_file)
    g_key_file_set_integer (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_UPLOAD_KEY, value);
}



static void
_xfce_firejail_widget_set_base_profile (XfceFirejailWidget *self,
                                        gchar *value)
{
  if (self->base_profile)
    g_free (self->base_profile);

  self->base_profile = value;

  if (self->key_file)
    g_key_file_set_string (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_PROFILE_KEY, value);
}



static void
_xfce_firejail_widget_set_fs_mode (XfceFirejailWidget *self,
                                   gint value)
{
  self->fs_mode = value;

  if (self->key_file)
    g_key_file_set_integer (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_FS_MODE_KEY, value);
}



static void
_xfce_firejail_widget_set_disposable_fs (XfceFirejailWidget *self,
                                         gboolean value)
{
  self->disposable = value;

  if (self->key_file)
    g_key_file_set_boolean (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_DISPOSABLE_KEY, value);
}



static void
_xfce_firejail_widget_save_sync_folders (XfceFirejailWidget *self)
{
  const gchar **strv;
  gsize         length;
  guint         i;
  GList        *link;

  if (self->key_file)
    {
      length = g_list_length (self->sync_folders);
      strv = g_malloc0 (sizeof (gchar *) * length + 1);

      for (i = 0, link = self->sync_folders; i <= length && link; i++, link = link->next)
        strv[i] = link->data;

      g_key_file_set_string_list (self->key_file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_FS_SYNC_FOLDERS_KEY, strv, length);

      g_free (strv);
    }
}



static void
on_run_in_firejail_checked (GtkToggleButton *button,
                            gpointer         user_data)
{
  XfceFirejailWidget *self   = XFCE_FIREJAIL_WIDGET (user_data);
  GtkWidget          *widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_settings_run_alignment"));
  gboolean            active = gtk_toggle_button_get_active (button);

  _xfce_firejail_widget_set_run_in_sandbox (self, active);

  gtk_widget_set_sensitive (widget, active);
}



static void
on_enable_network_checked (GtkToggleButton *button,
                           gpointer         user_data)
{
  XfceFirejailWidget *self   = XFCE_FIREJAIL_WIDGET (user_data);
  gboolean            active = gtk_toggle_button_get_active(button);
  GtkWidget          *widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "net_alignment_options"));

  gtk_widget_set_sensitive (widget, active);

  _xfce_firejail_widget_set_enable_network (self, active);
}



static void
on_bandwidth_download_changed (GtkSpinButton *button,
                               gpointer         user_data)
{
  XfceFirejailWidget *self   = XFCE_FIREJAIL_WIDGET (user_data);
  gint                value  = gtk_spin_button_get_value_as_int (button);

  _xfce_firejail_widget_set_bandwidth_download (self, value);
}



static void
on_bandwidth_upload_changed (GtkSpinButton *button,
                             gpointer         user_data)
{
  XfceFirejailWidget *self   = XFCE_FIREJAIL_WIDGET (user_data);
  gint                value  = gtk_spin_button_get_value_as_int (button);

  _xfce_firejail_widget_set_bandwidth_upload (self, value);
}



static gboolean
_xfce_firejail_widget_profile_list_item_is_separator (GtkTreeModel *model,
                                                      GtkTreeIter *iter,
                                                      gpointer data)
{
  gchar    *name = NULL;
  gboolean  sep  = FALSE;

  gtk_tree_model_get (model, iter, 0, &name, -1);
  if (name && g_strcmp0 (name, XFCE_FIREJAIL_WIDGET_PROFILE_LIST_SEPARATOR) == 0)
    sep = TRUE;

  return sep;
}



static void
add_to_profile_list (GtkWidget    *widget,
                     gchar        *id,
                     gchar        *display_name,
                     gboolean      is_generic,
                     const gchar  *preferred_profile,
                     gboolean     *active_set)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  gboolean      realloc = FALSE;

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));

  if (!is_generic)
    {
      gchar *desktop_name = g_strdup_printf ("%s.desktop", id);
      GDesktopAppInfo *info = g_desktop_app_info_new (desktop_name);
      if (info && g_app_info_get_name (G_APP_INFO (info)))
        {
          display_name = g_strdup (g_app_info_get_name (G_APP_INFO (info)));
          realloc = TRUE;    
        }
      g_free (desktop_name);
    }

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, id, 1, display_name, 2, is_generic? PANGO_STYLE_ITALIC:PANGO_STYLE_NORMAL, -1);

  if (realloc)
    g_free (display_name);

  if (preferred_profile && g_strcmp0 (id, preferred_profile) == 0)
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
     *active_set = TRUE;
    }
}



static void
on_disposable_toggled (GtkToggleButton *button,
                            gpointer user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  _xfce_firejail_widget_set_disposable_fs (self, gtk_toggle_button_get_active (button));
}



static void
on_full_home_privs_toggled (GtkToggleButton *button,
                            gpointer user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);
  gboolean active = gtk_toggle_button_get_active (button);
  GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_disposable_check"));
  GtkWidget *sync_folder_alignment = GTK_WIDGET (gtk_builder_get_object (self->builder, "sync_folder_vbox"));

  if (!active)
    {
      gtk_widget_set_sensitive (widget, TRUE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), self->disposable);
      gtk_widget_set_sensitive (sync_folder_alignment, TRUE);
    }
  else
    {
      g_signal_handlers_block_by_func (widget, on_disposable_toggled, self);
      gtk_widget_set_sensitive (widget, FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
      g_signal_handlers_unblock_by_func (widget, on_disposable_toggled, self);
      gtk_widget_set_sensitive (sync_folder_alignment, FALSE);

      _xfce_firejail_widget_set_fs_mode (self, FS_PRIV_FULL);
    }
}



static void
on_ro_home_privs_toggled (GtkToggleButton *button,
                          gpointer user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);
  gboolean active = gtk_toggle_button_get_active (button);

  if (active)
    _xfce_firejail_widget_set_fs_mode (self, FS_PRIV_READ_ONLY);
}



static void
on_private_home_privs_toggled (GtkToggleButton *button,
                               gpointer user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);
  gboolean active = gtk_toggle_button_get_active (button);

  if (active)
    _xfce_firejail_widget_set_fs_mode (self, FS_PRIV_PRIVATE);
}



static void
_xfce_firejail_widget_update_priv_list_hover (XfceFirejailWidget *self)
{
  GtkWidget *image_read_full      = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_read_full"));
  GtkWidget *image_mod_full       = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_mod_full"));
  GtkWidget *image_create_full    = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_create_full"));
  GtkWidget *image_overlay_full1  = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_full1"));
  GtkWidget *image_overlay_full2  = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_full2"));
  GtkWidget *image_read_ro        = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_read_ro"));
  GtkWidget *image_mod_ro         = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_mod_ro"));
  GtkWidget *image_create_ro      = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_create_ro"));
  GtkWidget *image_overlay_ro1    = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_ro1"));
  GtkWidget *image_overlay_ro2    = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_ro2"));
  GtkWidget *image_read_priv      = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_read_priv"));
  GtkWidget *image_mod_priv       = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_mod_priv"));
  GtkWidget *image_create_priv    = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_create_priv"));
  GtkWidget *image_overlay_priv1  = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_priv1"));
  GtkWidget *image_overlay_priv2  = GTK_WIDGET (gtk_builder_get_object (self->builder, "image_overlay_priv2"));
  GtkWidget *label_read      = GTK_WIDGET (gtk_builder_get_object (self->builder, "label_read"));
  GtkWidget *label_mod       = GTK_WIDGET (gtk_builder_get_object (self->builder, "label_mod"));
  GtkWidget *label_create    = GTK_WIDGET (gtk_builder_get_object (self->builder, "label_create"));
  GtkWidget *label_overlay1  = GTK_WIDGET (gtk_builder_get_object (self->builder, "label_overlay1"));
  GtkWidget *label_overlay2  = GTK_WIDGET (gtk_builder_get_object (self->builder, "label_overlay2"));

  if (self->hovered_row & FS_PRIV_FULL)
    {
      gtk_widget_set_sensitive (image_overlay_full1, FALSE);
      gtk_widget_set_sensitive (image_overlay_full2, FALSE);

      gtk_widget_set_sensitive (image_overlay_ro1, FALSE);
      gtk_widget_set_sensitive (image_overlay_ro2, FALSE);

      gtk_widget_set_sensitive (image_overlay_priv1, FALSE);
      gtk_widget_set_sensitive (image_overlay_priv2, FALSE);

      gtk_widget_set_sensitive (label_overlay1, FALSE);
      gtk_widget_set_sensitive (label_overlay2, FALSE);
    }
  else if (self->hovered_row & FS_PRIV_READ_ONLY)
    {
      gtk_widget_set_sensitive (image_mod_full, FALSE);
      gtk_widget_set_sensitive (image_create_full, FALSE);
      
      gtk_widget_set_sensitive (image_mod_ro, FALSE);
      gtk_widget_set_sensitive (image_create_ro, FALSE);
      
      gtk_widget_set_sensitive (image_mod_priv, FALSE);
      gtk_widget_set_sensitive (image_create_priv, FALSE);
      
      gtk_widget_set_sensitive (label_mod, FALSE);
      gtk_widget_set_sensitive (label_create, FALSE);
    }
  else if (self->hovered_row & FS_PRIV_PRIVATE)
    {
      gtk_widget_set_sensitive (image_read_full, FALSE);
      gtk_widget_set_sensitive (image_mod_full, FALSE);
      gtk_widget_set_sensitive (image_create_full, FALSE);
      gtk_widget_set_sensitive (image_overlay_full2, FALSE);
      
      gtk_widget_set_sensitive (image_read_ro, FALSE);
      gtk_widget_set_sensitive (image_mod_ro, FALSE);
      gtk_widget_set_sensitive (image_create_ro, FALSE);
      gtk_widget_set_sensitive (image_overlay_ro2, FALSE);
      
      gtk_widget_set_sensitive (image_read_priv, FALSE);
      gtk_widget_set_sensitive (image_mod_priv, FALSE);
      gtk_widget_set_sensitive (image_create_priv, FALSE);
      gtk_widget_set_sensitive (image_overlay_priv2, FALSE);
      
      gtk_widget_set_sensitive (label_read, FALSE);
      gtk_widget_set_sensitive (label_mod, FALSE);
      gtk_widget_set_sensitive (label_create, FALSE);
      gtk_widget_set_sensitive (label_overlay2, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (image_read_full, TRUE);
      gtk_widget_set_sensitive (image_mod_full, TRUE);
      gtk_widget_set_sensitive (image_create_full, TRUE);
      gtk_widget_set_sensitive (image_overlay_full1, TRUE);
      gtk_widget_set_sensitive (image_overlay_full2, TRUE);

      gtk_widget_set_sensitive (image_read_ro, TRUE);
      gtk_widget_set_sensitive (image_mod_ro, TRUE);
      gtk_widget_set_sensitive (image_create_ro, TRUE);
      gtk_widget_set_sensitive (image_overlay_ro1, TRUE);
      gtk_widget_set_sensitive (image_overlay_ro2, TRUE);

      gtk_widget_set_sensitive (image_read_priv, TRUE);
      gtk_widget_set_sensitive (image_mod_priv, TRUE);
      gtk_widget_set_sensitive (image_create_priv, TRUE);
      gtk_widget_set_sensitive (image_overlay_priv1, TRUE);
      gtk_widget_set_sensitive (image_overlay_priv2, TRUE);

      gtk_widget_set_sensitive (label_read, TRUE);
      gtk_widget_set_sensitive (label_mod, TRUE);
      gtk_widget_set_sensitive (label_create, TRUE);
      gtk_widget_set_sensitive (label_overlay1, TRUE);
      gtk_widget_set_sensitive (label_overlay2, TRUE);
    }
}



static gboolean
on_full_home_privs_hovered (GtkWidget *widget,
                            GdkEvent  *event,
                            gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row |= FS_PRIV_FULL;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static gboolean
on_full_home_privs_hover_leave (GtkWidget *widget,
                                GdkEvent  *event,
                                gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row &= ~FS_PRIV_FULL;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static gboolean
on_ro_home_privs_hovered (GtkWidget *widget,
                          GdkEvent  *event,
                          gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row |= FS_PRIV_READ_ONLY;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static gboolean
on_ro_home_privs_hover_leave (GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row &= ~FS_PRIV_READ_ONLY;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static gboolean
on_priv_home_privs_hovered (GtkWidget *widget,
                            GdkEvent  *event,
                            gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row |= FS_PRIV_PRIVATE;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static gboolean
on_priv_home_privs_hover_leave (GtkWidget *widget,
                                GdkEvent  *event,
                                gpointer   user_data)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (user_data);

  self->hovered_row &= ~FS_PRIV_PRIVATE;  
  _xfce_firejail_widget_update_priv_list_hover (self);

  return FALSE;
}



static void
_xfce_firejail_widget_init (XfceFirejailWidget *self)
{
  GError *error = NULL;

  self->builder = gtk_builder_new ();
  gtk_builder_add_from_string (self->builder,
                               xfce_firejail_widget_ui,
                               xfce_firejail_widget_ui_length,
                               &error);

  if (error)
    {
      g_critical ("Xfce Firejail widget: Unable to build widget: %s", error->message);
      g_error_free (error);
      g_return_if_reached ();
    }


  self->run_in_sandbox = XFCE_FIREJAIL_RUN_IN_SANDBOX_DEFAULT;
  self->enable_network = XFCE_FIREJAIL_ENABLE_NETWORK_DEFAULT;
  self->bandwidth_download = XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_DEFAULT;
  self->bandwidth_upload = XFCE_FIREJAIL_BANDWIDTH_UPLOAD_DEFAULT;
  self->base_profile = NULL;

  self->fs_mode = XFCE_FIREJAIL_FS_MODE_DEFAULT;
  self->disposable = XFCE_FIREJAIL_DISPOSABLE_DEFAULT;
  self->hovered_row = FS_PRIV_NONE;

  self->sync_folders = NULL;
}



static void
on_folder_list_selection_changed (GtkTreeView *treeview,
                                  gpointer   user_data)
{
  XfceFirejailWidget *self          = XFCE_FIREJAIL_WIDGET (user_data);
  GtkTreeSelection   *select        = gtk_tree_view_get_selection (treeview);
  GList              *selected_rows = gtk_tree_selection_get_selected_rows (select, NULL);
  GtkWidget          *widget;

  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "sync_folder_remove_btn"));
  gtk_widget_set_sensitive (widget, selected_rows != NULL);

  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
}



static struct
{
  GUserDirectory  type;
  const gchar    *icon_name;
}
xfce_ui_file_dirs[] =
{
  { G_USER_DIRECTORY_DESKTOP,      "user-desktop" },
  { G_USER_DIRECTORY_DOCUMENTS,    "folder-documents" },
  { G_USER_DIRECTORY_DOWNLOAD,     "folder-download" },
  { G_USER_DIRECTORY_MUSIC,        "folder-music" },
  { G_USER_DIRECTORY_PICTURES,     "folder-pictures" },
  { G_USER_DIRECTORY_PUBLIC_SHARE, "folder-publicshare" },
  { G_USER_DIRECTORY_TEMPLATES,    "folder-templates" },
  { G_USER_DIRECTORY_VIDEOS,       "folder-videos" }
};



static gchar *
xfce_ui_file_get_icon_name (GFile *gfile)
{
  gchar               *icon_name = NULL;
  gchar               *path;
  const gchar         *special_names[] = { NULL, "folder", NULL };
  guint                i;
  const gchar         *special_dir;
  GFileInfo           *fileinfo;

  fileinfo = g_file_query_info (gfile,
                                G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT,
                                G_FILE_QUERY_INFO_NONE, NULL, NULL);

  if (!fileinfo)
    return NULL;

  /* the system root folder has a special icon */
  if (g_file_info_get_file_type (fileinfo) == G_FILE_TYPE_DIRECTORY)
    {
      if (G_LIKELY (g_file_has_uri_scheme (gfile, "file")))
        {
          path = g_file_get_path (gfile);
          if (G_LIKELY (path != NULL))
            {
              if (strcmp (path, G_DIR_SEPARATOR_S) == 0)
                *special_names = "drive-harddisk";
              else if (strcmp (path, xfce_get_homedir ()) == 0)
                *special_names = "user-home";
              else
                {
                  for (i = 0; i < G_N_ELEMENTS (xfce_ui_file_dirs); i++)
                    {
                      special_dir = g_get_user_special_dir (xfce_ui_file_dirs[i].type);
                      if (special_dir != NULL
                          && strcmp (path, special_dir) == 0)
                        {
                          *special_names = xfce_ui_file_dirs[i].icon_name;
                          break;
                        }
                    }
                }

              g_free (path);
            }
        }
      else if (!g_file_get_parent (gfile))
        {
          if (g_file_has_uri_scheme (gfile, "trash"))
            {
              special_names[0] = g_file_info_get_attribute_uint32 (fileinfo, G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT) > 0 ? "user-trash-full" : "user-trash";
              special_names[1] = "user-trash";
            }
          else if (g_file_has_uri_scheme (gfile, "network"))
            {
              special_names[0] = "network-workgroup";
            }
          else if (g_file_has_uri_scheme (gfile, "recent"))
            {
              special_names[0] = "document-open-recent";
            }
          else if (g_file_has_uri_scheme (gfile, "computer"))
            {
              special_names[0] = "computer";
            }
        }

      if (*special_names != NULL)
        {
          if (G_LIKELY (special_names != NULL))
            {
              for (i = 0; special_names[i] != NULL; ++i)
                if (*special_names[i] != '(' )
                  {
                    icon_name = g_strdup (special_names[i]);
                    break;
                  }
            }
        }
    }

  /* store new name, fallback to legacy names, or empty string to avoid recursion */
  if (G_UNLIKELY (icon_name == NULL))
    {
      if (g_file_info_get_file_type (fileinfo) == G_FILE_TYPE_DIRECTORY)
        icon_name = "folder";
      else
        icon_name = NULL;
    }

  /* release */
  g_object_unref (G_OBJECT (fileinfo));

  return icon_name? g_strdup (icon_name) : NULL;
}



static void
on_add_btn_clicked (GtkButton *button,
                    gpointer   user_data)
{
  XfceFirejailWidget *self          = XFCE_FIREJAIL_WIDGET (user_data);
  GtkTreeView        *treeview;
  GtkListStore       *store;
  GtkTreeIter         iter;
  GtkWidget          *chooser;
  GFile              *file;
  gchar              *pathname;
  gchar              *iconname;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (self->builder, "sync_folder_tree_view"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  
  chooser = gtk_file_chooser_dialog_new (_("Select Synchronised Folder to Add"),
                                         GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                                         _("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         _("_Add"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      pathname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      file = g_file_new_for_path (pathname);
      iconname = xfce_ui_file_get_icon_name (file);
      g_object_unref (file);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                    0, pathname,
                    2, iconname? iconname : "gtk-dialog-error",
                    3, iconname? NULL : INVALID_PATH_TOOLTIP, -1);
      self->sync_folders = g_list_append (self->sync_folders, pathname);
      _xfce_firejail_widget_save_sync_folders (self);

      g_free (iconname);
    }

  gtk_widget_destroy (chooser);
  on_folder_list_selection_changed (treeview, self);
}



static void
on_remove_btn_clicked (GtkButton *button,
                       gpointer   user_data)
{
  XfceFirejailWidget *self          = XFCE_FIREJAIL_WIDGET (user_data);
  GtkTreeView        *treeview;
  GtkTreeModel       *model;
  GtkTreeSelection   *select;
  GtkTreeIter         iter;
  GList              *selected_rows;
  GList              *link;
  gchar              *pathname;

  treeview = GTK_TREE_VIEW (gtk_builder_get_object (self->builder, "sync_folder_tree_view"));
  model  = gtk_tree_view_get_model (treeview);
  select = gtk_tree_view_get_selection (treeview);

  /* we don't allow multiple selection so deleting the first is enough */  
  selected_rows = gtk_tree_selection_get_selected_rows (select, NULL);
  if (!selected_rows)
    return;

  if (!gtk_tree_model_get_iter (model, &iter, selected_rows->data))
    return; /* path describes a non-existing row - should not happen */

  gtk_tree_model_get (model, &iter, 0, &pathname, -1);
  gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

  link = g_list_find_custom (self->sync_folders, pathname, (GCompareFunc) g_strcmp0);
  if (link)
    {
      self->sync_folders = g_list_delete_link (self->sync_folders, link);
      _xfce_firejail_widget_save_sync_folders (self);
    }

  g_free (pathname);
  g_list_free_full (selected_rows, (GDestroyNotify) gtk_tree_path_free);
  on_folder_list_selection_changed (treeview, self);
}



static void
_xfce_firejail_widget_set_key_file (XfceFirejailWidget *self,
                                    GKeyFile           *file)
{
  g_return_if_fail (XFCE_IS_FIREJAIL_WIDGET (self));
  g_return_if_fail (file);

  g_key_file_ref (file);

  if (self->key_file)
    g_key_file_unref (self->key_file);

  self->key_file = file;
}



void
xfce_firejail_widget_load_from_key_file (XfceFirejailWidget *self,
                                         GKeyFile           *file)
{
  gchar **rclist;
  guint   i;

  g_return_if_fail (XFCE_IS_FIREJAIL_WIDGET (self));
  g_return_if_fail (file);

  if (self->key_file != file)
    _xfce_firejail_widget_set_key_file (self, file);

  /* updating widget values based on key file */
  self->run_in_sandbox = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_RUN_IN_SANDBOX_KEY, NULL)?
                         g_key_file_get_boolean (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_RUN_IN_SANDBOX_KEY, NULL) : XFCE_FIREJAIL_RUN_IN_SANDBOX_DEFAULT;
  self->base_profile = g_key_file_get_string (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_PROFILE_KEY, NULL);
  self->enable_network = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_ENABLE_NETWORK_KEY, NULL)?
                         g_key_file_get_boolean (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_ENABLE_NETWORK_KEY, NULL) : XFCE_FIREJAIL_ENABLE_NETWORK_DEFAULT;
  self->bandwidth_download = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_KEY, NULL)?
                         g_key_file_get_integer (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_KEY, NULL) : XFCE_FIREJAIL_BANDWIDTH_DOWNLOAD_DEFAULT;
  self->bandwidth_upload = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_UPLOAD_KEY, NULL)?
                         g_key_file_get_integer (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_BANDWIDTH_UPLOAD_KEY, NULL) : XFCE_FIREJAIL_BANDWIDTH_UPLOAD_DEFAULT;
  self->fs_mode = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_FS_MODE_KEY, NULL)?
                  g_key_file_get_integer (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_FS_MODE_KEY, NULL) : XFCE_FIREJAIL_FS_MODE_DEFAULT;
  self->disposable = g_key_file_has_key (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_DISPOSABLE_KEY, NULL)?
                     g_key_file_get_boolean (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_DISPOSABLE_KEY, NULL) : XFCE_FIREJAIL_DISPOSABLE_DEFAULT;

  rclist = g_key_file_get_string_list (file, G_KEY_FILE_DESKTOP_GROUP, XFCE_FIREJAIL_FS_SYNC_FOLDERS_KEY, NULL, NULL);
  for (i = 0; rclist && rclist[i]; ++i)
    self->sync_folders = g_list_append (self->sync_folders, rclist[i]);
  g_free (rclist);
}

static void
on_profile_box_changed (GtkComboBox *widget,
                        gpointer     user_data)
{
  XfceFirejailWidget *self  = XFCE_FIREJAIL_WIDGET (user_data);
  GtkTreeModel       *model = gtk_combo_box_get_model (widget);
  GtkTreeIter         iter;
  gchar              *name;

  if (gtk_combo_box_get_active_iter (widget, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &name, -1);
      _xfce_firejail_widget_set_base_profile (self, name);
    }
}



static void
_xfce_firejail_widget_constructed (GObject          *object)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (object);
  GtkWidget          *widget;
  GtkWidget          *spinbutton_dl;
  GtkWidget          *spinbutton_ul;
  GList              *profiles            = NULL;
  GList              *lp                  = NULL;
  GtkCellRenderer    *render;
  GtkTreeModel       *model;
  GtkTreeIter         iter;
  gboolean            active_set          = FALSE;
  gchar              *name                = NULL;
  gchar              *display_name;
  gchar              *new_name;
  gboolean            generic;
  GtkTreeViewColumn  *column;
  gchar              *pathname;
  gchar              *iconname;
  GtkTreeView        *treeview;
  GtkListStore       *store;
  GFile              *file;
  GList              *link;

  /* Initialize the widget */
  if (self->key_file)
    xfce_firejail_widget_load_from_key_file (self, self->key_file);

  /* disable the widget options when 'Run in sandbox' is unchecked */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_settings_run"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), self->run_in_sandbox);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_run_in_firejail_checked), self);
  on_run_in_firejail_checked (GTK_TOGGLE_BUTTON (widget), self);

  /* Enable network checkbox */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "enable_network_check_embed"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), self->enable_network);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_enable_network_checked), self);
  on_enable_network_checked (GTK_TOGGLE_BUTTON (widget), self);

  /* Bandwidth spinners */
  spinbutton_dl = GTK_WIDGET (gtk_builder_get_object (self->builder, "spinbutton_dl"));
  spinbutton_ul = GTK_WIDGET (gtk_builder_get_object (self->builder, "spinbutton_ul"));
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton_dl), 0, 200000);
  gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton_ul), 0, 200000);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton_dl), self->bandwidth_download);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton_ul), self->bandwidth_upload);
  g_signal_connect (spinbutton_dl, "value-changed", G_CALLBACK (on_bandwidth_download_changed), self);
  g_signal_connect (spinbutton_ul, "value-changed", G_CALLBACK (on_bandwidth_upload_changed), self);
  on_bandwidth_download_changed (GTK_SPIN_BUTTON (spinbutton_dl), self);
  on_bandwidth_upload_changed (GTK_SPIN_BUTTON (spinbutton_ul), self);

  /* populate the profile list */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "profile_combobox"));
  render = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), render, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), render, "text", 1, "style", 2, NULL);
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (widget), _xfce_firejail_widget_profile_list_item_is_separator, NULL, NULL);

  /* first with the two generic profiles */
  add_to_profile_list (widget, "generic", "Generic application profile", TRUE, self->base_profile, &active_set);
  add_to_profile_list (widget, "server", "Generic daemon profile", TRUE, self->base_profile, &active_set);
  add_to_profile_list (widget, XFCE_FIREJAIL_WIDGET_PROFILE_LIST_SEPARATOR, NULL, TRUE, self->base_profile, &active_set);

  profiles = xfce_get_firejail_profile_names (FALSE);
  for (lp = profiles; lp != NULL; lp = lp->next)
    {
      name = lp->data;
      add_to_profile_list (widget, name, name, FALSE, self->base_profile, &active_set);
      g_free (name);
    }
  g_list_free (profiles);

  /* if no active firejail profile was found, try to find one matching the executable name */
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      gboolean            found = FALSE;

      do
        {
          gtk_tree_model_get (model, &iter, 0, &name, 1, &display_name, 2, &generic, -1);
          
          if (self->key_file && g_strcmp0 (g_key_file_get_string (self->key_file, G_KEY_FILE_DESKTOP_GROUP, G_KEY_FILE_DESKTOP_KEY_EXEC, NULL), name) == 0)
            {
              /* mark as active if it matches the current app info and we need an active one */
              if (!active_set)
                {
                  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
                  active_set = TRUE;
                }

              /* mark as firejail default if it'd match the rules for a firejail default */
              if (!generic)
                {
                  new_name = g_strdup_printf ("%s <i>(recommended)</i>", display_name);
                  gtk_list_store_set (GTK_LIST_STORE (model), &iter, 1, new_name, -1);
                  g_free (new_name);
                }
            }

          g_free (name);
          g_free (display_name);
        }
      while (gtk_tree_model_iter_next (model, &iter) && (!found));

      /* use the generic profile if none was found that matches yet */
      if (!active_set)
        {
          gtk_tree_model_get_iter_first (model, &iter);
          gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
          active_set = TRUE;
        }
    }

  g_signal_connect (widget, "changed", G_CALLBACK (on_profile_box_changed), self);

  /* connect everything related to the privileges table */
  self->hovered_row = FS_PRIV_NONE;
  
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_home_full"));
  if (self->fs_mode & FS_PRIV_FULL)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_full_home_privs_toggled), self);
  g_signal_connect (widget, "enter-notify-event", G_CALLBACK (on_full_home_privs_hovered), self);
  g_signal_connect (widget, "leave-notify-event", G_CALLBACK (on_full_home_privs_hover_leave), self);

  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_home_ro"));
  if (self->fs_mode & FS_PRIV_READ_ONLY)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_ro_home_privs_toggled), self);
  g_signal_connect (widget, "enter-notify-event", G_CALLBACK (on_ro_home_privs_hovered), self);
  g_signal_connect (widget, "leave-notify-event", G_CALLBACK (on_ro_home_privs_hover_leave), self);
  
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_home_priv"));
  if (self->fs_mode & FS_PRIV_PRIVATE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);;
  g_signal_connect (widget, "toggled", G_CALLBACK (on_private_home_privs_toggled), self);
  g_signal_connect (widget, "enter-notify-event", G_CALLBACK (on_priv_home_privs_hovered), self);
  g_signal_connect (widget, "leave-notify-event", G_CALLBACK (on_priv_home_privs_hover_leave), self);

  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_home_full"));
  on_full_home_privs_toggled (GTK_TOGGLE_BUTTON (widget), self);

  /* initialise the disposable FS button */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "fs_disposable_check"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), self->disposable);
  g_signal_connect (widget, "toggled", G_CALLBACK (on_disposable_toggled), self);
  on_disposable_toggled (GTK_TOGGLE_BUTTON (widget), self);

  /* initialize rendering and content for the sync folder list */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "sync_folder_tree_view"));

  column = gtk_tree_view_column_new ();
  render = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, render, FALSE);
  gtk_tree_view_column_set_attributes (column, render, "icon-name", 2, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  column = gtk_tree_view_column_new ();
  render = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, render, TRUE);
  gtk_tree_view_column_set_attributes (column, render, "markup", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  g_signal_connect (widget, "cursor-changed", G_CALLBACK (on_folder_list_selection_changed), self);
  on_folder_list_selection_changed (GTK_TREE_VIEW (widget), self);

  /* connect the buttons for the sync folder list */
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "sync_folder_add_btn"));
  g_signal_connect (widget, "clicked", G_CALLBACK (on_add_btn_clicked), self);
  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "sync_folder_remove_btn"));
  g_signal_connect (widget, "clicked", G_CALLBACK (on_remove_btn_clicked), self);

  /* initialize the list store */
  treeview = GTK_TREE_VIEW (gtk_builder_get_object (self->builder, "sync_folder_tree_view"));
  store = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  for (link = self->sync_folders; link; link = link->next)
    {
      pathname = link->data;
      file = g_file_new_for_path (pathname);
      iconname = xfce_ui_file_get_icon_name (file);
      g_object_unref (file);

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          0, pathname,
                          2, iconname? iconname : "gtk-dialog-error",
                          3, iconname? NULL : INVALID_PATH_TOOLTIP, -1);

      g_free (iconname);
    }
}



static void
_xfce_firejail_widget_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (object);

  switch (property_id)
    {
    case PROP_KEY_FILE:
      _xfce_firejail_widget_set_key_file (self, g_value_get_pointer (value));
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
_xfce_firejail_widget_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (object);

  switch (property_id)
    {
    case PROP_KEY_FILE:
      g_value_set_pointer (value, self->key_file);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
_xfce_firejail_widget_finalize (GObject *object)
{
  XfceFirejailWidget *self = XFCE_FIREJAIL_WIDGET (object);

  g_object_unref (self->builder);
  g_key_file_unref (self->key_file);

  (*G_OBJECT_CLASS (_xfce_firejail_widget_parent_class)->finalize) (object);
}



XfceFirejailWidget *
xfce_firejail_widget_new (GKeyFile *file)
{
  XfceFirejailWidget *self;

  g_return_val_if_fail (file, NULL);

  self = g_object_new (XFCE_TYPE_FIREJAIL_WIDGET, "key-file", file, NULL);

  return self;
}



XfceFirejailWidget *
xfce_firejail_widget_new_from_app_info (GDesktopAppInfo *info)
{
  XfceFirejailWidget *self;
  const gchar *path;
  GKeyFile *file;

  g_return_val_if_fail (G_IS_DESKTOP_APP_INFO (info), NULL);
  g_return_val_if_fail (g_desktop_app_info_get_filename (info), NULL);

  path = g_desktop_app_info_get_filename (info);

  file = g_key_file_new ();
  g_key_file_load_from_file (file, path, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

  self = g_object_new (XFCE_TYPE_FIREJAIL_WIDGET, "key-file", file, NULL);
  g_key_file_unref (file);

  return self;
}



GtkWidget *
xfce_firejail_widget_get_widget (XfceFirejailWidget *self)
{
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), NULL);

  return GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_settings_embed"));
}



GtkWidget *
xfce_firejail_widget_get_run_checkbox (XfceFirejailWidget *self)
{
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), NULL);

  return GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_settings_run"));
}



GtkWidget *
xfce_firejail_widget_get_advanced_button (XfceFirejailWidget *self)
{
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), NULL);

  return GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_advanced_btn"));
}



gboolean
xfce_firejail_widget_get_advanced_button_visible (XfceFirejailWidget *self)
{
  GtkWidget *widget;
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), FALSE);

  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_advanced_alignment"));

  return gtk_widget_get_visible (widget);
}



void
xfce_firejail_widget_set_advanced_button_visible (XfceFirejailWidget *self,
                                                  gboolean            visible)
{
  GtkWidget *widget;
  g_return_if_fail (XFCE_IS_FIREJAIL_WIDGET (self));

  widget = GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_advanced_alignment"));

  gtk_widget_set_visible (widget, visible);
}



GtkWidget *
xfce_firejail_widget_get_advanced_dialog (XfceFirejailWidget *self)
{
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), NULL);

  return GTK_WIDGET (gtk_builder_get_object (self->builder, "firejail_settings_advanced"));
}



GKeyFile *
xfce_firejail_widget_get_key_file (XfceFirejailWidget *self)
{
  g_return_val_if_fail (XFCE_IS_FIREJAIL_WIDGET (self), NULL);
  return self->key_file;
}

#define __XFCE_FIREJAIL_WIDGET_C__
#include <libxfce4ui/libxfce4ui-aliasdef.c>
