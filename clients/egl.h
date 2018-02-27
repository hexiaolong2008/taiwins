#ifndef TW_EGL_IFACE_H
#define TW_EGL_IFACE_H

#include <EGL/egl.h>
#include <GL/gl.h>
#include <wayland-egl.h>
#include <stdio.h>
#include <stdbool.h>

#include "ui.h"

#ifdef __cplusplus
extern "C" {
#endif

//maybe move this to wl_globals later
struct egl_env {
	EGLDisplay egl_display;
	EGLContext egl_context;
	struct wl_display *wl_display;
	EGLConfig config;
};

struct eglapp;




bool egl_env_init(struct egl_env *env, struct wl_display *disp);

void egl_env_end(struct egl_env *env);

struct eglapp_surface;
void
eglapp_launch(struct eglapp *app, struct egl_env *env, struct wl_compositor *compositor);


#ifdef __cplusplus
}
#endif





#endif /* EOF */