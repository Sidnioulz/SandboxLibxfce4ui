/*-
 * Copyright (c) 2003-2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2011      Nick Schermer <nick@xfce.org>
 * Copyright (c) 2016      Steve Dodier-Lazaro <sidi@xfce.org>
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
 *
 * Parts of this file where taken from gnome-session/logout.c, which
 * was written by Owen Taylor <otaylor@redhat.com>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <gtk/gtk.h>

#include <libxfce4ui/xfce-security-dialog.h>
#include <libxfce4ui/xfce-fadeout.h>
#include <libxfce4ui/libxfce4ui-private.h>
#include <libxfce4ui/libxfce4ui-alias.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#endif

#define BORDER   6
#define SHOTSIZE 64


static void       xfce_security_dialog_finalize     (GObject            *object);
static void       xfce_security_dialog_activate     (XfceSecurityDialog *dialog);
static void       xfce_security_dialog_set_property (GObject          *object,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec);
static void       xfce_security_dialog_get_property (GObject          *object,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec);


enum
{
  PROP_GRAB_INPUT = 1,
  PROP_FADEOUT = 2,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };


struct _XfceSecurityDialogClass
{
  GtkDialogClass __parent__;
};

struct _XfceSecurityDialog
{
  GtkDialog         __parent__;

  /* custom widgets on specific areas */
  GtkWidget        *top_widget;
  GtkWidget        *top_widget_container;
  GtkWidget        *bottom_widget;
  GtkWidget        *bottom_widget_container;

  /* misc widgets */
  GtkWidget        *button_cancel;
  GtkWidget        *title_label;
  GtkWidget        *button_vbox;
  GtkWidget        *current_hbox;

  /* properties */
  gboolean          grab_input;
  XfceFadeout      *fadeout;
};



G_DEFINE_TYPE (XfceSecurityDialog, xfce_security_dialog, GTK_TYPE_DIALOG)



static void
xfce_security_dialog_class_init (XfceSecurityDialogClass *klass)
{
  GObjectClass *gobject_class;
  
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_security_dialog_finalize;
  gobject_class->set_property = xfce_security_dialog_set_property;
  gobject_class->get_property = xfce_security_dialog_get_property;

  obj_properties[PROP_GRAB_INPUT] =
    g_param_spec_boolean ("grab-input",
                         "Whether the dialog attempts to grab all input",
                         "Set whether the dialog attempts to grab all input",
                         FALSE,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  obj_properties[PROP_FADEOUT] =
    g_param_spec_pointer ("fadeout",
                         "The XfceFadeout object used as a background to the dialog",
                         "Set the XfceFadeout object used as a background to the dialog",
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}



static void
xfce_security_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  XfceSecurityDialog *self = XFCE_SECURITY_DIALOG (object);

  switch (property_id)
    {
    case PROP_GRAB_INPUT:
      self->grab_input = g_value_get_boolean (value);
      break;

    case PROP_FADEOUT:
      self->fadeout = g_value_get_pointer (value);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
xfce_security_dialog_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  XfceSecurityDialog *self = XFCE_SECURITY_DIALOG (object);

  switch (property_id)
    {
    case PROP_GRAB_INPUT:
      g_value_set_boolean (value, self->grab_input);
      break;

    case PROP_FADEOUT:
      g_value_set_pointer (value, self->fadeout);
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
xfce_security_dialog_init (XfceSecurityDialog *dialog)
{
  PangoAttrList *attrs;
  GtkWidget     *main_vbox;
  GtkWidget     *separator;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
#if !GTK_CHECK_VERSION(2,21,8)
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
#endif
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  /**
   * Cancel button
   **/
  dialog->button_cancel = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                                 "gtk-cancel",
                                                 GTK_RESPONSE_CANCEL);

#if GTK_CHECK_VERSION (3, 0, 0)
  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
#else
  main_vbox = gtk_vbox_new (FALSE, BORDER);
#endif
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), main_vbox);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), BORDER);
  gtk_widget_show (main_vbox);

  /**
   * Dialog title
   **/
  dialog->title_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (dialog->title_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->title_label, FALSE, TRUE, 0);
  gtk_widget_show (dialog->title_label);

  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_LARGE));
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  gtk_label_set_attributes (GTK_LABEL (dialog->title_label), attrs);
  pango_attr_list_unref (attrs);

#if GTK_CHECK_VERSION (3, 0, 0)
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
#else
  separator = gtk_hseparator_new ();
#endif
  gtk_box_pack_start (GTK_BOX (main_vbox), separator, FALSE, TRUE, 0);
  gtk_widget_show (separator);

  /**
   * Custom widget container - top
   **/
#if GTK_CHECK_VERSION (3, 0, 0)
  dialog->top_widget_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
#else
  dialog->top_widget_container = gtk_vbox_new (FALSE, BORDER);
#endif

  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->top_widget_container, FALSE, TRUE, 0);
  gtk_widget_show (dialog->top_widget_container);

  /**
   * Button area
   **/
