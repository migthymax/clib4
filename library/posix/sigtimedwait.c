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

static int
do_sigtimedwait(struct _clib4 *__clib4,const sigset_t *set, siginfo_t *info, const struct timespec64 *timeout) {
	// TODO: ML: How to set up an timout event 
	sigset_t tmp_mask;
    struct sigaction saved[NSIG];
    struct sigaction action;
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
    for (this = 1; this < NSIG; ++this)
        if (sigismember(set, this)) {
            /* Unblock this signal.  */
            sigdelset(&tmp_mask, this);
            /* Register temporary action handler.  */
            if (sigaction(this, &action, &saved[this]) != 0)
                goto restore_handler;
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