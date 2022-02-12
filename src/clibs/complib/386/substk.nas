; Software License Agreement
; 
;     Copyright(C) 1994-2022 David Lindauer, (LADSoft)
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

%ifdef __BUILDING_LSCRTL_DLL
[export ___substackp]
%endif
[global ___substackp]
[global __alloca_probe]
[global __alloca_probe_8]
[global __alloca_probe_16]
SECTION code CLASS=CODE USE32
__alloca_probe_8:
__alloca_probe_16: ; these work because __substackp does a paragraph align already
__alloca_probe:
    xchg    dword [esp],eax
    push    eax
___substackp:
    push    eax
    push    ecx
    mov     ecx, [esp + 12]
    or      ecx,ecx
    jns     down
    neg     ecx
    add		ecx,15
    and		ecx,0fffffff0h	
    mov     eax, [esp + 8]
    mov     [esp + ecx + 8],eax
    mov     eax, [esp + 4]
    mov     [esp + ecx+ 4],eax
    mov     eax, [esp]
    mov     [esp + ecx],eax
    add     esp, ecx
    pop     ecx
    pop     eax
    ret     4
down:
    add		ecx,15
    and		ecx,0fffffff0h	
    mov     eax,ecx
lp:
    cmp		ecx,4096
    jc		fin
    push	ecx
    sub		esp,4092
    sub		ecx,4096
    jmp		lp
fin:
    or		ecx,ecx
    jz		done
    push	ecx
    sub		ecx,4
    sub		esp,ecx
done:
    mov     ecx,eax
    mov     eax,[esp+ecx]
    mov     [esp],eax
    mov     eax,[esp+ecx+4]
    mov     [esp+4],eax
    mov     eax,[esp+ecx+8]
    mov     [esp+8],eax
    pop     ecx
    pop     eax
    ret     4
