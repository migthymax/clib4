#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


int main (int argc, char *argv[])
{
    sigset_t mask;
    sigset_t orig_mask;
    struct timespec timeout;


    sigemptyset (&mask);
    sigaddset (&mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
        perror ("sigprocmask");
        return 1;
    }

    timeout.tv_sec = 10;
    timeout.tv_nsec = 0;

 	printf ("Waiting for 10 seconds or CTRL-C\n");
 

    int v =sigtimedwait(&mask, NULL, &timeout);
    if (errno == EAGAIN) {
        printf ("Timeout\n");
        return -1;
       }
    if(v== SIGINT){
        printf("SIGINT\n");
        return 1;
        }


    return 0;
}