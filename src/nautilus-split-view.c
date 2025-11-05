/* nautilus-split-view.c - Split view/dual pane mode implementation
 *
 * Copyright (C) 2025 Niieux Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include "nautilus-split-view.h"
#include "nautilus-window-slot.h"

struct _NautilusSplitView
{
    GtkBox parent_instance;

    /* UI Components */
    GtkWidget *paned;
    GtkWidget *left_pane_box;
    GtkWidget *right_pane_box;
    GtkWidget *right_close_button;

    /* Window slots */
    NautilusWindowSlot *left_slot;
    NautilusWindowSlot *right_slot;

    /* State */
    gboolean is_split_active;
    gboolean right_pane_active;
    gint saved_pane_position;
};

G_DEFINE_TYPE (NautilusSplitView, nautilus_split_view, GTK_TYPE_BOX)

enum
{
    PROP_0,
    PROP_ACTIVE_PANE,
    PROP_IS_SPLIT_ACTIVE,
    N_PROPS
};

enum
{
    ACTIVE_PANE_CHANGED,
    SPLIT_ENABLED,
    SPLIT_DISABLED,
    LAST_SIGNAL
};

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void
on_pane_focus_in (GtkWidget         *widget,
                  NautilusSplitView *split_view)
{
    gboolean is_right = (widget == split_view->right_pane_box);
    
    if (split_view->right_pane_active != is_right)
    {
        nautilus_split_view_set_active_pane (split_view, is_right);
    }
}

static void
on_close_button_clicked (GtkButton         *button,
                         NautilusSplitView *split_view)
{
    nautilus_split_view_close_pane (split_view);
}

static void
nautilus_split_view_update_pane_styles (NautilusSplitView *split_view)
{
    GtkStyleContext *left_context;
    GtkStyleContext *right_context;

    left_context = gtk_widget_get_style_context (split_view->left_pane_box);
    right_context = gtk_widget_get_style_context (split_view->right_pane_box);

    if (split_view->right_pane_active)
    {
        gtk_style_context_remove_class (left_context, "active-pane");
        gtk_style_context_add_class (right_context, "active-pane");
    }
    else
    {
        gtk_style_context_add_class (left_context, "active-pane");
        gtk_style_context_remove_class (right_context, "active-pane");
    }
}

static void
nautilus_split_view_dispose (GObject *object)
{
    NautilusSplitView *split_view = NAUTILUS_SPLIT_VIEW (object);

    g_clear_object (&split_view->left_slot);
    g_clear_object (&split_view->right_slot);

    G_OBJECT_CLASS (nautilus_split_view_parent_class)->dispose (object);
}

