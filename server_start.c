#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

extern int server_socket;

int try_connect(int s)
{
    struct sockaddr_un addr;
    int res;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/prova.socket");

    res = connect(s, (struct sockaddr *) &addr, sizeof(addr));
    return res;
}

void wait_server_up()
{
    printf("Wait server up\n");
    sleep(1);
}

void fork_server()
{
    int pid;

    /* !!! stdin/stdout */

    pid = fork();
    switch (pid)
    {
        case 0: /* Child */
            server_main();
            break;
        case -1: /* Error */
            return;
        default: /* Parent */
            ;
    }
}

int ensure_server_up()
{
    int res;

    server_socket = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(server_socket != -1);

    res = try_connect(server_socket);

    /* Good connection */
    if (res == 0)
        return 1;

    /* If error other than "No one listens on the other end"... */
    if (errno != ENOENT)
        return 0;

    /* Try starting the server */
    fork_server();
    wait_server_up();
    res = try_connect(server_socket);

    /* The second time didn't work. Abort. */
    if (res == -1)
        return 0;

    printf("Good connection 2\n");
    /* Good connection on the second time */
    return 1;
}
