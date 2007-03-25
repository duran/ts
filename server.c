#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdio.h>

#include "msg.h"
#include "main.h"

enum
{
    MAXCONN=100
};

enum Break
{
    BREAK,
    NOBREAK
};

/* Prototypes */
static void server_loop(int ls);
static enum Break
    client_read(int index);
static void end_server(int ls);
static void s_newjob_ok(int index);
static void s_runjob(int index);

struct Client_conn
{
    int socket;
    int hasjob;
    int jobid;
};

/* Globals */
static struct Client_conn client_cs[MAXCONN];
static int nconnections;
static char *path;

void server_main(int notify_fd, char *_path)
{
    int ls;
    struct sockaddr_un addr;
    int res;

    path = _path;

    nconnections = 0;

    ls = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(ls != -1);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    res = bind(ls, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1)
    {
        perror("Error binding.");
        return;
    }

    res = listen(ls, 0);
    if (res == -1)
    {
        perror("Error listening.");
        return;
    }

    notify_parent(notify_fd);

    server_loop(ls);
}

static int get_conn_of_jobid(int jobid)
{
    int i;
    for(i=0; i< nconnections; ++i)
        if (client_cs[i].hasjob && client_cs[i].jobid == jobid)
            return i;
    return -1;
}

static void server_loop(int ls)
{
    fd_set readset;
    int i;
    int maxfd;
    int keep_loop = 1;
    int newjob;

    while (keep_loop)
    {
        FD_ZERO(&readset);
        FD_SET(ls,&readset);
        maxfd = ls;
        for(i=0; i< nconnections; ++i)
        {
            FD_SET(client_cs[i].socket, &readset);
            if (client_cs[i].socket > maxfd)
                maxfd = client_cs[i].socket;
        }
        select(maxfd + 1, &readset, NULL, NULL, NULL);
        if (FD_ISSET(ls,&readset))
        {
            int cs;
            cs = accept(ls, NULL, NULL);
            assert(cs != -1);
            client_cs[nconnections++].socket = cs;
        }
        for(i=0; i< nconnections; ++i)
            if (FD_ISSET(client_cs[i].socket, &readset))
            {
                enum Break b;
                b = client_read(i);
                /* Check if we should break */
                if (b == BREAK)
                    keep_loop = 0;
            }
        /* This will return firstjob->jobid or -1 */
        newjob = next_run_job();
        if (newjob != -1)
        {
            int conn;
            conn = get_conn_of_jobid(newjob);
            /* This next marks the firstjob state to RUNNING */
            s_mark_job_running();
            s_runjob(conn);
        }
    }

    end_server(ls);
}

static void end_server(int ls)
{
    close(ls);
    unlink(path);
    /* This comes from the parent, in the fork after server_main.
     * This is the last use of path in this process.*/
    free(path); 
}

static void remove_connection(int index)
{
    int i;

    if(client_cs[index].hasjob)
    {
        s_removejob(client_cs[index].jobid);
    }

    for(i=index; i<(nconnections-1); ++i)
    {
        memcpy(&client_cs[i], &client_cs[i+1], sizeof(client_cs[0]));
    }
    nconnections--;
}


static enum Break
    client_read(int index)
{
    struct msg m;
    int s;
    int res;

    s = client_cs[index].socket;

    /* Read the message */
    res = recv(s, &m, sizeof(m),0);
    if (res == -1)
    {
        fprintf(stderr, "Error reading from client %i, socket %i\n",
                index, s);
        perror("client read");
        exit(-1);
    }
    if (res == 0)
    {
        close(s);
        remove_connection(index);
        return NOBREAK;
    }

    client_cs[index].hasjob = 0;

    /* Process message */
    if (m.type == KILL_SERVER)
        return BREAK; /* break in the parent*/

    if (m.type == NEWJOB)
    {
        client_cs[index].jobid = s_newjob(s, &m);
        client_cs[index].hasjob = 1;
        s_newjob_ok(index);
    }

    if (m.type == RUNJOB_OK)
    {
        char *buffer = 0;
        if (m.u.output.store_output)
        {
            /* Receive the output filename */
            buffer = (char *) malloc(m.u.output.ofilename_size);
            res = recv_bytes(client_cs[index].socket, buffer,
                m.u.output.ofilename_size);
            assert(res == m.u.output.ofilename_size);
        }
        s_process_runjob_ok(client_cs[index].jobid, buffer,
                m.u.output.pid);
    }

    if (m.type == LIST)
    {
        s_list(client_cs[index].socket);
        /* We must actively close, meaning End of Lines */
        close(client_cs[index].socket);
        remove_connection(index);
    }

    if (m.type == ENDJOB)
    {
        job_finished(m.u.errorlevel);
        check_notify_list(client_cs[index].jobid);
    }

    if (m.type == CLEAR_FINISHED)
    {
        s_clear_finished();
    }

    if (m.type == ASK_OUTPUT)
    {
        s_send_output(client_cs[index].socket, m.u.jobid);
    }

    if (m.type == REMOVEJOB)
    {
        s_remove_job(client_cs[index].socket, m.u.jobid);
    }

    if (m.type == WAITJOB)
    {
        s_wait_job(client_cs[index].socket, m.u.jobid);
    }

    return NOBREAK; /* normal */
}

static void s_runjob(int index)
{
    int s;
    struct msg m;
    int res;
    
    assert(client_cs[index].hasjob);

    s = client_cs[index].socket;

    m.type = RUNJOB;

    res = send(s, &m, sizeof(m), 0);
    if(res == -1)
        perror("send");
}

static void s_newjob_ok(int index)
{
    int s;
    struct msg m;
    int res;
    
    assert(client_cs[index].hasjob);

    s = client_cs[index].socket;

    m.type = NEWJOB_OK;
    m.u.jobid = client_cs[index].jobid;

    res = send(s, &m, sizeof(m), 0);
    if(res == -1)
        perror("send");
}
