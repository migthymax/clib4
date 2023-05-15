/*
 * $Id: debug.c,v 1.6 2006-09-25 14:51:15 clib2devs Exp $
 *
 * :ts=8
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

/****************************************************************************/

#define NULL ((void *)0L)

#include <string.h>
#include <stdarg.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#include "debug.h"
#include <dos.h>

extern void KPrintF(const char *format, ...);
extern void KPutFmt(const char *format, va_list arg);

#define DEBUGLEVEL_OnlyAsserts  0
#define DEBUGLEVEL_Reports      1
#define DEBUGLEVEL_CallTracing  2

void
_SETPROGRAMNAME(char *name) {
    struct _clib2 *__clib2 = __CLIB2;
    if (name != NULL && name[0] != '\0') {
        int i;

        for (i = 0; name[i] != '\0' && i < (int) sizeof(__clib2->program_name) - 1; i++)
            __clib2->program_name[i] = name[i];

        __clib2->program_name[i] = '\0';
        __clib2->program_name_len = i;
    } else {
        __clib2->program_name_len = 0;
    }
}

int
_SETDEBUGLEVEL(int level) {
    struct _clib2 *__clib2 = __CLIB2;
    int old_level = __clib2->__debug_level;

    __clib2->__debug_level = level;

    return (old_level);
}

int
_GETDEBUGLEVEL(void) {
    struct _clib2 *__clib2 = __CLIB2;
    return (__clib2->__debug_level);
}

void
_PUSHDEBUGLEVEL(int level) {
    struct _clib2 *__clib2 = __CLIB2;
    __clib2->previous_debug_level = _SETDEBUGLEVEL(level);
}

void
_POPDEBUGLEVEL(void) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->previous_debug_level != -1) {
        _SETDEBUGLEVEL(__clib2->previous_debug_level);

        __clib2->previous_debug_level = -1;
    }
}

STATIC VOID
_INDENT(void) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->program_name_len > 0)
        KPrintF("(%s) ", __clib2->program_name);

    if (__clib2->__debug_level >= DEBUGLEVEL_CallTracing) {
        int i;

        for (i = 0; i < __clib2->indent_level; i++)
            KPrintF("   ");
    }
}

void
_SHOWVALUE(unsigned long value, int size, const char *name, const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        const char *fmt;

        switch (size) {
            case 1:

                fmt = "%s:%ld:%s = %ld, 0x%02lx";
                break;

            case 2:

                fmt = "%s:%ld:%s = %ld, 0x%04lx";
                break;

            default:

                fmt = "%s:%ld:%s = %ld, 0x%08lx";
                break;
        }

        _INDENT();

        KPrintF(fmt, file, line, name, value, value);

        if (size == 1 && value < 256) {
            if (value < ' ' || (value >= 127 && value < 160))
                KPrintF(", '\\x%02lx'", value);
            else
                KPrintF(", '%lc'", value);
        }

        KPrintF("\n");
    }
}

void
_SHOWPOINTER(const void *pointer, const char *name, const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        const char *fmt;

        _INDENT();

        if (pointer != NULL)
            fmt = "%s:%ld:%s = 0x%08lx\n";
        else
            fmt = "%s:%ld:%s = NULL\n";

        KPrintF(fmt, file, line, name, pointer);
    }
}

void
_SHOWSTRING(const char *string, const char *name, const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        _INDENT();
        KPrintF("%s:%ld:%s = 0x%08lx \"%s\"\n", file, line, name, string, string);
    }
}

void
_SHOWWSTRING(const wchar_t *string, const char *name, const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        _INDENT();
        char buf[BUFSIZ] = {0};
        char *ptr = (char *) &buf;
        int n = wctomb(ptr, *string);
        if (n > 0)
            KPrintF("%s:%ld:%s = 0x%08lx \"%s\" (converted from wchar_t *)\n", file, line, name, string, ptr);
        else
            KPrintF("%s:%ld:%s = 0x%08lx\n", file, line, name, string);
    }
}

void
_SHOWMSG(const char *string, const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        _INDENT();
        KPrintF("%s:%ld:%s\n", file, line, string);
    }
}

void
_DPRINTF_HEADER(const char *file, int line) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        _INDENT();
        KPrintF("%s:%ld:", file, line);
    }
}

void
_DPRINTF(const char *fmt, ...) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        va_list args;

        va_start(args, fmt);
        KPutFmt(fmt, args);
        va_end(args);

        KPrintF("\n");
    }
}

void
_DLOG(const char *fmt, ...) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_Reports) {
        va_list args;

        va_start(args, fmt);
        KPutFmt(fmt, args);
        va_end(args);
    }
}

void
_ENTER(const char *file, int line, const char *function) {
    struct _clib2 *__clib2 = __CLIB2;
    if (__clib2->__debug_level >= DEBUGLEVEL_CallTracing) {
        _INDENT();
        KPrintF("%s:%ld:Entering %s\n", file, line, function);
    }

    __clib2->indent_level++;
}

void
_LEAVE(const char *file, int line, const char *function) {
    struct _clib2 *__clib2 = __CLIB2;
    __clib2->indent_level--;

    if (__clib2->__debug_level >= DEBUGLEVEL_CallTracing) {
        _INDENT();
        KPrintF("%s:%ld: Leaving %s\n", file, line, function);
    }
}

void
_RETURN(const char *file, int line, const char *function, unsigned long result) {
    struct _clib2 *__clib2 = __CLIB2;
    __clib2->indent_level--;

    if (__clib2->__debug_level >= DEBUGLEVEL_CallTracing) {
        _INDENT();
        KPrintF("%s:%ld: Leaving %s (result 0x%08lx, %ld)\n", file, line, function, result, result);
    }
}
