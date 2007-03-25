#include <assert.h>
#include <stdio.h>
#include "msg.h"
#include "main.h"

static void c_end_of_job(int errorlevel);

void c_new_job(const char *command)
{
    struct msg m;

    m.type = NEWJOB;

    /* global */
    m.u.newjob.command_size = strlen(command) + 1; /* add null */

    /* Send the message */
    send_msg(server_socket, &m);

    /* Send the command */
    send_bytes(server_socket, command, m.u.newjob.command_size);
}

void c_wait_server_commands(const char *my_command)
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
            int errorlevel;
            errorlevel = run_job(my_command);
            c_end_of_job(errorlevel);
            break;
        }
    }
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

static void c_end_of_job(int errorlevel)
{
    struct msg m;
    int res;

    m.type = ENDJOB;
    m.u.errorlevel = errorlevel;

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

int c_clear_finished()
{
    struct msg m;
    int res;

    m.type = CLEAR_FINISHED;
    res = send(server_socket, &m, sizeof(m), 0);
    assert(res != -1);
}
