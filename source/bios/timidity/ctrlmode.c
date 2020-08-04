/*

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   ctrlmode.c

   */

#include <stdarg.h>
#include <stdio.h>

#include "config.h"
#include "ctrlmode.h"

static char timidity_error[1024] = "";

static int ctl_open(int using_stdin, int using_stdout)
{
  return 0;
}

static void ctl_close(void) {}

static int ctl_read(int32 *valp)
{
  return RC_NONE;
}

static int ctl_cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;
  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl->verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  vsprintf(timidity_error, fmt, ap);
  va_end(ap);
  fputs(timidity_error, stderr);
  fputs("\n", stderr);
  return 0;
}

static void ctl_int(int a) {}

static void ctl_intint(int a, int b) {}

static const ControlMode descent_ctl = {
	"Descent_ctl", 'd',
	0, 0, 0,
	ctl_open,
	ctl_close,
	ctl_read,
	ctl_cmsg,

	ctl_close,
	ctl_close,
	ctl_int,
	ctl_int,

	ctl_int,
	ctl_int,
	ctl_intint,
	ctl_intint,
	ctl_intint,
	ctl_intint,
	ctl_intint,
	ctl_intint
};

const ControlMode const* ctl=&descent_ctl;
