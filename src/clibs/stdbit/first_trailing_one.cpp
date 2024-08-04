/*  Software License Agreement
 *  
 *      Copyright(C) 1994-2024 David Lindauer, (LADSoft)
 *  
 *      This file is part of the Orange C Compiler package.
 *  
 *      The Orange C Compiler package is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *  
 *      The Orange C Compiler package is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *  
 *      You should have received a copy of the GNU General Public License
 *      along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *      contact information:
 *          email: TouchStone222@runbox.com <David Lindauer>
 *  
 */

#include <stddef.h>
#include <limits.h>

template <class T>
static inline unsigned int first_trailing_one(T arg)
{
    int rv = 1;
    T val;
    for (val = 1; val && !(val & arg); rv++, val <<= 1);
    return val ? rv : 0;
}

extern "C"
{

unsigned int _RTL_FUNC stdc_first_trailing_one_uc(unsigned char value) { return first_trailing_one(value); }
unsigned int _RTL_FUNC stdc_first_trailing_one_us(unsigned short value) { return first_trailing_one(value); }
unsigned int _RTL_FUNC stdc_first_trailing_one_ui(unsigned int value) { return first_trailing_one(value); }
unsigned int _RTL_FUNC stdc_first_trailing_one_ul(unsigned long value) { return first_trailing_one(value); }
unsigned int _RTL_FUNC stdc_first_trailing_one_ull(unsigned long long value) { return first_trailing_one(value); }

}