/*
 * $Id: unistd_chown.c,v 1.12 2006-09-25 15:41:50 obarthel Exp $
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

#ifndef _STDLIB_NULL_POINTER_CHECK_H
#include "stdlib_null_pointer_check.h"
#endif /* _STDLIB_NULL_POINTER_CHECK_H */

/****************************************************************************/

#ifndef _UNISTD_HEADERS_H
#include "unistd_headers.h"
#endif /* _UNISTD_HEADERS_H */

/****************************************************************************/

/* The following is not part of the ISO 'C' (1994) standard. */

/****************************************************************************/

int chown(const char *path_name, uid_t owner, gid_t group)
{
#if defined(UNIX_PATH_SEMANTICS)
	struct name_translation_info path_name_nti;
#endif /* UNIX_PATH_SEMANTICS */
	struct DevProc *dvp = NULL;
	BPTR file_lock = ZERO;
	BOOL owner_changed = TRUE;
	struct ExamineData *status = NULL;
	int result = ERROR;

	ENTER();

	SHOWSTRING(path_name);
	SHOWVALUE(owner);
	SHOWVALUE(group);

	assert(path_name != NULL);

	if (__check_abort_enabled)
		__check_abort();

#if defined(CHECK_FOR_NULL_POINTERS)
	{
		if (path_name == NULL)
		{
			SHOWMSG("invalid path name");

			__set_errno(EFAULT);
			goto out;
		}
	}
#endif /* CHECK_FOR_NULL_POINTERS */

#if defined(UNIX_PATH_SEMANTICS)
	{
		if (__global_clib2->__unix_path_semantics)
		{
			if (path_name[0] == '\0')
			{
				SHOWMSG("no name given");

				__set_errno(ENOENT);
				goto out;
			}

			if (__translate_unix_to_amiga_path_name(&path_name, &path_name_nti) != 0)
				goto out;

			if (path_name_nti.is_root)
			{
				__set_errno(EACCES);
				goto out;
			}
		}
	}
#endif /* UNIX_PATH_SEMANTICS */

	/* A value of -1 for either the owner or the group ID means
	   that what's currently being used should not be changed. */
	if (owner == (uid_t)-1 || group == (gid_t)-1)
	{
		D_S(struct FileInfoBlock, fib);

		PROFILE_OFF();

		/* Try to find out which owner/group information
		   is currently in use. */
		file_lock = Lock((STRPTR)path_name, SHARED_LOCK);
		if (file_lock == ZERO || ExamineObjectTags(EX_LockInput, file_lock, TAG_DONE) == NULL)
		{
			PROFILE_ON();

			__set_errno(__translate_access_io_error_to_errno(IoErr()));
			goto out;
		}

		UnLock(file_lock);
		file_lock = ZERO;

		PROFILE_ON();

		/* Replace the information that should not be changed. */
		if (owner == (uid_t)-1)
			owner = fib->fib_OwnerUID;

		if (group == (gid_t)-1)
			group = fib->fib_OwnerGID;

		/* Is anything different at all? */
		if (owner == fib->fib_OwnerUID && group == fib->fib_OwnerGID)
			owner_changed = FALSE;
	}

	if (owner > 65535 || group > 65535)
	{
		SHOWMSG("invalid owner or group");

		__set_errno(EINVAL);
		goto out;
	}

	if (owner_changed)
	{
		D(("changing owner of '%s'", path_name));

		PROFILE_OFF();
		int32 ret = SetOwnerInfoTags(OI_StringNameInput, (STRPTR)path_name, OI_OwnerUID, (LONG)((((ULONG)owner) << 16) | group), TAG_DONE);
		PROFILE_ON();

		if (ret == DOSFALSE)
		{
			__set_errno(__translate_io_error_to_errno(IoErr()));
			goto out;
		}
	}

	result = OK;

out:
	if (status != NULL) {
		FreeDosObject(DOS_EXAMINEDATA, status);
	}
	
	PROFILE_OFF();

	FreeDeviceProc(dvp);

	if (file_lock != ZERO)
		UnLock(file_lock);

	PROFILE_ON();

	RETURN(result);
	return (result);
}
