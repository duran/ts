/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "main.h"

static enum
{
    FREE, /* No task is running */
    WAITING /* A task is running, and the server is waiting */
} state = FREE;

struct Notify
{
    int socket;
    int jobid;
    struct Notify *next;
};

/* Globals */
static struct Job *firstjob = 0;
static struct Job *first_finished_job = 0;
static int jobids = 0;

static struct Notify *first_notify = 0;

static void send_list_line(int s, const char * str)
{
    struct msg m;

    /* Message */
    m.type = LIST_LINE;
    m.u.size = strlen(str) + 1;

    send_msg(s, &m);

    /* Send the line */
    send_bytes(s, str, m.u.size);
}

static void send_urgent_ok(int s)
{
    struct msg m;

    /* Message */
    m.type = URGENT_OK;

    send_msg(s, &m);
}

static void send_swap_jobs_ok(int s)
{
    struct msg m;

    /* Message */
    m.type = SWAP_JOBS_OK;

    send_msg(s, &m);
}

static struct Job * find_previous_job(const struct Job *final)
{
    struct Job *p;

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        if (p->next == final)
            return p;
        p = p->next;
    }

    return 0;
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

const char * jstate2string(enum Jobstate s)
{
    const char * jobstate;
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
        case SKIPPED:
            jobstate = "skipped";
            break;
    }
    return jobstate;
}

void s_list(int s)
{
    struct Job *p;
    char *buffer;

    /* Times:   0.00/0.00/0.00 - 4+4+4+2 = 14*/ 
    buffer = joblist_headers();
    send_list_line(s,buffer);
    free(buffer);

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        buffer = joblist_line(p);
        send_list_line(s,buffer);
        free(buffer);
        p = p->next;
    }

    p = first_finished_job;

    /* Show Finished jobs */
    while(p != 0)
    {
        buffer = joblist_line(p);
        send_list_line(s,buffer);
        free(buffer);
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
        firstjob->output_filename = 0;
        firstjob->command = 0;
        return firstjob;
    }

    p = firstjob;
    while(p->next != 0)
        p = p->next;

    p->next = (struct Job *) malloc(sizeof(*p));
    p->next->next = 0;
    p->next->output_filename = 0;
    p->next->command = 0;

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
    p->should_keep_finished = m->u.newjob.should_keep_finished;
    p->depend = m->u.newjob.depend;

    pinfo_init(&p->info);
    pinfo_set_enqueue_time(&p->info);

    /* load the command */
    p->command = malloc(m->u.newjob.command_size);
    if (p->command == 0)
        error("Cannot allocate memory in s_newjob command_size (%i)",
                m->u.newjob.command_size);
    res = recv_bytes(s, p->command, m->u.newjob.command_size);
    if (res == -1)
        error("wrong bytes received");

    /* load the label */
    p->label = 0;
    if (m->u.newjob.label_size > 0)
    {
        char *ptr;
        ptr = (char *) malloc(m->u.newjob.label_size);
        if (ptr == 0)
            error("Cannot allocate memory in s_newjob env_size(%i)",
                    m->u.newjob.env_size);
        res = recv_bytes(s, ptr, m->u.newjob.label_size);
        if (res == -1)
            error("wrong bytes received");
        p->label = ptr;
    }

    /* load the info */
    if (m->u.newjob.env_size > 0)
    {
        char *ptr;
        ptr = (char *) malloc(m->u.newjob.env_size);
        if (ptr == 0)
            error("Cannot allocate memory in s_newjob env_size(%i)",
                    m->u.newjob.env_size);
        res = recv_bytes(s, ptr, m->u.newjob.env_size);
        if (res == -1)
            error("wrong bytes received");
        pinfo_addinfo(&p->info, m->u.newjob.env_size+100,
                "Environment:\n%s", ptr);
        free(ptr);
    }

    return p->jobid;
}

