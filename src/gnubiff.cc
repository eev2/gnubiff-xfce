/* gnubiff -- a mail notification program
 * Copyright (c) 2000-2004 Nicolas Rougier
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This file is part of gnubiff.
 */
#ifdef HAVE_CONFIG_H
#  include <../config.h>
#endif
#include <gtk/gtk.h>
#include <popt.h>
#include "Biff.h"

#ifdef USE_GNOME
#  include <gnome.h>
#  include <panel-applet.h>
#  include <panel-applet-gconf.h>
#endif


int main (int argc, char **argv);
int mainGTK (int argc, char **argv);
int mainGNOME (int argc, char **argv);

int main (int argc, char **argv) {
#ifdef USE_GNOME
	mainGNOME (argc, argv);
#else
	mainGTK (argc, argv);
#endif  
}

int mainGTK (int argc, char **argv) {
	poptContext poptcon;
	int status;
	char *config_file = 0;
	int no_configure = false;
	struct poptOption options[] = {
		{"config",     'c', POPT_ARG_STRING, &config_file,  0, _("configuration file to use"),  _("file")},
		{"noconfigure",'n', POPT_ARG_NONE,   &no_configure, 0, _("skip the configure process"), NULL},
		{"gtk", 'g', POPT_ARG_NONE, 0, 0, 0, NULL},
		POPT_AUTOHELP
		{NULL, '\0', 0, NULL, 0, NULL, NULL}
	};

	// Initialize i18n support
	setlocale (LC_ALL, "");
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNUBIFF_LOCALEDIR);
#    ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#    endif
	textdomain (PACKAGE_NAME);
#endif

	// Parse command line
	poptcon = poptGetContext ("gnubiff", argc,  (const char **) argv, options, 0);
	while ((status = poptGetNextOpt(poptcon)) >= 0);
	if (status < -1) {
		fprintf (stderr, "%s: %s\n\n", poptBadOption(poptcon, POPT_BADOPTION_NOALIAS),  poptStrerror(status));
		poptPrintHelp (poptcon, stderr, 0);
		exit (1);
	}
	poptGetNextOpt(poptcon);

#ifndef USE_GNOME
	// Thread initialization
	g_thread_init (NULL);
	gdk_threads_init ();
#endif

	// GTK inialisation
	gtk_init (&argc, &argv);

	// Create biff with configuration file (or not)
	Biff *biff;
	if (config_file)
		biff = new Biff(GTK_MODE, config_file);
	else
		biff = new Biff(GTK_MODE);

	// Show setup panel or start gnubiff directly
	if (no_configure) {
		biff->applet()->update();
		biff->applet()->show();
		if (biff->_state == STATE_RUNNING)
			biff->applet()->watch_now();
	}
	else
		biff->setup()->show();

	// GTK main loop
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	// Exit
	return 0;
}


#ifdef USE_GNOME
static gboolean gnubiff_applet_factory (PanelApplet *applet, const gchar *iid, gpointer data) {

	if (!strcmp (iid, "OAFIID:GNOME_gnubiffApplet")) {
#ifdef DEBUG
		g_message ("Creating Biff structure");
#endif
		Biff *biff = new Biff (GNOME_MODE);

#ifdef DEBUG
		g_message ("Docking applet");
#endif
		biff->applet()->dock ((GtkWidget *) applet);

#ifdef DEBUG
		g_message ("Load configuration file");
#endif
		biff->load();

#ifdef DEBUG
		g_message ("Update setup panel");
#endif
		biff->setup()->update();

#ifdef DEBUG
		g_message ("Show applet");
#endif
		biff->applet()->show ();

#ifdef DEBUG
		g_message ("Update applet");
#endif
		biff->applet()->update ();

		if (biff->_state == STATE_RUNNING)
			biff->applet()->watch_now ();
	}
	return TRUE;
}

int mainGNOME (int argc, char **argv) {
	// Initialize i18n support
	setlocale (LC_ALL, "");
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNUBIFF_LOCALEDIR);
#   ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#   endif
	textdomain (GETTEXT_PACKAGE);
#endif

	// Thread initialization
	g_thread_init (NULL);
	gdk_threads_init ();

	// Look for a possible --gtk option
	for (gint i=0; i<argc; i++)
		if ((std::string(argv[i]) == "--gtk") || (std::string(argv[i]) == "-g")) {
			mainGTK (argc, argv);
			exit (0);
		}
		else if (std::string(argv[i]) == "--version") {
			g_print ("gnubiff version "PACKAGE_VERSION"\n");
			exit (0);
		}
		else if (std::string(argv[i]) == "--help") {
			g_print (_("\nThis version of gnubiff has been compiled with GNOME support\n"));
			g_print (_("If you want to use the GTK version, type gnubiff --gtk\n"));
			g_print (_(" then use -c file to specify an alternate configuration file\n"));
			g_print (_("      and -n to skip configuration process\n"));
			g_print (_("If you want to use the GNOME version, use gnome panel\n"));
			g_print (_("Have a nice day\n"));
			exit (0);
		}

	g_warning (_("\nThis version of gnubiff has been compiled with GNOME support"));
	g_print   (_("If you want to use the GTK version, type gnubiff --gtk\n"));
	g_print   (_("Now I will hang forever...\n"));

#if defined(PREFIX) && defined(SYSCONFDIR) && defined(DATADIR) && defined(LIBDIR)
	gnome_program_init ("gnubiff", "0", LIBGNOMEUI_MODULE, argc, argv, GNOME_PROGRAM_STANDARD_PROPERTIES, NULL);
#else
	gnome_program_init ("gnubiff", "0", LIBGNOMEUI_MODULE, argc, argv, GNOME_PARAM_NONE);
#endif

	panel_applet_factory_main ("OAFIID:GNOME_gnubiffApplet_Factory", PANEL_TYPE_APPLET, gnubiff_applet_factory, 0);

	return 0;
}
#endif
