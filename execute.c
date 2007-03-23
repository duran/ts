#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "msg.h"
#include "main.h"

static void program_signal();

static void run_parent()
{
    int status;
    wait(&status);
};

static void run_child(const char *command)
{
    int p[2];
    /* Closing input */
    pipe(&p);
    close(p[1]); /* closing the write handle */
    close(0);

    dup(p[0]); /* the pipe reading goes to stdin */
    execlp("bash", "bash", "-c", command, NULL);
}

void run_job(const char *command)
{
    int pid;

    pid = fork();

    /* For the parent */
    /*program_signal(); Still not needed*/

    switch(pid)
    {
        case 0:
            close(server_socket);
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

static void sigchld_handler(int val)
{
}

static void program_signal()
{
  struct sigaction act;

  act.sa_handler = sigchld_handler;
  /* Reset the mask */
  memset(&act.sa_mask,0,sizeof(act.sa_mask));
  act.sa_flags = SA_NOCLDSTOP;
  act.sa_restorer = NULL;

  sigaction(SIGCHLD, &act, NULL);
}
