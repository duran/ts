/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include "msg.h"
#include "main.h"

void send_bytes(const int fd, const char *data, const int bytes)
{
    int res;
    /* Send the message */
    res = send(fd, data, bytes, 0);
    if(res == -1)
        warning("Sending %i bytes to %i.", bytes, fd);
}

int recv_bytes(const int fd, char *data, const int bytes)
{
    int res;
    /* Send the message */
    res = recv(fd, data, bytes, 0);
    if(res == -1)
        warning("Receiving %i bytes from %i.", bytes, fd);

    return res;
}

void send_msg(const int fd, const struct msg *m)
{
    int res;
    /* Send the message */
    msgdump(m);
    res = send(fd, m, sizeof(*m), 0);
    if(res == -1)
        warning("Sending a message to %i.", fd);
}

int recv_msg(const int fd, struct msg *m)
{
    int res;
    /* Send the message */
    res = recv(fd, m, sizeof(*m), 0);
    if(res == -1)
        warning("Receiving a message from %i.", fd);
    if (res == sizeof(*m))
        msgdump(m);

    return res;
}
