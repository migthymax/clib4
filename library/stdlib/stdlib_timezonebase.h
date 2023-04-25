/*
 * $Id: stdlib_timebasebase.h,v 1.0 2021-01-15 10:06:25 clib2devs Exp $
*/

#ifndef _STDLIB_TIMEZONEBASE_H
#define _STDLIB_TIMEZONEBASE_H

/****************************************************************************/

#ifndef __NOLIBBASE__
#define __NOLIBBASE__
#endif /* __NOLIBBASE__ */

#ifndef __NOGLOBALIFACE__
#define __NOGLOBALIFACE__
#endif /* __NOGLOBALIFACE__ */

#include <proto/timezone.h>

/****************************************************************************/

#ifndef _MACROS_H
#include "macros.h"
#endif /* _MACROS_H */

extern struct Library *__TimezoneBase;
extern struct TimezoneIFace *__ITimezone;

#define DECLARE_TIMEZONEBASE() \
	struct Library *		UNUSED	TimezoneBase	= __TimezoneBase; \
	struct TimezoneIFace *			ITimezone		= __ITimezone

#endif /* _STDLIB_TIMEZONEBASE_H */
