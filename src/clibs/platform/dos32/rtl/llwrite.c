/* Software License Agreement
 * 
 *     Copyright(C) 1994-2020 David Lindauer, (LADSoft)
 * 
 *     This file is part of the Orange C Compiler package.
 * 
 *     The Orange C Compiler package is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     The Orange C Compiler package is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *     As a special exception, if other files instantiate templates or
 *     use macros or inline functions from this file, or you compile
 *     this file and link it with other works to produce a work based
 *     on this file, this file does not by itself cause the resulting
 *     work to be covered by the GNU General Public License. However
 *     the source code for this file must still be made available in
 *     accordance with section (3) of the GNU General Public License.
 *     
 *     This exception does not invalidate any other reasons why a work
 *     based on this file might be covered by the GNU General Public
 *     License.
 * 
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 * 
 */

#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include "libp.h"
#include <dpmi.h>
#include "llp.h"

extern int __dtabuflen ;
int __ll_write(int fd, void *buf, size_t size)
{
    DPMI_REGS regs;
    int mod = size%__dtabuflen,i,tsize = 0;
    for (i=0; i < (size&-__dtabuflen); i +=__dtabuflen) {
        regs.h.dx = __buftodta((char *)buf+i,__dtabuflen);
        regs.h.bx = fd;
        regs.h.cx = __dtabuflen;
        __doscall(0x40,&regs);
        if (regs.h.flags & 1)
            return -1;
        tsize += regs.h.ax;
    }
    regs.h.dx = __buftodta((char *)buf+i,mod);
    regs.h.bx = fd;
    regs.h.cx = mod;
    __doscall(0x40,&regs);
    if (regs.h.flags & 1)
        return -1;
    tsize += regs.h.ax;
    return tsize;
}
