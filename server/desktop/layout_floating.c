/*
 * layout_floating.c - taiwins desktop floating layout implementation
 *
 * Copyright (c) 2019 Xichen Zhou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <stdlib.h>
#include <ctypes/helpers.h>
#include <ctypes/sequential.h>
#include "layout.h"
#include "workspace.h"

static void
emplace_float(UNUSED_ARG(const enum layout_command command),
              UNUSED_ARG(const struct layout_op *arg),
              UNUSED_ARG(struct weston_view *v), UNUSED_ARG(struct layout *l),
              struct layout_op *ops);

static void
emplace_noop(UNUSED_ARG(const enum layout_command command),
             UNUSED_ARG(const struct layout_op *arg),
             UNUSED_ARG(struct weston_view *v), UNUSED_ARG(struct layout *l),
             struct layout_op *ops)
{
	ops[0].end = true;
}



////////////////////////////////////////////////////////////////////////////////
///////////////////////////// floating layout //////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


void
floating_layout_init(struct layout *layout, struct weston_layer *layer)
{
	layout_init(layout, layer);
	layout->command = emplace_float;
}

void floating_layout_end(struct layout *layout)
{
	layout_release(layout);
}


static void
floating_add(UNUSED_ARG(const enum layout_command command),
             UNUSED_ARG(const struct layout_op *arg),
             UNUSED_ARG(struct weston_view *v), UNUSED_ARG(struct layout *l),
             struct layout_op *ops)
{
	struct weston_geometry geo = {
		v->output->x, v->output->y,
		v->output->width, v->output->height,
	};
	if (arg->default_geometry.width != -1 &&
	    arg->default_geometry.height != -1) {
		ops[0].pos.x = arg->default_geometry.x;
		ops[0].pos.y = arg->default_geometry.y;
		ops[0].size.width = arg->default_geometry.width;
		ops[0].size.height = arg->default_geometry.height;
	} else {
		ops[0].pos.x = rand() % (geo.width / 4);
		ops[0].pos.y = rand() % (geo.height / 4);
		//so this is invalid size
		ops[0].size.width = 0;
		ops[0].size.height = 0;
	}
	assert(!ops[0].end);
	ops[0].end = false;
	ops[0].v = v;
	ops[1].end = 1;
}

static void
floating_deplace(UNUSED_ARG(const enum layout_command command),
                 UNUSED_ARG(const struct layout_op *arg),
                 UNUSED_ARG(struct weston_view *v),
                 UNUSED_ARG(struct layout *l),
                 struct layout_op *ops)
{
	struct weston_position curr_pos = {
		v->geometry.x,
		v->geometry.y
	};
	curr_pos.x += arg->dx;
	curr_pos.y += arg->dy;
	//here is the delimma, we should maybe make the
	ops[0].pos = curr_pos;
	ops[0].end = false;
	ops[0].v = v;
	ops[1].end = 1;
}

static void
floating_resize(UNUSED_ARG(const enum layout_command command),
                UNUSED_ARG(const struct layout_op *arg),
                UNUSED_ARG(struct weston_view *v), UNUSED_ARG(struct layout *l),
                struct layout_op *ops)
{
	struct weston_geometry visible = get_recent_view(v)->visible_geometry;
	struct weston_geometry buttom_right = {
		.x = v->geometry.x + visible.x + visible.width,
		.y = v->geometry.y + visible.y + visible.height,
	};
	//set position unchanged
	//we are adding visible.xy here because we will subtract
	ops[0].pos.x = v->geometry.x + visible.x;
	ops[0].pos.y = v->geometry.y + visible.y;
	ops[0].v = v;
	ops[0].end = false;

	double rx, ry; //ratio
	rx = wl_fixed_to_double(arg->sx) / (double)v->surface->width;
	ry = wl_fixed_to_double(arg->sy) / (double)v->surface->height;
	//only by moving buttom,right part does not affect the position
	if (rx < 0.5 || ry < 0.5) {
		ops[0].pos.x += arg->dx;
		ops[0].pos.y += arg->dy;
		ops[0].size.width = (int32_t)(buttom_right.x - ops[0].pos.x);
		ops[0].size.height = (int32_t)(buttom_right.y - ops[0].pos.y);
	} else {
		ops[0].size.width = (int32_t)(visible.width + arg->dx);
		ops[0].size.height = (int32_t)(visible.height + arg->dy);
	}
	ops[1].end = true;
}

void
emplace_float(const enum layout_command command, const struct layout_op *arg,
              struct weston_view *v, struct layout *l,
              struct layout_op *ops)
{
	struct placement_node {
		enum layout_command command;
		layout_fun_t fun;
	};

	static struct placement_node float_ops[] = {
		{DPSR_focus, emplace_noop},
		{DPSR_add, floating_add},
		{DPSR_del, emplace_noop},
		{DPSR_deplace, floating_deplace},
		{DPSR_toggle, emplace_noop},
		{DPSR_resize, floating_resize},
		{DPSR_vsplit, emplace_noop},
		{DPSR_hsplit, emplace_noop},
		{DPSR_merge, emplace_noop},
		{DPSR_output_resize, emplace_noop},
	};
	assert(float_ops[command].command == command);
	float_ops[command].fun(command, arg, v, l, ops);
}
