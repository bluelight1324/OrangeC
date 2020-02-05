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

#ifndef __STDARG_H
#define __STDARG_H

#ifndef __STDDEF_H
#    include <stddef.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __MSIL__
    typedef void* __va_list__ va_list;
#else
typedef void* va_list;
#endif
#ifdef __cplusplus
};
#endif

#ifdef __MSIL__

#    define va_start(ap, parmN) ap = __va_start__()
#    define va_arg(ap, type) (*((type*)__va_arg__(ap, __va_typeof__(type))))
#    define va_end(ap)

#else

#    define __sizeof__(x) ((sizeof(x) + sizeof(int) - 1) & ~(sizeof(int) - 1))

#    define va_start(ap, parmN) ap = (va_list)((char*)(&parmN) + sizeof(parmN))
#    define va_arg(ap, type) (*(type*)(((*(char**)&(ap)) += __sizeof__(type)) - (__sizeof__(type))))
#    define va_end(ap)

#endif

#if __STDC_VERSION__ >= 199901L
#    define va_copy(ap_d, ap_s) (void)(ap_d = ap_s)
#endif

#define _va_ptr (...)

#endif /* __STDARG_H */
