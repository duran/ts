#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

char * joblist_headers()
{
    char * line;

    line = malloc(100);
    /* We limit to 100 bytes for output and 200 for the command.
     * We also put spaces between the data, for assuring parseability. */
    /* Times:   0.00/0.00/0.00 - 4+4+4+2 = 14*/ 
    sprintf(line, "%-4s %-10s %-20.100s %-8s %-14s %.200s\n",
            "ID",
            "State",
            "Output",
            "E-Level",
            "Times(r/u/s)",
            "Command");

    return line;
}

static int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

static const char * ofilename_shown(const struct Job *p)
{
    const char * output_filename;

    if (p->store_output)
    {
        if (p->state == QUEUED)
        {
            output_filename = "(file)";
        } else
        {
            if (p->output_filename == 0)
                /* This may happen due to concurrency
                 * problems */
                output_filename = "(...)";
            else
                output_filename = p->output_filename;
        }
    } else
        output_filename = "stdout";

    return output_filename;
}

static char * print_noresult(const struct Job *p)
{
    const char * jobstate;
    const char * output_filename;
    int maxlen;
    char * line;

    jobstate = jstate2string(p->state);
    output_filename = ofilename_shown(p);

    maxlen = 4 + 1 + 10 + 1 + max(20, strlen(output_filename)) + 1 + 8 + 1
        + 14 + 1 + strlen(p->command) + 20; /* 20 is the margin for errors */

    line = (char *) malloc(maxlen);
    if (line == NULL)
        error("Malloc for %i failed.\n", maxlen);

    sprintf(line, "%-4i %-10s %-20s %-8s %14s %s\n",
            p->jobid,
            jobstate,
            output_filename,
            "",
            "",
            p->command);

    return line;
}

static char * print_result(const struct Job *p)
{
    const char * jobstate;
    int maxlen;
    char * line;
    const char * output_filename;

    jobstate = jstate2string(p->state);
    output_filename = ofilename_shown(p);

    maxlen = 4 + 1 + 10 + 1 + max(20, strlen(output_filename)) + 1 + 8 + 1
        + 14 + 1 + strlen(p->command) + 20; /* 20 is the margin for errors */

    line = (char *) malloc(maxlen);
    if (line == NULL)
        error("Malloc for %i failed.\n", maxlen);

    sprintf(line, "%-4i %-10s %-20s %-8i %0.2f/%0.2f/%0.2f %s\n",
            p->jobid,
            jobstate,
            output_filename,
            p->result.errorlevel,
            p->result.real_ms,
            p->result.user_ms,
            p->result.system_ms,
            p->command);

    return line;
}

char * joblist_line(const struct Job *p)
{
    char * line;

    if (p->state == FINISHED)
        line = print_result(p);
    else
        line = print_noresult(p);

    return line;
}
