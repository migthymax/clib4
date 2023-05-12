/*
 * $Id: socket_bind.c,v 1.8 2006-11-16 10:41:15 clib2devs Exp $
*/

#ifndef _SOCKET_HEADERS_H
#include "socket_headers.h"
#endif /* _SOCKET_HEADERS_H */

int
bind(int sockfd, const struct sockaddr *name, socklen_t namelen) {
    struct fd *fd;
    int result = ERROR;
    struct _clib2 *__clib2 = __CLIB2;

    ENTER();

    SHOWVALUE(sockfd);
    SHOWPOINTER(name);
    SHOWVALUE(namelen);

    assert(name != NULL);
    DECLARE_SOCKETBASE();

    if (name == NULL) {
        SHOWMSG("invalid name parameter");

        __set_errno(EFAULT);
        goto out;
    }

    assert(sockfd >= 0 && sockfd < __clib2->__num_fd);
    assert(__clib2->__fd[sockfd] != NULL);
    assert(FLAG_IS_SET(__clib2->__fd[sockfd]->fd_Flags, FDF_IN_USE));
    assert(FLAG_IS_SET(__clib2->__fd[sockfd]->fd_Flags, FDF_IS_SOCKET));

    fd = __get_file_descriptor_socket(sockfd);
    if (fd == NULL)
        goto out;

    result = __bind(fd->fd_Socket, (struct sockaddr *) name, namelen);


out:

    __check_abort();

    RETURN(result);
    return (result);
}
