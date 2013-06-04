/*
	Software License Agreement (BSD License)
	
	Copyright (c) 1997-2008, David Lindauer, (LADSoft).
	All rights reserved.
	
	Redistribution and use of this software in source and binary forms, with or without modification, are
	permitted provided that the following conditions are met:
	
	* Redistributions of source code must retain the above
	  copyright notice, this list of conditions and the
	  following disclaimer.
	
	* Redistributions in binary form must reproduce the above
	  copyright notice, this list of conditions and the
	  following disclaimer in the documentation and/or other
	  materials provided with the distribution.
	
	* Neither the name of LADSoft nor the names of its
	  contributors may be used to endorse or promote products
	  derived from this software without specific prior
	  written permission of LADSoft.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
	TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*  types.h

    Types for dealing with time.

*/

#if !defined(__TYPES_H)
#define __TYPES_H

#ifdef __cplusplus
namespace std {
#endif /* __cplusplus */

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned size_t;
typedef int ssize_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

#ifdef __CRTDLL_DLL
typedef short dev_t;
#else
typedef int   dev_t;
#endif
typedef short ino_t;
typedef short mode_t;
typedef int   nlink_t;
typedef short uid_t;
typedef short gid_t;
typedef long  off_t;


#ifdef __cplusplus
} ;
#endif

#endif  /* __TYPES_H */

#if defined( __cplusplus) && !defined(__USING_CNAME__) && !defined(__TYPES_H_USING_LIST)
#define __TYPES_H_USING_LIST
using std::time_t;
using std::dev_t;
using std::ino_t;
using std::mode_t;
using std::nlink_t;
using std::uid_t;
using std::gid_t;
using std::off_t;
using std::size_t;
using std::ssize_t ;
#endif
