// ========================================================================
// gnubiff -- a mail notification program
// Copyright (c) 2000-2011 Nicolas Rougier, 2004-2011 Robert Sowada
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

#include <glib.h>
#include <gtk/gtk.h>

#include "ui-applet-systray.h"
#include "support.h"
#include "biff.h"

// ============================================================================
//  base
// ============================================================================

/**
 *  Constructor.
 *
 *  @param  biff  Pointer to gnubiff's biff
 */
AppletSystray::AppletSystray (Biff *biff) : AppletGtk (biff, this)
{
	// Create the system tray icon
  trayicon_ = gtk_status_icon_new();
 	str_trayicon_image_="";
	str_trayicon_text_="";
	pixbuf_trayicon_image_=NULL;
	is_show_text_=false;
	widget_max_width_=0;
	widget_max_height_=0;

	// Connect signals to system tray icon
  g_signal_connect(G_OBJECT(trayicon_), "button-press-event",
       G_CALLBACK(on_trayicon_btn_press), this);
  g_signal_connect (trayicon_, "notify::size",
                      G_CALLBACK (signal_size_allocate), this);

	// Show the tray icon
	gtk_status_icon_set_visible(trayicon_, TRUE);
}

/// Destructor.
AppletSystray::~AppletSystray (void)
{
}

// ============================================================================
//  tools
// ============================================================================
/**
 *  Return the panel's orientation.
 *
 *  @param  orient Panel's orientation
 *  @return        Boolean indicating success
 */
gboolean 
AppletSystray::get_orientation (GtkOrientation &orient)
{
	return get_orientation (orient);
}

// ============================================================================
//  main
// ============================================================================

/**
 *  Show the applet. Also the applet's appearance is updated.
 *
 *  @param  name  This parameter is ignored (the default is "dialog").
 */
void 
AppletSystray::show (std::string name)
{
	gtk_status_icon_set_visible(trayicon_, TRUE);
}

/**
 *  This function is called automatically when the system tray icon is resized.
 *
 *  @param  width  New width of the icon.
 *  @param  height New height of the icon.
 */
void 
AppletSystray::resize (void)/*(guint width, guint height)*/
{
	// Do we need to have the image rescaled?
	guint width=gtk_status_icon_get_size(trayicon_);
	guint height=width;
	if ((width != widget_max_width_) || (height != widget_max_height_)) {
 		widget_max_width_ = width;
		widget_max_height_ = height;
		update (); 
	}
}

// ============================================================================
//  callbacks
// ============================================================================
/**
 *  Callback function that is called when the size of the system tray icon
 *  is changed. This function calls
 *
 *  @param  widget     System tray icon
 *  @param  allocation Position and size of {\em widget}
 *  @param  user_data  Pointer to the corresponding AppletSystray object
 */
void
AppletSystray::signal_size_allocate (GObject *gobject, 
                    GParamSpec *arg1, 
                    gpointer data)
{
	if (data)
		(static_cast<AppletSystray *>(data))->resize ( );
	else
		unknown_internal_error ();
}

gboolean 
AppletSystray::on_trayicon_btn_press(GtkStatusIcon  * trayicon, 
										GdkEventButton * event, 
										gpointer data)
{
	if (data)
		return  (static_cast<AppletSystray *>(data)) -> on_button_press(event);
	else 
		return false;
}

// ============================================================================
//  main
// ============================================================================

/**
 *  Update applet's status and appearance.
 *
 *  @param  init  True if called when initializing gnubiff (the default is
 *                false)
 *  @return       True if new messages are present
 */
gboolean
AppletSystray::update (gboolean init)
{
  // Is there another update going on?
  if (!g_mutex_trylock (update_mutex_))
    return false;

  gboolean newmail = update (init, "image", "unread", "fixed");

  tooltip_update ();
  show ();

  g_mutex_unlock (update_mutex_);
  return newmail;
}


/**
 *  Update the applet status. This includes showing the
 *  image/animation that corresponds to the current status of gnubiff
 *  (no new messages or new messages are present). Also the text with
 *  the current number of new messages is updated. If present a
 *  container widget that contains the widgets for the image and the
 *  text may be updated too. The status of the popup window is updated
 *  (this is not done when this function is called during the
 *  initialization of gnubiff).
 *
 *  @param  init             True if called when initializing gnubiff (the
 *                           default is false).
 *  @param  widget_image     Name of the widget that contains the image for
 *                           gnubiff's status or the empty string if
 *                           no image shall * be updated. The default
 *                           is the empty string.
 *  @param  widget_text      Name of the widget that contains the text for
 *                           gnubiff's status or the empty string if
 *                           no text shall be updated. The default is
 *                           the empty string.
 *  @param  widget_container Name of the widget that contains the image and
 *                           text widget. If it's an empty string the container
 *                           widget (if present) will not be updated. The
 *                           default is the empty string.
 *  @return                  True if new messages are present
 */
