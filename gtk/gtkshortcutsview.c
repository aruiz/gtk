/* gtkshortcutsview.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkshortcutsviewprivate.h"

#include "gtkshortcutspageprivate.h"
#include "gtkstack.h"
#include "gtkstackswitcher.h"
#include "gtkstylecontext.h"
#include "gtkorientable.h"
#include "gtkprivate.h"
#include "gtkintl.h"

struct _GtkShortcutsView
{
  GtkBox            parent_instance;

  gchar            *name;
  gchar            *title;

  GtkStack         *stack;
  GtkStackSwitcher *switcher;

  guint             last_page_num;
};

struct _GtkShortcutsViewClass
{
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE (GtkShortcutsView, gtk_shortcuts_view, GTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_TITLE,
  PROP_VIEW_NAME,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static void
adjust_page_buttons (GtkWidget *widget,
                     gpointer   data)
{
  gint *count = data;

  /*
   * TODO: This is a hack to get the GtkStackSwitcher radio
   *       buttons to look how we want. However, it's very
   *       much font size specific.
   */
  gtk_widget_set_size_request (widget, 34, 34);

  (*count)++;
}

static void
gtk_shortcuts_view_add (GtkContainer *container,
                        GtkWidget    *child)
{
  GtkShortcutsView *self = (GtkShortcutsView *)container;

  if (GTK_IS_SHORTCUTS_PAGE (child))
    {
      gchar *title = NULL;
      guint count = 0;

      title = g_strdup_printf ("%u", ++self->last_page_num);
      gtk_container_add_with_properties (GTK_CONTAINER (self->stack), child,
                                         "title", title,
                                         NULL);

      gtk_container_foreach (GTK_CONTAINER (self->switcher), adjust_page_buttons, &count);
      gtk_widget_set_visible (GTK_WIDGET (self->switcher), (count > 1));
      g_free (title);
    }
  else
    {
      GTK_CONTAINER_CLASS (gtk_shortcuts_view_parent_class)->add (container, child);
    }
}

static void
gtk_shortcuts_view_finalize (GObject *object)
{
  GtkShortcutsView *self = (GtkShortcutsView *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (gtk_shortcuts_view_parent_class)->finalize (object);
}

static void
gtk_shortcuts_view_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GtkShortcutsView *self = (GtkShortcutsView *)object;

  switch (prop_id)
    {
    case PROP_VIEW_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_shortcuts_view_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GtkShortcutsView *self = (GtkShortcutsView *)object;

  switch (prop_id)
    {
    case PROP_VIEW_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;

    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_shortcuts_view_class_init (GtkShortcutsViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->finalize = gtk_shortcuts_view_finalize;
  object_class->get_property = gtk_shortcuts_view_get_property;
  object_class->set_property = gtk_shortcuts_view_set_property;

  container_class->add = gtk_shortcuts_view_add;

  properties[PROP_VIEW_NAME] =
    g_param_spec_string ("view-name",
                         P_("View Name"),
                         P_("View Name"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         P_("Title"),
                         P_("Title"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gtk_shortcuts_view_init (GtkShortcutsView *self)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);
  gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
  gtk_box_set_spacing (GTK_BOX (self), 22);
  gtk_container_set_border_width (GTK_CONTAINER (self), 24);

  self->stack = g_object_new (GTK_TYPE_STACK,
                              "homogeneous", TRUE,
                              "transition-type", GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT,
                              "vexpand", TRUE,
                              "visible", TRUE,
                              NULL);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->stack));

  self->switcher = g_object_new (GTK_TYPE_STACK_SWITCHER,
                                 "halign", GTK_ALIGN_CENTER,
                                 "stack", self->stack,
                                 "spacing", 12,
                                 "no-show-all", TRUE,
                                 NULL);
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self->switcher)), "round");
  gtk_style_context_remove_class (gtk_widget_get_style_context (GTK_WIDGET (self->switcher)), "linked");
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->switcher));
}

const gchar *
gtk_shortcuts_view_get_view_name (GtkShortcutsView *self)
{
  g_return_val_if_fail (GTK_IS_SHORTCUTS_VIEW (self), NULL);

  return self->name;
}

const gchar *
gtk_shortcuts_view_get_title (GtkShortcutsView *self)
{
  g_return_val_if_fail (GTK_IS_SHORTCUTS_VIEW (self), NULL);

  return self->title;
}