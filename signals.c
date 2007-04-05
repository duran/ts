#include <signal.h>
#include <stdlib.h> /* for NULL */

/* Some externs refer to this variable */
static sigset_t normal_sigmask;

void ignore_sigpipe()
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, &normal_sigmask);
}

void restore_sigmask()
{
    sigprocmask(SIG_SETMASK, &normal_sigmask, NULL);
}

