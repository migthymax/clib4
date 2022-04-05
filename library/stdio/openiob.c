/*
 * $Id: stdio_openiob.c,v 1.15 2008-09-04 12:07:58 clib2devs Exp $
*/

#ifndef _STDIO_HEADERS_H
#include "stdio_headers.h"
#endif /* _STDIO_HEADERS_H */

/****************************************************************************/

#ifndef _STDLIB_MEMORY_H
#include "stdlib_memory.h"
#endif /* _STDLIB_MEMORY_H */

/****************************************************************************/

int __open_iob(const char *filename, const char *mode, int file_descriptor, int slot_number)
{
	struct SignalSemaphore *lock;
	ULONG file_flags;
	int result = ERROR;
	int open_mode;
	struct fd *fd = NULL;
	STRPTR buffer = NULL;
	STRPTR aligned_buffer;
	struct iob *file;

	ENTER();

	SHOWSTRING(filename);
	SHOWSTRING(mode);
	SHOWVALUE(slot_number);

	if (__check_abort_enabled)
		__check_abort();

	__stdio_lock();

	assert(mode != NULL && 0 <= slot_number && slot_number < __num_iob);

	file = __iob[slot_number];

	assert(FLAG_IS_CLEAR(file->iob_Flags, IOBF_IN_USE));

	/* Figure out if the file descriptor provided is any use. */
	if (file_descriptor >= 0)
	{
		assert(file_descriptor < __num_fd);
		assert(__fd[file_descriptor] != NULL);
		assert(FLAG_IS_SET(__fd[file_descriptor]->fd_Flags, FDF_IN_USE));

		fd = __get_file_descriptor(file_descriptor);
		if (fd == NULL)
		{
			__set_errno(EBADF);
			goto out;
		}
	}

	/* The first character selects the access mode: read, write or append. */
	switch (mode[0])
	{
	case 'r':

		SHOWMSG("read mode");

		open_mode = O_RDONLY;
		break;

	case 'w':

		SHOWMSG("write mode");

		open_mode = O_WRONLY | O_CREAT | O_TRUNC;
		break;

	case 'a':

		SHOWMSG("append mode");

		open_mode = O_WRONLY | O_CREAT | O_APPEND;
		break;

	default:

		D(("unsupported file open mode '%lc'", mode[0]));

		__set_errno(EINVAL);
		goto out;
	}

	/* If the second or third character is a '+', switch to read/write mode. */
	if ((mode[1] == '+') || (mode[1] != '\0' && mode[2] == '+'))
	{
		SHOWMSG("read/write access");

		CLEAR_FLAG(open_mode, O_RDONLY);
		CLEAR_FLAG(open_mode, O_WRONLY);

		SET_FLAG(open_mode, O_RDWR);
	}

	SHOWMSG("allocating file buffer");

	/* Allocate a little more memory than necessary. */
	buffer = calloc(1, BUFSIZ + (__cache_line_size - 1));
	if (buffer == NULL)
	{
		SHOWMSG("that didn't work");

		__set_errno(ENOBUFS);
		goto out;
	}

	/* Align the buffer start address to a cache line boundary. */
	aligned_buffer = (char *)((ULONG)(buffer + (__cache_line_size - 1)) & ~(__cache_line_size - 1));

	if (file_descriptor < 0)
	{
		assert(filename != NULL);

		file_descriptor = open(filename, open_mode);
		if (file_descriptor < 0)
		{
			SHOWMSG("couldn't open the file");
			goto out;
		}
	}
	else
	{
		/* Update the append flag. */
		if (FLAG_IS_SET(open_mode, O_APPEND))
			SET_FLAG(fd->fd_Flags, FDF_APPEND);
		else
			CLEAR_FLAG(fd->fd_Flags, FDF_APPEND);
	}

	/* Allocate memory for an arbitration mechanism, then
		initialize it. */
	lock = __create_semaphore();
	if (lock == NULL)
		goto out;

	/* Figure out the buffered file access mode by looking at the open mode. */
	file_flags = IOBF_IN_USE | IOBF_NO_NUL;

	if (FLAG_IS_SET(open_mode, O_RDONLY) || FLAG_IS_SET(open_mode, O_RDWR))
		SET_FLAG(file_flags, IOBF_READ);

	if (FLAG_IS_SET(open_mode, O_WRONLY) || FLAG_IS_SET(open_mode, O_RDWR))
		SET_FLAG(file_flags, IOBF_WRITE);

	__initialize_iob(file, __iob_hook_entry,
					 buffer,
					 aligned_buffer, BUFSIZ,
					 file_descriptor,
					 slot_number,
					 file_flags,
					 lock);

	buffer = NULL;

	result = OK;

out:

	if (buffer != NULL)
		free(buffer);

	__stdio_unlock();

	RETURN(result);
	return (result);
}
