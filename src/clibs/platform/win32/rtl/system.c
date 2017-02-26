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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <process.h>
#include <wchar.h>
#include <locale.h>
#include "libp.h"
#include <dir.h>

int _RTL_FUNC system(const char *string)
{
	FILE *f;
    char buf[1024],*a;
	if (*string)
	{
		int n = string[3];
		while (isspace(*string))
			string++;
		string[3] = 0;
		if (!stricmp(string, "cd "))
		{
			string[3] = n;
			return chdir(string + 3);
		}
		string[3] = n;

	}
	a = getenv("COMSPEC");
	if (!a)
		a = searchpath("cmd.exe");
	if (!string) {
		if (!a)
			return 0;
		else
			if (f = fopen(a,"r")) {
				fclose(f);
				return 1;
			}
			else
				return 0;
	}
	if (!a) {
		errno = ENOENT;
		return -1;
	}
	buf[0] = ' ';
    buf[1] = '/';
    buf[2] = 'C';
    buf[3] = ' ';
    strcpy(buf+4,string);
	return spawnlp(P_WAIT,a,a,buf,0);
}
