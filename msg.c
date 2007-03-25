#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"

void send_bytes(const int fd, const char *data, const int bytes)
{
    int res;
    /* Send the message */
    res = send(fd, data, bytes, 0);
    if(res == -1)
    {
        perror("c: send");
        exit(-1);
    }
}

int recv_bytes(const int fd, char *data, const int bytes)
{
    int res;
    /* Send the message */
    res = recv(fd, data, bytes, 0);

    return res;
}

void send_msg(const int fd, const struct msg *m)
{
    int res;
    /* Send the message */
    msgdump(m);
    res = send(fd, m, sizeof(*m), 0);
    if(res == -1)
    {
        perror("c: send");
        exit(-1);
    }
}

int recv_msg(const int fd, struct msg *m)
{
    int res;
    /* Send the message */
    res = recv(fd, m, sizeof(*m), 0);
    if (res == sizeof(*m))
        msgdump(m);

    return res;
}
