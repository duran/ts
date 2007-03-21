#include <assert.h>
#include "msg.h"

struct Job
{
    int jobid;
    char command[CMD_LEN];
    enum Jobstate state;
    struct Job *next;
};

/* Globals */
static struct Job *firstjob = 0;
static jobids = 0;

void s_list(int s)
{
    int i;
    struct Job *p;

    printf("ID\tState\tCommand\n");

    p = firstjob;

    while(p != 0)
    {
        printf("%i\t", p->jobid);
        printf("%i\t", p->state);
        printf("%s\n", p->command);
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
int s_newjob(struct msg *m)
{
    struct Job *p;

    p = newjobptr();

    p->jobid = jobids++;
    p->state = QUEUED;
    strcpy(p->command, m->u.command);

    return p->jobid;
}

void s_removejob(int jobid)
{
    struct Job *p;
    struct Job *newnext;

    printf("Remove job %i\n", jobid);
    if (firstjob->jobid == jobid)
    {
        struct Job *newfirst;
        /* First job is to be removed */
        newfirst = firstjob->next;
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

    free(p->next);
    p->next = newnext;
}
