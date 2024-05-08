#pragma once
/* Software License Agreement
 * 
 *     Copyright(C) 1994-2023 David Lindauer, (LADSoft)
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

#include <stack>
namespace Parser
{
extern attributes basisAttribs;

extern std::list<SYMBOL*> nameSpaceList;
extern char anonymousNameSpaceName[512];
extern int noNeedToSpecialize;
extern int parsingUsing;

typedef struct
{
    VTABENTRY* entry;
    SYMBOL* func;
    SYMBOL* name;
} THUNK;

// constexpr.cpp
bool isConstexprConstructor(SYMBOL* sym);

//ccerr.cpp
bool MustSpecialize(const char* name);
void SpecializationError(char* str);
void SpecializationError(SYMBOL* sym);
void warnCPPWarnings(SYMBOL* sym, bool localClassWarnings);
// decl group
void calculateVTabEntries(SYMBOL* sym, SYMBOL* base, std::list<VTABENTRY*>** pos, int offset);
void calculateVirtualBaseOffsets(SYMBOL* sym);
void deferredInitializeStructFunctions(SYMBOL* cur);
void deferredInitializeStructMembers(SYMBOL* cur);
bool usesVTab(SYMBOL* sym);
LexList* baseClasses(LexList* lex, SYMBOL* funcsp, SYMBOL* declsym, AccessLevel defaultAccess);
void checkPackedType(SYMBOL* sym);
void checkPackedExpression(EXPRESSION* exp);
void checkUnpackedExpression(EXPRESSION* exp);
void expandPackedBaseClasses(SYMBOL* cls, SYMBOL* funcsp, std::list<MEMBERInitializerS*>::iterator& init,
    std::list<MEMBERInitializerS*>::iterator& initend, std::list<MEMBERInitializerS*>* mi,
    std::list<BASECLASS*>* bc, std::list<VBASEENTRY*>* vbase);
void expandPackedMemberInitializers(SYMBOL* cls, SYMBOL* funcsp,
    std::list<TEMPLATEPARAMPAIR>* templatePack,
    std::list<MEMBERInitializerS*>** p,
    LexList* start, std::list<Argument*>* list);
void checkOperatorArgs(SYMBOL* sp, bool asFriend);
LexList* handleStaticAssert(LexList* lex);
LexList* insertNamespace(LexList* lex, Linkage linkage, StorageClass storage_class, bool* linked);
void CheckIsLiteralClass(Type* tp);
LexList* GetStructuredBinding(LexList* lex, SYMBOL* funcsp, StorageClass storage_class, Linkage linkage,
    std::list<FunctionBlock*>& block);
// templatedecl.cpp
bool declaringTemplate(SYMBOL* sym);
// cpplookup.cpp
void unvisitUsingDirectives(NAMESPACEVALUEDATA* v);
void CollapseReferences(Type* tp_in);
// stmt.cpp
EXPRESSION* addLocalDestructor(EXPRESSION* exp, SYMBOL* decl);
//
void dumpVTab(SYMBOL* sym);
int classRefCount(SYMBOL* base, SYMBOL* derived);
void CheckCalledException(SYMBOL* cst, EXPRESSION* exp);
void deferredCompileOne(SYMBOL* cur);
void deferredInitializeDefaultArg(SYMBOL* arg, SYMBOL* func);
Type* PerformDeferredInitialization(Type* tp, SYMBOL* funcsp);
bool hasPackedExpression(EXPRESSION* exp, bool useAuto);
void GatherPackedTypes(int* count, SYMBOL** arg, Type* tp);
void GatherPackedVars(int* count, SYMBOL** arg, EXPRESSION* packedExp);
int CountPacks(std::list<TEMPLATEPARAMPAIR>* packs);
void expandPackedInitList(std::list<Argument*>** lptr, SYMBOL* funcsp, LexList* start, EXPRESSION* packedExp);
LexList* insertUsing(LexList* lex, SYMBOL** sp_out, AccessLevel access, StorageClass storage_class, bool inTemplate, bool hasAttributes);
Type* AttributeFinish(SYMBOL* sym, Type* tp);
bool ParseAttributeSpecifiers(LexList** lex, SYMBOL* funcsp, bool always);
bool MatchesConstFunction(SYMBOL* sym);
LexList* getDeclType(LexList* lex, SYMBOL* funcsp, Type** tn);
}  // namespace Parser