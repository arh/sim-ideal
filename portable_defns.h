#ifndef __PORTABLE_DEFNS_H__
#define __PORTABLE_DEFNS_H__

//------------------------------------------------------------------------------
// File    : protable_defns.h
// Author  : Ms.Moran Tzafrir
// Written : 13 April 2009
//
// Multi-Platform C++ framework
//
// Copyright (C) 2009 Moran Tzafrir.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//------------------------------------------------------------------------------

#if defined(SPARC)
#include "sparc_defns.h"
#elif defined(SPARC64)
#include "sparc64_defns.h"
#elif defined(INTEL)
#include "intel_defns.h"
#elif defined(INTEL64)
#include "intel64_defns.h"
#elif defined(WIN32)
#include "win_defns.h"
#elif defined(WIN64)
#include "win64_defns.h"
#elif defined(PPC)
#include "ppc_defns.h"
#elif defined(IA64)
#include "ia64_defns.h"
#elif defined(MIPS)
#include "mips_defns.h"
#elif defined(ALPHA)
#include "alpha_defns.h"
#else
#error "A valid architecture has not been defined"
#endif

#if defined(WIN32) || defined(WIN64)
#define __thread__ __declspec(thread)
#else
#define __thread__ __thread
#endif


#include <time.h>
#include <math.h>
#include <signal.h>
#include <sstream>
#include <iostream>
#include <limits.h>
#include <sys/timeb.h>
#include <stdlib.h>


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef null
#define null (0)
#endif

#define final const
#define boolean bool

#define System_out_println(x) {fprintf(stdout, x); fprintf(stdout, "/n");}
#define System_err_println(x)  std::cerr<<(x)<<std::endl
#define System_out_format printf

#endif /* __PORTABLE_DEFNS_H__ */
