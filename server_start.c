#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "main.h"

extern int server_socket;

static int fork_server();

int try_connect(int s)
{
    struct sockaddr_un addr;
    int res;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/prova.socket");

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
            server_main(p[1]);
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

    server_socket = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(server_socket != -1);

    res = try_connect(server_socket);

    /* Good connection */
    if (res == 0)
        return 1;

    /* If error other than "No one listens on the other end"... */
    if (!(errno == ENOENT || errno == ECONNREFUSED))
    {
        perror("c: cannot connect to the server");
        exit(-1);
    }

    if (errno == ECONNREFUSED)
        unlink("/tmp/prova.socket");

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

    /* Good connection on the second time */
    return 1;
}
