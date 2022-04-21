/*
 * $Id: signal_sigemptyset.c,v 1.6 2006-01-08 12:04:24 clib2devs Exp $
*/

#ifndef _SIGNAL_HEADERS_H
#include "signal_headers.h"
#endif /* _SIGNAL_HEADERS_H */

int
sigemptyset(sigset_t *set) {
    int result = ERROR;

    ENTER();

    SHOWPOINTER(set);

    assert(set != NULL);

    if (set == NULL) {
        __set_errno(EFAULT);
        goto out;
    }

    (*set) = 0;

    result = OK;

out:

    RETURN(result);
    return (result);
}
