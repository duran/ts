/*
    Task Spooler - a task queue system for the unix user
    Copyright (C) 2007  Lluís Batlle i Rossell

    Please find the license in the provided COPYING file.
*/
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h> /* BAD. main requires this header. */
#include <sys/time.h> /* BAD. main requires this header. */

#include "main.h"

void c_run_tail(const char *filename)
{
    restore_sigmask();
    execlp("tail", "tail", "-f", filename, NULL);
}

void c_run_cat(const char *filename)
{
    restore_sigmask();
    execlp("cat", "cat", filename, NULL);
}
