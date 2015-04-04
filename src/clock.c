/* $Id$ */
/* Copyright (c) 2013-2015 Pierre Pronchery <khorben@defora.org> */
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
	/* alarms */
	GtkListStore * al_store;
	GtkWidget * al_view;
	/* clock */
	GtkWidget * cl_toggle;
	GtkWidget * cl_day;
	GtkWidget * cl_month;
	GtkWidget * cl_year;
	GtkWidget * cl_hour;
	GtkWidget * cl_minute;
	GtkWidget * cl_second;
	/* timers */
	GtkListStore * ti_store;
	GtkWidget * ti_view;
	/* actions */
	GtkWidget * apply;
};

typedef enum _ClockAlarmColumn
{
	CAC_ACTIVE = 0,
	CAC_TITLE,
	CAC_TIME
} ClockAlarmColumn;
#define CAC_LAST CAC_TIME
#define CAC_COUNT (CAC_LAST + 1)

typedef enum _ClockTimerColumn
{
	CTC_ACTIVE = 0,
	CTC_TITLE,
	CTC_TIME
} ClockTimerColumn;
#define CTC_LAST CTC_TIME
#define CTC_COUNT (CTC_LAST + 1)


/* prototypes */
/* useful */
static int _clock_error(Clock * clock, char const * message, int ret);

/* callbacks */
static void _clock_on_apply(gpointer data);
static void _clock_on_close(gpointer data);
static gboolean _clock_on_timeout(gpointer data);
static void _clock_on_toggled(gpointer data);
static gboolean _clock_on_window_closex(gpointer data);

/* alarm */
static void _clock_on_alarm_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data);

/* timer */
static void _clock_on_timer_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data);


/* public */
/* functions */
/* clock_new */
static void _new_alarms(Clock * clock, GtkWidget * notebook);
static void _new_alarms_on_new(gpointer data);
static void _new_alarms_on_title_edited(GtkCellRendererText * renderer,
		gchar * path, gchar * text, gpointer data);
static void _new_date(Clock * clock, GtkWidget * notebook);
static void _new_timers(Clock * clock, GtkWidget * notebook);
static void _new_timers_on_new(gpointer data);
static void _new_timers_on_title_edited(GtkCellRendererText * renderer,
		gchar * path, gchar * text, gpointer data);

Clock * clock_new(void)
{
	Clock * clock;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;

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
	/* notebook */
	widget = gtk_notebook_new();
	_new_date(clock, widget);
	_new_alarms(clock, widget);
	_new_timers(clock, widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
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

static void _new_alarms(Clock * clock, GtkWidget * notebook)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* toolbar */
	widget = gtk_toolbar_new();
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_new_alarms_on_new), clock);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_PASTE);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	clock->al_store = gtk_list_store_new(CAC_COUNT,
			G_TYPE_BOOLEAN,		/* CAC_ACTIVE */
			G_TYPE_STRING,		/* CAC_TITLE */
			G_TYPE_STRING);		/* CAC_TIME */
	clock->al_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				clock->al_store));
	/* active */
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(
				_clock_on_alarm_toggled), clock);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"active", CAC_ACTIVE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->al_view), column);
	/* title */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_new_alarms_on_title_edited), clock);
	column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer,
			"text", CAC_TITLE, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->al_view), column);
	/* time */
	renderer = gtk_cell_renderer_text_new();
	/* FIXME popup when editing */
	column = gtk_tree_view_column_new_with_attributes(_("Time"), renderer,
			"text", CAC_TIME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->al_view), column);
	gtk_container_add(GTK_CONTAINER(widget), clock->al_view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new(_("Alarms")));
}

static void _new_alarms_on_new(gpointer data)
{
	Clock * clock = data;
	GtkTreeIter iter;

	gtk_list_store_append(clock->al_store, &iter);
	gtk_list_store_set(clock->al_store, &iter, CAC_ACTIVE, FALSE,
			CAC_TITLE, _("Alarm"), -1);
}

static void _new_alarms_on_title_edited(GtkCellRendererText * renderer,
		gchar * path, gchar * text, gpointer data)
{
	Clock * clock = data;
	GtkTreeModel * model = GTK_TREE_MODEL(clock->al_store);
	GtkTreeIter iter;

	if(gtk_tree_model_get_iter_from_string(model, &iter, path) != TRUE)
		return;
	gtk_list_store_set(clock->al_store, &iter, CAC_TITLE, text, -1);
}

