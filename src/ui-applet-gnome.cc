// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2005 Nicolas Rougier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
// ========================================================================
//
// File          : $RCSfile$
// Revision      : $Revision$
// Revision date : $Date$
// Author(s)     : Nicolas Rougier
// Short         : 
//
// This file is part of gnubiff.
//
// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
// ========================================================================

#include "support.h"

#include <sstream>
#include <cstdio>

#include "ui-applet-gnome.h"
#include "ui-preferences.h"
#include "ui-popup.h"
#include "mailbox.h"
#include "gtk_image_animation.h"



/**
 * "C" binding
 **/
extern "C" {
	void APPLET_GNOME_on_enter (GtkWidget *widget,
								GdkEventCrossing *event,
								gpointer data)
	{
		if (data)
			((AppletGnome *) data)->tooltip_update ();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_change_orient (GtkWidget *widget,
										PanelAppletOrient orient,
										gpointer data)
	{
		if (data)
			((AppletGnome *) data)->update();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_change_size (GtkWidget *widget,
									  int size,
									  gpointer data)
	{
		if (data)
			((AppletGnome *) data)->update ();
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_change_background (GtkWidget *widget,
											PanelAppletBackgroundType type,
											GdkColor *color,
											GdkPixmap *pixmap,
											gpointer data)
	{
		if (data)
			((AppletGnome *) data)->update ();
		else
			unknown_internal_error ();
	}

	gboolean APPLET_GNOME_on_button_press (GtkWidget *widget,
										   GdkEventButton *event,
										   gpointer data)
	{
		if (data)
			return ((AppletGnome *) data)->on_button_press (event);
		else
			unknown_internal_error ();
		return false;
	}

	void APPLET_GNOME_on_menu_properties (BonoboUIComponent *uic,
										  gpointer data,
										  const gchar *verbname)
	{
		if (data)
			((AppletGnome *) data)->on_menu_properties (uic, verbname);
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_menu_command (BonoboUIComponent *uic,
									   gpointer data,
									   const gchar *verbname)
	{
		if (data)
			((AppletGnome *) data)->on_menu_command (uic, verbname);
		else
			unknown_internal_error ();
	}

	void APPLET_GNOME_on_menu_mail_read (BonoboUIComponent *uic,
										 gpointer data,
										 const gchar *verbname)
	{
		if (data)
			((AppletGnome *) data)->on_menu_mail_read (uic, verbname);
		else
			unknown_internal_error ();
	}

	gboolean APPLET_GNOME_reconnect (gpointer data)
	{
		if (data) {
			g_signal_connect (G_OBJECT (((AppletGnome *)data)->panelapplet()),
							  "change_background",
							  GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_background),
							  data);
		}
		else
			unknown_internal_error ();
		return false;
	}
}


AppletGnome::AppletGnome (Biff *biff) : Applet (biff, GNUBIFF_DATADIR"/applet-gnome.glade")
{
}

AppletGnome::~AppletGnome (void)
{
}


void
AppletGnome::dock (GtkWidget *applet)
{
	static const BonoboUIVerb gnubiffMenuVerbs [] = {
		BONOBO_UI_VERB ("Props",   APPLET_GNOME_on_menu_properties),
		BONOBO_UI_VERB ("MailApp", APPLET_GNOME_on_menu_command),
		BONOBO_UI_VERB ("MailRead", APPLET_GNOME_on_menu_mail_read),
		BONOBO_UI_VERB_END
	};
 
	panel_applet_setup_menu_from_file (PANEL_APPLET (applet),
									   NULL,
									   "GNOME_gnubiffApplet.xml",
									   NULL,
									   gnubiffMenuVerbs,
									   this);
	gtk_widget_reparent (get ("table"), applet);  

	gtk_container_set_border_width (GTK_CONTAINER (applet), 0);
	GtkTooltips *applet_tips = gtk_tooltips_new ();
	gtk_tooltips_set_tip (applet_tips, applet, "", "");

	GtkImageAnimation *anim = new GtkImageAnimation (GTK_IMAGE(get("image")));
	g_object_set_data (G_OBJECT(get("image")), "_animation_", anim);
	anim->open (biff_->value_string ("newmail_image"));
	anim->start();

	g_signal_connect (G_OBJECT (applet), "enter_notify_event",  GTK_SIGNAL_FUNC (APPLET_GNOME_on_enter), this);
	g_signal_connect (G_OBJECT (applet), "change_orient",       GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_orient), this);
	g_signal_connect (G_OBJECT (applet), "change_size",         GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_size), this);
	g_signal_connect (G_OBJECT (applet), "change_background",	GTK_SIGNAL_FUNC (APPLET_GNOME_on_change_background), this);
	g_signal_connect (G_OBJECT (applet), "button_press_event",	GTK_SIGNAL_FUNC (APPLET_GNOME_on_button_press), this);

	applet_ = applet;
	return;
}

void
AppletGnome::update (gboolean no_popup)
{
	// Is there another update going on ?
	if (!g_mutex_trylock (update_mutex_))
		return;

	Applet::update (no_popup);

	std::string text;
	guint unread = unread_markup (text);
	gtk_label_set_markup (GTK_LABEL(get ("hunread")), text.c_str());
	gtk_label_set_markup (GTK_LABEL(get ("vunread")), text.c_str());


	GtkImageAnimation *anim = (GtkImageAnimation *) g_object_get_data (G_OBJECT(get("image")), "_animation_");
	// Pick image/animation
	if (unread > 0) {
		if (!biff_->value_bool ("use_newmail_image")) {
			gtk_widget_hide (get("image"));
		}
		else {
			gtk_widget_show (get("image"));
			anim->open (biff_->value_string ("newmail_image"));
		}
	}
	else {
		if (!biff_->value_bool ("use_nomail_image")) {
			gtk_widget_hide (get("image"));
		}
		else {
			gtk_widget_show (get("image"));
			anim->open (biff_->value_string ("nomail_image"));
		}
	}

	guint size = panel_applet_get_size (PANEL_APPLET (applet_));
	PanelAppletOrient orient = panel_applet_get_orient (PANEL_APPLET (applet_));
	// The panel is oriented horizontally
	if ((orient == PANEL_APPLET_ORIENT_DOWN) || (orient == PANEL_APPLET_ORIENT_UP)) {
		gtk_widget_hide (get ("vunread"));
		if (((unread > 0) && (biff_->value_bool ("use_newmail_text")))
			|| ((unread == 0) && (biff_->value_bool ("use_nomail_text"))))
			gtk_widget_show (get ("hunread"));
		else
			gtk_widget_hide (get ("hunread"));
		if (anim->scaled_height() != size) {
			anim->stop ();
			anim->resize (anim->scaled_width()*size/anim->scaled_height(), size);
		}
	}
	// The panel is oriented vertically
	else {
		gtk_widget_hide (get ("hunread"));
		if (((unread > 0) && (biff_->value_bool ("use_newmail_text")))
			|| ((unread == 0) && (biff_->value_bool ("use_nomail_text"))))
			gtk_widget_show (get ("vunread"));
		else
			gtk_widget_hide (get ("vunread"));
		if (anim->scaled_width() > size) {
			anim->stop ();
			anim->resize (size, anim->scaled_height()*size/anim->scaled_width());
		}
	}


	// Background
	PanelAppletBackgroundType type;
	GdkColor color;
	GdkPixmap *pixmap = NULL;
	type = panel_applet_get_background (PANEL_APPLET(applet_), &color, &pixmap);
	if (pixmap && G_IS_OBJECT(pixmap)) {		
		GtkStyle* style = gtk_style_copy (gtk_widget_get_style (applet_));
		style->bg_pixmap[0] = pixmap;
		gtk_widget_set_style (applet_, style);
		gtk_widget_set_style (get("table"), style);
		g_object_unref (style);
	}
	else {
		if (type == PANEL_NO_BACKGROUND) {
			GtkRcStyle *rc_style = gtk_rc_style_new ();
			gtk_widget_modify_style (applet_, rc_style);
			gtk_rc_style_unref (rc_style);
		}
		else {
			gtk_widget_modify_bg (get("applet_"), GTK_STATE_NORMAL, &color);
		}
	}


	if (GTK_WIDGET_VISIBLE(get("image")))
		anim->start ();
	g_mutex_unlock (update_mutex_);
}


void
AppletGnome::show (std::string name)
{
	gtk_widget_show (applet_);
}

void
AppletGnome::hide (std::string name)
{
	gtk_widget_hide (applet_);
}

void
AppletGnome::tooltip_update (void)
{
	std::string text = tooltip_text ();
	GtkTooltipsData *data = gtk_tooltips_data_get (applet_);
	gtk_tooltips_set_tip (data->tooltips, applet_, text.c_str(), "");
}

gboolean
AppletGnome::on_button_press (GdkEventButton *event)
{
	// Double left click : start mail app
	if ((event->type == GDK_2BUTTON_PRESS) && (event->button == 1)) {
		if ((biff_->value_bool ("use_double_command")) &&
			 (!biff_->value_string ("double_command").empty ())) {
			std::string command = biff_->value_string ("double_command")+" &";
			system (command.c_str());
		}
	}
	// Single left click : popup menu
	else if (event->button == 1) {
		force_popup_ = true;
		update ();
	}
	// Single middle click : mark mails as read
	else if (event->button == 2) {
		for (unsigned int i=0; i<biff_->size(); i++)
			biff_->mailbox(i)->read();
		force_popup_ = true;
		biff_->popup()->hide();
		update();
	}

	return FALSE;
}

void
AppletGnome::on_menu_properties (BonoboUIComponent *uic, const gchar *verbname)
{
	biff_->popup()->hide();
	biff_->preferences()->show();
}

void
AppletGnome::on_menu_command (BonoboUIComponent *uic, const gchar *verbname)
{
	if ((biff_->value_bool ("use_double_command")) &&
		 (!biff_->value_string ("double_command").empty ())) {
		std::string command = biff_->value_string ("double_command") + " &";
		system (command.c_str());
	}
}

void
AppletGnome::on_menu_mail_read (BonoboUIComponent *uic, const gchar *verbname)
{
	for (unsigned int i=0; i<biff_->size(); i++)
		biff_->mailbox(i)->read();
	force_popup_ = true;
	biff_->popup()->hide();
	update();
}

