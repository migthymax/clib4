/*
 * $Id: signal_sigaction.c,v 1.0 2022-02-16 20:25:00 apalmate Exp $
 *
 * :ts=4
 *
 * Portable ISO 'C' (1994) runtime library for the Amiga computer
 * Copyright (c) 2002-2015 by Olaf Barthel <obarthel (at) gmx.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   - Neither the name of Olaf Barthel nor the names of contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIGNAL_HEADERS_H
#include "signal_headers.h"
#endif /* _SIGNAL_HEADERS_H */

/* This implementation of sigaction is tailored to native Windows behavior:
   signal() has SysV semantics (ie. the handler is uninstalled before
   it is invoked).  This is an inherent data race if an asynchronous
   signal is sent twice in a row before we can reinstall our handler,
   but there's nothing we can do about it.  Meanwhile, sigprocmask()
   is not present, and while we can use the gnulib replacement to
   provide critical sections, it too suffers from potential data races
   in the face of an ill-timed asynchronous signal.  And we compound
   the situation by reading static storage in a signal handler, which
   POSIX warns is not generically async-signal-safe.  Oh well.
   Additionally:
     - We don't implement SA_NOCLDSTOP or SA_NOCLDWAIT, because SIGCHLD
       is not defined.
     - We don't implement SA_ONSTACK, because sigaltstack() is not present.
     - We ignore SA_RESTART, because blocking native Windows API calls are
       not interrupted anyway when an asynchronous signal occurs, and the
       MSVCRT runtime never sets errno to EINTR.
     - We don't implement SA_SIGINFO because it is impossible to do so
       portably.
   POSIX states that an application should not mix signal() and
   sigaction().  We support the use of signal() within the gnulib
   sigprocmask() substitute, but all other application code linked
   with this module should stick with only sigaction().  */

/* A signal handler.  */
typedef void (*handler_t)(int signal);

/* Set of current actions.  If sa_handler for an entry is NULL, then
   that signal is not currently handled by the sigaction handler.  */
static struct sigaction volatile action_array[NSIG] /* = 0 */;

/* Signal handler that is installed for signals.  */
static void
sigaction_handler(int sig) {
    handler_t handler;
    sigset_t mask;
    sigset_t oldmask;
    int saved_errno = errno;
    if (sig < 0 || NSIG <= sig || !action_array[sig].sa_handler) {
        /* Unexpected situation; be careful to avoid recursive abort.  */
        if (sig == SIGABRT)
            signal(SIGABRT, SIG_DFL);
        abort();
    }
    /* Reinstall the signal handler when required; otherwise update the
       bookkeeping so that the user's handler may call sigaction and get
       accurate results.  We know the signal isn't currently blocked, or
       we wouldn't be in its handler, therefore we know that we are not
       interrupting a sigaction() call.  There is a race where any
       asynchronous instance of the same signal occurring before we
       reinstall the handler will trigger the default handler; oh
       well.  */
    handler = action_array[sig].sa_handler;
    if ((action_array[sig].sa_flags & SA_RESETHAND) == 0)
        signal(sig, sigaction_handler);
    else
        action_array[sig].sa_handler = NULL;
    /* Block appropriate signals.  */
    mask = action_array[sig].sa_mask;
    if ((action_array[sig].sa_flags & SA_NODEFER) == 0)
        sigaddset(&mask, sig);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    /* Invoke the user's handler, then restore prior mask.  */
    errno = saved_errno;
    handler(sig);
    saved_errno = errno;
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    __set_errno(saved_errno);
}

/* Change and/or query the action that will be taken on delivery of
   signal SIG.  If not NULL, ACT describes the new behavior.  If not
   NULL, OACT is set to the prior behavior.  Return 0 on success, or
   set errno and return -1 on failure.  */
int
sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact) {
    sigset_t mask;
    sigset_t oldmask;
    int saved_errno;
    if (sig < 0 || NSIG <= sig || sig == SIGKILL || sig == SIGSTOP
        || (act && act->sa_handler == SIG_ERR)) {
        __set_errno(EINVAL);
        return -1;
    }

    /* POSIX requires sigaction() to be async-signal-safe.  In other
       words, if an asynchronous signal can occur while we are anywhere
       inside this function, the user's handler could then call
       sigaction() recursively and expect consistent results.  We meet
       this rule by using sigprocmask to block all signals before
       modifying any data structure that could be read from a signal
       handler; this works since we know that the gnulib sigprocmask
       replacement does not try to use sigaction() from its handler.  */
    if (!act && !oact)
        return 0;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    if (oact) {
        if (action_array[sig].sa_handler)
            *oact = action_array[sig];
        else {
            /* Safe to change the handler at will here, since all
               signals are currently blocked.  */
            oact->sa_handler = signal(sig, SIG_DFL);
            if (oact->sa_handler == SIG_ERR)
                goto failure;
            signal(sig, oact->sa_handler);
            oact->sa_flags = SA_RESETHAND | SA_NODEFER;
            sigemptyset(&oact->sa_mask);
        }
    }
    if (act) {
        /* Safe to install the handler before updating action_array,
           since all signals are currently blocked.  */
        if (act->sa_handler == SIG_DFL || act->sa_handler == SIG_IGN) {
            if (signal(sig, act->sa_handler) == SIG_ERR)
                goto failure;
            action_array[sig].sa_handler = NULL;
        } else {
            if (signal(sig, sigaction_handler) == SIG_ERR)
                goto failure;
            action_array[sig] = *act;
        }
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    return 0;
    failure:
    saved_errno = errno;
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    __set_errno(errno);
    return -1;
}