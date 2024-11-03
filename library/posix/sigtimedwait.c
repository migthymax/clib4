/*
 * $Id: sigtimedwait.c,v 1.0 2024-08-30 14:58:00 clib4devs Exp $
*/

#ifndef _SIGNAL_HEADERS_H
#include "signal_headers.h"
#endif /* _SIGNAL_HEADERS_H */

static void
ignore_signal(int sig) {
    struct _clib4 *__clib4 = __CLIB4;
    /* Remember the signal.  */
    __clib4->__was_sig = sig;
}

void DoStuff(void);

void DoStuff(void) {
    struct itimerval old_value;
    getitimer(ITIMER_REAL, &old_value);
    printf("Timer went off. getitimer now is %d sec, %d msec.\n",
           (int) old_value.it_value.tv_sec,
           (int) old_value.it_value.tv_usec);
}

static int
do_sigtimedwait(struct _clib4 *__clib4,const sigset_t *set, siginfo_t *info, const struct timespec64 *timeout) {
	sigset_t tmp_mask;
    struct sigaction saved[NSIG];
    struct sigaction action;
	struct itimerval it_val;
    int save_errno;
    int this;

	ENTER();

	SHOWPOINTER(set);
	SHOWPOINTER(info);
	SHOWPOINTER(timeout);

    /* Prepare set.  */
    sigfillset(&tmp_mask);
    /* Unblock all signals in the SET and register our nice handler.  */
    action.sa_handler = ignore_signal;
    action.sa_flags = 0;
    sigfillset(&action.sa_mask);    /* Block all signals for handler.  */
    /* Make sure we recognize error conditions by setting WAS_SIG to a
       value which does not describe a legal signal number.  */
    __clib4->__was_sig = -1;
    for (this = 1; this < NSIG; ++this) {
        if (sigismember(set, this)) {
            /* Unblock this signal.  */
            sigdelset(&tmp_mask, this);
            /* Register temporary action handler.  */
            if (sigaction(this, &action, &saved[this]) != 0)
                goto restore_handler;
        }
	}
	/* Prepare a timmer if timeout is requested */
	if( timeout ) {
		SHOWMSG( "TIMEOUT requestes!" );
		if (signal(SIGALRM, (void (*)(int)) DoStuff) == SIG_ERR) {
			perror("Unable to catch SIGALRM");
			exit(1);
		}
	/*		
		if (!sigismember(set, SIGALRM)) {
			SHOWMSG( "SIGALARM NOT PART OF THE SET!" );
            /* Unblock this signal.  * /
            sigdelset(&tmp_mask, SIGALRM);
            /* Register temporary action handler.  * /
            if ( sigaction(SIGALRM, &action, NULL) != 0)
                goto restore_handler;
			SHOWMSG( "ALARAM HA LDER REGSITERED!" );
		}
		*/
		it_val.it_interval.tv_sec 	= 0;
		it_val.it_interval.tv_usec	= 0;
		it_val.it_value.tv_sec		= timeout->tv_sec;
    	it_val.it_value.tv_usec  	= timeout->tv_nsec;

		if (setitimer(ITIMER_REAL, &it_val, NULL ) == -1 ) {
	        perror("error calling setitimer()");
    	    exit(1);
		}
	}
    /* Now we can wait for signals.  */
    sigsuspend(&tmp_mask);	
restore_handler:
    save_errno = errno;
    while (--this >= 1)
        if (sigismember(set, this))
            /* We ignore errors here since we must restore all handlers.  */
            sigaction(this, &saved[this], NULL);
    __set_errno(save_errno);
    /* Store the result and return.  */
	if( info ) {
		info->si_signo = __clib4->__was_sig;
		// TODO: ML: addtional fields, from where to get the data
	}
	/* Check if it ran into a timeout */
	if(__clib4->__was_sig ==  SIGALRM ) {
		__set_errno(EAGAIN);

		__clib4->__was_sig = -1;
	}

	RETURN(__clib4->__was_sig);

    return __clib4->__was_sig;
}

int
sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout) {
    struct _clib4 *__clib4 = __CLIB4;

	struct timespec64 ts64;
    if (timeout != NULL)
        ts64 = valid_timespec_to_timespec64(*timeout);

    /* __sigsuspend should be a cancellation point.  */
    return do_sigtimedwait(__clib4, set, info, &ts64);
}