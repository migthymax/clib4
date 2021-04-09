/*
 * $Id: timezone_init_exit.c,v 1.0 2021-01-15 10:01:23 apalmate Exp $
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

#ifndef _TIMEZONE_HEADERS_H
#include "timezone_headers.h"
#endif /* _TIMEZONE_HEADERS_H */

/****************************************************************************/

#ifndef _STDLIB_CONSTRUCTOR_H
#include "stdlib_constructor.h"
#endif /* _STDLIB_CONSTRUCTOR_H */

/****************************************************************************/

struct Library * NOCOMMON __TimezoneBase;
struct TimezoneIFace * NOCOMMON __ITimezone;

char *tzname[2];  /* Current timezone names.  */
int  daylight;                      /* If daylight-saving time is ever in use.  */
long int timezone;                  /* Seconds west of UTC.  */

void
__timezone_exit(void)
{
	ENTER();

	__timezone_lock();

	if(__TimezoneBase != NULL)
	{
		DECLARE_TIMEZONEBASE();

		if(__ITimezone != NULL)
		{
			DropInterface((struct Interface *)__ITimezone);
			__ITimezone = NULL;
		}

		CloseLibrary(__TimezoneBase);
		__TimezoneBase = NULL;
	}

	__timezone_unlock();

	LEAVE();
}

/****************************************************************************/

int
__timezone_init(void)
{
	int result = ERROR;

	ENTER();

	PROFILE_OFF();

	__timezone_lock();

	if(__TimezoneBase == NULL)
	{
		__TimezoneBase = OpenLibrary("timezone.library", 52);

		if (__TimezoneBase != NULL)
		{
			__ITimezone = (struct TimezoneIFace *)GetInterface(__TimezoneBase, "main", 1, 0);
			if(__ITimezone == NULL)
			{
				CloseLibrary(__TimezoneBase);
				__TimezoneBase = NULL;
			}
		}
	}

	if(__TimezoneBase != NULL)
	{
        DECLARE_TIMEZONEBASE();

		// Set global timezone variable
        uint32 gmtoffset = 0;
        int8 dstime = -1;
        tzname[0] = calloc(1, MAX_TZSIZE);
        tzname[1] = calloc(1, MAX_TZSIZE);

        GetTimezoneAttrs(NULL, 
            TZA_Timezone, tzname[0], 
            TZA_TimezoneSTD, tzname[1], 
            TZA_UTCOffset, &gmtoffset, 
            TZA_TimeFlag, &dstime, 
            TAG_DONE);
            
		timezone = 60 * gmtoffset;
		daylight = dstime & TFLG_ISDST;

		result = OK;
	}
	else {
		/* default values */
		timezone = 0;
		daylight = 0;
		tzname[0] = (char *) "GMT";
		tzname[1] = (char *) "AMT";
	}

	__timezone_unlock();

	PROFILE_ON();

	RETURN(result);
	return(result);
}

static struct SignalSemaphore * timezone_lock;

void
__timezone_lock(void)
{
	if(timezone_lock != NULL)
		ObtainSemaphore(timezone_lock);
}

void
__timezone_unlock(void)
{
	if(timezone_lock != NULL)
		ReleaseSemaphore(timezone_lock);
}

CLIB_DESTRUCTOR(timezone_exit)
{
	ENTER();

	__timezone_exit();

	__delete_semaphore(timezone_lock);
	timezone_lock = NULL;

	LEAVE();
}

/****************************************************************************/

CLIB_CONSTRUCTOR(timezone_init)
{
	BOOL success = FALSE;
	int i;

	ENTER();

	timezone_lock = __create_semaphore();
	if(timezone_lock == NULL)
		goto out;

    __timezone_init();

	success = TRUE;

 out:

	SHOWVALUE(success);
	LEAVE();

	if(success)
		CONSTRUCTOR_SUCCEED();
	else
		CONSTRUCTOR_FAIL();
}