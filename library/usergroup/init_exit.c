/*
 * $Id: usergroup_init_exit.c,v 1.12 2006-01-08 12:04:27 clib2devs Exp $
*/

#ifndef _USERGROUP_HEADERS_H
#include "usergroup_headers.h"
#endif /* _USERGROUP_HEADERS_H */

#ifndef _STDLIB_CONSTRUCTOR_H
#include "stdlib_constructor.h"
#endif /* _STDLIB_CONSTRUCTOR_H */

/****************************************************************************/

/* Pointer to errno, length == sizeof(long) */
#define UGT_ERRNOLPTR 0x80000004

CLIB_DESTRUCTOR(usergroup_exit) {
	ENTER();
    struct _clib2 *__clib2 = __CLIB2;

	if (__clib2->__IUserGroup != NULL) {
		DropInterface((struct Interface *)__clib2->__IUserGroup);
        __clib2->__IUserGroup = NULL;
	}

	if (__clib2->__UserGroupBase != NULL) {
		CloseLibrary(__clib2->__UserGroupBase);
        __clib2->__UserGroupBase = NULL;
	}

	LEAVE();
}

CLIB_CONSTRUCTOR(usergroup_init) {
	struct TagItem tags[2];
	BOOL success = FALSE;
    struct _clib2 *__clib2 = __CLIB2;

	ENTER();

    __clib2->__UserGroupBase = OpenLibrary("usergroup.library", 0);

	if (__clib2->__UserGroupBase != NULL) {
        __clib2->__IUserGroup = (struct UserGroupIFace *)GetInterface(__clib2->__UserGroupBase, "main", 1, 0);
		if (__clib2->__IUserGroup == NULL) {
			CloseLibrary(__clib2->__UserGroupBase);
            __clib2->__UserGroupBase = NULL;
		}
	}

	if (__clib2->__UserGroupBase == NULL) {
		SHOWMSG("usergroup.library did not open");

		__show_error("\"usergroup.library\" could not be opened.");
		goto out;
	}

	/* Wire the library's errno variable to our local errno. */
	tags[0].ti_Tag = UGT_ERRNOLPTR;
	tags[0].ti_Data = (ULONG)&errno;

	tags[1].ti_Tag = TAG_END;

	if (__ug_SetupContextTagList(__clib2->__progname, tags) != 0)
	{
		SHOWMSG("could not initialize usergroup.library");

		__show_error("\"usergroup.library\" could not be initialized.");
		goto out;
	}

	success = TRUE;

out:

	SHOWVALUE(success);
	LEAVE();

	if (success)
		CONSTRUCTOR_SUCCEED();
	else
		CONSTRUCTOR_FAIL();
}
