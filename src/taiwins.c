#include <stdio.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <compositor.h>
#include <wayland-server.h>
#include "taiwins.h"
#include <os/os-compatibility.h>

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
	if (execlp(path, path, NULL) == -1) {
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

//you probabely want to add this
static int on_term_signal(int signal_number, void *data)
{
	struct wl_display *display = (struct wl_display *)data;
	wl_display_terminate(display);
	return 0;
}