#if GTK_CHECK_VERSION (3, 0, 0)
  dialog->button_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
#else
  dialog->button_vbox = gtk_vbox_new (FALSE, BORDER);
#endif
  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->button_vbox, FALSE, TRUE, 0);
  gtk_widget_show (dialog->button_vbox);

  /**
   * Custom widget container - bottom
   **/
#if GTK_CHECK_VERSION (3, 0, 0)
  dialog->bottom_widget_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
#else
  dialog->bottom_widget_container = gtk_vbox_new (FALSE, BORDER);
#endif
  gtk_box_pack_start (GTK_BOX (main_vbox), dialog->bottom_widget_container, FALSE, TRUE, 0);
  gtk_widget_show (dialog->bottom_widget_container);
}



static void
xfce_security_dialog_finalize (GObject *object)
{
  XfceSecurityDialog *dialog = XFCE_SECURITY_DIALOG (object);

  if (dialog->fadeout != NULL)
    xfce_fadeout_destroy (dialog->fadeout);

  (*G_OBJECT_CLASS (xfce_security_dialog_parent_class)->finalize) (object);
}



static void
xfce_security_dialog_button_clicked (GtkWidget          *button,
                                     XfceSecurityDialog *dialog)
{
  gint *val;

  val = g_object_get_data (G_OBJECT (button), "response");
  g_assert (val != NULL);

  gtk_dialog_response (GTK_DIALOG (dialog), *val);
}



GtkWidget *
xfce_security_dialog_add_button (XfceSecurityDialog *dialog,
                                 const gchar        *title,
                                 const gchar        *icon_name,
                                 const gchar        *icon_name_fallback,
                                 GtkResponseType     type,
                                 gboolean            new_row,
                                 gboolean            grab_focus)
{
  GtkWidget    *button;
  GtkWidget    *vbox;
  GdkPixbuf    *pixbuf;
  GtkWidget    *image;
  GtkWidget    *label;
  static gint   icon_size = 0;
  gint          w, h;
  gint         *val;
  GtkIconTheme *icon_theme;

  g_return_val_if_fail (XFCE_IS_SECURITY_DIALOG (dialog), NULL);

  val = g_new0 (gint, 1);
  *val = type;

  button = gtk_button_new ();
  g_object_set_data_full (G_OBJECT (button), "response", val, g_free);
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (xfce_security_dialog_button_clicked), dialog);

#if GTK_CHECK_VERSION (3, 0, 0)
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, BORDER);
#else
  vbox = gtk_vbox_new (FALSE, BORDER);
#endif
  gtk_container_set_border_width (GTK_CONTAINER (vbox), BORDER);
  gtk_container_add (GTK_CONTAINER (button), vbox);
  gtk_widget_show (vbox);

  if (G_UNLIKELY (icon_size == 0))
    {
      if (gtk_icon_size_lookup (GTK_ICON_SIZE_DND, &w, &h))
        icon_size = MAX (w, h);
      else
        icon_size = 32;
    }

  icon_theme = gtk_icon_theme_get_for_screen (gtk_window_get_screen (GTK_WINDOW (dialog)));
  pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name, icon_size, 0, NULL);
  if (G_UNLIKELY (pixbuf == NULL))
    {
      pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name_fallback,
                                         icon_size, GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                         NULL);
    }

  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  label = gtk_label_new_with_mnemonic (title);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (new_row || dialog->current_hbox == NULL)
    {
#if GTK_CHECK_VERSION (3, 0, 0)
      dialog->current_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, BORDER);
#else
      dialog->current_hbox = gtk_hbox_new (TRUE, BORDER);
#endif
      gtk_box_pack_start (GTK_BOX (dialog->button_vbox), dialog->current_hbox, FALSE, TRUE, 0);
      gtk_widget_show_all (dialog->current_hbox);
    }

  gtk_box_pack_start (GTK_BOX (dialog->current_hbox), button, TRUE, TRUE, 0);
  gtk_widget_show_all (button);

  if (grab_focus)
    gtk_widget_grab_focus (button);

  return button;
}



static void
xfce_security_dialog_activate (XfceSecurityDialog *dialog)
{
  g_return_if_fail (XFCE_IS_SECURITY_DIALOG (dialog));
  gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
}



