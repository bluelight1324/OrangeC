/* Software License Agreement
 * 
 *     Copyright(C) 1994-2022 David Lindauer, (LADSoft)
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

#include <stdatomic.h>

void _RTL_FUNC(atomic_thread_fence)(int a) { atomic_thread_fence(a); }
void _RTL_FUNC(atomic_signal_fence)(int a) { atomic_signal_fence(a); }
unsigned char _RTL_FUNC(atomic_flag_test_and_set)(atomic_flag* f) { return atomic_flag_test_and_set(f); }
unsigned char _RTL_FUNC(atomic_flag_test_and_set_explicit)(atomic_flag* f, int mo)
{
    return atomic_flag_test_and_set_explicit(f, mo);
}
void _RTL_FUNC(atomic_flag_clear)(atomic_flag* f) { atomic_flag_clear(f); }
void _RTL_FUNC(atomic_flag_clear_explicit)(atomic_flag* f, int mo) { atomic_flag_clear_explicit(f, mo); }
