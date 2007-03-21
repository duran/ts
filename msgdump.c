#include <stdio.h>
#include "msg.h"
#include "main.h"

void msgdump(const struct msg *m)
{
    printf("msgdump:\n");
    switch(m->type)
    {
        case KILL:
            printf("  KILL\n");
            break;
        case NEWJOB:
            printf(" NEWJOB\n");
            printf(" Command: '%s'\n", m->u.command);
            break;
        case NEWJOB_OK:
            printf(" NEWJOB_OK\n");
            printf(" JobID: '%s'\n", m->u.jobid);
            break;
        case RUNJOB:
            printf(" RUNJOB\n");
            break;
        case ENDJOB:
            printf(" ENDJOB\n");
            break;
        case LIST:
            printf(" LIST\n");
            break;
    }
}
