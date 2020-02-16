/*
 * taiwins.c - taiwins server shared functions
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

#include <linux/limits.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dlfcn.h>
#include <wayland-server.h>
#include <os/os-compatibility.h>

#include "taiwins.h"

/* static void sigup_handler(int signum) */
/* { */
/*	raise(SIGTERM); */
/* } */

static int
exec_wayland_client(const char *path, int fd)
{
	if (seteuid(getuid()) == -1) {
		return -1;
	}
	//unblock all the signals

	sigset_t allsignals;
	sigfillset(&allsignals);
	sigprocmask(SIG_UNBLOCK, &allsignals, NULL);
	/* struct sigaction sa = { */
	/*	.sa_mask = allsignals, */
	/*	.sa_flags = SA_RESTART, */
	/*	.sa_handler = sigup_handler, */
	/* }; */
	/* there seems to be no actual automatic mechism to notify child process
	 * of the termination, I will have to do it myself. */

	char sn[10];
	int clientid = dup(fd);
	snprintf(sn, sizeof(sn), "%d", clientid);
	setenv("WAYLAND_SOCKET", sn, 1);
	/* const char *for_shell = "/usr/lib/x86_64-linux-gnu/mesa:/usr/lib/x86_64-linux-gnu/mesa-egl:"; */
	/* const char new_env[strlen(for_shell) + 1]; */
	/* strcpy(new_env, for_shell); */
	/* setenv(("LD_LIBRARY_PATH"), new_env, 1); */

	if (execlp(path, path, NULL) == -1) {
		weston_log("failed to execute the client %s\n", path);
		close(clientid);
		return -1;
	}
	return 0;
}

//return the client pid
struct wl_client *
tw_launch_client(struct weston_compositor *ec, const char *path)
{
	int sv[2];
	pid_t pid;
	struct wl_client *client;
	//now we have to do the close on exec thing
	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, sv)) {
		weston_log("taiwins_client launch: "
			   "failed to create the socket for client `%s`\n",
			   path);
		return NULL;
	}

	pid = fork();
	if (pid == -1) {
		close(sv[0]);
		close(sv[1]);
		weston_log("taiwins_client launch: "
			   "failed to create the process, `%s`\n",
			path);
		return NULL;
	}
	if (pid == 0) {
		//you should close the server end
		close(sv[0]);
		//find a way to exec the child with sv[0];
		//okay, this is basically setting the environment variables
		exec_wayland_client(path, sv[1]);
		//replace this
		exit(-1);
	}
	//for parents, we don't need socket[1]
	close(sv[1]);
	//wayland client launch setup: by leveraging the UNIX
	//socketpair functions. when the socket pair is created. The
	client = wl_client_create(ec->wl_display, sv[0]);
	if (!client) {
		//this can't be happening, but housework need to be done
		close(sv[0]);
		weston_log("taiwins_client launch: "
			   "failed to create wl_client for %s", path);
		return NULL;
	}
	//now we some probably need to add signal on the server side
	return client;
}


void
tw_end_client(struct wl_client *client)
{
	pid_t pid; uid_t uid; gid_t gid;
	wl_client_get_credentials(client, &pid, &uid, &gid);
	kill(pid, SIGINT);
}


void
tw_lose_surface_focus(struct weston_surface *surface)
{
	struct weston_compositor *ec = surface->compositor;
	struct weston_seat *seat;

	wl_list_for_each(seat, &ec->seat_list, link) {
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);
		//struct weston_pointer *pointer =
		//	weston_seat_get_pointer(seat);
		//struct weston_touch *touch =
		//	weston_seat_get_touch(seat);
		if (keyboard &&
		    (weston_surface_get_main_surface(keyboard->focus) == surface))
			weston_keyboard_set_focus(keyboard, NULL);
		//it maynot be a good idea to do the pointer and touch as well,
		//since FIRST only keyboard gets the focus of a surface, the
		//rest gets the focus from view; SECOND if we do this when we
		//need focused output, there is no thing we can do

		//focus = (pointer) ? pointer->focus : NULL;
		//if (focus &&
		//    (weston_surface_get_main_surface(focus->surface) == surface))
		//	weston_pointer_set_focus(pointer, NULL,
		//				 wl_fixed_from_int(0),
		//				 wl_fixed_from_int(0));

		//focus = (touch) ? touch->focus : NULL;
		//if (focus &&
		//    (weston_surface_get_main_surface(focus->surface) == surface))
		//	weston_touch_set_focus(touch, NULL);
	}
}

void
tw_focus_surface(struct weston_surface *surface)
{
	struct weston_seat *active_seat = container_of(surface->compositor->seat_list.next,
						       struct weston_seat, link);
	struct weston_keyboard *keyboard = active_seat->keyboard_state;
	weston_keyboard_set_focus(keyboard, surface);
}

struct weston_output *
tw_get_focused_output(struct weston_compositor *compositor)
{
	struct weston_seat *seat;
	struct weston_output *output = NULL;

	wl_list_for_each(seat, &compositor->seat_list, link) {
		struct weston_touch *touch = weston_seat_get_touch(seat);
		struct weston_pointer *pointer = weston_seat_get_pointer(seat);
		struct weston_keyboard *keyboard =
			weston_seat_get_keyboard(seat);

		/* Priority has touch focus, then pointer and
		 * then keyboard focus. We should probably have
		 * three for loops and check frist for touch,
		 * then for pointer, etc. but unless somebody has some
		 * objections, I think this is sufficient. */
		if (touch && touch->focus)
			output = touch->focus->output;
		else if (pointer && pointer->focus)
			output = pointer->focus->output;
		else if (keyboard && keyboard->focus)
			output = keyboard->focus->output;

		if (output)
			break;
	}

	return output;
}

void *
tw_load_weston_module(const char *name, const char *entrypoint)
{
	void *module, *init;

	if (name == NULL || entrypoint == NULL)
		return NULL;

	//our modules are in the rpath as we do not have the
	//LIBWESTON_MODULEDIR, so we need to test name and
	module = dlopen(name, RTLD_NOW | RTLD_NOLOAD);
	if (module) {
		weston_log("Module '%s' already loaded\n", name);
		return NULL;
	} else {
		module = dlopen(name, RTLD_NOW);
		if (!module) {
			weston_log("Failed to load the module %s\n", name);
			return NULL;
		}
	}

	init = dlsym(module, entrypoint);
	if (!init) {
		weston_log("Faild to lookup function in module: %s\n",
		           dlerror());
		dlclose(module);
		return NULL;
	}
	return init;

}
