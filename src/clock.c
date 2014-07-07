/* $Id$ */
/* Copyright (c) 2013-2014 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Accessories */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <libintl.h>
#include <System.h>
#include <gtk/gtk.h>
#include "clock.h"
#define _(string) gettext(string)


/* Clock */
/* private */
/* types */
struct _Clock
{
	/* internal */
	guint source;

	/* widgets */
	GtkWidget * window;
	GtkWidget * toggle;
	GtkWidget * day;
	GtkWidget * month;
	GtkWidget * year;
	GtkWidget * hour;
	GtkWidget * minute;
	GtkWidget * second;
	GtkWidget * apply;
};


/* prototypes */
/* useful */
static int _clock_error(Clock * clock, char const * message, int ret);

/* callbacks */
static void _clock_on_apply(gpointer data);
static void _clock_on_close(gpointer data);
static gboolean _clock_on_timeout(gpointer data);
static void _clock_on_toggled(gpointer data);
static gboolean _clock_on_window_closex(gpointer data);


/* public */
/* functions */
/* clock_new */
Clock * clock_new(void)
{
	Clock * clock;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	char const * p;

	if((clock = object_new(sizeof(*clock))) == NULL)
		return NULL;
	clock->window = gtk_dialog_new();
	gtk_window_set_default_size(GTK_WINDOW(clock->window), 200, 300);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_window_set_icon_name(GTK_WINDOW(clock->window), "appointment-soon");
#endif
	gtk_window_set_position(GTK_WINDOW(clock->window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(clock->window),
			_("Date and time settings"));
	g_signal_connect_swapped(clock->window, "delete-event", G_CALLBACK(
				_clock_on_window_closex), clock);
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(clock->window));
#else
	vbox = GTK_DIALOG(clock->window)->vbox;
#endif
	/* toggle */
	clock->toggle = gtk_check_button_new_with_mnemonic(
			_("_Set the time and date:"));
	g_signal_connect_swapped(clock->toggle, "toggled", G_CALLBACK(
				_clock_on_toggled), clock);
	gtk_box_pack_start(GTK_BOX(vbox), clock->toggle, FALSE, TRUE, 0);
	/* date */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Date: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* day */
	clock->day = gtk_spin_button_new_with_range(1.0, 31.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->day), 0);
	gtk_widget_set_sensitive(clock->day, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->day, TRUE, TRUE, 0);
	/* month */
	clock->month = gtk_spin_button_new_with_range(1.0, 12.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->month), 0);
	gtk_widget_set_sensitive(clock->month, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->month, TRUE, TRUE, 0);
	/* year */
	clock->year = gtk_spin_button_new_with_range(1970.0, 2038.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->year), 0);
	gtk_widget_set_sensitive(clock->year, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->year, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* time */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Time: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* hour */
	clock->hour = gtk_spin_button_new_with_range(0.0, 23.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->hour), 0);
	gtk_widget_set_sensitive(clock->hour, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->hour, TRUE, TRUE, 0);
	/* minutes */
	clock->minute = gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->minute), 0);
	gtk_widget_set_sensitive(clock->minute, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->minute, TRUE, TRUE, 0);
	/* seconds */
	clock->second = gtk_spin_button_new_with_range(0.0, 61.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->second), 0);
	gtk_widget_set_sensitive(clock->second, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->second, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* timezone */
	if((p = getenv("TZ")) != NULL)
	{
		hbox = gtk_hbox_new(FALSE, 0);
		widget = gtk_label_new(_("Timezone: "));
		gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
		widget = gtk_label_new(p);
		gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	}
	/* button box */
#if GTK_CHECK_VERSION(2, 14, 0)
	hbox = gtk_dialog_get_action_area(GTK_DIALOG(clock->window));
#else
	hbox = GTK_DIALOG(clock->window)->action_area;
#endif
	clock->apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	gtk_widget_set_sensitive(clock->apply, FALSE);
	g_signal_connect_swapped(clock->apply, "clicked", G_CALLBACK(
				_clock_on_apply), clock);
	gtk_container_add(GTK_CONTAINER(hbox), clock->apply);
	widget = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect_swapped(widget, "clicked", G_CALLBACK(_clock_on_close),
			clock);
	gtk_container_add(GTK_CONTAINER(hbox), widget);
	clock->source = g_timeout_add(1000, _clock_on_timeout, clock);
	_clock_on_timeout(clock);
	gtk_widget_show_all(clock->window);
	return clock;
}


/* clock_delete */
void clock_delete(Clock * clock)
{
	if(clock->source != 0)
		g_source_remove(clock->source);
	gtk_widget_destroy(clock->window);
	object_delete(clock);
}


/* private */
/* functions */
/* useful */
/* clock_error */
static int _clock_error(Clock * clock, char const * message, int ret)
{
	GtkWidget * dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(clock->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}


/* callbacks */
/* clock_on_apply */
static void _clock_on_apply(gpointer data)
{
	Clock * clock = data;
	struct tm t;
	struct timeval tv;

	memset(&t, 0, sizeof(t));
	t.tm_mday = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->day));
	t.tm_mon = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->month)) - 1;
	t.tm_year = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->year))
		- 1900;
	t.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->hour));
	t.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->minute));
	t.tm_sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->second));
	tv.tv_sec = mktime(&t);
	tv.tv_usec = 0;
	if(settimeofday(&tv, NULL) != 0)
		_clock_error(clock, strerror(errno), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clock->toggle),
				FALSE);
}


/* clock_on_close */
static void _clock_on_close(gpointer data)
{
	Clock * clock = data;

	gtk_widget_hide(clock->window);
	gtk_main_quit();
}


/* clock_on_timeout */
static gboolean _clock_on_timeout(gpointer data)
{
	Clock * clock = data;
	struct timeval tv;
	struct tm t;

	/* do not update if the time is being set */
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(clock->toggle)))
		return TRUE;
	if(gettimeofday(&tv, NULL) != 0
			|| localtime_r(&tv.tv_sec, &t) == NULL)
		/* XXX report error */
		return TRUE;
	/* date */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->day), t.tm_mday);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->month), t.tm_mon + 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->year),
			t.tm_year + 1900);
	/* time */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->hour), t.tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->minute), t.tm_min);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->second), t.tm_sec);
	return TRUE;
}


/* clock_on_toggled */
static void _clock_on_toggled(gpointer data)
{
	Clock * clock = data;
	gboolean sensitive;

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				clock->toggle));
	gtk_widget_set_sensitive(clock->day, sensitive);
	gtk_widget_set_sensitive(clock->month, sensitive);
	gtk_widget_set_sensitive(clock->year, sensitive);
	gtk_widget_set_sensitive(clock->hour, sensitive);
	gtk_widget_set_sensitive(clock->minute, sensitive);
	gtk_widget_set_sensitive(clock->second, sensitive);
	gtk_widget_set_sensitive(clock->apply, sensitive);
	if(sensitive == FALSE)
		_clock_on_timeout(clock);
}


/* clock_on_window_closex */
static gboolean _clock_on_window_closex(gpointer data)
{
	Clock * clock = data;

	gtk_widget_hide(clock->window);
	gtk_main_quit();
	return TRUE;
}
