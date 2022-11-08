// To trace the system calls in a user function

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int new_static_p;
    int pid;

    if (argc <= 1) {
        fprintf(2, "Usage: setpriority new priority...\n");
        exit(1);
    }

    if (argc <= 2) {
        fprintf(2, "Usage: setpriority pid...\n");
        exit(1);
    }

    new_static_p = atoi(argv[1]);
    pid = atoi(argv[2]);

    set_priority(new_static_p, pid);
    
    exit(0);
}