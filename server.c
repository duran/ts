/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>

#include <stdio.h>

#include "main.h"

enum
{
    MAXCONN=1000
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
static int max_descriptors;

static void sigterm_handler(int n)
{
    const char *dumpfilename;
    int fd;

    /* Dump the job list if we should to */
    dumpfilename = getenv("TS_SAVELIST");
    if (dumpfilename != NULL)
    {
        fd = open(dumpfilename, O_WRONLY | O_APPEND | O_CREAT, 0600);
        if (fd != -1)
        {
            joblist_dump(fd);
            close(fd);
        } else
            warning("The TS_SAVELIST file \"%s\" cannot be opened",
                    dumpfilename);
    }

    /* path will be initialized for sure, before installing the handler */
    unlink(path);
    exit(1);
}

static void install_sigterm_handler()
{
  struct sigaction act;

  act.sa_handler = sigterm_handler;
  /* Reset the mask */
  memset(&act.sa_mask,0,sizeof(act.sa_mask));
  act.sa_flags = 0;

  sigaction(SIGTERM, &act, NULL);
}

static int get_max_descriptors()
{
    const int MARGIN = 5; /* stdin, stderr, listen socket, and whatever */
    int max;
    struct rlimit rlim;
    int res;

    max = MAXCONN;
    if (max > FD_SETSIZE)
        max = FD_SETSIZE;

    /* I'd like to use OPEN_MAX or NR_OPEN, but I don't know if any
     * of them is POSIX compliant */

    res = getrlimit(RLIMIT_NOFILE, &rlim);
    if (res != 0)
        warning("getrlimit for open files");
    else
    {
        if (max > rlim.rlim_cur)
            max = rlim.rlim_cur;
    }

    if (max - MARGIN < 1)
        error("Too few opened descriptors available");

    return max - MARGIN;
}

void server_main(int notify_fd, char *_path)
{
    int ls;
    struct sockaddr_un addr;
    int res;

    process_type = SERVER;
    max_descriptors = get_max_descriptors();

    path = _path;

    nconnections = 0;

    ls = socket(AF_UNIX, SOCK_STREAM, 0);
    if(ls == -1)
        error("cannot create the listen socket in the server");

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    res = bind(ls, (struct sockaddr *) &addr, sizeof(addr));
    if (res == -1)
        error("Error binding.");

    res = listen(ls, 0);
    if (res == -1)
        error("Error listening.");

    install_sigterm_handler();

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
        maxfd = 0;
        /* If we can accept more connections, go on.
         * Otherwise, the system block them (no accept will be done). */
        if (nconnections < max_descriptors)
        {
            FD_SET(ls,&readset);
            maxfd = ls;
        }
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
            if (cs == -1)
                error("Accepting from %i", ls);
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
    res = recv_msg(s, &m);
    if (res == -1)
    {
        warning("client read");
        close(s);
        remove_connection(index);
        /* It will not fail, even if the index is not a notification */
        s_remove_notification(index);
        return NOBREAK;
    }
    if (res == 0)
    {
        close(s);
        remove_connection(index);
        /* It will not fail, even if the index is not a notification */
        s_remove_notification(index);
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
            if (res != m.u.output.ofilename_size)
                error("Reading the ofilename");
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
        job_finished(&m.u.result);
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

    if (m.type == URGENT)
    {
        s_move_urgent(client_cs[index].socket, m.u.jobid);
    }

    if (m.type == SWAP_JOBS)
    {
        s_swap_jobs(client_cs[index].socket, m.u.swap.jobid1,
                m.u.swap.jobid2);
    }

    if (m.type == GET_STATE)
    {
        s_send_state(client_cs[index].socket, m.u.jobid);
    }

    return NOBREAK; /* normal */
}

static void s_runjob(int index)
{
    int s;
    struct msg m;
    
    if (!client_cs[index].hasjob)
        error("Run job of the client %i which doesn't have any job", index);

    s = client_cs[index].socket;

    m.type = RUNJOB;

    send_msg(s, &m);
}

static void s_newjob_ok(int index)
{
    int s;
    struct msg m;
    
    if (!client_cs[index].hasjob)
        error("Run job of the client %i which doesn't have any job", index);

    s = client_cs[index].socket;

    m.type = NEWJOB_OK;
    m.u.jobid = client_cs[index].jobid;

    send_msg(s, &m);
}

static void dump_conn_struct(FILE *out, const struct Client_conn *p)
{
    fprintf(out, "  new_conn\n");
    fprintf(out, "    socket %i\n", p->socket);
    fprintf(out, "    hasjob \"%i\"\n", p->hasjob);
    fprintf(out, "    jobid %i\n", p->jobid);
}

void dump_conns_struct(FILE *out)
{
    int i;

    fprintf(out, "New_conns");

    for(i=0; i < nconnections; ++i)
    {
        dump_conn_struct(out, &client_cs[i]);
    }
}
