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
#include <unordered_map>
namespace Parser
{

template <class T>
class NestedStack;
template <class T>
class StackList
{
  private:
    template <class>
    friend class NestedStack;
    std::stack<T>& Get()
    {
        if (pos == stacks.size())
            stacks[pos] = std::stack<T>();
        pos++;
        return stacks[pos - 1];
    }
    void Release()
    {
        if (pos)
        {
            std::stack<T>& stk = stacks[pos - 1];
            while (!stk.empty())
                stk.pop();
            pos--;
        }
    }

    std::unordered_map<int, std::stack<T>> stacks;
    int pos = 0;
};

template <class T>
class NestedStack
{
  public:
    NestedStack(StackList<T>& stacklist) : list(stacklist), thisStack(stacklist.Get()) {}
    ~NestedStack() { list.Release(); }
    T& top() { return thisStack.top(); }
    void push(const T& val) { thisStack.push(val); }
    void pop() { thisStack.pop(); }
    bool empty() { return thisStack.empty(); }

  private:
    std::stack<T>& thisStack;
    StackList<T>& list;
};

extern int anonymousNotAlloc;
bool ismemberdata(SYMBOL* sym);
bool ismember(SYMBOL* sym);
void helpinit(void);
void deprecateMessage(SYMBOL* sym);
bool isDerivedFromTemplate(Type* tp);
bool isconstexpr(const EXPRESSION* expa);
EXPRESSION* createTemporary(Type* tp, EXPRESSION* val);
EXPRESSION* msilCreateTemporary(Type* tp, EXPRESSION* val);
void DeduceAuto(Type** pat, Type* nt, EXPRESSION* exp);
SYMBOL* getFunctionSP(Type** tp);
LEXLIST* concatStringsInternal(LEXLIST* lex, STRING** str, int* elems);
LEXLIST* concatStrings(LEXLIST* lex, EXPRESSION** expr, e_lexType* tp, int* elems);
bool isintconst(EXPRESSION* exp);
bool isfloatconst(EXPRESSION* exp);
bool isimaginaryconst(EXPRESSION* exp);
bool iscomplexconst(EXPRESSION* exp);
void undoAnonymousVar(SYMBOL* sp);
EXPRESSION* anonymousVar(StorageClass storage_class, Type* tp);
EXPRESSION* anonymousBits(StorageClass storageClass, bool issigned, int bits);
void deref(Type* tp, EXPRESSION** exp);
int sizeFromType(Type* tp);
void cast(Type* tp, EXPRESSION** exp);
bool castvalue(EXPRESSION* exp);
bool xvalue(EXPRESSION* exp);
bool lvalue(EXPRESSION* exp);
EXPRESSION* convertInitToExpression(Type* tp, SYMBOL* sym, EXPRESSION* expsym, SYMBOL* funcsp, std::list<INITIALIZER*>* init,
                                    EXPRESSION* thisptr, bool isdest);
bool assignDiscardsConst(Type* dest, Type* source);
bool isconstzero(Type* tp, EXPRESSION* exp);
bool fittedConst(Type* tp, EXPRESSION* exp);
bool isarithmeticconst(EXPRESSION* exp);
bool isconstaddress(EXPRESSION* exp);
SYMBOL*(CopySymbol)(SYMBOL* sym_in, bool full);
BasicType btmax(BasicType left, BasicType right);
Type* destSize(Type* tp1, Type* tp2, EXPRESSION** exp1, EXPRESSION** exp2, bool minimizeInt, Type* atp);
EXPRESSION* RemoveAutoIncDec(EXPRESSION* exp);
EXPRESSION* EvaluateDest(EXPRESSION*exp, Type* tp);
}  // namespace Parser