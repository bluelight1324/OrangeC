#pragma once
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
 *     contact information:
 *         email: TouchStone222@runbox.com <David Lindauer>
 *
 */
extern IMODE* returnImode;
extern int retcount;
extern int consIndex;
extern EXPRESSION* xcexp;
extern int catchLevel;
extern int codeLabelOffset;
extern SimpleSymbol* currentFunction;

void genstmtini(void);
IMODE* imake_label(int label);
IMODE* make_direct(int i);
void gen_genword(STATEMENT* stmt, SYMBOL* funcsp);
IMODE* set_symbol(const char* name, int isproc);
IMODE* call_library(const char* lib_name, int size);
void SubProfilerData(void);
void count_cases(CASEDATA* cd, struct cases* cs);
void gather_cases(CASEDATA* cd, struct cases* cs);
int gcs_compare(void const* left, void const* right);
void genxswitch(STATEMENT* stmt, SYMBOL* funcsp);
void genselect(STATEMENT* stmt, SYMBOL* funcsp, bool jmptrue);
void genreturn(STATEMENT* stmt, SYMBOL* funcsp, int flag, int noepilogue, IMODE* allocaAP);
void gen_varstart(void* exp);
void gen_func(void* exp, int start);
void gen_dbgblock(int start);
void gen_asm(STATEMENT* stmt);
void gen_asmdata(STATEMENT* stmt);
IMODE* genstmt(STATEMENT* stmt, SYMBOL* funcsp);
void CopyVariables(SYMBOL *funcsp);
void genfunc(SYMBOL* funcsp, bool doOptimize);
void genASM(STATEMENT* st);

