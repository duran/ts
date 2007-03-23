#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

static void run_parent()
{
    int status;
    wait(&status);
    printf("End of child\n");
};

static void run_child(const char *command)
{
    execlp("bash", "bash", "-c", command, NULL);
}

void run_job(const char *command)
{
    int pid;

    pid = fork();

    switch(pid)
    {
        case 0:
            run_child(command);
            break;
        case -1:
            perror("Error in fork");
            exit(-1);
            ;
        default:
            run_parent();
            break;
    }
}