static void _new_date(Clock * clock, GtkWidget * notebook)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	char const * p;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	/* toggle */
	clock->cl_toggle = gtk_check_button_new_with_mnemonic(
			_("_Set the time and date:"));
	g_signal_connect_swapped(clock->cl_toggle, "toggled", G_CALLBACK(
				_clock_on_toggled), clock);
	gtk_box_pack_start(GTK_BOX(vbox), clock->cl_toggle, FALSE, TRUE, 0);
	/* date */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new(_("Date: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* day */
	clock->cl_day = gtk_spin_button_new_with_range(1.0, 31.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_day), 0);
	gtk_widget_set_sensitive(clock->cl_day, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_day, TRUE, TRUE, 0);
	/* month */
	clock->cl_month = gtk_spin_button_new_with_range(1.0, 12.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_month), 0);
	gtk_widget_set_sensitive(clock->cl_month, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_month, TRUE, TRUE, 0);
	/* year */
	clock->cl_year = gtk_spin_button_new_with_range(1970.0, 2038.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_year), 0);
	gtk_widget_set_sensitive(clock->cl_year, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_year, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* time */
	hbox = gtk_hbox_new(FALSE, 4);
	widget = gtk_label_new(_("Time: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	/* hour */
	clock->cl_hour = gtk_spin_button_new_with_range(0.0, 23.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_hour), 0);
	gtk_widget_set_sensitive(clock->cl_hour, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_hour, TRUE, TRUE, 0);
	/* minutes */
	clock->cl_minute = gtk_spin_button_new_with_range(0.0, 59.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_minute), 0);
	gtk_widget_set_sensitive(clock->cl_minute, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_minute, TRUE, TRUE, 0);
	/* seconds */
	clock->cl_second = gtk_spin_button_new_with_range(0.0, 61.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(clock->cl_second), 0);
	gtk_widget_set_sensitive(clock->cl_second, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), clock->cl_second, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* timezone */
	if((p = getenv("TZ")) != NULL)
	{
		hbox = gtk_hbox_new(FALSE, 0);
		widget = gtk_label_new(_("Timezone: "));
		gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
		/* FIXME make it an editable combo box (with drop-down list) */
		widget = gtk_label_new(p);
		gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
		gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	}
	/* automatic update */
	/* FIXME add a button to start ntpdate? */
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new(_("Clock")));
}

static void _new_timers(Clock * clock, GtkWidget * notebook)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkToolItem * toolitem;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	/* toolbar */
	widget = gtk_toolbar_new();
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	g_signal_connect_swapped(toolitem, "clicked", G_CALLBACK(
				_new_timers_on_new), clock);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_PASTE);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_DELETE);
	gtk_toolbar_insert(GTK_TOOLBAR(widget), toolitem, -1);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* view */
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	clock->ti_store = gtk_list_store_new(CTC_COUNT,
			G_TYPE_BOOLEAN,		/* CTC_ACTIVE */
			G_TYPE_STRING,		/* CTC_TITLE */
			G_TYPE_STRING);		/* CTC_TIME */
	clock->ti_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				clock->ti_store));
	/* active */
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(
				_clock_on_timer_toggled), clock);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"active", CTC_ACTIVE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->ti_view), column);
	/* title */
	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(
				_new_timers_on_title_edited), clock);
	column = gtk_tree_view_column_new_with_attributes(_("Title"), renderer,
			"text", CTC_TITLE, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->ti_view), column);
	/* duration */
	renderer = gtk_cell_renderer_text_new();
	/* FIXME popup when editing */
	column = gtk_tree_view_column_new_with_attributes(_("Duration"),
			renderer, "text", CTC_TIME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(clock->ti_view), column);
	gtk_container_add(GTK_CONTAINER(widget), clock->ti_view);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
			gtk_label_new(_("Timers")));
}

static void _new_timers_on_new(gpointer data)
{
	Clock * clock = data;
	GtkTreeIter iter;

	gtk_list_store_append(clock->ti_store, &iter);
	gtk_list_store_set(clock->ti_store, &iter, CTC_ACTIVE, FALSE,
			CTC_TITLE, _("Timer"), -1);
}

static void _new_timers_on_title_edited(GtkCellRendererText * renderer,
		gchar * path, gchar * text, gpointer data)
{
	Clock * clock = data;
	GtkTreeModel * model = GTK_TREE_MODEL(clock->ti_store);
	GtkTreeIter iter;

	if(gtk_tree_model_get_iter_from_string(model, &iter, path) != TRUE)
		return;
	gtk_list_store_set(clock->ti_store, &iter, CTC_TITLE, text, -1);
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
	t.tm_mday = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_day));
	t.tm_mon = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_month))
		- 1;
	t.tm_year = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_year))
		- 1900;
	t.tm_hour = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_hour));
	t.tm_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_minute));
	t.tm_sec = gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock->cl_second));
	tv.tv_sec = mktime(&t);
	tv.tv_usec = 0;
	if(settimeofday(&tv, NULL) != 0)
		_clock_error(clock, strerror(errno), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clock->cl_toggle),
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
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(clock->cl_toggle)))
		return TRUE;
	if(gettimeofday(&tv, NULL) != 0
			|| localtime_r(&tv.tv_sec, &t) == NULL)
		/* XXX report error */
		return TRUE;
	/* date */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_day), t.tm_mday);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_month),
			t.tm_mon + 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_year),
			t.tm_year + 1900);
	/* time */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_hour), t.tm_hour);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_minute), t.tm_min);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock->cl_second), t.tm_sec);
	return TRUE;
}


/* clock_on_toggled */
static void _clock_on_toggled(gpointer data)
{
	Clock * clock = data;
	gboolean sensitive;

	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				clock->cl_toggle));
	gtk_widget_set_sensitive(clock->cl_day, sensitive);
	gtk_widget_set_sensitive(clock->cl_month, sensitive);
	gtk_widget_set_sensitive(clock->cl_year, sensitive);
	gtk_widget_set_sensitive(clock->cl_hour, sensitive);
	gtk_widget_set_sensitive(clock->cl_minute, sensitive);
	gtk_widget_set_sensitive(clock->cl_second, sensitive);
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


/* alarm */
/* clock_on_alarm_toggled */
static void _clock_on_alarm_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Clock * clock = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(clock->al_store),
			&iter, path);
	gtk_list_store_set(clock->al_store, &iter, CAC_ACTIVE,
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
	/* FIXME really implement */
}


/* timer */
static void _clock_on_timer_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Clock * clock = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(clock->ti_store),
			&iter, path);
	gtk_list_store_set(clock->ti_store, &iter, CTC_ACTIVE,
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
	/* FIXME really implement */
}
