// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2007 Nicolas Rougier, 2004-2007 Robert Sowada
//
// This program is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ========================================================================
//
// File          : $RCSfile$
// Revision      : $Revision$
// Revision date : $Date$
// Author(s)     : Nicolas Rougier, Robert Sowada
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#ifndef __APPLET_SYSTRAY_H__
#define __APPLET_SYSTRAY_H__

#include <gtk/gtk.h>

#include "ui-applet-gtk.h"


class AppletSystray : public AppletGtk {
 private:
	/// Icon in the system tray
	GtkStatusIcon *trayicon_;
	std::string str_trayicon_image_;
	std::string str_trayicon_text_;
	GdkPixbuf * pixbuf_trayicon_image_;
	gboolean is_show_text_;

	GdkPixbuf * load_image_and_add_text( const gchar *filename,
                        	const gchar *markup ,
													guint maxw,
													guint maxh );

 public:
	// ========================================================================
	//  base
	// ========================================================================
	AppletSystray (class Biff *biff);
	~AppletSystray (void);

	// ========================================================================
	//  tools
	// ========================================================================
	gboolean get_orientation (GtkOrientation &orient);

	// ========================================================================
	//  main
	// ========================================================================
  gboolean update (gboolean init = false);
  gboolean update (gboolean init,
               std::string widget_image = "",
               std::string widget_text = "",
               std::string widget_container = "");
//	gboolean update (gboolean init = false);
	void show (std::string name = "dialog");
	void resize (void);//(guint width, guint height);

	// ========================================================================
	//  callbacks
	// ========================================================================
/*	static void signal_size_allocate (GtkWidget *widget,
									  GtkAllocation *allocation,
									  gpointer data);*/
	static void signal_size_allocate (GObject *gobject, 
                    GParamSpec *arg1, 
                    gpointer data);
	static gboolean on_trayicon_btn_press(GtkStatusIcon  *, 
										GdkEventButton *, 
										gpointer );
	void tooltip_update (void);
};

#endif
