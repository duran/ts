/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007-2009  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>

#include "main.h"

extern int server_socket;

static char *socket_path;

static int fork_server();

void create_socket_path(char **path)
{
    char *tmpdir;
    char userid[20];
    int size;

    /* As a priority, TS_SOCKET mandates over the path creation */
    *path = getenv("TS_SOCKET");
    if (*path != 0)
    {
        /* We need this in our memory, for forks and future 'free'. */
        size = strlen(*path) + 1;
        *path = (char *) malloc(size);
        strcpy(*path, getenv("TS_SOCKET"));
        return;
    }

    /* ... if the $TS_SOCKET doesn't exist ... */

    /* Create the path */
    tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL)
        tmpdir = "/tmp";

    sprintf(userid, "%u", (unsigned int) getuid());

    /* Calculate the size */
    size = strlen(tmpdir) + strlen("/socket-ts.") + strlen(userid) + 1;

    /* Freed after preparing the socket address */
    *path = (char *) malloc(size);

    sprintf(*path, "%s/socket-ts.%s", tmpdir, userid);
}

int try_connect(int s)
{
    struct sockaddr_un addr;
    int res;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);

    res = connect(s, (struct sockaddr *) &addr, sizeof(addr));
    return res;
}

void wait_server_up(int fd)
{
    char a;

    read(fd, &a, 1);
    close(fd);
}

/* Returns the fd where to wait for the parent notification */
static int fork_server()
{
    int pid;
    int p[2];

    /* !!! stdin/stdout */
    pipe(p);

    pid = fork();
    switch (pid)
    {
        case 0: /* Child */
            close(p[0]);
            close(server_socket);
            /* Close all std handles for the server */
            close(0);
            close(1);
            close(2);
            setsid();
            server_main(p[1], socket_path);
            exit(0);
            break;
        case -1: /* Error */
            return -1;
        default: /* Parent */
            close(p[1]);
    }
    /* Return the read fd */
    return p[0];
}

void notify_parent(int fd)
{
    char a = 'a';
    write(fd, &a, 1);
    close(fd);
}

int ensure_server_up()
{
    int res;
    int notify_fd;

    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1)
        error("getting the server socket");

    create_socket_path(&socket_path);

    res = try_connect(server_socket);

    /* Good connection */
    if (res == 0)
        return 1;

    /* If error other than "No one listens on the other end"... */
    if (!(errno == ENOENT || errno == ECONNREFUSED))
        error("c: cannot connect to the server");

    if (errno == ECONNREFUSED)
        unlink(socket_path);

    /* Try starting the server */
    notify_fd = fork_server();
    wait_server_up(notify_fd);
    res = try_connect(server_socket);

    /* The second time didn't work. Abort. */
    if (res == -1)
    {
        fprintf(stderr, "The server didn't come up.\n");
        exit(-1);
    }

    free(socket_path);

    /* Good connection on the second time */
    return 1;
}