gint
xfce_security_dialog_run (GtkDialog *dialog)
{
  GdkWindow          *window;
  gint                ret;
  XfceSecurityDialog *scdlg;

  g_return_val_if_fail (XFCE_IS_SECURITY_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  scdlg = XFCE_SECURITY_DIALOG (dialog);

  if (scdlg->grab_input)
    {
      gtk_widget_show_now (GTK_WIDGET (dialog));

      window = gtk_widget_get_window (GTK_WIDGET (dialog));
      if (gdk_keyboard_grab (window, FALSE, GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
        g_critical ("Failed to grab the keyboard for logout window");

#ifdef GDK_WINDOWING_X11
      /* force input to the dialog */
      gdk_error_trap_push ();
      XSetInputFocus (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                      GDK_WINDOW_XID (window),
                      RevertToParent, CurrentTime);
      gdk_error_trap_pop ();
#endif
    }

  g_object_ref (dialog);
  ret = gtk_dialog_run (dialog);

  if (scdlg->grab_input)
    gdk_keyboard_ungrab (GDK_CURRENT_TIME);
  g_object_unref (dialog);

  return ret;
}



void
xfce_security_dialog_set_title (XfceSecurityDialog *dialog,
                                const gchar        *title)
{
  g_return_if_fail (XFCE_IS_SECURITY_DIALOG (dialog));
  gtk_label_set_markup (GTK_LABEL (dialog->title_label), title);
}



static void
xfce_security_dialog_set_custom_widget (XfceSecurityDialog *dialog,
                                        GtkWidget          *container,
                                        GtkWidget          *widget)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (container));
  GList *iter;

  for (iter = children; iter; iter = iter->next)
      gtk_container_remove (GTK_CONTAINER (container), iter->data);

  gtk_box_pack_start (GTK_BOX (container), widget, TRUE, TRUE, 0);
}



void
xfce_security_dialog_set_top_widget (XfceSecurityDialog *dialog,
                                     GtkWidget          *widget)
{
  g_return_if_fail (XFCE_IS_SECURITY_DIALOG (dialog));
  xfce_security_dialog_set_custom_widget (dialog, dialog->top_widget_container, widget);
}



void
xfce_security_dialog_set_bottom_widget (XfceSecurityDialog *dialog,
                                        GtkWidget          *widget)
{
  g_return_if_fail (XFCE_IS_SECURITY_DIALOG (dialog));
  xfce_security_dialog_set_custom_widget (dialog, dialog->bottom_widget_container, widget);
}



static void
xfce_security_dialog_add_border (GtkWindow *window)
{
  GtkWidget *box1, *box2;
  GtkStyle *style;

  gtk_widget_realize(GTK_WIDGET(window));

  style = gtk_widget_get_style (GTK_WIDGET(window));

  box1 = gtk_event_box_new ();
  gtk_widget_modify_bg (box1, GTK_STATE_NORMAL, &style->bg[GTK_STATE_SELECTED]);
  gtk_widget_show (box1);

  box2 = gtk_event_box_new ();
  gtk_widget_show (box2);
  gtk_container_add (GTK_CONTAINER (box1), box2);

  gtk_container_set_border_width (GTK_CONTAINER (box2), 6);
  gtk_widget_reparent (gtk_bin_get_child (GTK_BIN (window)), box2);

  gtk_container_add (GTK_CONTAINER (window), box1);
}



GtkWidget *
xfce_security_dialog_new (GdkScreen        *screen,
                          GtkWindow        *parent,
                          const gchar      *title)
{
  GtkWidget        *hidden;
  gboolean          a11y;
  GtkWidget        *dialog;
  XfceFadeout      *fadeout;

  if (G_UNLIKELY (screen == NULL))
      screen = gdk_screen_get_default ();

  /* check if accessibility is enabled */
  hidden = gtk_invisible_new_for_screen (screen);
  gtk_widget_show (hidden);
  a11y = GTK_IS_ACCESSIBLE (gtk_widget_get_accessible (hidden));

  if (G_LIKELY (!a11y))
    {
      /* wait until we can grab the keyboard, we need this for
       * the dialog when running it */
      for (;;)
        {
          if (gdk_keyboard_grab (gtk_widget_get_window (hidden), FALSE,
                                 GDK_CURRENT_TIME) == GDK_GRAB_SUCCESS)
            {
              gdk_keyboard_ungrab (GDK_CURRENT_TIME);
              break;
            }

          g_usleep (G_USEC_PER_SEC / 20);
        }

      /* display fadeout */
      fadeout = xfce_fadeout_new (gdk_screen_get_display (screen));
      gdk_flush ();

      dialog = g_object_new (XFCE_TYPE_SECURITY_DIALOG,
                             "type", GTK_WINDOW_POPUP,
                             "screen", screen,
                             "grab-input", !a11y,
                             "fadeout", fadeout, NULL);

      xfce_security_dialog_add_border (GTK_WINDOW (dialog));

      gtk_widget_realize (dialog);
      gdk_window_set_override_redirect (gtk_widget_get_window (dialog), TRUE);
      gdk_window_raise (gtk_widget_get_window (dialog));
    }
  else
    {
      dialog = g_object_new (XFCE_TYPE_SECURITY_DIALOG,
                             "decorated", !a11y,
                             "grab-input", !a11y,
                             "screen", screen, NULL);

      gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
      atk_object_set_role (gtk_widget_get_accessible (dialog), ATK_ROLE_ALERT);
    }

  gtk_widget_destroy (hidden);

  if (title)
    xfce_security_dialog_set_title (XFCE_SECURITY_DIALOG (dialog), title);

  return dialog;
}


#define __XFCE_SECURITY_DIALOG_C__
#include <libxfce4ui/libxfce4ui-aliasdef.c>
