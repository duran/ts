#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "msg.h"

static enum
{
    FREE, /* No task is running */
    WAITING /* A task is running, and the server is waiting */
} state = FREE;

struct Job
{
    int jobid;
    char *command;
    enum Jobstate state;
    int errorlevel;
    struct Job *next;
};

/* Globals */
static struct Job *firstjob = 0;
static struct Job *first_finished_job = 0;
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

void s_mark_job_running()
{
    firstjob->state = RUNNING;
}

static const char * jstate2string(enum Jobstate s)
{
    char * jobstate;
    switch(s)
    {
        case QUEUED:
            jobstate = "queued";
            break;
        case RUNNING:
            jobstate = "running";
            break;
        case FINISHED:
            jobstate = "finished";
            break;
    }
    return jobstate;
}

void s_list(int s)
{
    int i;
    struct Job *p;
    char buffer[LINE_LEN];

    sprintf(buffer, " ID\tState\tCommand\n");
    send_list_line(s,buffer);

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        const char * jobstate;
        jobstate = jstate2string(p->state);
        sprintf(buffer, "%i\t%s\t%s\n",
                p->jobid,
                jobstate,
                p->command);
        send_list_line(s,buffer);
        p = p->next;
    }

    p = first_finished_job;

    if (p != 0)
    {
        sprintf(buffer, "Finsihed jobs:\n");
        send_list_line(s,buffer);

        sprintf(buffer, " ID\tState\tE-level\tCommand\n");
        send_list_line(s,buffer);

        /* Show Finished jobs */
        while(p != 0)
        {
            const char * jobstate;
            jobstate = jstate2string(p->state);
            sprintf(buffer, "%i\t%s\t%i\t%s\n",
                    p->jobid,
                    jobstate,
                    p->errorlevel,
                    p->command);
            send_list_line(s,buffer);
            p = p->next;
        }
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

/* Add the job to the finished queue. */
static void new_finished_job(struct Job *j)
{
    struct Job *p;

    if (first_finished_job == 0)
    {
        first_finished_job = j;
        first_finished_job->next = 0;
        return;
    }

    p = first_finished_job;
    while(p->next != 0)
        p = p->next;

    p->next = j;
    p->next->next = 0;

    return;
}

void job_finished(int errorlevel)
{
    struct Job *newfirst;

    assert(state == WAITING);
    assert(firstjob != 0);

    assert(firstjob->state == RUNNING);

    /* Mark state */
    firstjob->state = FINISHED;
    firstjob->errorlevel = errorlevel;

    /* Add it to the finished queue */
    new_finished_job(firstjob);

    /* Remove it from the run queue */
    firstjob = firstjob->next;

    state = FREE;
}

void s_clear_finished()
{
    struct Job *p;

    p = first_finished_job->next;
    if (p == 0)
    {
        free(first_finished_job);
        first_finished_job = 0;
        return;
    }

    while (p->next != 0)
    {
        struct Job *tmp;
        tmp = p->next;
        free(p);
        p = tmp;
    }

    free(p->next);
}
