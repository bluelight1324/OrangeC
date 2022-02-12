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
[export _lldiv]
[export _imaxdiv]
%endif

[global _lldiv]
[global _imaxdiv]

SECTION code CLASS=CODE USE32
_lldiv:
_imaxdiv:
                push    ebp
                mov     ebp, esp
                push    esi
                push    edi
                push    ebx
                sub		edi,edi
                mov     eax, [ebp+12]
                mov     edx, [ebp+16]
                mov     ebx, [ebp+20]
                mov     ecx, [ebp+24]
                or      ecx, ecx
        jnz	loc_1101A
                or      edx, edx
        jz	near loc_1107F
                or      ebx, ebx
        jz	near loc_1107F

loc_1101A:
                or      edx, edx
        jns	loc_1102E
                neg     edx
                neg     eax
                sbb     edx, 0
                or      edi, 0Ch

loc_1102E:
                or      ecx, ecx
        jns	loc_1103C
                neg     ecx
                neg     ebx
                sbb     ecx, 0
                xor     edi, 4

loc_1103C: 
                mov     ebp, ecx
                mov     ecx, 40h ; ' '
                push    edi
                xor     edi, edi
                xor     esi, esi

loc_11046:
                shl     eax, 1
                rcl     edx, 1
                rcl     esi, 1
                rcl     edi, 1
                cmp     edi, ebp
        jb	loc_1105D
        ja	loc_11058
                cmp     esi, ebx
        jb	loc_1105D

loc_11058:
                sub     esi, ebx
                sbb     edi, ebp
                inc     eax

loc_1105D:
        loop	loc_11046
                pop     ebx

loc_1106C: 
                test    ebx, 4
        jz	loc_11079				
                neg     edx
                neg     eax
                sbb     edx, 0

loc_11079: 
                test    ebx, 8
        jz	loc_11079a
                neg     edi
                neg     esi
                sbb     edi, 0

loc_11079a:
                mov		ecx,[esp + 12 + 8]
                mov		[ecx + 0],eax 
                mov		[ecx + 4],edx 
                mov		[ecx + 8],esi 
                mov		[ecx + 12],edi 
                mov		eax,ecx
                pop     ebx
                pop     edi
                pop     esi
                pop     ebp
                ret
loc_1107F: 
                div     ebx
                mov		esi, edx
                xor     edx, edx
                xor		edi, edi
                jmp	 loc_11079a
