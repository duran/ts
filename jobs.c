#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "msg.h"

static enum
{
    FREE,
    WAITING
} state = FREE;

struct Job
{
    int jobid;
    char *command;
    enum Jobstate state;
    struct Job *next;
};

/* Globals */
static struct Job *firstjob = 0;
static jobids = 0;

static void send_list_line(int s, const char * str)
{
    struct msg m;
    int res;

    m.type = LIST_LINE;

    strcpy(m.u.line, str);

    res = write(s, &m, sizeof(m));
    if(res == -1)
        perror("write");
}

void s_list(int s)
{
    int i;
    struct Job *p;
    char buffer[LINE_LEN];

    sprintf(buffer, "ID\tState\tCommand\n");
    send_list_line(s,buffer);

    p = firstjob;

    while(p != 0)
    {
        sprintf(buffer, "%i\t%i\t%s\n",
                p->jobid,
                p->state,
                p->command);
        send_list_line(s,buffer);
        p = p->next;
    }
}

static struct Job * newjobptr()
{
    struct Job *p;

    if (firstjob == 0)
    {
        firstjob = (struct Job *) malloc(sizeof(*firstjob));
        firstjob->next = 0;
        return firstjob;
    }

    p = firstjob;
    while(p->next != 0)
        p = p->next;

    p->next = (struct Job *) malloc(sizeof(*p));
    p->next->next = 0;

    return p->next;
}

/* Returns job id */
int s_newjob(int s, struct msg *m)
{
    struct Job *p;
    int res;

    p = newjobptr();

    p->jobid = jobids++;
    p->state = QUEUED;

    /* load the command */
    p->command = malloc(m->u.newjob.command_size);
    /* !!! Check retval */
    res = recv_bytes(s, p->command, m->u.newjob.command_size);
    assert(res != -1);

    return p->jobid;
}

void s_removejob(int jobid)
{
    struct Job *p;
    struct Job *newnext;

    if (firstjob->jobid == jobid)
    {
        struct Job *newfirst;
        /* First job is to be removed */
        newfirst = firstjob->next;
        free(firstjob->command);
        free(firstjob);
        firstjob = newfirst;
        return;
    }

    p = firstjob;
    /* Not first job */
    while (p->next != 0)
    {
        if (p->next->jobid == jobid)
            break;
        p = p->next;
    }
    assert(p->next != 0);

    newnext = p->next->next;

    free(p->next->command);
    free(p->next);
    p->next = newnext;
}

/* -1 if no one should be run. */
int next_run_job()
{
    if (state == WAITING)
        return -1;

    if (firstjob != 0)
    {
        state = WAITING;
        return firstjob->jobid;
    }

    return -1;
}

void job_finished()
{
    struct Job *newfirst;

    assert(state == WAITING);
    assert(firstjob != 0);

    newfirst = firstjob->next;
    free(firstjob);
    firstjob = newfirst;

    state = FREE;
}
