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
    m.u.newjob.store_output = command_line.store_output;

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
        if (m.type == NEWJOB_OK)
            ;
        if (m.type == RUNJOB)
        {
            int errorlevel;
            /* This will send RUNJOB_OK */
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

void c_send_runjob_ok(const char *ofname)
{
    struct msg m;
    int res;

    /* Prepare the message */
    m.type = RUNJOB_OK;
    m.u.output.store_output = command_line.store_output;
    if (command_line.store_output)
        m.u.output.ofilename_size = strlen(ofname) + 1;
    else
        m.u.output.ofilename_size = 0;

    send_msg(server_socket, &m);

    /* Send the filename */
    if (command_line.store_output)
        send_bytes(server_socket, ofname, m.u.output.ofilename_size);
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

    m.type = KILL_SERVER;
    send_msg(server_socket, &m);
}

int c_clear_finished()
{
    struct msg m;

    m.type = CLEAR_FINISHED;
    send_msg(server_socket, &m);
}

void c_tail()
{
    struct msg m;
    int res;
    char *string = 0;

    /* Send the request */
    m.type = ASK_OUTPUT;
    m.u.jobid = command_line.jobid;
    send_msg(server_socket, &m);

    /* Receive the answer */
    res = recv_msg(server_socket, &m);
    assert(res == sizeof(m));
    switch(m.type)
    {
    case ANSWER_OUTPUT:
        if (m.u.output.store_output)
        {
            /* Receive the output file name */
            string = (char *) malloc(m.u.output.ofilename_size);
            recv_bytes(server_socket, string, m.u.output.ofilename_size);
            close(server_socket);
            c_run_tail(string);
            /* WILL NOT RETURN */
        }
        fprintf(stderr, "The output is not stored. Cannot tail.\n");
        exit(-1);
        /* WILL NOT GO FURTHER */
    case LIST_LINE: /* Only ONE line accepted */
        string = (char *) malloc(m.u.line_size);
        res = recv_bytes(server_socket, string, m.u.line_size);
        assert(res == m.u.line_size);
        fprintf(stderr, "Error in the request: %s", 
                string);
        exit(-1);
        /* WILL NOT GO FURTHER */
    }
}
