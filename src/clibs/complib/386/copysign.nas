; Software License Agreement
; 
;     Copyright(C) 1994-2021 David Lindauer, (LADSoft)
; 
;     This file is part of the Orange C Compiler package.
; 
;     The Orange C Compiler package is free software: you can redistribute it and/or modify
;     it under the terms of the GNU General Public License as published by
;     the Free Software Foundation, either version 3 of the License, or
;     (at your option) any later version.
; 
;     The Orange C Compiler package is distributed in the hope that it will be useful,
;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;     GNU General Public License for more details.
; 
;     You should have received a copy of the GNU General Public License
;     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
; 
;     As a special exception, if other files instantiate templates or
;     use macros or inline functions from this file, or you compile
;     this file and link it with other works to produce a work based
;     on this file, this file does not by itself cause the resulting
;     work to be covered by the GNU General Public License. However
;     the source code for this file must still be made available in
;     accordance with section (3) of the GNU General Public License.
;     
;     This exception does not invalidate any other reasons why a work
;     based on this file might be covered by the GNU General Public
;     License.
; 
;     contact information:
;         email: TouchStone222@runbox.com <David Lindauer>
; 

%include "matherr.inc"

%ifdef __BUILDING_LSCRTL_DLL
[export __copysign]
[export _copysign]
[export _copysignf]
[export _copysignl]
%endif
[global _copysign]
[global __copysign]
[global _copysignf]
[global _copysignl]
SECTION data CLASS=DATA USE32
nm	db	"copysign",0

SECTION code CLASS=CODE USE32
_copysignf:
    lea	ecx,[esp+4]
    lea	edx,[esp+8]
    and word [ecx +2],07fffh
    mov ax,[edx+2]
    and ax,8000h
    or word [ecx +2],ax
    fld	dword[ecx]
    ret
_copysign:
__copysign:
_copysignl:
    lea	ecx,[esp+4]
    lea	edx,[esp+12]
    and word [ecx +6],07fffh
    mov ax,[edx+6]
    and ax,8000h
    or word [ecx +6],ax
    fld	qword[ecx]
    ret