/* This assumes the jobid exists */
void s_removejob(int jobid)
{
    struct Job *p;
    struct Job *newnext;

    if (firstjob->jobid == jobid)
    {
        struct Job *newfirst;

        if (state == WAITING)
            state = FREE;

        /* First job is to be removed */
        newfirst = firstjob->next;
        free(firstjob->command);
        free(firstjob->output_filename);
        pinfo_free(&firstjob->info);
        free(firstjob->label);
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
    if (p->next == 0)
        error("Job to be removed not found. jobid=%i", jobid);

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

/* Returns 1000 if no limit, The limit otherwise. */
static int get_max_finished_jobs()
{
    char *limit;

    limit = getenv("TS_MAXFINISHED");
    if (limit == NULL)
        return 1000;
    return abs(atoi(limit));
}

/* Add the job to the finished queue. */
static void new_finished_job(struct Job *j)
{
    struct Job *p;
    int count, max;

    max = get_max_finished_jobs();
    count = 0;

    if (first_finished_job == 0 && count < max)
    {
        first_finished_job = j;
        first_finished_job->next = 0;
        return;
    }

    ++count;

    p = first_finished_job;
    while(p->next != 0)
    {
        p = p->next;
        ++count;
    }

    /* If too many jobs, wipe out the first */
    if (count >= max)
    {
        struct Job *tmp;
        tmp = first_finished_job;
        first_finished_job = first_finished_job->next;
        free(tmp->command);
        free(tmp->output_filename);
        pinfo_free(&tmp->info);
        free(tmp->label);
        free(tmp);
    }
    p->next = j;
    p->next->next = 0;

    return;
}

void job_finished(const struct Result *result)
{
    struct Job *newfirst;

    if (state != WAITING)
        error("Wrong state in the server. Not WAITING, but %i", state);
    if (firstjob == 0)
        error("on job finished, firstjob values %x", firstjob);

    if (firstjob->state != RUNNING)
        error("on job finished, the firstjob is not running but %i",
                firstjob->state);

    /* Mark state */
    if (result->skipped)
        firstjob->state = SKIPPED;
    else
        firstjob->state = FINISHED;
    firstjob->result = *result;
    pinfo_set_end_time(&firstjob->info);

    /* Add it to the finished queue */
    newfirst = firstjob->next;
    if (firstjob->should_keep_finished)
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

    while (p != 0)
    {
        struct Job *tmp;
        tmp = p->next;
        free(p->command);
        free(p->output_filename);
        pinfo_free(&p->info);
        free(p->label);
        free(p);
        p = tmp;
    }
}

void s_process_runjob_ok(int jobid, char *oname, int pid)
{
    struct Job *p;
    p = findjob(jobid);
    if (p == 0)
        error("Job %i already run not found on runjob_ok", jobid);
    if (p->state != RUNNING)
        error("Job %i not running, but %i on runjob_ok", jobid,
                p->state);

    p->pid = pid;
    p->output_filename = oname;
    pinfo_set_start_time(&p->info);
}

void s_job_info(int s, int jobid)
{
    struct Job *p = 0;
    struct msg m;

    if (jobid == -1)
    {
        /* This means that we want the job info of the running task, or that
         * of the last job run */
        if (state == WAITING)
        {
            p = firstjob;
            if (p == 0)
                error("Internal state WAITING, but job not run."
                        "firstjob = %x", firstjob);
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
        p = firstjob;
        while (p != 0 && p->jobid != jobid)
            p = p->next;

        /* Look in finished jobs if needed */
        if (p == 0)
        {
            p = first_finished_job;
            while (p != 0 && p->jobid != jobid)
                p = p->next;
        }
    }

    if (p == 0)
    {
        char tmp[50];
        sprintf(tmp, "Job %i not finished or not running.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    m.type = INFO_DATA;
    send_msg(s, &m);
    pinfo_dump(&p->info, s);
    fd_nprintf(s, 100, "Command: %s", p->depend?"&& ":"");
    write(s, p->command, strlen(p->command));
    fd_nprintf(s, 100, "\n");
    fd_nprintf(s, 100, "Enqueue time: %s",
            ctime(&p->info.enqueue_time.tv_sec));
    if (p->state == RUNNING)
    {
        fd_nprintf(s, 100, "Start time: %s",
                ctime(&p->info.start_time.tv_sec));
        fd_nprintf(s, 100, "Time running: %fs\n",
                pinfo_time_until_now(&p->info));
    } else if (p->state == FINISHED)
    {
        fd_nprintf(s, 100, "Start time: %s",
                ctime(&p->info.start_time.tv_sec));
        fd_nprintf(s, 100, "End time: %s",
                ctime(&p->info.end_time.tv_sec));
        fd_nprintf(s, 100, "Time run: %fs\n",
                pinfo_time_run(&p->info));
    }
}

void s_send_output(int s, int jobid)
{
    struct Job *p = 0;
    struct msg m;

    if (jobid == -1)
    {
        /* This means that we want the output info of the running task, or that
         * of the last job run */
        if (state == WAITING)
        {
            p = firstjob;
            if (p == 0)
                error("Internal state WAITING, but job not run."
                        "firstjob = %x", firstjob);
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

    if (p->state == SKIPPED)
    {
        char tmp[50];
        sprintf(tmp, "Job %i was skipped due to a dependency.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    m.type = ANSWER_OUTPUT;
    m.u.output.store_output = p->store_output;
    m.u.output.pid = p->pid;
    if (m.u.output.store_output)
        m.u.output.ofilename_size = strlen(p->output_filename) + 1;
    else
        m.u.output.ofilename_size = 0;
    send_msg(s, &m);
    send_bytes(s, p->output_filename, m.u.output.ofilename_size);
}

int s_remove_job(int s, int jobid)
{
    struct Job *p = 0;
    struct msg m;
    struct Job *before_p = 0;

    if (jobid == -1)
    {
        /* Find the last job added */
        p = firstjob;
        if (p != 0)
        {
            while (p->next != 0)
            {
                before_p = p;
                p = p->next;
            }
        }
    }
    else
    {
        p = firstjob;
        if (p != 0)
        {
            while (p->next != 0 && p->jobid != jobid)
            {
                before_p = p;
                p = p->next;
            }
        }
    }

    if (p == 0 || p->state != QUEUED || before_p == 0)
    {
        char tmp[50];
        if (jobid == -1)
            sprintf(tmp, "The last job cannot be removed.\n");
        else
            sprintf(tmp, "The job %i cannot be removed.\n", jobid);
        send_list_line(s, tmp);
        return 0;
    }

    before_p->next = p->next;
    free(p->command);
    free(p->output_filename);
    pinfo_free(&p->info);
    free(p->label);
    free(p);
    m.type = REMOVEJOB_OK;
    send_msg(s, &m);
    return 1;
}

static void add_to_notify_list(int s, int jobid)
{
    struct Notify *n;
    struct Notify *new;

    new = (struct Notify *) malloc(sizeof(*n));

    new->socket = s;
    new->jobid = jobid;
    new->next = 0;

    n = first_notify;
    if (n == 0)
    {
        first_notify = new;
        return;
    }

    while(n->next != 0)
        n = n->next;

    n->next = new;
}

static void send_waitjob_ok(int s, int errorlevel)
{
    struct msg m;

    m.type = WAITJOB_OK;
    m.u.result.errorlevel = errorlevel;
    send_msg(s, &m);
}

static struct Job *
get_job(int jobid)
{
    struct Job *j;

    j = findjob(jobid);
    if (j != NULL)
        return j;

    j = find_finished_job(jobid);

    if (j != NULL)
        return j;

    return 0;
}

/* Don't complain, if the socket doesn't exist */
void s_remove_notification(int s)
{
    struct Notify *n;
    struct Notify *previous;
    n = first_notify;
    while (n != 0 && n->socket != s)
        n = n->next;
    if (n == 0)
        return;

    /* Remove the notification */
    previous = first_notify;
    if (n == previous)
    {
        free(first_notify);
        first_notify = 0;
        return;
    }

    /* if not the first... */
    while(previous->next != n)
        previous = previous->next;

    previous->next = n->next;
    free(n);
}

/* This is called when a job finishes */
void check_notify_list(int jobid)
{
    struct Notify *n;
    struct Job *j;

    n = first_notify;
    while (n != 0 && n->jobid != jobid)
    {
        n = n->next;
    }

    if (n == 0)
    {
        return;
    }

    j = get_job(jobid);
    /* If the job finishes, notify the waiter */
    if (j->state == FINISHED)
    {
        send_waitjob_ok(n->socket, j->result.errorlevel);
        s_remove_notification(n->socket);
    }
}

void s_wait_job(int s, int jobid)
{
    struct Job *p = 0;

    if (jobid == -1)
    {
        /* Find the last job added */
        p = firstjob;

        if (p != 0)
            while (p->next != 0)
                p = p->next;

        /* Look in finished jobs if needed */
        if (p == 0)
        {
            p = first_finished_job;
            if (p != 0)
                while (p->next != 0)
                    p = p->next;
        }
    }
    else
    {
        p = firstjob;
        while (p != 0 && p->jobid != jobid)
            p = p->next;

        /* Look in finished jobs if needed */
        if (p == 0)
        {
            p = first_finished_job;
            while (p != 0 && p->jobid != jobid)
                p = p->next;
        }
    }

    if (p == 0)
    {
        char tmp[50];
        if (jobid == -1)
            sprintf(tmp, "The last job cannot be waited.\n");
        else
            sprintf(tmp, "The job %i cannot be waited.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    if (p->state == FINISHED)
    {
        send_waitjob_ok(s, p->result.errorlevel);
    }
    else
        add_to_notify_list(s, p->jobid);
}

void s_move_urgent(int s, int jobid)
{
    struct Job *p = 0;
    struct Job *tmp1;

    if (jobid == -1)
    {
        /* Find the last job added */
        p = firstjob;

        if (p != 0)
            while (p->next != 0)
                p = p->next;
    }
    else
    {
        p = firstjob;
        while (p != 0 && p->jobid != jobid)
            p = p->next;
    }

    if (p == 0 || firstjob->next == 0)
    {
        char tmp[50];
        if (jobid == -1)
            sprintf(tmp, "The last job cannot be urged.\n");
        else
            sprintf(tmp, "The job %i cannot be urged.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    /* Interchange the pointers */
    tmp1 = find_previous_job(p);
    tmp1->next = p->next;
    p->next = firstjob->next;
    firstjob->next = p;


    send_urgent_ok(s);
}

void s_swap_jobs(int s, int jobid1, int jobid2)
{
    struct Job *p1, *p2;
    struct Job *prev1, *prev2;
    struct Job *tmp;

    p1 = findjob(jobid1);
    p2 = findjob(jobid2);

    if (p1 == 0 || p2 == 0 || p1 == firstjob || p2 == firstjob)
    {
        char prev[60];
        sprintf(prev, "The jobs %i and %i cannot be swapped.\n", jobid1, jobid2);
        send_list_line(s, prev);
        return;
    }

    /* Interchange the pointers */
    prev1 = find_previous_job(p1);
    prev2 = find_previous_job(p2);
    prev1->next = p2;
    prev2->next = p1;
    tmp = p1->next;
    p1->next = p2->next;
    p2->next = tmp;

    send_swap_jobs_ok(s);
}

static void send_state(int s, enum Jobstate state)
{
    struct msg m;

    m.type = ANSWER_STATE;
    m.u.state = state;

    send_msg(s, &m);
}

void s_send_state(int s, int jobid)
{
    struct Job *p = 0;

    if (jobid == -1)
    {
        /* Find the last job added */
        p = firstjob;

        if (p != 0)
            while (p->next != 0)
                p = p->next;

        /* Look in finished jobs if needed */
        if (p == 0)
        {
            p = first_finished_job;
            if (p != 0)
                while (p->next != 0)
                    p = p->next;
        }

    }
    else
    {
        p = get_job(jobid);
    }

    if (p == 0)
    {
        char tmp[50];
        if (jobid == -1)
            sprintf(tmp, "The last job cannot be stated.\n");
        else
            sprintf(tmp, "The job %i cannot be stated.\n", jobid);
        send_list_line(s, tmp);
        return;
    }

    /* Interchange the pointers */
    send_state(s, p->state);
}

static void dump_job_struct(FILE *out, const struct Job *p)
{
    fprintf(out, "  new_job\n");
    fprintf(out, "    jobid %i\n", p->jobid);
    fprintf(out, "    command \"%s\"\n", p->command);
    fprintf(out, "    state %s\n",
            jstate2string(p->state));
    fprintf(out, "    result.errorlevel %i\n", p->result.errorlevel);
    fprintf(out, "    output_filename \"%s\"\n",
            p->output_filename ? p->output_filename : "NULL");
    fprintf(out, "    store_output %i\n", p->store_output);
    fprintf(out, "    pid %i\n", p->pid);
    fprintf(out, "    should_keep_finished %i\n", p->pid);
}

void dump_jobs_struct(FILE *out)
{
    const struct Job *p;

    fprintf(out, "New_jobs\n");

    p = firstjob;
    while (p != 0)
    {
        dump_job_struct(out, p);
        p = p->next;
    }

    p = first_finished_job;
    while (p != 0)
    {
        dump_job_struct(out, p);
        p = p->next;
    }
}

void joblist_dump(int fd)
{
    struct Job *p;
    char *buffer;

    buffer = joblistdump_headers();
    write(fd,buffer, strlen(buffer));
    free(buffer);

    /* We reuse the headers from the list */
    buffer = joblist_headers();
    write(fd, "# ", 2);
    write(fd, buffer, strlen(buffer));

    /* Show Finished jobs */
    p = first_finished_job;
    while(p != 0)
    {
        buffer = joblist_line(p);
        write(fd, "# ", 2);
        write(fd,buffer, strlen(buffer));
        free(buffer);
        p = p->next;
    }

    write(fd, "\n", 1);

    /* Show Queued or Running jobs */
    p = firstjob;
    while(p != 0)
    {
        buffer = joblistdump_torun(p);
        write(fd,buffer,strlen(buffer));
        free(buffer);
        p = p->next;
    }
}
