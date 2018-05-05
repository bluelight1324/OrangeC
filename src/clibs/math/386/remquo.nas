; Software License Agreement
; 
;     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
; 
;     This file is part of the Orange C Compiler package.
; 
;     The Orange C Compiler package is free software: you can redistribute it and/or modify
;     it under the terms of the GNU General Public License as published by
;     the Free Software Foundation, either version 3 of the License, or
;     (at your option) any later version, with the addition of the 
;     Orange C "Target Code" exception.
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

%include "matherr.inc"

%ifdef _BUILDING_LSCRTL_DLL
[export _remquo]
[export _remquof]
[export _remquol]
%endif
[global _remquo]
[global _remquof]
[global _remquol]
SECTION data CLASS=DATA USE32
nm	db	"remquo",0

eight dd 8

SECTION code CLASS=CODE USE32
_remquof:
    lea	ecx,[esp+4]
    lea	edx,[esp+8]
    fld	dword[edx]
    fld	dword[ecx]
    mov ecx,[esp+12]
    sub dl,dl
    jmp short remquo
_remquol:
    lea	ecx,[esp+4]
    lea	edx,[esp+16]
    fld	tword[edx]
    fld	tword[ecx]
    mov ecx,[esp+28]
    mov dl,2
    jmp short remquo
_remquo:
    lea	ecx,[esp+4]
    lea	edx,[esp+12]
    fld	qword[edx]
    fld	qword[ecx]
    mov ecx,[esp+20]
    mov dl,1
remquo:
    lea eax,[nm]
    call clearmath
    fxch
    ftst
    fxch
    fnstsw ax
    sahf
    jz moderr
    fld st1
    fld st1
reprem:
    fprem1
    fstsw ax
    sahf
    jp reprem
    fxch
    popone
    fxch st2
    fdivp st1
    fild dword [eight]
    fld st1
reprem2:
    fprem1 
    fstsw ax
    sahf
    jp reprem2
    frndint
    fxch
    fcomp st1
    fxch
    ftst
    fstsw ax
    mov dh,ah
    fxch
    ftst
    fstsw ax
    fxch
    popone
    sahf
    jae noadjust
    fild dword [eight]
    faddp st1
noadjust:
    je noadj2
    mov ah,dh
    sahf
    jae noadj2
    fild dword [eight]
    fchs
    faddp st1
noadj2:
    fistp dword [ecx]    
    jmp wrapmath
moderr:
    popone
    jmp domainerr