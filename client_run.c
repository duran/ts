#include <sys/types.h>
#include <unistd.h>

#include "main.h"

void c_run_tail(const char *filename)
{
    execlp("tail", "tail", "-f", filename, NULL);
}

void c_run_cat(const char *filename)
{
    execlp("cat", "cat", filename, NULL);
}
