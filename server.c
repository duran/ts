#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>

#include "msg.h"
#include "main.h"


const char path[] = "/tmp/prova.socket";

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

void server_main()
{
    int ls,cs;
    struct sockaddr_un addr;
    int res;

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

    msgdump(&m);

    /* Process message */
    if (m.type == KILL)
        return BREAK; /* break in the parent*/

    if (m.type == NEWJOB)
    {
        client_cs[index].jobid = s_newjob(s, &m);
        client_cs[index].hasjob = 1;
        s_newjob_ok(index);
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
