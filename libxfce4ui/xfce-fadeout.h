/* $Id$ */
/*-
 * Copyright (c) 2004 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2016 Steve Dodier-Lazaro <sidi@xfce.org>
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

#ifndef __XFCE_FADEOUT_H__
#define __XFCE_FADEOUT_H__

#include <gdk/gdk.h>


G_BEGIN_DECLS;

typedef struct _XfceFadeout XfceFadeout;

XfceFadeout *xfce_fadeout_new     (GdkDisplay  *display);
void         xfce_fadeout_clear   (XfceFadeout *fadeout);
void         xfce_fadeout_destroy (XfceFadeout *fadeout);

G_END_DECLS;


#endif /* !__XFCE_FADEOUT_H__ */
