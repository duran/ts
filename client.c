#include <assert.h>
#include <stdio.h>
#include "msg.h"
#include "main.h"

static char *client_command = 0;
static void c_end_of_job();

void c_new_job(const char *command)
{
    struct msg m;
    int res;

    m.type = NEWJOB;

    /* global */
    client_command = command;

    strcpy(m.u.command, command);

    res = send(server_socket, &m, sizeof(m), 0);
    if(res == -1)
    {
        perror("c: send");
        exit(-1);
    }
}

void c_wait_server_commands()
{
    struct msg m;
    int res;

    while (1)
    {
        res = recv(server_socket, &m, sizeof(m), 0);
        if(res == -1)
        {
            perror("read");
            exit(-1);
        }

        if (res == 0)
            break;
        if (res != sizeof(m))
        {
            fprintf(stderr, "c: recv() message size wrong: %i instead of %i\n",
                res, (int) sizeof(m));
        }
        assert(res == sizeof(m));
        msgdump(&m);
        if (m.type == NEWJOB_OK)
            ;
        if (m.type == RUNJOB)
        {
            run_job(client_command);
            c_end_of_job();
            break;
        }
    }
    close(server_socket);
}

void c_wait_server_lines()
{
    struct msg m;
    int res;

    while (1)
    {
        res = recv(server_socket, &m, sizeof(m),0);
        if(res == -1)
            perror("read");

        if (res == 0)
            break;
        assert(res == sizeof(m));
        msgdump(&m);
        if (m.type == LIST_LINE)
        {
            printf("%s", m.u.line);
        }
    }
}

void c_list_jobs()
{
    struct msg m;
    int res;

    m.type = LIST;

    res = send(server_socket, &m, sizeof(m), 0);
    if(res == -1)
        perror("send");
}

static void c_end_of_job()
{
    struct msg m;
    int res;

    m.type = ENDJOB;

    res = send(server_socket, &m, sizeof(m),0);
    if(res == -1)
        perror("send");
}

int c_shutdown_server()
{
    struct msg m;
    int res;

    m.type = KILL;
    res = send(server_socket, &m, sizeof(m), 0);
    assert(res != -1);
}
