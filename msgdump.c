#include <stdio.h>
#include "msg.h"
#include "main.h"

void msgdump(const struct msg *m)
{
    return;

    printf("msgdump:\n");
    switch(m->type)
    {
        case KILL_SERVER:
            printf("  KILL SERVER\n");
            break;
        case NEWJOB:
            printf(" NEWJOB\n");
            printf(" Commandsize: %i\n", m->u.newjob.command_size);
            break;
        case NEWJOB_OK:
            printf(" NEWJOB_OK\n");
            printf(" JobID: '%i'\n", m->u.jobid);
            break;
        case RUNJOB:
            printf(" RUNJOB\n");
            break;
        case RUNJOB_OK:
            printf(" RUNJOB_OK\n");
            printf(" Outputsize: %i\n", m->u.output.ofilename_size);
            printf(" pid: %i\n", m->u.output.pid);
            break;
        case ENDJOB:
            printf(" ENDJOB\n");
            break;
        case LIST:
            printf(" LIST\n");
            break;
        case LIST_LINE:
            printf(" LIST_LINE\n");
            printf(" Linesize: %i\n", m->u.line_size);
            break;
        case ASK_OUTPUT:
            printf(" ASK_OUTPUT\n");
            printf(" Jobid: %i\n", m->u.jobid);
            break;
        case ANSWER_OUTPUT:
            printf(" ANSWER_OUTPUT\n");
            printf(" Outputsize: %i\n", m->u.output.ofilename_size);
            printf(" PID: %i\n", m->u.output.pid);
            break;
    }
}
