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

void c_wait_server_commands(const char *my_command, int store_output)
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
            /* This will send RUNJOB_OK */
            errorlevel = run_job(my_command, store_output);
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
        res = recv_msg(server_socket, &m);
        if(res == -1)
        {
            perror("read");
            exit(-1);
        }

        if (res == 0)
            break;
        assert(res == sizeof(m));
        if (m.type == LIST_LINE)
        {
            char * buffer;
            buffer = (char *) malloc(m.u.line_size);
            recv_bytes(server_socket, buffer, m.u.line_size);
            printf("%s", buffer);
            free(buffer);
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

void c_send_runjob_ok(int store_output, const char *ofname)
{
    struct msg m;
    int res;

    /* Prepare the message */
    m.type = RUNJOB_OK;
    m.u.runjob_ok.store_output = store_output;
    if (store_output)
        m.u.runjob_ok.ofilename_size = strlen(ofname) + 1;
    else
        m.u.runjob_ok.ofilename_size = 0;

    send_msg(server_socket, &m);

    /* Send the filename */
    if (store_output)
        send_bytes(server_socket, ofname, m.u.runjob_ok.ofilename_size);
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
