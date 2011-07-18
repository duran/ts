/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007-2009  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <stdio.h>
#include <sys/time.h>
#include "main.h"

void msgdump(FILE *f, const char *note, const struct msg *m)
{
    const char *name;
    if (process_type == SERVER)
        name = "server";
    else
        name = "client";
    fprintf(f, "msgdump (%s,%s):\n", name, note);
    switch(m->type)
    {
        case KILL_SERVER:
            fprintf(f, " KILL SERVER\n");
            break;
        case NEWJOB:
            fprintf(f, " NEWJOB\n");
            fprintf(f, " Commandsize: %i\n", m->u.newjob.command_size);
            break;
        case NEWJOB_OK:
            fprintf(f, " NEWJOB_OK\n");
            fprintf(f, " JobID: '%i'\n", m->u.jobid);
            break;
        case RUNJOB:
            fprintf(f, " RUNJOB\n");
            break;
        case RUNJOB_OK:
            fprintf(f, " RUNJOB_OK\n");
            fprintf(f, " Outputsize: %i\n", m->u.output.ofilename_size);
            fprintf(f, " pid: %i\n", m->u.output.pid);
            break;
        case ENDJOB:
            fprintf(f, " ENDJOB\n");
            break;
        case LIST:
            fprintf(f, " LIST\n");
            break;
        case LIST_LINE:
            fprintf(f, " LIST_LINE\n");
            fprintf(f, " Linesize: %i\n", m->u.size);
            break;
        case CLEAR_FINISHED:
            fprintf(f, " CLEAR_FINISHED\n");
        case ASK_OUTPUT:
            fprintf(f, " ASK_OUTPUT\n");
            fprintf(f, " Jobid: %i\n", m->u.jobid);
            break;
        case ANSWER_OUTPUT:
            fprintf(f, " ANSWER_OUTPUT\n");
            fprintf(f, " Outputsize: %i\n", m->u.output.ofilename_size);
            fprintf(f, " PID: %i\n", m->u.output.pid);
            break;
        case REMOVEJOB:
            fprintf(f, " REMOVE_JOB\n");
            break;
        case REMOVEJOB_OK:
            fprintf(f, " REMOVE_JOB_OK\n");
            break;
        case WAITJOB:
            fprintf(f, " WAITJOB\n");
            break;
        case WAIT_RUNNING_JOB:
            fprintf(f, " WAIT_RUNNING_JOB\n");
            break;
        case WAITJOB_OK:
            fprintf(f, " WAITJOB_OK\n");
            break;
        case URGENT:
            fprintf(f, " URGENT\n");
            break;
        case URGENT_OK:
            fprintf(f, " URGENT_OK\n");
            break;
        case GET_STATE:
            fprintf(f, " GET_STATE\n");
            break;
        case ANSWER_STATE:
            fprintf(f, " ANSWER_STATE\n");
            break;
        case SWAP_JOBS:
            fprintf(f, " SWAP_JOBS\n");
            break;
        case SWAP_JOBS_OK:
            fprintf(f, " SWAP_JOBS_OK\n");
            break;
        case INFO:
            fprintf(f, " INFO\n");
            break;
        case INFO_DATA:
            fprintf(f, " INFO_DATA\n");
            break;
        case SET_MAX_SLOTS:
            fprintf(f, " SET_MAX_SLOTS\n");
            break;
        case GET_MAX_SLOTS:
            fprintf(f, " GET_MAX_SLOTS\n");
            break;
        case GET_MAX_SLOTS_OK:
            fprintf(f, " GET_MAX_SLOTS_OK\n");
            break;
        case GET_VERSION:
            fprintf(f, " GET_VERSION\n");
            break;
        case VERSION:
            fprintf(f, " VERSION\n");
            break;
        default:
            fprintf(f, " Unknown message: %i\n", m->type);
    }
}
