#ifndef TW_SHELL_CLIENT_H
#define TW_SHRLL_CLIENT_H

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <cairo/cairo.h>
#include <poll.h>
#include <wayland-taiwins-desktop-client-protocol.h>
#include <wayland-client.h>
#include <sequential.h>
#include <os/file.h>
#include <client.h>
#include <egl.h>
#include <nk_backends.h>
#include "../shared_config.h"
#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shell_output {
	struct desktop_shell *shell;
	struct tw_output *output;
	//options
	struct {
		struct bbox bbox;
		int index;
	};
	struct tw_ui *bg_ui;
	struct tw_ui *pn_ui;
	struct app_surface background;
	struct app_surface panel;
	struct app_event_filter background_events;
	//a temporary struct
	double widgets_span;
};

//state of current widget and widget to launch
struct widget_launch_info {
	uint32_t x;
	uint32_t y;
	struct shell_widget *widget;
	struct shell_widget *current;
};

struct desktop_shell {
	struct wl_globals globals;
	struct tw_shell *interface;
	enum tw_shell_panel_pos panel_pos;
	//pannel configuration
	struct {
		struct nk_wl_backend *panel_backend;
		struct nk_style_button label_style;
		char wallpaper_path[128];
		//TODO calculated from font size
		size_t panel_height;
	};
	//widget configures
	struct {
		struct nk_wl_backend *widget_backend;
		struct wl_list shell_widgets;
		struct widget_launch_info widget_launch;
		//surface like locker, on-screen-keyboard will use this surface.
		struct tw_ui *transient_ui;
		struct app_surface transient;
	};
	//outputs
	struct shell_output *main_output;
	struct shell_output shell_outputs[16];


};

static inline int
desktop_shell_n_outputs(struct desktop_shell *shell)
{
	for (int i = 0; i < 16; i++)
		if (shell->shell_outputs[i].shell == NULL)
			return i;
	return 16;
}


void shell_init_bg_for_output(struct shell_output *output);
void shell_resize_bg_for_output(struct shell_output *output);

void shell_init_panel_for_output(struct shell_output *output);
void shell_resize_panel_for_output(struct shell_output *output);

void shell_locker_init(struct desktop_shell *shell);

void shell_process_msg(struct desktop_shell *shell, uint32_t type,
		       const struct wl_array *data);

static inline void
shell_end_transient_surface(struct desktop_shell *shell)
{
	tw_ui_destroy(shell->transient_ui);
	shell->transient_ui = NULL;
	app_surface_release(&shell->transient);
}

#ifdef __cplusplus
}
#endif


#endif /* EOF */