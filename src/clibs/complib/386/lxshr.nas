; Software License Agreement
; 
;     Copyright(C) 1994-2023 David Lindauer, (LADSoft)
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
;     contact information:
;         email: TouchStone222@runbox.com <David Lindauer>
; 

%ifdef __BUILDING_LSCRTL_DLL
[export __LXSHR]
%endif
[global __LXSHR]

SECTION code CLASS=CODE USE32
__LXSHR:
        mov     eax,[esp+4]
        mov     edx,[esp+8]
        mov     ecx,[esp+12]
        cmp     cl,32
        jae     gt32
        shrd    eax,edx,cl
        shr     edx,cl
        jmp     short quit
gt32:
        sub     ecx,32
        mov     eax,edx
        sub     edx,edx
        shr     eax,cl
quit:
        ret     12
