#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "msg.h"
#include "main.h"

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
    char *output_filename;
    int store_output;
    int pid;
};

/* Globals */
static struct Job *firstjob = 0;
static struct Job *first_finished_job = 0;
static jobids = 0;

static void send_list_line(int s, const char * str)
{
    struct msg m;
    int res;

    /* Message */
    m.type = LIST_LINE;
    m.u.line_size = strlen(str) + 1;

    send_msg(s, &m);

    /* Send the line */
    send_bytes(s, str, m.u.line_size);
}

static struct Job * findjob(int jobid)
{
    struct Job *p;

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        if (p->jobid == jobid)
            return p;
        p = p->next;
    }

    return 0;
}

static struct Job * find_finished_job(int jobid)
{
    struct Job *p;

    /* Show Queued or Running jobs */
    p = first_finished_job;
    while(p != 0)
    {
        if (p->jobid == jobid)
            return p;
        p = p->next;
    }

    return 0;
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

    sprintf(buffer, " ID\tState\tOutput\tCommand\n");
    send_list_line(s,buffer);

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        const char * jobstate;
        const char * output_filename;
        jobstate = jstate2string(p->state);
        if (p->store_output)
        {
            if (p->state == RUNNING)
            {
                if (p->output_filename == 0)
                    /* This may happen due to concurrency
                     * problems */
                    output_filename = "(...)";
                else
                    output_filename = p->output_filename;
            } else
                output_filename = "(file)";
        } else
            output_filename = "stdout";

           
        sprintf(buffer, "%i\t%s\t%s\t%s\n",
                p->jobid,
                jobstate,
                output_filename,
                p->command);
        send_list_line(s,buffer);
        p = p->next;
    }

    p = first_finished_job;

    if (p != 0)
    {
        sprintf(buffer, "Finsihed jobs:\n");
        send_list_line(s,buffer);

        sprintf(buffer, " ID\tState\tOutput\tE-level\tCommand\n");
        send_list_line(s,buffer);

        /* Show Finished jobs */
        while(p != 0)
        {
            const char * jobstate;
            const char * output_filename;
            jobstate = jstate2string(p->state);
            if (p->output_filename == 0)
                output_filename = "stdout";
            else
                output_filename = p->output_filename;
            sprintf(buffer, "%i\t%s\t%s\t%i\t%s\n",
                    p->jobid,
                    jobstate,
                    output_filename,
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
    p->store_output = m->u.newjob.store_output;

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
    newfirst = firstjob->next;
    new_finished_job(firstjob);

    /* Remove it from the run queue */
    firstjob = newfirst;

    state = FREE;
}

void s_clear_finished()
{
    struct Job *p;

    if (first_finished_job == 0)
        return;

    p = first_finished_job;
    first_finished_job = 0;

    while (p->next != 0)
    {
        struct Job *tmp;
        tmp = p->next;
        free(p->command);
        free(p->output_filename);
        free(p);
        p = tmp;
    }

    free(p->next);
}

void s_process_runjob_ok(int jobid, char *oname, int pid)
{
    struct Job *p;
    p = findjob(jobid);
    assert(p != 0);
    assert(p->state == RUNNING);

    p->pid = pid;
    p->output_filename = oname;
}

void s_send_output(int s, int jobid)
{
    struct Job *p = 0;
    struct msg m;

    if (jobid == -1)
    {
        /* This means that we want the tail of the running task, or that
         * of the last job run */
        if (state == WAITING)
        {
            p = firstjob;
            assert(p != 0);
        }
        else
        {
            p = first_finished_job;
            if (p == 0)
            {
                send_list_line(s, "No jobs.\n");
                return;
            }
            while(p->next != 0)
                p = p->next;
        }
    } else
    {
        if (state == WAITING && firstjob->jobid == jobid)
            p = firstjob;
        else
            p = find_finished_job(jobid);
    }

    if (p == 0)
    {
        char tmp[50];
        sprintf(tmp, "Job %i not finished or not running.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    m.type = ANSWER_OUTPUT;
    if (!p->store_output)
    {
        send_list_line(s, "The job hasn't output stored.\n");
        return;
    }
    m.u.output.store_output = p->store_output;
    m.u.output.pid = p->pid;
    m.u.output.ofilename_size = strlen(p->output_filename) + 1;
    send_msg(s, &m);
    send_bytes(s, p->output_filename, m.u.output.ofilename_size);
}