gboolean 
AppletSystray::update (gboolean init, std::string widget_image,
				   std::string widget_text, std::string widget_container)
{
	GError *err=NULL;

	// Update applet's status: GUI-independent things to do
	gboolean newmail = Applet::update (init);

	// Get number of unread messages
	guint unread;
	biff_->get_number_of_unread_messages (unread);

	// Update popup

	if (!init && (popup_)) {
		// If there are no mails to display then hide popup
		if (!unread && (biff_->value_bool ("use_popup") || force_popup_))
			hide_dialog_popup ();

		// Update and display the popup
		if (unread && ((biff_->value_bool ("use_popup")) || force_popup_)
			&& (newmail || visible_dialog_popup () || force_popup_))
			show_dialog_popup ();
	}

		if ( widget_max_width_ <= 0 ) {
			//do not proceed if "resize" does not yet happen.
			return newmail;
		}

	// Update trayicon's image
	// only update the image when it really needs to.
	std::string image="";
	std::string text="";
	if (widget_image != "") 
	{
		// Determine image
		if ((unread == 0) && biff_->value_bool ("use_nomail_image"))
			image = biff_->value_string ("nomail_image");
		if ((unread >  0) && biff_->value_bool ("use_newmail_image"))
			image = biff_->value_string ("newmail_image");
	}
	// Update applet's text
	if (widget_text != "") {
		text = get_number_of_unread_messages ();
	}

	gboolean is_image_changed = ( str_trayicon_image_ != image);
	gboolean is_text_changed = ( str_trayicon_text_ != text);
	gboolean is_set_icon = false;
	gboolean is_use_text = (((unread == 0) && biff_->value_bool ("use_nomail_text")) ||
       											 ((unread >  0) && biff_->value_bool ("use_newmail_text")) );

	if( ( is_image_changed ||  (is_show_text_!=is_use_text)) && !is_use_text ) {
			// cases that we need not to draw text:
			// a)icon image is changed  OR b) icon image unchanged but it is changed from text to non-text 
			
			//free previous image
			if(pixbuf_trayicon_image_) {
				g_object_unref(pixbuf_trayicon_image_);
				pixbuf_trayicon_image_=NULL;
			}

			//load current image
			pixbuf_trayicon_image_=gdk_pixbuf_new_from_file(image.c_str(), &err);

			//If image is smaller than (widget_max_width_,widget_max_height_), zoom the image up.
			//Otherwise, GtkStatusIcon will act oddly when we set a small icon after it displays a largel icon (probably a bug of GtkStatusIcon).
			if( pixbuf_trayicon_image_ && 
					(gdk_pixbuf_get_width(pixbuf_trayicon_image_) < (int)widget_max_width_ || gdk_pixbuf_get_height(pixbuf_trayicon_image_) < (int)widget_max_height_)){
				GdkPixbuf * pbtmp = gdk_pixbuf_scale_simple ( pixbuf_trayicon_image_, widget_max_width_, widget_max_height_, GDK_INTERP_BILINEAR );
				g_object_unref(pixbuf_trayicon_image_);
				pixbuf_trayicon_image_=pbtmp;
			}
			str_trayicon_image_=image;
			str_trayicon_text_=text;
			is_show_text_ = is_use_text;
			is_set_icon=true;	
	} else { 
		//cases that we need to draw text on trayicon
		if ( is_text_changed || ( (is_image_changed || (is_show_text_!=is_use_text) ) && is_use_text) ) {
	      if(pixbuf_trayicon_image_) {
	        g_object_unref(pixbuf_trayicon_image_);
  	      pixbuf_trayicon_image_=NULL;
				}
    	  pixbuf_trayicon_image_= load_image_and_add_text(image.c_str(), (gchar*)text.c_str(), widget_max_width_, widget_max_height_);
      	str_trayicon_image_=image;
        str_trayicon_text_=text;
        is_set_icon=true;
				is_show_text_ = is_use_text;
		}
	}

	if (pixbuf_trayicon_image_ && is_set_icon)
	{
		gtk_status_icon_set_from_pixbuf(trayicon_, NULL);
		gtk_status_icon_set_from_pixbuf(trayicon_, pixbuf_trayicon_image_);
		/*
		if (unread >  0)
			gtk_status_icon_set_blinking(trayicon_,true);
		else
			gtk_status_icon_set_blinking(trayicon_,false);
		*/
	}

	// Reset the force popup boolean
	force_popup_ = false;
	return newmail;
}