static void
nautilus_split_view_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
    NautilusSplitView *split_view = NAUTILUS_SPLIT_VIEW (object);

    switch (prop_id)
    {
        case PROP_ACTIVE_PANE:
        {
            g_value_set_boolean (value, split_view->right_pane_active);
        }
        break;

        case PROP_IS_SPLIT_ACTIVE:
        {
            g_value_set_boolean (value, split_view->is_split_active);
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_split_view_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
    NautilusSplitView *split_view = NAUTILUS_SPLIT_VIEW (object);

    switch (prop_id)
    {
        case PROP_ACTIVE_PANE:
        {
            nautilus_split_view_set_active_pane (split_view,
                                                g_value_get_boolean (value));
        }
        break;

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_split_view_class_init (NautilusSplitViewClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = nautilus_split_view_dispose;
    object_class->get_property = nautilus_split_view_get_property;
    object_class->set_property = nautilus_split_view_set_property;

    properties[PROP_ACTIVE_PANE] =
        g_param_spec_boolean ("active-pane",
                             "Active pane",
                             "Which pane is active (FALSE=left, TRUE=right)",
                             FALSE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

    properties[PROP_IS_SPLIT_ACTIVE] =
        g_param_spec_boolean ("is-split-active",
                             "Split active",
                             "Whether split view mode is active",
                             FALSE,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    signals[ACTIVE_PANE_CHANGED] =
        g_signal_new ("active-pane-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL, NULL,
                      G_TYPE_NONE,
                      1,
                      G_TYPE_BOOLEAN);

    signals[SPLIT_ENABLED] =
        g_signal_new ("split-enabled",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL, NULL,
                      G_TYPE_NONE,
                      0);

    signals[SPLIT_DISABLED] =
        g_signal_new ("split-disabled",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      0,
                      NULL, NULL, NULL,
                      G_TYPE_NONE,
                      0);
}

static void
nautilus_split_view_init (NautilusSplitView *split_view)
{
    GtkWidget *header_box;
    GtkWidget *close_icon;

    gtk_orientable_set_orientation (GTK_ORIENTABLE (split_view),
                                   GTK_ORIENTATION_HORIZONTAL);

    /* Create the paned widget for resizable split */
    split_view->paned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_hexpand (split_view->paned, TRUE);
    gtk_widget_set_vexpand (split_view->paned, TRUE);
    gtk_box_append (GTK_BOX (split_view), split_view->paned);

    /* Create left pane */
    split_view->left_pane_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand (split_view->left_pane_box, TRUE);
    gtk_widget_set_vexpand (split_view->left_pane_box, TRUE);
    gtk_style_context_add_class (gtk_widget_get_style_context (split_view->left_pane_box),
                                "split-pane");
    gtk_paned_set_start_child (GTK_PANED (split_view->paned),
                              split_view->left_pane_box);

    /* Create right pane with header for close button */
    split_view->right_pane_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand (split_view->right_pane_box, TRUE);
    gtk_widget_set_vexpand (split_view->right_pane_box, TRUE);
    gtk_style_context_add_class (gtk_widget_get_style_context (split_view->right_pane_box),
                                "split-pane");

    /* Add close button header to right pane */
    header_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_add_css_class (header_box, "split-pane-header");
    gtk_box_append (GTK_BOX (split_view->right_pane_box), header_box);

    /* Spacer to push close button to the right */
    GtkWidget *spacer = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (spacer, TRUE);
    gtk_box_append (GTK_BOX (header_box), spacer);

    /* Close button */
    split_view->right_close_button = gtk_button_new ();
    close_icon = gtk_image_new_from_icon_name ("window-close-symbolic");
    gtk_button_set_child (GTK_BUTTON (split_view->right_close_button), close_icon);
    gtk_widget_add_css_class (split_view->right_close_button, "flat");
    gtk_widget_set_tooltip_text (split_view->right_close_button, "Close split view");
    g_signal_connect (split_view->right_close_button, "clicked",
                     G_CALLBACK (on_close_button_clicked), split_view);
    gtk_box_append (GTK_BOX (header_box), split_view->right_close_button);

    gtk_paned_set_end_child (GTK_PANED (split_view->paned),
                            split_view->right_pane_box);

    /* Set up focus tracking */
    GtkEventController *left_focus_controller;
    GtkEventController *right_focus_controller;

    left_focus_controller = gtk_event_controller_focus_new ();
    g_signal_connect_swapped (left_focus_controller, "enter",
                             G_CALLBACK (on_pane_focus_in), split_view);
    gtk_widget_add_controller (split_view->left_pane_box, left_focus_controller);

    right_focus_controller = gtk_event_controller_focus_new ();
    g_signal_connect_swapped (right_focus_controller, "enter",
                              G_CALLBACK (on_pane_focus_in), split_view);
    gtk_widget_add_controller (split_view->right_pane_box, right_focus_controller);

    /* Initialize state */
    split_view->is_split_active = FALSE;
    split_view->right_pane_active = FALSE;
    split_view->saved_pane_position = -1;

    /* Hide right pane by default */
    gtk_widget_set_visible (split_view->right_pane_box, FALSE);

    nautilus_split_view_update_pane_styles (split_view);
}

NautilusSplitView *
nautilus_split_view_new (void)
{
    return g_object_new (NAUTILUS_TYPE_SPLIT_VIEW, NULL);
}

void
nautilus_split_view_set_active_pane (NautilusSplitView *split_view,
                                    gboolean           is_right_pane)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));

    if (split_view->right_pane_active != is_right_pane)
    {
        split_view->right_pane_active = is_right_pane;
        nautilus_split_view_update_pane_styles (split_view);
        
        g_object_notify_by_pspec (G_OBJECT (split_view),
                                 properties[PROP_ACTIVE_PANE]);
        g_signal_emit (split_view, signals[ACTIVE_PANE_CHANGED], 0,
                      is_right_pane);
    }
}

gboolean
nautilus_split_view_get_active_pane (NautilusSplitView *split_view)
{
    g_return_val_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view), FALSE);

    return split_view->right_pane_active;
}

