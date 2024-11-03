/* 
 * sigwaitchild.c
 *
 * This is an example of a parent process that creates some child
 * processes and then waits for them to terminate.  The waiting is
 * done using sigwaitinfo().  When a child process terminates, the
 * SIGCHLD signal is set on the parent.  sigwaitinfo() will return
 * when the signal arrives.
*/

#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void signal_handler(int signo)
{
    // do nothing
}

int main(void)
{
    char                *args[] = { "child", NULL };
    int                 i;
    pid_t               pid;
    sigset_t            mask;
    siginfo_t           info;
    struct sigaction    action;

    // mask out the SIGCHLD signal so that it will not interrupt us,
    // (side note: the child inherits the parents mask)
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    // by default, SIGCHLD is set to be ignored so unless we happen
    // to be blocked on sigwaitinfo() at the time that SIGCHLD
    // is set on us we will not get it.  To fix this, we simply
    // register a signal handler.  Since we've masked the signal
    // above, it will not affect us.  At the same time we will make
    // it a queued signal so that if more than one are set on us,
    // sigwaitinfo() will get them all.
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO; // make it a queued signal
    sigaction(SIGCHLD, &action, NULL);

    // create 3 child processes
    for (i = 0; i < 3; i++) {
        if ((pid = spawn("child", 0, NULL, &inherit, args, environ)) == -1)
            perror("spawn() failed");
        else
            printf("spawned child, pid = %d\n", pid);
    }

    while (1) {
        if (sigwaitinfo(&mask, &info) == -1) {
            perror("sigwaitinfo() failed");
            continue;
        }
        switch (info.si_signo) {
        case SIGCHLD:
            // info.si_pid is pid of terminated process, it is not POSIX
            printf("A child terminated; pid = %d\n", info.si_pid);
            break;
        default:
            // Should not get here since we asked only for SIGCHLD
            printf("Unexpected signal: %d\n", info.si_signo);
        }
    }
}