GdkPixbuf *
AppletSystray::load_image_and_add_text( const gchar *filename, const gchar *markup , guint maxw, guint maxh)
{
    cairo_surface_t *surface;
		cairo_surface_t *surface_img;
    cairo_t         *cr;
    GdkPixbuf       *pixbuf;
    PangoLayout     *layout;
    gint             i_width, i_width_img,
                     i_height, i_height_img,
                     l_width,
                     l_height,
                     dx,
                     dy,
                     s_stride,
                     p_stride,
                     i,
                     j;
    guchar          *s_data,
                    *p_data;
    PangoFontDescription *desc;

   
		surface= cairo_image_surface_create ( CAIRO_FORMAT_ARGB32 , maxw, maxh);
		//surface= cairo_surface_create_similar ( surface_img, CAIRO_CONTENT_COLOR_ALPHA, (double)widget_max_width_, (double)widget_max_height_);
	  //cr = cairo_create( surface );

		i_width=widget_max_width_;
		i_height=widget_max_height_;

		cr = cairo_create( surface );

		//paint background image
    surface_img = cairo_image_surface_create_from_png( filename );
		if (cairo_surface_status(surface_img) == CAIRO_STATUS_SUCCESS) {
    	i_width_img  = cairo_image_surface_get_width( surface_img );
    	i_height_img = cairo_image_surface_get_height( surface_img );
 
			cairo_save(cr);
			cairo_scale(cr, ((double)maxw)/i_width_img, ((double)maxh)/i_height_img);
			cairo_set_source_surface(cr, surface_img, 0, 0);
			cairo_paint(cr);
			cairo_restore (cr);
		}

	  cairo_surface_destroy( surface_img );

		//paint text	
		if ( markup != "" ) {
    	layout = pango_cairo_create_layout( cr );
/*  	pango_layout_set_text( layout, markup, -1 );
    	desc = pango_font_description_from_string( "Sans 14" );
    	pango_layout_set_font_description( layout, desc );
    	pango_font_description_free( desc );
*/
			pango_layout_set_markup(layout, markup, -1);

    /* Center text */
    	pango_layout_get_size( layout, &l_width, &l_height );
    	l_width  /= PANGO_SCALE;
	    l_height /= PANGO_SCALE;


	    dx = ( i_width - l_width ) * .5;
    	dy = ( i_height - l_height ) * .5;

  	  cairo_move_to( cr, dx, dy );

		//pango_cairo_update_layout(cr, layout);
	    pango_cairo_show_layout( cr, layout );
  	  cairo_fill( cr );

	    g_object_unref( layout );

		}

    cairo_destroy( cr );

    /* Convert cairo surface to pixbuf */
    pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, TRUE, 8, i_width, i_height );
    s_stride = cairo_image_surface_get_stride( surface );
    p_stride = gdk_pixbuf_get_rowstride( pixbuf );
    s_data = cairo_image_surface_get_data( surface );
    p_data = gdk_pixbuf_get_pixels( pixbuf );

    for( i = 0; i < i_height; i++ )
    {
        for( j = 0; j < i_width; j++ )
        {
            gint    s_index = i * s_stride + j * 4,
                    p_index = i * p_stride + j * 4;
            gdouble alpha;

            alpha = s_data[s_index + 3] ?
                        (gdouble)s_data[s_index + 3] / 0xff : 1.0;

            p_data[p_index    ] = s_data[s_index + 2] / alpha;
            p_data[p_index + 1] = s_data[s_index + 1] / alpha;
            p_data[p_index + 2] = s_data[s_index    ] / alpha;
            p_data[p_index + 3] = s_data[s_index + 3];
        }
    }

    cairo_surface_destroy( surface );

    return( pixbuf );
}

void
AppletSystray::tooltip_update (void)
{

// Get text for tooltip
  std::string text = get_mailbox_status_text ();
	gtk_status_icon_set_tooltip_text(trayicon_,text.c_str());
}


