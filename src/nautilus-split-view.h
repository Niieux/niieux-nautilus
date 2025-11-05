/* nautilus-split-view.h - Split view/dual pane mode for Nautilus
 *
 * Copyright (C) 2025 Niieux Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#pragma once

#include <gtk/gtk.h>
#include "nautilus-window-slot.h"

G_BEGIN_DECLS

#define NAUTILUS_TYPE_SPLIT_VIEW (nautilus_split_view_get_type())

G_DECLARE_FINAL_TYPE (NautilusSplitView, nautilus_split_view, NAUTILUS, SPLIT_VIEW, GtkBox)

NautilusSplitView *nautilus_split_view_new              (void);

void               nautilus_split_view_set_active_pane  (NautilusSplitView *split_view,
                                                         gboolean           is_right_pane);

gboolean           nautilus_split_view_get_active_pane  (NautilusSplitView *split_view);

void               nautilus_split_view_set_left_slot    (NautilusSplitView  *split_view,
                                                         NautilusWindowSlot *slot);

void               nautilus_split_view_set_right_slot   (NautilusSplitView  *split_view,
                                                         NautilusWindowSlot *slot);

NautilusWindowSlot *nautilus_split_view_get_left_slot   (NautilusSplitView *split_view);

NautilusWindowSlot *nautilus_split_view_get_right_slot  (NautilusSplitView *split_view);

NautilusWindowSlot *nautilus_split_view_get_active_slot (NautilusSplitView *split_view);

gboolean           nautilus_split_view_is_active        (NautilusSplitView *split_view);

void               nautilus_split_view_enable           (NautilusSplitView  *split_view,
                                                         GFile              *location);

void               nautilus_split_view_disable          (NautilusSplitView *split_view);

void               nautilus_split_view_close_pane       (NautilusSplitView *split_view);

G_END_DECLS