void
nautilus_split_view_set_left_slot (NautilusSplitView  *split_view,
                                   NautilusWindowSlot *slot)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));
    g_return_if_fail (slot == NULL || NAUTILUS_IS_WINDOW_SLOT (slot));

    if (g_set_object (&split_view->left_slot, slot))
    {
        if (slot != NULL)
        {
            gtk_box_append (GTK_BOX (split_view->left_pane_box),
                          GTK_WIDGET (slot));
        }
    }
}

void
nautilus_split_view_set_right_slot (NautilusSplitView  *split_view,
                                    NautilusWindowSlot *slot)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));
    g_return_if_fail (slot == NULL || NAUTILUS_IS_WINDOW_SLOT (slot));

    if (g_set_object (&split_view->right_slot, slot))
    {
        if (slot != NULL)
        {
            /* Append after the header (which is the first child) */
            gtk_box_append (GTK_BOX (split_view->right_pane_box),
                          GTK_WIDGET (slot));
        }
    }
}

NautilusWindowSlot *
nautilus_split_view_get_left_slot (NautilusSplitView *split_view)
{
    g_return_val_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view), NULL);

    return split_view->left_slot;
}

NautilusWindowSlot *
nautilus_split_view_get_right_slot (NautilusSplitView *split_view)
{
    g_return_val_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view), NULL);

    return split_view->right_slot;
}

NautilusWindowSlot *
nautilus_split_view_get_active_slot (NautilusSplitView *split_view)
{
    g_return_val_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view), NULL);

    if (!split_view->is_split_active)
    {
        return split_view->left_slot;
    }

    return split_view->right_pane_active ? split_view->right_slot
                                         : split_view->left_slot;
}

gboolean
nautilus_split_view_is_active (NautilusSplitView *split_view)
{
    g_return_val_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view), FALSE);

    return split_view->is_split_active;
}

void
nautilus_split_view_enable (NautilusSplitView *split_view,
                            GFile             *location)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));
    g_return_if_fail (G_IS_FILE (location));

    if (split_view->is_split_active)
    {
        /* Already active, just navigate right pane to location */
        if (split_view->right_slot != NULL)
        {
            nautilus_window_slot_open_location_full (split_view->right_slot,
                                                    location, 0, NULL);
        }
        return;
    }

    split_view->is_split_active = TRUE;

    /* Show right pane */
    gtk_widget_set_visible (split_view->right_pane_box, TRUE);
    
    /* Set equal pane sizes or restore previous position */
    gint width = gtk_widget_get_width (split_view->paned);
    if (split_view->saved_pane_position > 0)
    {
        gtk_paned_set_position (GTK_PANED (split_view->paned),
                               split_view->saved_pane_position);
    }
    else
    {
        gtk_paned_set_position (GTK_PANED (split_view->paned), width / 2);
    }

    /* Open location in right pane if slot exists */
    if (split_view->right_slot != NULL && location != NULL)
    {
        nautilus_window_slot_open_location_full (split_view->right_slot,
                                                location, 0, NULL);
    }

    g_object_notify_by_pspec (G_OBJECT (split_view),
                             properties[PROP_IS_SPLIT_ACTIVE]);
    g_signal_emit (split_view, signals[SPLIT_ENABLED], 0);
}

void
nautilus_split_view_disable (NautilusSplitView *split_view)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));

    if (!split_view->is_split_active)
    {
        return;
    }

    split_view->is_split_active = FALSE;

    /* Save current pane position */
    split_view->saved_pane_position = gtk_paned_get_position (GTK_PANED (split_view->paned));

    /* Hide right pane */
    gtk_widget_set_visible (split_view->right_pane_box, FALSE);
    split_view->right_pane_active = FALSE;
    nautilus_split_view_update_pane_styles (split_view);

    g_object_notify_by_pspec (G_OBJECT (split_view),
                             properties[PROP_IS_SPLIT_ACTIVE]);
    g_signal_emit (split_view, signals[SPLIT_DISABLED], 0);
}

void
nautilus_split_view_close_pane (NautilusSplitView *split_view)
{
    g_return_if_fail (NAUTILUS_IS_SPLIT_VIEW (split_view));

    nautilus_split_view_disable (split_view);
}
