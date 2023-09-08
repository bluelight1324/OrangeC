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

#include "compiler.h"
#include "rtti.h"
#include "PreProcessor.h"
#include "ccerr.h"
#include "config.h"
#include "initbackend.h"
#include "stmt.h"
#include "ildata.h"
#include "mangle.h"
#include "declcpp.h"
#include "lambda.h"
#include "template.h"
#include "declare.h"
#include "help.h"
#include "expr.h"
#include "cpplookup.h"
#include "occparse.h"
#include "lex.h"
#include "memory.h"
#include "init.h"
#include "exprcpp.h"
#include "template.h"
#include "OptUtils.h"
#include "constopt.h"
#include "libcxx.h"
#include "beinterf.h"
#include "types.h"
#include "declcons.h"
#include "Property.h"
#include "browse.h"
#include "ildata.h"
#include "optmodules.h"
#include "config.h"
#include "constexpr.h"
#include "AsmLexer.h"
#include "symtab.h"
#include "ListFactory.h"
#include "inline.h"
// there is a bug where the compiler needs constant values for the memory order,
// but parsed code may not provide it directly.
// e.g. when an atomic primitive is called from inside a function.
//
// we probably have to force inlining a little better to get around it, but would still need
// some kind of error in the back Keyword::end_ if they do try to pass a non-constant value for a memory order.
//
// It isn't critical on the x86 so I'm putting in a temporary workaround for expediency
// this will have to be revisited when we do other architectures though.
//
#define NEED_CONST_MO

namespace Parser
{
int packIndex;

int argumentNesting;
int inAssignRHS;


std::list<SYMBOL*> importThunks;
/* lvaule */
/* handling of const int */
/*--------------------------------------------------------------------------------------------------------------------------------
 */
static EXPRESSION* nodeSizeof(TYPE* tp, EXPRESSION* exp, int flags = 0);
static LEXLIST* expression_primary(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable,
                                   int flags);
static LEXLIST* expression_pm(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags);
LEXLIST* expression_assign(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags);
static LEXLIST* expression_msilfunc(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags);

std::unordered_map<std::string, unsigned, StringHash> cattributes = { 
 {"deprecated" , 202311},
 {"fallthrough" , 202311},
 {"nodiscard" , 202311},
 {"noreturn" , 202311},
 {"maybe_unused" , 202311}
};

void expr_init(void)
{
    packIndex = -1;
    importThunks.clear();
    inGetUserConversion = 0;
    inSearchingFunctions = 0;
    inNothrowHandler = 0;
    argFriend = nullptr;
}
void SetRuntimeData(LEXLIST* lex, EXPRESSION* exp, SYMBOL* sym)
{
    if ((Optimizer::cparams.prm_stackprotect & STACK_UNINIT_VARIABLE) && sym->sb->runtimeSym && lex->data->errfile)
    {
        auto runtimeData = Allocate<Optimizer::RUNTIMEDATA>();
        const char* p = strrchr(lex->data->errfile, '/');
        if (!p)
            p = strrchr(lex->data->errfile, '\\');
        if (!p)
            p = lex->data->errfile;
        else
            p++;
        runtimeData->fileName = p;
        runtimeData->varName = sym->sb->decoratedName;
        runtimeData->lineno = lex->data->errline;
        runtimeData->runtimeSymOrig = sym->sb->runtimeSym;
        exp->runtimeData = runtimeData;
    }
}
void thunkForImportTable(EXPRESSION** exp)
{
    SYMBOL* sym;
    if (Optimizer::architecture == ARCHITECTURE_MSIL)
        return;
    if ((*exp)->type == ExpressionNode::pc_)
        sym = (*exp)->v.sp;
    else
        sym = (*exp)->v.func->sp;
    // order is important here as we might get into this function with an already sanitized symbol
    if (sym && sym->sb->attribs.inheritable.linkage2 == Linkage::import_ && isfunction(sym->tp) &&
        ((*exp)->type == ExpressionNode::pc_ || !(*exp)->v.func->ascall))
    {
        bool found = false;
        for (auto search : importThunks)
        {
            if (search->sb->mainsym == sym)
            {
                *exp = varNode(ExpressionNode::pc_, (SYMBOL*)search);
                found = true;
                break;
            }
        }
        if (!found)
        {
            SYMBOL* newThunk;
            char buf[2048];
            Optimizer::my_sprintf(buf, "@@%s", sym->sb->decoratedName);
            newThunk = makeID(StorageClass::global_, &stdpointer, nullptr, litlate(buf));
            newThunk->sb->decoratedName = newThunk->name;
            newThunk->sb->mainsym = sym;  // mainsym is the symbol this was derived from
            newThunk->sb->attribs.inheritable.linkage4 = Linkage::virtual_;
            newThunk->sb->importThunk = true;
            importThunks.push_back(newThunk);
            *exp = varNode(ExpressionNode::pc_, newThunk);
            Optimizer::SymbolManager::Get(newThunk)->generated = true;
            Optimizer::EnterExternal(Optimizer::SymbolManager::Get(sym));
        }
    }
}
static EXPRESSION* GetUUIDData(SYMBOL* cls)
{
    if (cls && cls->sb->uuidLabel)
    {
        return intNode(ExpressionNode::labcon_, cls->sb->uuidLabel);
    }
    else
    {
        EXPRESSION* rv;
        STRING* data = Allocate<STRING>();
        data->pointers = Allocate<Optimizer::SLCHAR*>();
        data->size = 1;
        data->strtype = l_astr;
        data->pointers[0] = Allocate<Optimizer::SLCHAR>();
        data->pointers[0]->count = 16;
        data->pointers[0]->str = Allocate<LCHAR>(16);
        if (cls && cls->sb->uuid)
        {
            int i;
            for (i = 0; i < 16; i++)
                data->pointers[0]->str[i] = cls->sb->uuid[i];
        }
        rv = stringlit(data);
        if (cls)
            cls->sb->uuidLabel = rv->v.i;
        return rv;
    }
}
static EXPRESSION* GetManagedFuncData(TYPE* tp)
{
    // while managed functions seem to be always stdcall, the caller may be
    // either expecting a stdcall function, or a cdecl function
    // we take care of this by using the 'stdcall' attribute on the function
    // to determine what a caller might expect.
    // this function makes a little thunk table to tell the thunk generator
    // how to set up arguments for the call.  The arguments have to be reversed
    // but we cannot do a naive reversal because arguments of 8 bytes or more have to
    // be copied in order rather than reversed.
    char buf[512], *save = buf;
    int i;
    int sz = 0;
    STRING* data = Allocate<STRING>();
    SYMBOL* sym;
    if (ispointer(tp))
        tp = basetype(tp)->btp;
    sym = basetype(tp)->sp;
    *save++ = 0;  // space for the number of dwords
    *save++ = 0;
    *save++ = sym->sb->attribs.inheritable.linkage == Linkage::stdcall_;
    auto it = basetype(tp)->syms->begin();
    auto itend = basetype(tp)->syms->end();
    if (it != itend && (*it)->tp->type != BasicType::void_)
    {
        sz++;
        ++it;
        if (it != itend)
        {
            sz++;
            ++it;
            for (; it != itend; ++it)
            {
                int n = (*it)->tp->size;
                n += 3;
                n /= 4;
                if (n > 32767)
                    diag("GetManagedFuncData: passing too large structure");
                if (n > 0x7f)
                {
                    *save++ = (n >> 8) | 0x80;
                    *save++ = n & 0xff;
                }
                else
                {
                    *save++ = n;
                }
                sz += n;
            }
        }
    }
    buf[0] = (sz >> 8) | 0x80;
    buf[1] = sz & 0xff;
    data->pointers = Allocate<Optimizer::SLCHAR*>();
    data->size = 1;
    data->strtype = l_astr;
    data->pointers[0] = Allocate<Optimizer::SLCHAR>();
    data->pointers[0]->count = save - buf;
    data->pointers[0]->str = Allocate<LCHAR>(save - buf);
    for (i = 0; i < save - buf; i++)
        data->pointers[0]->str[i] = buf[i];
    return stringlit(data);
}
void ValidateMSILFuncPtr(TYPE* dest, TYPE* src, EXPRESSION** exp)
{
    bool managedDest = false;
    bool managedSrc = false;
    if ((*exp)->type == ExpressionNode::func_ && (*exp)->v.func->ascall)
        return;
    // this implementation marshals functions that are put into a pointer variable
    // declared with STDCALL.   If it is not declared with stdcall, then putting it
    // into a structure and passing it to unmanaged code will not work.
    if (isfunction(dest))
    {
        // function arg or assignment to function constant
        managedDest = basetype(dest)->sp->sb->attribs.inheritable.linkage2 != Linkage::unmanaged_ && msilManaged(basetype(dest)->sp);
    }
    else if (isfuncptr(dest))
    {
        // function var
        managedDest = basetype(basetype(dest)->btp)->sp->sb->attribs.inheritable.linkage2 != Linkage::unmanaged_ &&
                      msilManaged(basetype(basetype(dest)->btp)->sp);
    }
    else
    {
        // unknown
        errorConversionOrCast(true, src, dest);
    }
    if (isfunction(src))
    {
        // function arg or assignment to function constant
        managedSrc = msilManaged(basetype(src)->sp);
    }
    else if (isfuncptr(src))
    {
        // function var
        managedSrc = msilManaged(basetype(basetype(src)->btp)->sp);
    }
    else
    {
        // unknown
        errorConversionOrCast(true, src, dest);
    }
    if (managedDest != managedSrc)
    {
        SYMBOL* sym;
        if (managedSrc)
        {
            sym = gsearch("__OCCMSIL_GetProcThunkToManaged");
            if (sym)
            {
                int n = 0;
                FUNCTIONCALL* functionCall = Allocate<FUNCTIONCALL>();
                TYPE* tp1 = src;
                if (ispointer(tp1))
                    tp1 = basetype(tp1)->btp;

                for (auto sp : *basetype(tp1)->syms)
                {
                    int m = sp->tp->size;
                    m += 3;
                    m /= 4;
                    n += m;
                }
                sym = (SYMBOL*)basetype(sym->tp)->syms->front();
                functionCall->sp = sym;
                functionCall->functp = sym->tp;
                functionCall->fcall = varNode(ExpressionNode::pc_, sym);
                functionCall->arguments = initListListFactory.CreateList();
                auto arg = Allocate<INITLIST>();
                arg->tp = &stdpointer;
                arg->exp = *exp;
                functionCall->arguments->push_back(arg);
                arg = Allocate<INITLIST>();
                arg->tp = &stdpointer;
                arg->exp = GetManagedFuncData(tp1);
                functionCall->arguments->push_back(arg);
                functionCall->ascall = true;
                *exp = varNode(ExpressionNode::func_, nullptr);
                (*exp)->v.func = functionCall;
            }
            else
            {
                diag("ValidateMSILFuncPtr: missing conversion func definition");
            }
        }
        else
        {
            sym = gsearch("__OCCMSIL_GetProcThunkToUnmanaged");
            if (sym)
            {
                FUNCTIONCALL* functionCall = Allocate<FUNCTIONCALL>();
                sym = (SYMBOL*)basetype(sym->tp)->syms->front();
                functionCall->sp = sym;
                functionCall->functp = sym->tp;
                functionCall->fcall = varNode(ExpressionNode::pc_, sym);
                functionCall->arguments = initListListFactory.CreateList();
                auto arg = Allocate<INITLIST>();
                arg->tp = &stdpointer;
                arg->exp = *exp;
                functionCall->arguments->push_back(arg);
                functionCall->ascall = true;
                *exp = varNode(ExpressionNode::func_, nullptr);
                (*exp)->v.func = functionCall;
            }
            else
            {
                diag("ValidateMSILFuncPtr: missing conversion func definition");
            }
        }
    }
}
EXPRESSION* exprNode(ExpressionNode type, EXPRESSION* left, EXPRESSION* right)
{
    EXPRESSION* rv = Allocate<EXPRESSION>();
    rv->type = type;
    rv->left = left;
    rv->right = right;
    return rv;
}
EXPRESSION* varNode(ExpressionNode type, SYMBOL* sym)
{
    EXPRESSION* rv = Allocate<EXPRESSION>();
    rv->type = type;
    rv->v.sp = sym;
    return rv;
}
static bool inreg(EXPRESSION* exp, bool first)
{
    while (castvalue(exp))
        exp = exp->left;
    if (first && lvalue(exp))
    {
        exp = exp->left;
        first = false;
    }
    if (exp->type == ExpressionNode::auto_)
    {
        return exp->v.sp->sb->storage_class == StorageClass::register_;
    }
    else if (exp->type == ExpressionNode::add_ || exp->type == ExpressionNode::arrayadd_ || exp->type == ExpressionNode::arraymul_ || exp->type == ExpressionNode::lsh_ ||
             exp->type == ExpressionNode::structadd_ || exp->type == ExpressionNode::arraylsh_)
        return inreg(exp->left, first) || inreg(exp->right, first);
    else
        return false;
}
EXPRESSION* typeNode(TYPE* tp)
{
    EXPRESSION* rv = Allocate<EXPRESSION>();
    rv->type = ExpressionNode::type_;
    rv->v.tp = tp;
    return rv;
}
EXPRESSION* intNode(ExpressionNode type, long long val)
{
    EXPRESSION* rv = Allocate<EXPRESSION>();
    rv->type = type;
    rv->v.i = val;
    return rv;
}
static bool isstructuredmath(TYPE* tp1, TYPE* tp2 = nullptr)
{
    if (Optimizer::cparams.prm_cplusplus)
    {
        if (isstructured(tp1) || basetype(tp1)->type == BasicType::enum_ || (isint(tp1) && tp1->scoped))
        {
            return true;
        }
        if (tp2 && (isstructured(tp2) || basetype(tp2)->type == BasicType::enum_ || (isint(tp2) && tp2->scoped)))
        {
            return true;
        }
    }
    return false;
}
void checkauto(TYPE* tp1, int err)
{
    if (isautotype(tp1))
    {
        error(err);
        while (tp1->type == BasicType::const_ || tp1->type == BasicType::volatile_ || tp1->type == BasicType::lrqual_ || tp1->type == BasicType::rrqual_)
        {
            tp1->size = getSize(BasicType::int_);
            tp1 = tp1->btp;
        }
        tp1->type = BasicType::int_;
        tp1->size = getSize(BasicType::int_);
    }
}
static void tagNonConst(SYMBOL* sym, TYPE* tp)
{
    if (sym && tp)
    {
        if (isref(tp))
            tp = basetype(tp)->btp;
        sym->sb->nonConstVariableUsed |= !isconst(tp);
    }
}

static LEXLIST* variableName(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    char idname[512];
    FUNCTIONCALL* funcparams = nullptr;
    SYMBOL* sym = nullptr;
    SYMBOL* strSym = nullptr;
    std::list<NAMESPACEVALUEDATA*>* nsv = nullptr;
    LEXLIST* placeholder = lex;
    if (ismutable)
        *ismutable = false;
    if (Optimizer::cparams.prm_cplusplus ||
        ((Optimizer::architecture == ARCHITECTURE_MSIL) && Optimizer::cparams.msilAllowExtensions))
    {
        lex = id_expression(lex, funcsp, &sym, &strSym, &nsv, nullptr, false, false, idname);
    }
    else
    {
        sym = gsearch(lex->data->value.s.a);
    }
    if (sym)
    {
        browse_usage(sym, lex->data->linedata->fileindex);
        *tp = sym->tp;
        lex = getsym();
        if (sym->sb && sym->sb->attribs.uninheritable.deprecationText)
        {
            deprecateMessage(sym);
        }
        switch (sym->sb ? sym->sb->storage_class : StorageClass::templateparam_)
        {
            case StorageClass::member_:
            case StorageClass::mutable_:
                sym = lambda_capture(sym, cmThis, false);
                break;
            case StorageClass::auto_:
            case StorageClass::register_:
            case StorageClass::parameter_:
                sym = lambda_capture(sym, cmNone, false);
                break;
            case StorageClass::constant_:
                if ((flags & _F_AMPERSAND) && sym->sb->parent)
                    sym = lambda_capture(sym, cmNone, false);
                break;
            default:
                break;
        }
        if (sym->sb && sym->sb->templateLevel && !isfunction(sym->tp) && sym->sb->storage_class != StorageClass::type_ && !ismember(sym) &&
            MATCHKW(lex, Keyword::lt_))
        {
            lex = getsym();
            if (startOfType(lex, nullptr, false))
            {
                std::list<TEMPLATEPARAMPAIR>* lst = nullptr;
                SYMBOL* sp1 = sym;
                lex = backupsym();
                lex = GetTemplateArguments(lex, funcsp, sp1, &lst);
                sp1 = GetVariableTemplate(sp1, lst);

                if (sp1)
                {
                    sym = sp1;
                    *tp = sym->tp;
                }
                else
                    errorsym(ERR_NO_TEMPLATE_MATCHES, sym);
            }
            else
            {
                lex = backupsym();
            }
        }
        if ((!sym->sb || sym->sb->storage_class == StorageClass::templateparam_) && sym->tp->type == BasicType::templateparam_ &&
            (sym->tp->templateParam->second->type != Keyword::int_ ||
             (sym->tp->templateParam->second->type == Keyword::int_ && sym->tp->templateParam->second->byNonType.val)))
        {
            switch (sym->tp->templateParam->second->type)
            {
                case Keyword::typename_:
                case Keyword::template_:
                    lex = prevsym(placeholder);
                    *tp = nullptr;
                    if ((flags & (_F_SIZEOF | _F_PACKABLE)) == (_F_SIZEOF | _F_PACKABLE))
                    {
                        *exp = varNode(ExpressionNode::templateparam_, sym);
                        *tp = sym->tp;
                        lex = getsym();
                        if (MATCHKW(lex, Keyword::openpa_))
                        {
                            lex = getsym();
                            errskim(&lex, skim_closepa, false);
                            lex = getsym();
                        }
                        else if (MATCHKW(lex, Keyword::begin_))
                        {
                            lex = getsym();
                            errskim(&lex, skim_end, false);
                            lex = getsym();
                        }
                    }
                    else
                    {
                        lex = expression_func_type_cast(lex, funcsp, tp, exp, flags);
                    }
                    return lex;
                case Keyword::int_:
                    if (sym->tp->templateParam->second->packed)
                    {
                        if (packIndex >= 0)
                        {
                            if (sym->tp->templateParam->second->byPack.pack)
                            {
                                auto it = sym->tp->templateParam->second->byPack.pack->begin();
                                auto ite = sym->tp->templateParam->second->byPack.pack->end();
                                int n = packIndex;
                                for (; n && it != ite; n--, ++it);
                                if (it != ite)
                                {
                                    *exp = it->second->byNonType.val;
                                }
                                else
                                {
                                    *exp = intNode(ExpressionNode::c_i_, 0);
                                }
                            }
                            else
                            {
                                *exp = intNode(ExpressionNode::c_i_, 0);
                            }
                        }
                        else
                        {
                            *exp = exprNode(ExpressionNode::templateparam_, nullptr, nullptr);
                            (*exp)->v.sp = sym;
                        }
                    }
                    else
                    {
                        *exp = sym->tp->templateParam->second->byNonType.val;
                    }
                    *tp = sym->tp->templateParam->second->byNonType.tp;
                    if ((*tp)->type == BasicType::templateparam_)
                    {
                        TYPE* tp1 = (*tp)->templateParam->second->byClass.val;
                        if (tp1)
                        {
                            *tp = tp1;
                        }
                        else
                        {
                            SYMBOL* rv = nullptr;
                            for (auto&& s : structSyms)
                            {
                                if (rv)
                                    break;
                                if (s.tmpl)
                                    rv = templatesearch((*tp)->templateParam->first->name, s.tmpl);
                            }
                            if (rv && rv->tp->templateParam->second->type == Keyword::typename_)
                                *tp = rv->tp->templateParam->second->byClass.val;
                        }
                    }
                    lex = expression_func_type_cast(lex, funcsp, tp, exp, flags | _F_NOEVAL);
                    if (!*exp)
                        *exp = intNode(ExpressionNode::c_i_, 0);
                    return lex;
                default:
                    break;
            }
        }
        else
        {
            if (sym->tp->type == BasicType::templateparam_)
            {

                if (((sym->sb && sym->sb->storage_class == StorageClass::parameter_) || sym->tp->templateParam->second->type == Keyword::int_) &&
                    sym->tp->templateParam->second->packed)
                {
                    if (packIndex >= 0)
                    {
                        std::list<TEMPLATEPARAMPAIR>::iterator itt, ite = itt;
                        if (sym->tp->templateParam->second->byPack.pack)
                        {
                            itt = sym->tp->templateParam->second->byPack.pack->begin();
                            ite = sym->tp->templateParam->second->byPack.pack->end();
                            int i;
                            for (i = 0; i < packIndex && itt != ite; i++, ++itt);
                        }
                        if (itt != ite)
                        {
                            if (itt->second->type == Keyword::int_ && !itt->second->packsym)
                            {
                                *tp = itt->second->byNonType.tp;
                                if ((*tp)->type == BasicType::templateparam_)
                                {
                                    if ((*tp)->templateParam->second->byClass.val)
                                    {
                                        *tp = (*tp)->templateParam->second->byClass.val;
                                    }
                                    else
                                    {
                                        *tp = sym->tp;
                                        if ((*tp)->type == BasicType::templateparam_)
                                        {
                                            if ((*tp)->templateParam->second->byNonType.tp)
                                            {
                                                *tp = (*tp)->templateParam->second->byNonType.tp;
                                                if ((*tp)->type == BasicType::templateparam_)
                                                {
                                                    if ((*tp)->templateParam->second->byClass.val)
                                                    {
                                                        *tp = (*tp)->templateParam->second->byClass.val;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                *exp = itt->second->byNonType.val;
                            }
                            else
                            {
                                sym = itt->second->packsym;
                                *tp = sym->tp;
                                *exp = varNode(ExpressionNode::auto_, sym);
                            }
                        }
                        else
                        {
                            *exp = intNode(ExpressionNode::packedempty_, 0);
                        }
                    }
                    else
                    {
                        sym->packed = true;
                        *exp = varNode(ExpressionNode::auto_, sym);
                    }
                }
                else
                {
                    *exp = varNode(ExpressionNode::templateparam_, sym);
                }
            }
            else if (sym->sb)
            {
                switch (sym->sb->storage_class)
                {
                    case StorageClass::mutable_:
                        if (ismutable)
                            *ismutable = true;
                    case StorageClass::member_:
                        /*
                        if (flags & _F_SIZEOF)
                        {
                            *exp = intNode(ExpressionNode::c_i_, 0);
                        }
                        else
                        */
                        if (Optimizer::cparams.prm_cplusplus && (flags & _F_AMPERSAND) && strSym)
                        {
                            *exp = getMemberPtr(sym, strSym, tp, funcsp);
                        }
                        else
                        {
                            *exp = getMemberNode(sym, strSym, tp, funcsp);
                        }
                        break;
                    case StorageClass::type_:
                    case StorageClass::typedef_:
                        lex = prevsym(placeholder);
                        *tp = nullptr;
                        lex = expression_func_type_cast(lex, funcsp, tp, exp, flags);
                        if (!*exp)
                            *exp = intNode(ExpressionNode::c_i_, 0);
                        return lex;
                    case StorageClass::overloads_:
                    {
                        funcparams = Allocate<FUNCTIONCALL>();
                        if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::lt_))
                        {
                            lex = GetTemplateArguments(lex, funcsp, sym, &funcparams->templateParams);
                            funcparams->astemplate = true;
                        }
                        if (Optimizer::cparams.prm_cplusplus || basetype(sym->tp)->syms->size() > 1)
                        {
                            funcparams->nameSpace = strSym ? nullptr : nsv ? nsv->front() : nullptr;
                            funcparams->ascall = MATCHKW(lex, Keyword::openpa_);
                            funcparams->sp = sym;
                        }
                        else
                        {
                            // we only get here for C language, sadly we have to do
                            // argument based lookup in C++...
                            funcparams->sp = basetype(sym->tp)->syms->front();
                            funcparams->sp->sb->attribs.inheritable.used = true;
                            funcparams->fcall = varNode(ExpressionNode::pc_, funcparams->sp);
                            if (!MATCHKW(lex, Keyword::openpa_))
                                if (funcparams->sp->sb->attribs.inheritable.isInline)
                                    InsertInline(funcparams->sp);
                        }
                        funcparams->functp = funcparams->sp->tp;
                        *tp = funcparams->sp->tp;
                        funcparams->asaddress = !!(flags & _F_AMPERSAND);
                        if (Optimizer::cparams.prm_cplusplus && ismember(basetype(*tp)->sp) && !MATCHKW(lex, Keyword::openpa_))
                        {
                            EXPRESSION* exp1 = Allocate<EXPRESSION>();
                            exp1->type = ExpressionNode::memberptr_;
                            exp1->left = *exp;
                            exp1->v.sp = funcparams->sp;
                            *exp = exp1;
                            getMemberPtr(sym, strSym, tp, funcsp);
                        }
                        else
                        {
                            *exp = varNode(ExpressionNode::func_, nullptr);
                            (*exp)->v.func = funcparams;
                        }
                        break;
                    }
                    case StorageClass::catchvar_:
                        makeXCTab(funcsp);
                        *exp = varNode(ExpressionNode::auto_, funcsp->sb->xc->xctab);
                        *exp = exprNode(ExpressionNode::add_, *exp, intNode(ExpressionNode::c_i_, XCTAB_INSTANCE_OFS));
                        deref(&stdpointer, exp);
                        break;
                    case StorageClass::enumconstant_:
                        *exp = intNode(ExpressionNode::c_i_, sym->sb->value.i);
                        break;
                    case StorageClass::constant_:
                        *exp = varNode(ExpressionNode::const_, sym);
                        break;
                    case StorageClass::auto_:
                    case StorageClass::register_: /* register variables are treated as
                                       * auto variables in this compiler
                                       * of course the usage restraints of the
                                       * register keyword are enforced elsewhere
                                       */
                        *exp = varNode(ExpressionNode::auto_, sym);
                        sym->sb->anyTry |= tryLevel != 0;
                        SetRuntimeData(lex, *exp, sym);              
                        break;
                    case StorageClass::parameter_:
                        if (sym->packed)
                        {
                            if (!(flags & _F_PACKABLE))
                                error(ERR_PACK_SPECIFIER_MUST_BE_USED_IN_ARGUMENT);
                            if (packIndex >= 0)
                            {
                                TYPE* tp1 = sym->tp;
                                std::list<TEMPLATEPARAMPAIR>* templateParam;
                                int i;
                                while (ispointer(tp1) || isref(tp1))
                                    tp1 = basetype(tp1)->btp;
                                tp1 = basetype(tp1);
                                if (tp1->type == BasicType::templateparam_)
                                {
                                    std::list<TEMPLATEPARAMPAIR>::iterator itt, ite = itt;
                                    if (sym->tp->templateParam->second->byPack.pack)
                                    {
                                        itt = sym->tp->templateParam->second->byPack.pack->begin();
                                        ite = sym->tp->templateParam->second->byPack.pack->end();
                                        int i;
                                        for (i = 0; i < packIndex && itt != ite; i++, ++itt);
                                    }
                                    if (itt != ite)
                                    {
                                        sym = itt->second->packsym;
                                        *tp = sym->tp;
                                        *exp = varNode(ExpressionNode::auto_, sym);
                                    }
                                    else
                                    {
                                        *exp = intNode(ExpressionNode::packedempty_, 0);
                                    }
                                }
                                else
                                {
                                    bool found = false;
                                    SymbolTable<SYMBOL>::iterator it, itend;
                                    SymbolTable<SYMBOL>* tables = localNameSpace->front()->syms;

                                    while (tables && !found)
                                    {
                                        for (auto it1 = tables->begin(); it1 != tables->end(); ++it1)
                                        {
                                            if (*it1 == sym)
                                            {
                                                it = it1;
                                                itend = tables->end();
                                                found = true;
                                            }
                                        }
                                        tables = tables->Next();
                                    }
                                    if (found)
                                    {
                                        int i;
                                        for (i = 0; it != itend && i < packIndex; i++)
                                            ++it;
                                        if (it != itend)
                                        {
                                            sym = *it;
                                        }
                                        *exp = varNode(ExpressionNode::auto_, sym);
                                        *tp = sym->tp;
                                    }
                                    else
                                    {
                                        *exp = intNode(ExpressionNode::packedempty_, 0);
                                    }
                                }
                            }
                            else
                            {
                                *exp = varNode(ExpressionNode::auto_, sym);
                            }
                        }
                        else
                        {
                            *exp = varNode(ExpressionNode::auto_, sym);
                        }
                        /* derefereance parameters which are declared as arrays */
                        {
                            TYPE* tpa = basetype(sym->tp);
                            if (isref(tpa))
                                tpa = basetype(tpa->btp);
                            if (tpa->array)
                                deref(&stdpointer, exp);
                        }
                        sym->sb->anyTry |= tryLevel != 0;
                        break;

                    case StorageClass::localstatic_:
                        tagNonConst(funcsp, sym->tp);
                        if (funcsp && funcsp->sb->attribs.inheritable.isInline)
                        {
                            if (funcsp->sb->promotedToInline || Optimizer::cparams.prm_cplusplus)
                            {
                                funcsp->sb->attribs.inheritable.isInline =
                                    funcsp->sb->promotedToInline = false;
                            }
                        }
                        if (sym->sb->attribs.inheritable.linkage3 == Linkage::threadlocal_)
                        {
                            funcsp->sb->nonConstVariableUsed = true;
                            *exp = varNode(ExpressionNode::threadlocal_, sym);
                        }
                        else
                        {
                            *exp = varNode(ExpressionNode::global_, sym);
                        }
                        sym->sb->attribs.inheritable.used = true;
                        break;
                    case StorageClass::absolute_:
                        funcsp->sb->nonConstVariableUsed = true;
                        *exp = varNode(ExpressionNode::absolute_, sym);
                        break;
                    case StorageClass::static_:
                        sym->sb->attribs.inheritable.used = true;
                    case StorageClass::global_:
                    case StorageClass::external_: {
                        tagNonConst(funcsp, sym->tp);
                        SYMBOL* tpl = sym;
                        while (tpl)
                        {
                            if (tpl->sb->templateLevel)
                                break;
                            tpl = tpl->sb->parentClass;
                        }
                        if (tpl && tpl->sb->instantiated)
                        {
                            TemplateDataInstantiate(sym, false, false);
                        }
                        if (sym->sb->parentClass && !isExpressionAccessible(nullptr, sym, funcsp, nullptr, false))
                            errorsym(ERR_CANNOT_ACCESS, sym);
                        if (sym->sb->attribs.inheritable.linkage3 == Linkage::threadlocal_)
                            *exp = varNode(ExpressionNode::threadlocal_, sym);
                        else
                            *exp = varNode(ExpressionNode::global_, sym);
                        if (sym->sb->attribs.inheritable.linkage2 == Linkage::import_)
                        {
                            deref(&stdpointer, exp);
                        }
                        break;
                    }
                    case StorageClass::namespace_:
                    case StorageClass::namespace_alias_:
                        errorsym(ERR_INVALID_USE_OF_NAMESPACE, sym);
                        *exp = intNode(ExpressionNode::c_i_, 1);
                        break;
                    default:
                        error(ERR_IDENTIFIER_EXPECTED);
                        *exp = intNode(ExpressionNode::c_i_, 1);
                        break;
                }
            }
            else
            {
                if (!templateNestingCount)
                    error(ERR_IDENTIFIER_EXPECTED);
                *exp = intNode(ExpressionNode::c_i_, 1);
            }
        }
        sym->tp->used = true;
        if (sym->sb && sym->sb->templateLevel && istype(sym))
        {
            lex = prevsym(placeholder);
            *tp = nullptr;
            lex = expression_func_type_cast(lex, funcsp, tp, exp, flags);
        }
        else
        {
            if (sym->tp->type == BasicType::templateparam_)
            {
                if (*exp && (*exp)->type != ExpressionNode::packedempty_ && !sym->tp->templateParam->second->packed)
                {
                    if (sym->tp->templateParam->second->type == Keyword::int_)
                        *tp = sym->tp->templateParam->second->byNonType.tp;
                    else
                        *tp = &stdint;
                    if (sym->sb && sym->sb->templateLevel && sym->sb->storage_class != StorageClass::type_ && !ismember(sym) &&
                        !isfunction(sym->tp))
                    {
                        if (!(flags & _F_AMPERSAND))
                            deref(&stdint, exp);
                    }
                    else if (!templateNestingCount)
                    {
                        *exp = intNode(ExpressionNode::c_i_, 0);
                    }
                    if (MATCHKW(lex, Keyword::openpa_) || MATCHKW(lex, Keyword::begin_))
                    {
                        lex = prevsym(placeholder);
                        TYPE* tp1 = nullptr;
                        EXPRESSION* exp1;
                        lex = expression_func_type_cast(lex, funcsp, &tp1, &exp1, flags);
                        return lex;
                    }
                }
            }
            else if (sym->tp->type == BasicType::any_)
                deref(&stdint, exp);
            else
            {
                if (isref(*tp))
                {
                    TYPE* tp1 = *tp;
                    deref(*tp, exp);
                    while (isref(tp1))
                        tp1 = basetype(tp1)->btp;
                    *tp = CopyType(tp1);
                    UpdateRootTypes(*tp);
                    basetype(*tp)->lref = false;
                    basetype(*tp)->rref = false;
                }
                if (sym->sb && sym->sb->storage_class != StorageClass::overloads_)
                {
                    if (!isstructured(*tp) && basetype(*tp)->type != BasicType::memberptr_ && !isfunction(*tp) &&
                        sym->sb->storage_class != StorageClass::constant_ && sym->sb->storage_class != StorageClass::enumconstant_ &&
                        sym->tp->type != BasicType::void_)
                    {
                        if (!(*tp)->array || (*tp)->vla || (!(*tp)->msil && sym->sb->storage_class == StorageClass::parameter_))
                            if ((*tp)->vla)
                                deref(&stdpointer, exp);
                            else
                                deref(*tp, exp);
                        else if ((*tp)->array && inreg(*exp, true))
                            error(ERR_CANNOT_TAKE_ADDRESS_OF_REGISTER);
                    }
                }
            }
            *tp = ResolveTemplateSelectors(sym, *tp);

            if (lvalue(*exp) && (*exp)->type != ExpressionNode::l_object_)
                (*exp)->v.sp = sym;  // catch for constexpr
            (*exp)->pragmas = preProcessor->GetStdPragmas();
            if (isvolatile(*tp))
                (*exp)->isvolatile = true;
            if (isrestrict(*tp))
                (*exp)->isrestrict = true;
            if (isatomic(*tp))
            {
                (*exp)->isatomic = true;
                if (needsAtomicLockFromType(*tp))
                    (*exp)->lockOffset = basetype(*tp)->size;
            }
            if (strSym && funcparams)
                funcparams->novtab = true;
            if (Optimizer::cparams.prm_cplusplus && sym->sb &&
                (sym->sb->storage_class == StorageClass::member_ || sym->sb->storage_class == StorageClass::mutable_))
            {
                qualifyForFunc(funcsp, tp, *ismutable);
            }
        }
    }
    else if (parsingPreprocessorConstant)
    {
        *tp = &stdint;
        *exp = intNode(ExpressionNode::c_i_, 0);
        lex = getsym();
    }
    else
    {
        char* name;
        if (strSym && strSym->tp->type == BasicType::templateselector_)
        {
            SYMBOL* sym = (*basetype(strSym->tp)->sp->sb->templateSelector)[1].sp;
            if ((!templateNestingCount || instantiatingTemplate) && (sym->sb && sym->sb->instantiated && !declaringTemplate(sym) && (!sym->sb->templateLevel || allTemplateArgsSpecified(sym, (*strSym->tp->sp->sb->templateSelector)[1].templateParams))))
            {
                errorNotMember(sym, nsv ? nsv->front() : nullptr, ISID(lex) ? lex->data->value.s.a : "__unknown");
            }
            *exp = exprNode(ExpressionNode::templateselector_, nullptr, nullptr);
            (*exp)->v.templateSelector = strSym->tp->sp->sb->templateSelector;
            *tp = strSym->tp;
            lex = getsym();
            if (MATCHKW(lex, Keyword::lt_))
            {
                std::list<TEMPLATEPARAMPAIR>* current = nullptr;
                lex = GetTemplateArguments(lex, funcsp, nullptr, &current);
            }
            return lex;
        }
        if (strSym && basetype(strSym->tp)->type == BasicType::templatedecltype_)
        {
            *tp = strSym->tp;
            *exp = intNode(ExpressionNode::c_i_, 0);
            lex = getsym();
            return lex;
        }
        if (ISID(lex))
            name = litlate(lex->data->value.s.a);
        else
            name = litlate("__unknown");
        sym = SymAlloc();
        sym->name = name;
        sym->sb->attribs.inheritable.used = true;
        sym->sb->declfile = sym->sb->origdeclfile = lex->data->errfile;
        sym->sb->declline = sym->sb->origdeclline = lex->data->errline;
        sym->sb->realdeclline = lex->data->linedata->lineno;
        sym->sb->declfilenum = lex->data->linedata->fileindex;
        lex = getsym();
        if (MATCHKW(lex, Keyword::openpa_))
        {
            if (Optimizer::cparams.prm_cplusplus)
            {
                sym->sb->storage_class = StorageClass::overloads_;
                (*tp) = MakeType(BasicType::aggregate_);
                UpdateRootTypes(*tp);
                (*tp)->sp = sym;
            }
            else
            {
                /* no prototype error will be added later */
                sym->sb->storage_class = StorageClass::external_;
                sym->sb->attribs.inheritable.linkage = Linkage::c_;
                sym->tp = MakeType(BasicType::func_, CopyType(&stdint));
                sym->tp->syms = symbols.CreateSymbolTable();
                sym->tp->sp = sym;
                sym->sb->oldstyle = true;
                SetLinkerNames(sym, Linkage::c_);
                *tp = sym->tp;
            }
            SetLinkerNames(sym, Linkage::c_);
            if (sym->sb->storage_class != StorageClass::overloads_)
            {
                InsertSymbol(sym, sym->sb->storage_class, Linkage::none_, false);
                *exp = varNode(sym->sb->storage_class == StorageClass::auto_ ? ExpressionNode::auto_ : ExpressionNode::global_, sym);
            }
            else
            {
                sym->tp = MakeType(BasicType::func_, MakeType(BasicType::int_));
                sym->tp->syms = symbols.CreateSymbolTable();
                sym->tp->sp = sym;
                sym->sb->oldstyle = true;
                sym->sb->externShim = !!(flags & _F_INDECLTYPE);
                funcparams = Allocate<FUNCTIONCALL>();
                funcparams->sp = sym;
                funcparams->functp = sym->tp;
                funcparams->fcall = varNode(ExpressionNode::pc_, sym);
                funcparams->nameSpace = strSym ? nullptr : nsv ? nsv->front() : nullptr;
                *tp = sym->tp;
                *exp = varNode(ExpressionNode::func_, nullptr);
                (*exp)->v.func = funcparams;
            }
        }
        else
        {
            if (Optimizer::cparams.prm_cplusplus && atp && isfuncptr(atp))
            {
                SYMBOL* sym = nullptr;
                // this code was written this way before I fixed the unitialized variable.   Did we really never get here?
                sym->sb->storage_class = StorageClass::overloads_;
                (*tp) = MakeType(BasicType::aggregate_);
                UpdateRootTypes(*tp);
                (*tp)->sp = sym;
                funcparams = Allocate<FUNCTIONCALL>();
                funcparams->ascall = true;
                sym = GetOverloadedFunction(tp, &funcparams->fcall, sym, nullptr, atp, true, false, flags);
                if (sym)
                {
                    sym->sb->throughClass = sym->sb->throughClass;
                    sym = sym;
                    if (!isExpressionAccessible(nullptr, sym, funcsp, nullptr, false))
                        errorsym(ERR_CANNOT_ACCESS, sym);
                }
                funcparams->sp = sym;
                funcparams->functp = funcparams->sp->tp;
                funcparams->nameSpace = nsv->front();
                *exp = varNode(ExpressionNode::func_, nullptr);
                (*exp)->v.func = funcparams;
            }
            else
            {
                sym->sb->storage_class = funcsp ? StorageClass::auto_ : StorageClass::global_;
                sym->tp = MakeType(BasicType::any_);
                sym->sb->parentClass = strSym;
                *tp = sym->tp;
                deref(&stdint, exp);
                SetLinkerNames(sym, Linkage::c_);
                if (!nsv && (!strSym || !templateNestingCount ||
                             (!strSym->sb->templateLevel && strSym->tp->type != BasicType::templateselector_ &&
                              strSym->tp->type != BasicType::templatedecltype_)))
                {
                    char buf[4000];
                    buf[0] = 0;
                    LEXLIST* find = placeholder;
                    while (lex != find)
                    {
                        if (ISKW(find))
                        {
                            strcat(buf, find->data->kw->name);
                        }
                        else if (ISID(find))
                        {
                            strcat(buf, find->data->value.s.a);
                        }
                        find = find->next;
                    }
                    if (!(flags & _F_MEMBER))
                    {
                        errorstr(ERR_UNDEFINED_IDENTIFIER, buf);
                    }
                    if (sym->sb->storage_class != StorageClass::overloads_ &&
                        (localNameSpace->front()->syms || sym->sb->storage_class != StorageClass::auto_))
                        InsertSymbol(sym, sym->sb->storage_class, Linkage::none_, false);
                }
                if (nsv)
                {
                    errorNotMember(strSym, nsv->front(), sym->name);
                }
                if (sym->sb->storage_class != StorageClass::overloads_)
                {
                    *exp = varNode(ExpressionNode::global_, sym);
                }
                else
                {
                    funcparams = Allocate<FUNCTIONCALL>();
                    funcparams->nameSpace = nsv->front();
                    *exp = varNode(ExpressionNode::func_, nullptr);
                    (*exp)->v.func = funcparams;
                }
            }
        }
    }
    if (!*exp)
        *exp = intNode(ExpressionNode::c_i_, 0);
    if (sym && sym->sb)
    {
        sym->sb->allocate = true;
    }
    return lex;
}
static LEXLIST* expression_member(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    TYPE *typein = *tp, *typein2 = isarray(typein) ? typein : nullptr;
    bool points = false;
    const char* tokenName = ".";
    if (ISKW(lex))
        tokenName = lex->data->kw->name;
    (void)funcsp;
    // find structured version of arithmetic types for msil member matching
    if ((Optimizer::architecture == ARCHITECTURE_MSIL) && Optimizer::cparams.msilAllowExtensions &&
        (isarithmetic(*tp) || (*tp)->type == BasicType::string_ || (isarray(*tp) && basetype(*tp)->msil)))
    {
        // auto-boxing for msil
        TYPE* tp1 = find_boxed_type(basetype(*tp));
        if (tp1)
        {
            while (castvalue(*exp))
                *exp = (*exp)->left;
            if (!lvalue(*exp))
                *exp = msilCreateTemporary(*tp, *exp);
            else
                *exp = (*exp)->left;
            *tp = tp1;
            typein = tp1;
        }
    }
    if (MATCHKW(lex, Keyword::pointsto_))
    {
        TYPE* nesting[100];
        int n = 0;
        points = true;
        do
        {
            if (isstructured(*tp))
            {
                TYPE* x = basetype(*tp);
                int i;
                for (i = 0; i < n; i++)
                    if (nesting[i] == x)
                    {
                        break;
                    }
                nesting[n++] = x;
                if (n >= sizeof(nesting) / sizeof(nesting[0]))
                {
                    break;
                }
            }
        } while ((Optimizer::cparams.prm_cplusplus || (Optimizer::architecture == ARCHITECTURE_MSIL)) &&
                 insertOperatorFunc(ovcl_pointsto, Keyword::pointsto_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags));
        typein = *tp;
        if (ispointer(*tp))
        {
            *tp = basetype(*tp);
            *tp = (*tp)->btp;
        }
    }
    *tp = PerformDeferredInitialization(*tp, funcsp);
    if (MATCHKW(lex, Keyword::dot_) || MATCHKW(lex, Keyword::pointsto_))
        lex = getsym();
    if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::complx_))
    {
        // direct destructor or psuedo-destructor
        enum Linkage linkage = Linkage::none_, linkage2 = Linkage::none_, linkage3 = Linkage::none_;
        bool defd = false;
        bool notype = false;
        TYPE* tp1 = nullptr;
        lex = getsym();
        lex = getBasicType(lex, funcsp, &tp1, nullptr, false, StorageClass::auto_, &linkage, &linkage2, &linkage3, AccessLevel::public_, &notype, &defd,
                           nullptr, nullptr, false, true, false, false, false);
        if (!tp1)
        {
            error(ERR_TYPE_NAME_EXPECTED);
        }
        else if (!comparetypes(*tp, tp1, true))
        {
            if (!templateNestingCount)
                error(ERR_DESTRUCTOR_MUST_MATCH_CLASS);
        }
        else if (isstructured(*tp))
        {
            // destructor
            SYMBOL* sp2 = search((basetype(*tp)->sp)->tp->syms, overloadNameTab[CI_DESTRUCTOR]);
            if (sp2)
            {
                if (flags && _F_IS_NOTHROW)
                    inNothrowHandler++;
                callDestructor(basetype(*tp)->sp, nullptr, exp, nullptr, true, false, false, !points);
                if (flags && _F_IS_NOTHROW)
                    inNothrowHandler--;
            }
            if (needkw(&lex, Keyword::openpa_))
                needkw(&lex, Keyword::closepa_);
        }
        else
        {
            // psuedo-destructor, no further activity required.
            if (needkw(&lex, Keyword::openpa_))
                needkw(&lex, Keyword::closepa_);
        }
        *tp = &stdvoid;
    }
    else if (!isstructured(*tp) || (points && !ispointer(typein)) ||
             (parsingTrailingReturnOrUsing && isstructured(*tp) && templateNestingCount && !instantiatingTemplate &&
              basetype(*tp)->sp->sb->templateLevel))
    {
        if (Optimizer::cparams.prm_cplusplus && ISKW(lex) && (lex->data->kw->tokenTypes & TT_BASETYPE))
        {
            // possible psuedo destructor with selector
            enum Linkage linkage = Linkage::none_, linkage2 = Linkage::none_, linkage3 = Linkage::none_;
            bool defd = false;
            bool notype = false;
            TYPE* tp1 = nullptr;
            lex = getBasicType(lex, funcsp, &tp1, nullptr, false, StorageClass::auto_, &linkage, &linkage2, &linkage3, AccessLevel::public_, &notype,
                               &defd, nullptr, nullptr, false, true, false, false, false);
            if (!tp1)
            {
                error(ERR_TYPE_NAME_EXPECTED);
            }
            else if (!comparetypes(*tp, tp1, true))
            {
                errorConversionOrCast(true, tp1, *tp);
            }
            if (!MATCHKW(lex, Keyword::classsel_))
            {
                error(ERR_INVALID_PSUEDO_DESTRUCTOR);
            }
            else
            {
                lex = getsym();
                if (!MATCHKW(lex, Keyword::complx_))
                {
                    error(ERR_INVALID_PSUEDO_DESTRUCTOR);
                }
                else
                {
                    lex = getsym();
                    tp1 = nullptr;
                    lex = getBasicType(lex, funcsp, &tp1, nullptr, false, StorageClass::auto_, &linkage, &linkage2, &linkage3, AccessLevel::public_,
                                       &notype, &defd, nullptr, nullptr, false, true, false, false, false);
                    if (!tp1)
                    {
                        error(ERR_TYPE_NAME_EXPECTED);
                    }
                    else if (!comparetypes(*tp, tp1, true))
                    {
                        error(ERR_DESTRUCTOR_MUST_MATCH_CLASS);
                    }
                    else if (needkw(&lex, Keyword::openpa_))
                        needkw(&lex, Keyword::closepa_);
                }
            }
            *tp = &stdvoid;
        }
        else if (templateNestingCount && Optimizer::cparams.prm_cplusplus)
        {
            *exp = exprNode(points ? ExpressionNode::pointsto_ : ExpressionNode::dot_, *exp, nullptr);
            EXPRESSION** ptr = &(*exp)->right;
            while (ISID(lex) || MATCHKW(lex, Keyword::operator_))
            {
                TYPE* tp2 = nullptr;
                EXPRESSION* exp = nullptr;
                lex = expression_pm(lex, funcsp, nullptr, &tp2, ptr, nullptr, _F_MEMBER);
                if (!MATCHKW(lex, Keyword::pointsto_) && !MATCHKW(lex, Keyword::dot_))
                    break;
                *ptr = exprNode(MATCHKW(lex, Keyword::pointsto_) ? ExpressionNode::pointsto_ : ExpressionNode::dot_, *ptr, nullptr);
                ptr = &(*ptr)->right;
                lex = getsym();
            }
            if (!*ptr)
            {
                *ptr = intNode(ExpressionNode::c_i_, 0);
            }
            else
            {
                *tp = MakeType(BasicType::templatedecltype_);
                (*tp)->templateDeclType = *exp;
            }
        }
        else
        {
            if (points)
                errorstr(ERR_POINTER_TO_STRUCTURE_EXPECTED, tokenName);
            else
                errorstr(ERR_STRUCTURED_TYPE_EXPECTED, tokenName);
            while (ISID(lex) || MATCHKW(lex, Keyword::operator_))
            {
                TYPE* tp = nullptr;
                EXPRESSION* exp = nullptr;
                lex = expression_pm(lex, funcsp, nullptr, &tp, &exp, nullptr, 0);
                if (!MATCHKW(lex, Keyword::pointsto_) && !MATCHKW(lex, Keyword::dot_))
                    break;
                lex = getsym();
            }
        }
    }
    else
    {
        bool isTemplate = false;
        if (MATCHKW(lex, Keyword::template_))
        {
            lex = getsym();
            isTemplate = true;
        }
        if (!ISID(lex) && !MATCHKW(lex, Keyword::operator_))
        {
            error(ERR_IDENTIFIER_EXPECTED);
        }
        else
        {
            SYMBOL* sp2 = nullptr;
            if ((*exp)->type == ExpressionNode::literalclass_)
            {
                CONSTEXPRSYM* ces = (CONSTEXPRSYM*)search((*exp)->v.syms, lex->data->value.s.a);
                if (ces)
                {
                    lex = getsym();
                    *exp = ces->exp;
                    *tp = ces->sp->tp;
                    return lex;
                }
                else
                {
                    STRUCTSYM l;
                    l.str = basetype(*tp)->sp;
                    addStructureDeclaration(&l);
                    lex = id_expression(lex, funcsp, &sp2, nullptr, nullptr, &isTemplate, false, true, nullptr);
                    dropStructureDeclaration();
                    if (!sp2)
                    {
                        errorNotMember(basetype(*tp)->sp, nullptr, lex->data->value.s.a);
                        lex = getsym();
                        while (ISID(lex))
                        {
                            lex = getsym();
                            if (!MATCHKW(lex, Keyword::pointsto_) && !MATCHKW(lex, Keyword::dot_))
                                break;
                            lex = getsym();
                        }
                        *exp = intNode(ExpressionNode::c_i_, 0);
                        *tp = &stdint;
                        return lex;
                    }
                    lex = getsym();
                    if (sp2->tp->type != BasicType::aggregate_)
                    {
                        error(ERR_CONSTANT_FUNCTION_EXPECTED);
                        *exp = intNode(ExpressionNode::c_i_, 0);
                        *tp = &stdint;
                        return lex;
                    }
                    if (MATCHKW(lex, Keyword::openpa_))
                    {
                        FUNCTIONCALL* funcparams = Allocate<FUNCTIONCALL>();
                        SYMBOL* match;
                        TYPE* tp1 = nullptr;
                        EXPRESSION* exp1 = nullptr;
                        lex = getArgs(lex, funcsp, funcparams, Keyword::closepa_, true, flags);
                        funcparams->thisptr = intNode(ExpressionNode::c_i_, 0);
                        funcparams->thistp = MakeType(BasicType::pointer_, *tp);
                        funcparams->ascall = true;
                        match = GetOverloadedFunction(&tp1, &exp1, sp2, funcparams, nullptr, true, false, flags);
                        if (match)
                        {
                            funcparams->sp = match;
                            funcparams->functp = match->tp;
                            *exp = substitute_params_for_function(funcparams, (*exp)->v.syms);
                            optimize_for_constants(exp);
                            *tp = basetype(match->tp)->btp;
                            if (!match->sb->constexpression || !IsConstantExpression(*exp, true, false))
                                error(ERR_CONSTANT_FUNCTION_EXPECTED);
                        }
                        else
                        {
                            *exp = intNode(ExpressionNode::c_i_, 0);
                            *tp = &stdint;
                        }
                        return lex;
                    }
                    else
                    {
                        needkw(&lex, Keyword::openpa_);
                        *exp = intNode(ExpressionNode::c_i_, 0);
                        *tp = &stdint;
                        return lex;
                    }
                }
            }
            if (Optimizer::cparams.prm_cplusplus)
            {
                STRUCTSYM l;
                l.str = basetype(*tp)->sp;
                addStructureDeclaration(&l);
                lex = id_expression(lex, funcsp, &sp2, nullptr, nullptr, &isTemplate, false, true, nullptr);
                dropStructureDeclaration();
            }
            else
            {
                sp2 = search((basetype(*tp)->sp)->tp->syms, lex->data->value.s.a);
            }
            if (!sp2)
            {
                if (!templateNestingCount || !basetype(*tp)->sp->sb->templateLevel)
                    errorNotMember(basetype(*tp)->sp, nullptr, lex->data->value.s.a ? lex->data->value.s.a : "unknown");
                lex = getsym();
                while (ISID(lex))
                {
                    lex = getsym();
                    if (!MATCHKW(lex, Keyword::pointsto_) && !MATCHKW(lex, Keyword::dot_))
                        break;
                    lex = getsym();
                }
            }
            else
            {
                TYPE* tpb;
                TYPE* basetp = *tp;
                TYPE* typ2 = typein;
                if (sp2->sb->attribs.uninheritable.deprecationText)
                    deprecateMessage(sp2);
                browse_usage(sp2, lex->data->linedata->fileindex);
                if (ispointer(typ2))
                    typ2 = basetype(typ2)->btp;
                (*exp)->isatomic = false;
                lex = getsym();
                sp2->sb->attribs.inheritable.used = true;
                *tp = sp2->tp;
                while ((*tp)->type == BasicType::typedef_)
                    (*tp) = (*tp)->btp;
                tpb = basetype(*tp);
                if (sp2->sb->storage_class == StorageClass::overloads_)
                {
                    FUNCTIONCALL* funcparams = Allocate<FUNCTIONCALL>();
                    if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::lt_))
                    {
                        bool isdest = basetype(sp2->tp)->syms->front()->sb->isDestructor;
                        bool found = false;
                        for (auto sp : *basetype(sp2->tp)->syms)
                        {
                            if (sp->sb->templateLevel)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (found)
                        {
                            lex = GetTemplateArguments(lex, funcsp, nullptr, &funcparams->templateParams);
                            funcparams->astemplate = true;
                        }
                        else if (isdest && sp2->sb->parentClass->sb->templateLevel)
                        {
                            std::list<TEMPLATEPARAMPAIR>* lst = nullptr;
                            lex = GetTemplateArguments(lex, funcsp, nullptr, &lst);
                            if (!exactMatchOnTemplateArgs(lst, sp2->sb->parentClass->templateParams->front().second->bySpecialization.types))
                                error(ERR_DESTRUCTOR_MUST_MATCH_CLASS);
                        }
                        else if (isTemplate)
                        {
                            errorsym(ERR_NOT_A_TEMPLATE, sp2);
                        }
                    }
                    else if (isTemplate)
                    {
                        errorsym(ERR_NOT_A_TEMPLATE, sp2);
                    }
                    funcparams->sp = sp2;
                    funcparams->thisptr = *exp;
                    funcparams->thistp = MakeType(BasicType::pointer_, basetp);

                    if (!points && (*exp)->type != ExpressionNode::l_ref_ && !typein->lref && !typein->rref)
                        funcparams->novtab = true;
                    *exp = varNode(ExpressionNode::func_, nullptr);
                    (*exp)->v.func = funcparams;
                }
                else
                {
                    SYMBOL* sp3 = sp2->sb->parentClass;
                    if (ismutable && sp2->sb->storage_class == StorageClass::mutable_)
                        *ismutable = true;
                    if (sp2->sb->templateLevel)
                    {
                        if (!sp2->sb->instantiated)
                        {
                            std::list<TEMPLATEPARAMPAIR>* lst = nullptr;
                            SYMBOL* sp1 = sp2;
                            if (MATCHKW(lex, Keyword::lt_))
                            {
                                lex = GetTemplateArguments(lex, funcsp, sp2, &lst);
                                if (sp1)
                                {
                                    sp1 = GetClassTemplate(sp1, lst, false);
                                    if (sp1)
                                        sp2 = sp1;
                                    else
                                        errorsym(ERR_NO_TEMPLATE_MATCHES, sp2);
                                }
                            }
                            else if (sp2)
                            {
                                sp1 = sp2;
                            }
                        }
                    }
                    else if (isTemplate)
                    {
                        errorsym(ERR_NOT_A_TEMPLATE, sp2);
                    }
                    if (sp2->sb->storage_class == StorageClass::external_ || sp2->sb->storage_class == StorageClass::static_)
                    {
                        SYMBOL* tpl = sp2;
                        while (tpl)
                        {
                            if (tpl->sb->templateLevel)
                                break;
                            tpl = tpl->sb->parentClass;
                        }
                        if (tpl && tpl->sb->instantiated)
                        {
                            TemplateDataInstantiate(sp2, false, false);
                        }
                    }
                    {
                        enum AccessLevel access = AccessLevel::public_;
                        SYMBOL* ssp = getStructureDeclaration();
                        while (ssp)
                        {
                            if (ssp == basetype(typ2)->sp)
                                access = AccessLevel::protected_;
                            ssp = ssp->sb->parentClass;
                        }
                        ssp = getStructureDeclaration();
                        if (!isExpressionAccessible(funcsp ? funcsp->sb->parentClass : nullptr, sp2, funcsp, *exp, true))
                        {
                            errorsym(ERR_CANNOT_ACCESS, sp2);
                        }
                    }
                    bool structuredAlias = false;
                    if (sp2->sb->storage_class == StorageClass::constant_)
                    {
                        *exp = varNode(ExpressionNode::const_, sp2);
                    }
                    else if (sp2->sb->storage_class == StorageClass::enumconstant_)
                    {
                        *exp = intNode(ExpressionNode::c_i_, sp2->sb->value.i);
                    }
                    else if (sp2->sb->storage_class == StorageClass::static_ || sp2->sb->storage_class == StorageClass::external_)
                    {
                        if (Optimizer::architecture == ARCHITECTURE_MSIL)
                        {
                            *exp = varNode(ExpressionNode::global_, sp2);
                        }
                        else
                        {
                            EXPRESSION* exp2 = varNode(ExpressionNode::global_, sp2);
                            *exp = exprNode(ExpressionNode::void_, *exp, exp2);
                        }
                    }
                    else
                    {
                        if (isstructured(typein) && !typein->lref && !typein->rref && !(flags & _F_AMPERSAND) && basetype(typein)->sp->sb->structuredAliasType)
                        {
                            structuredAlias = true;
                            // use it as-is
                        }
                        else
                        {
                            EXPRESSION* offset;
                            if (Optimizer::architecture == ARCHITECTURE_MSIL)
                            {
                                offset = varNode(ExpressionNode::structelem_, sp2);  // prepare for the MSIL ldflda instruction
                            }
                            else
                                offset = intNode(ExpressionNode::c_i_, sp2->sb->offset);
                            if (!typein2 && sp2->sb->parentClass != basetype(typ2)->sp)
                            {
                                *exp = baseClassOffset(sp2->sb->parentClass, basetype(typ2)->sp, *exp);
                            }
                            *exp = exprNode(ExpressionNode::structadd_, *exp, offset);
                            if (sp3)
                            {
                                do
                                {
                                    if (basetype(sp3->tp)->type == BasicType::union_)
                                    {
                                        offset->unionoffset = true;
                                        break;
                                    }
                                    if (sp3 != sp2 && ispointer(sp3->tp))
                                        break;
                                    sp3 = sp3->sb->parent;
                                } while (sp3);
                            }
                        }
                        if (isref(*tp))
                        {
                            deref(*tp, exp);
                            *tp = basetype(*tp)->btp;
                        }
                        if (typein2)
                            deref(typein2, exp);
                    }
                    if (tpb->hasbits)
                    {
                        (*exp) = exprNode(ExpressionNode::bits_, *exp, 0);
                        (*exp)->bits = tpb->bits;
                        (*exp)->startbit = tpb->startbit;
                    }
                    if (sp2->sb->storage_class != StorageClass::constant_ && sp2->sb->storage_class != StorageClass::enumconstant_)
                    {
                        deref(*tp, exp);
                    }
                    if (sp2->sb->storage_class != StorageClass::enumconstant_ && !structuredAlias)
                    {
                        (*exp)->v.sp = sp2;  // caching the member symbol in the enode for constexpr handling
                    }
                    if (isatomic(basetp))
                    {
                        // note this won't work in C++ because of offset2...
                        (*exp)->isatomic = true;
                        if (needsAtomicLockFromType(*tp))
                            (*exp)->lockOffset = basetp->size - ATOMIC_FLAG_SPACE - sp2->sb->offset;
                    }
                }
                if (!points && ((*exp)->type == ExpressionNode::not__lvalue_ || (*exp)->type == ExpressionNode::func_ || (*exp)->type == ExpressionNode::void_))
                    if (ISKW(lex) && lex->data->kw->key >= Keyword::assign_ && lex->data->kw->key <= Keyword::asxor_)
                        error(ERR_LVALUE);
            }
        }
    }
    return lex;
}
TYPE* LookupSingleAggregate(TYPE* tp, EXPRESSION** exp, bool memberptr)
{
    auto tp1 = tp;
    if (tp->type == BasicType::aggregate_)
    {
        auto sp = tp->syms->front();
        if (memberptr && sp->sb->parentClass && sp->sb->storage_class != StorageClass::static_ && sp->sb->storage_class != StorageClass::global_ &&
            sp->sb->storage_class != StorageClass::external_)
        {
            EXPRESSION* rv;
            tp1 = MakeType(BasicType::memberptr_, sp->tp);
            tp1->sp = sp->sb->parentClass;
            *exp = varNode(ExpressionNode::memberptr_, sp);
            (*exp)->isfunc = true;
        }
        else
        {
            if ((*exp)->type == ExpressionNode::func_ && (*exp)->v.func->templateParams)
            {
                sp = detemplate(sp, (*exp)->v.func, nullptr);
                if (!sp)
                {
                    sp = tp->syms->front();
                }
                else
                {
                    if (sp->sb->templateLevel && !templateNestingCount && sp->templateParams)
                    {
                        sp = TemplateFunctionInstantiate(sp, false);
                    }
                }
            }
            *exp = varNode(ExpressionNode::pc_, sp);
        }
        auto it = tp->syms->begin();
        for (++it; it != tp->syms->end(); ++it)
        {
            auto sp = *it;
            if (!sp->sb->templateLevel || !sp->sb->mainsym)
            {
                errorsym(ERR_OVERLOADED_FUNCTION_AMBIGUOUS, sp);
                break;
            }
        }
    }
    return tp1;
}
static EXPRESSION* MsilRebalanceArray(EXPRESSION* in)
{
    /* sizeof operators will already be present */
    if (in->type == ExpressionNode::add_ && in->right->type == ExpressionNode::umul_ && in->right->right->type == ExpressionNode::sizeof_)
    {
        if (in->left->type == ExpressionNode::add_ && in->left->right->type == ExpressionNode::umul_ && in->left->right->right->type == ExpressionNode::sizeof_)
        {
            if (comparetypes(in->right->right->left->v.tp, in->left->right->right->left->v.tp, 0))
            {
                EXPRESSION* temp = exprNode(ExpressionNode::add_, in->left->right->left, in->right->left);
                temp = exprNode(ExpressionNode::umul_, temp, in->right->right);
                in->left = in->left->left;
                in->right = temp;
            }
        }
    }
    return in;
}
static LEXLIST* expression_bracket(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    TYPE* tp2 = nullptr;
    EXPRESSION* expr2 = nullptr;
    lex = getsym();
    if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::begin_))
    {
        std::list<INITLIST*>* args = nullptr;
        lex = getInitList(lex, funcsp, &args);
        if ((Optimizer::cparams.prm_cplusplus || (Optimizer::architecture == ARCHITECTURE_MSIL)) &&
            insertOperatorFunc(ovcl_openbr, Keyword::openbr_, funcsp, tp, exp, nullptr, nullptr, args, flags))
        {
        }
        else
        {
            error(ERR_CANNOT_USE_INIT_LIST_WITH_BUILTIN_BRACKET);
        }
    }
    else
    {
        lex = expression_comma(lex, funcsp, nullptr, &tp2, &expr2, nullptr, flags);
        if (tp2)
        {
            LookupSingleAggregate(tp2, &expr2);
            if ((isstructuredmath(*tp, tp2) || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                insertOperatorFunc(ovcl_openbr, Keyword::openbr_, funcsp, tp, exp, tp2, expr2, nullptr, flags))
            {
            }
            else if (isvoid(*tp) || isvoid(tp2) || (*tp)->type == BasicType::aggregate_ || ismsil(*tp) || ismsil(tp2))
                error(ERR_NOT_AN_ALLOWED_TYPE);
            else if (basetype(tp2)->type == BasicType::memberptr_ || basetype(*tp)->type == BasicType::memberptr_)
                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
            else if ((basetype(tp2)->scoped || basetype(*tp)->scoped) && !(flags & _F_SCOPEDENUM))
                error(ERR_SCOPED_TYPE_MISMATCH);
            else if (isarray(*tp) && (*tp)->msil)
            {
                if (!isint(tp2) && basetype(tp2)->type != BasicType::enum_)
                {
                    if (ispointer(tp2))
                        error(ERR_NONPORTABLE_POINTER_CONVERSION);
                    else
                        error(ERR_NEED_INTEGER_EXPRESSION);
                }
                if ((*exp)->type != ExpressionNode::msil_array_access_)
                {
                    EXPRESSION* expr3 = exprNode(ExpressionNode::msil_array_access_, nullptr, nullptr);
                    TYPE* btp = *tp;
                    int count = 0;
                    while (isarray(btp) && btp->msil)
                    {
                        count++;
                        btp = btp->btp;
                    }
                    expr3->v.msilArray = (MSIL_ARRAY*)Alloc(sizeof(MSIL_ARRAY) + count - 1 * sizeof(EXPRESSION*));
                    expr3->v.msilArray->max = count;
                    expr3->v.msilArray->count = 0;
                    expr3->v.msilArray->base = (*exp);
                    expr3->v.msilArray->tp = *tp;
                    (*exp) = expr3;
                }
                (*exp)->v.msilArray->indices[(*exp)->v.msilArray->count++] = expr2;
                *tp = basetype(*tp)->btp;
                if (!MATCHKW(lex, Keyword::closebr_))
                {
                    error(ERR_ARRAY_NEED_CLOSEBRACKET);
                    errskim(&lex, skim_closebr);
                }
                skip(&lex, Keyword::closebr_);
                if ((*exp)->v.msilArray->count < (*exp)->v.msilArray->max && !MATCHKW(lex, Keyword::openbr_))
                {
                    error(ERR_ASSIGN_ONLY_MSIL_ARRAY_ELEMENTS);
                }
                *tp = PerformDeferredInitialization(*tp, funcsp);
                return lex;
            }
            else if (ispointer(*tp) && !isfuncptr(*tp))
            {
                if (!isint(tp2) && basetype(tp2)->type != BasicType::enum_)
                {
                    if (ispointer(tp2))
                        error(ERR_NONPORTABLE_POINTER_CONVERSION);
                    else
                        error(ERR_NEED_INTEGER_EXPRESSION);
                }
                /* unreference pointer to VLA */
                if (!(*tp)->vla && basetype(basetype(*tp)->btp)->vla)
                    (*tp) = basetype(*tp)->btp;
                if ((*tp)->vla)
                {
                    int soa = getSize(BasicType::pointer_);
                    int sou = getSize(BasicType::unsigned_);
                    EXPRESSION* vlanode = varNode(ExpressionNode::auto_, (*tp)->sp);
                    EXPRESSION* exp1 = exprNode(ExpressionNode::add_, vlanode, intNode(ExpressionNode::c_i_, soa + sou * (1 + (*tp)->vlaindex)));
                    deref(&stdint, &exp1);
                    cast(&stdpointer, &expr2);
                    exp1 = exprNode(ExpressionNode::umul_, exp1, expr2);
                    *exp = exprNode(ExpressionNode::add_, *exp, exp1);
                    *tp = basetype(*tp)->btp;
                }
                else
                {
                    EXPRESSION* exp1 = nullptr;
                    *tp = basetype(*tp)->btp;
                    if (isstructured(*tp))
                        *tp = PerformDeferredInitialization(*tp, funcsp);
                    cast(&stdint, &expr2);
                    if (Optimizer::architecture == ARCHITECTURE_MSIL)
                    {
                        cast(&stdinative, &expr2);
                        if (!isarray(*tp))
                        {
                            auto exp3 = exprNode(ExpressionNode::sizeof_, typeNode(*tp), nullptr);
                            exp1 = exprNode(ExpressionNode::umul_, expr2, exp3);
                        }
                        else
                        {
                            TYPE* tp1 = *tp;
                            int n = tp1->size;
                            while (isarray(tp1))
                                tp1 = tp1->btp;
                            n = n / tp1->size;
                            auto exp3 = exprNode(ExpressionNode::sizeof_, typeNode(tp1), nullptr);
                            auto exp4 = intNode(ExpressionNode::c_i_, n);
                            expr2 = exprNode(ExpressionNode::umul_, expr2, exp4);
                            exp1 = exprNode(ExpressionNode::umul_, expr2, exp3);
                        }
                    }
                    else
                    {
                        exp1 = exprNode(ExpressionNode::umul_, expr2, intNode(ExpressionNode::c_i_, (*tp)->size));
                    }
                    *exp = exprNode(ExpressionNode::add_, *exp, exp1);
                    if (Optimizer::architecture == ARCHITECTURE_MSIL)
                    {
                        *exp = MsilRebalanceArray(*exp);
                    }
                }
                if (!(*tp)->array && !(*tp)->vla)
                    deref(*tp, exp);
            }
            else if (ispointer(tp2) && !isfuncptr(tp2))
            {
                if (!isint(*tp))
                {
                    if (ispointer(*tp))
                        error(ERR_NONPORTABLE_POINTER_CONVERSION);
                    else
                        error(ERR_NEED_INTEGER_EXPRESSION);
                }
                if (tp2->vla)
                {
                    int soa = getSize(BasicType::pointer_);
                    int sou = getSize(BasicType::unsigned_);
                    EXPRESSION* vlanode = varNode(ExpressionNode::auto_, tp2->sp);
                    expr2 = exprNode(ExpressionNode::add_, expr2, intNode(ExpressionNode::c_i_, soa + sou * (1 + tp2->vlaindex)));
                    deref(&stdint, exp);
                    *exp = exprNode(ExpressionNode::umul_, expr2, *exp);
                    deref(&stdpointer, &vlanode);
                    *exp = exprNode(ExpressionNode::add_, *exp, vlanode);
                    *tp = basetype(tp2)->btp;
                }
                else
                {
                    EXPRESSION* exp1 = nullptr;
                    *tp = basetype(tp2)->btp;
                    if (Optimizer::architecture == ARCHITECTURE_MSIL)
                    {
                        cast(&stdinative, exp);
                        if (!isarray(*tp))
                        {
                            auto exp3 = exprNode(ExpressionNode::sizeof_, typeNode(*tp), nullptr);
                            exp1 = exprNode(ExpressionNode::umul_, *exp, exp3);
                        }
                        else
                        {
                            TYPE* tp1 = *tp;
                            int n = tp1->size;
                            while (isarray(tp1))
                                tp1 = tp1->btp;
                            n = n / tp1->size;
                            auto exp3 = exprNode(ExpressionNode::sizeof_, typeNode(tp1), nullptr);
                            auto exp4 = intNode(ExpressionNode::c_i_, n);
                            expr2 = exprNode(ExpressionNode::umul_, *exp, exp4);
                            exp1 = exprNode(ExpressionNode::umul_, *exp, exp3);
                        }
                    }
                    else
                    {
                        exp1 = exprNode(ExpressionNode::umul_, *exp, intNode(ExpressionNode::c_i_, (*tp)->size));
                    }
                    *exp = exprNode(ExpressionNode::add_, expr2, exp1);
                    if (Optimizer::architecture == ARCHITECTURE_MSIL)
                    {
                        *exp = MsilRebalanceArray(*exp);
                    }
                }
                if (!(*tp)->array && !(*tp)->vla)
                    deref(*tp, exp);
            }
            else if (!templateNestingCount || basetype(*tp)->type != BasicType::templateselector_)
            {
                error(ERR_DEREF);
            }
        }
        else
            error(ERR_EXPRESSION_SYNTAX);
    }
    if (!MATCHKW(lex, Keyword::closebr_))
    {
        error(ERR_ARRAY_NEED_CLOSEBRACKET);
        errskim(&lex, skim_closebr);
    }
    skip(&lex, Keyword::closebr_);
    *tp = PerformDeferredInitialization(*tp, funcsp);
    return lex;
}
static bool smallint(TYPE* tp)
{
    tp = basetype(tp);
    if (tp->type < BasicType::int_)
    {
        return true;
    }
    else if (isbitint(tp))
    {
        int sz = getSize(BasicType::int_) * CHAR_BIT;
        return tp->bitintbits < sz;
    }
    else
    {
        return false;
    }
}
static bool largenum(TYPE* tp)
{
    tp = basetype(tp);
    if (tp->type > BasicType::int_)
    {
        if (isbitint(tp))
        {
            int sz = getSize(BasicType::int_) * CHAR_BIT;
            return tp->bitintbits > sz || (tp->bitintbits == sz && tp->type == BasicType::unsigned_bitint_);
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}
void checkArgs(FUNCTIONCALL* params, SYMBOL* funcsp)
{
    std::list<INITLIST*>::iterator itp, itpe = itp;
    if (params->arguments)
    {
        itp = params->arguments->begin();
        itpe = params->arguments->end();
    }
    
    bool matching = true;
    bool tooshort = false;
    bool toolong = false;
    bool noproto = false;  // params->sp ? params->sp->sb->oldstyle : false;
    bool vararg = false;
    bool hasEllipse = false;
    int argnum = 0;

    auto it = basetype(params->functp)->syms->begin();
    auto itend = basetype(params->functp)->syms->end();
    if (it != itend && (*it)->sb->thisPtr)
        ++it;
    if (it == itend)
    {
        matching = false;
        noproto = params->sp->sb->oldstyle;
    }
    if (noproto)
    {
        if (params->sp->sb->hasproto)
            noproto = false;
    }
    if (it != itend && (*it)->tp->type == BasicType::void_)
    {
        if (itp != itpe)
            errorsym(ERR_FUNCTION_TAKES_NO_PARAMETERS, params->sp);
    }
    else
        while (it != itend || itp != itpe)
        {
            TYPE* dest = nullptr;
            SYMBOL* decl = nullptr;
            if (it == itend || (*it)->tp->type != BasicType::any_)
            {
                argnum++;
                if (matching && it != itend)
                {
                    decl = *it;
                    if (!decl->tp)
                        noproto = true;
                    else if (basetype(decl->tp)->type == BasicType::ellipse_)
                    {
                        hasEllipse = true;
                        vararg = (Optimizer::architecture == ARCHITECTURE_MSIL) && msilManaged(params->sp);
                        params->vararg = vararg;
                        matching = false;
                        decl = nullptr;
                        it = itend;
                        if (itp == itpe)
                            break;
                    }
                }
                if (!decl && !hasEllipse && (Optimizer::architecture == ARCHITECTURE_MSIL))
                    toolong = true;
                if (matching)
                {
                    if (!decl)
                        toolong = true;
                    else if (itp == itpe && !(*it)->sb->init && !(*it)->sb->deferredCompile)
                        tooshort = true;
                    else
                    {
                        if (decl->sb->attribs.inheritable.zstring)
                        {
                            EXPRESSION* exp = (*itp)->exp;
                            if (!exp)
                            {
                                exp = (*itp)->exp = intNode(ExpressionNode::c_i_, 0);
                            }
                            if (lvalue(exp))
                                exp = exp->left;
                            switch (exp->type)
                            {
                                case ExpressionNode::global_:
                                case ExpressionNode::auto_:
                                case ExpressionNode::absolute_:
                                case ExpressionNode::pc_:
                                case ExpressionNode::threadlocal_:
                                    if (exp->v.sp->sb->attribs.inheritable.nonstring)
                                    {
                                        error(ERR_NULL_TERMINATED_STRING_REQUIRED);
                                    }
                                    break;
                            }
                        }
                        if (isref(decl->tp))
                        {
                            TYPE* tpb = basetype(basetype(decl->tp)->btp);
                            TYPE* tpd = (*itp)->tp;
                            (*itp)->byRef = true;
                            if (isref(tpd))
                                tpd = basetype(tpd)->btp;
                            tpd = basetype(tpd);
                            if (isstructured(tpb) && isstructured(tpd))
                            {
                                SYMBOL* base = basetype(tpb)->sp;
                                SYMBOL* derived = basetype(tpd)->sp;
                                if (base != derived && classRefCount(base, derived) != 1)
                                    errorConversionOrCast(true, (*itp)->tp, decl->tp);
                                if (base != derived && !isAccessible(derived, derived, base, funcsp, AccessLevel::public_, false))
                                    errorsym(ERR_CANNOT_ACCESS, base);
                            }
                            else
                            {
                                goto join;
                            }
                        }
                        else
                        {
                        join:
                            if (itp == itpe || !(*itp)->tp)
                            {
                            }
                            else if (basetype(decl->tp)->type == BasicType::string_)
                            {
                                if ((*itp)->exp->type == ExpressionNode::labcon_)
                                    (*itp)->exp->type = ExpressionNode::c_string_;
                            }
                            else if (!comparetypes((*itp)->tp, decl->tp, false))
                            {
                                if ((Optimizer::architecture != ARCHITECTURE_MSIL) || !isstructured(decl->tp))
                                {
                                    if (basetype(decl->tp)->type == BasicType::object_)
                                    {
                                        if (!isstructured((*itp)->tp) && (!isarray((*itp)->tp) || !basetype((*itp)->tp)->msil))
                                        {
                                            (*itp)->exp = exprNode(ExpressionNode::x_object_, (*itp)->exp, nullptr);
                                        }
                                    }
                                    else if ((!isbitint((*itp)->tp) && !isint((*itp)->tp)) || (!isbitint(decl->tp) && !isint(decl->tp)))
                                        if (basetype(decl->tp)->type != BasicType::memberptr_)
                                            errorarg(ERR_TYPE_MISMATCH_IN_ARGUMENT, argnum, decl, params->sp);
                                }
                                else if (isstructured((*itp)->tp))
                                {
                                    SYMBOL* base = basetype(decl->tp)->sp;
                                    SYMBOL* derived = basetype((*itp)->tp)->sp;
                                    if (base != derived && classRefCount(base, derived) != 1)
                                        errorConversionOrCast(true, (*itp)->tp, decl->tp);
                                    if (base != derived && !isAccessible(derived, derived, base, funcsp, AccessLevel::public_, false))
                                        errorsym(ERR_CANNOT_ACCESS, base);
                                }
                                else if (!isconstzero((*itp)->tp, (*itp)->exp))
                                    errorarg(ERR_TYPE_MISMATCH_IN_ARGUMENT, argnum, decl, params->sp);
                            }
                            else if (assignDiscardsConst(decl->tp, (*itp)->tp))
                            {
                                if (Optimizer::cparams.prm_cplusplus)
                                    errorConversionOrCast(true, (*itp)->tp, decl->tp);
                                else
                                    error(ERR_SUSPICIOUS_POINTER_CONVERSION);
                            }
                            else if (ispointer(decl->tp))
                            {
                                if (!ispointer((*itp)->tp) && (!isarithmeticconst((*itp)->exp) || !isconstzero(decl->tp, (*itp)->exp)))
                                {
                                    if (!isfunction((*itp)->tp) ||
                                        (!isvoidptr(decl->tp) &&
                                         (!isfuncptr(decl->tp) || (Optimizer::cparams.prm_cplusplus &&
                                                                   !comparetypes(basetype(decl->tp)->btp, (*itp)->tp, true)))))
                                        errorarg(ERR_TYPE_MISMATCH_IN_ARGUMENT, argnum, decl, params->sp);
                                }
                                else if (!comparetypes(decl->tp, (*itp)->tp, true))
                                {
                                    if (!isconstzero((*itp)->tp, (*itp)->exp))
                                    {
                                        if (!isvoidptr(decl->tp) && !isvoidptr((*itp)->tp))
                                        {
                                            if (Optimizer::cparams.prm_cplusplus)
                                            {
                                                TYPE* tpb = basetype(decl->tp)->btp;
                                                TYPE* tpd = basetype((*itp)->tp)->btp;
                                                if (isstructured(tpb) && isstructured(tpd))
                                                {
                                                    SYMBOL* base = basetype(tpb)->sp;
                                                    SYMBOL* derived = basetype(tpd)->sp;
                                                    if (base != derived && classRefCount(base, derived) != 1)
                                                        errorConversionOrCast(true, (*itp)->tp, decl->tp);
                                                    if (isAccessible(derived, derived, base, funcsp, AccessLevel::public_, false))
                                                        errorsym(ERR_CANNOT_ACCESS, base);
                                                }
                                                else
                                                {
                                                    errorConversionOrCast(true, (*itp)->tp, decl->tp);
                                                }
                                            }
                                            else
                                            {
                                                if (!matchingCharTypes((*itp)->tp, decl->tp))
                                                    error(ERR_SUSPICIOUS_POINTER_CONVERSION);
                                            }
                                        }
                                    }
                                }
                            }
                            else if (ispointer((*itp)->tp))
                            {
                                if (!ispointer(decl->tp) && basetype(decl->tp)->type != BasicType::bool_)
                                    errorarg(ERR_TYPE_MISMATCH_IN_ARGUMENT, argnum, decl, params->sp);
                            }
                            else if (isarithmetic((*itp)->tp) && isarithmetic(decl->tp))
                            {
                                if (basetype(decl->tp)->type != basetype((*itp)->tp)->type && largenum(decl->tp))
                                {
                                    cast(decl->tp, &(*itp)->exp);
                                }
                            }
                        }
                        dest = decl->tp;
                    }
                }
                else if ((*itp)->tp->vla)
                {
                    // cast to a regular pointer if there is no declared param
                    (*itp)->tp = MakeType(BasicType::pointer_, (*itp)->tp);
                    while ((*itp)->tp->btp->vla)
                        (*itp)->tp->btp = (*itp)->tp->btp->btp;
                }
                else if (isint((*itp)->tp))
                {
                    if (basetype((*itp)->tp)->type <= BasicType::int_)
                        dest = &stdint;
                    else if (!(Optimizer::architecture == ARCHITECTURE_MSIL))
                        cast((*itp)->tp, &(*itp)->exp);
                }
                else if (isfloat((*itp)->tp))
                {
                    if (basetype((*itp)->tp)->type < BasicType::double_)
                        dest = &stddouble;
                    else if (!(Optimizer::architecture == ARCHITECTURE_MSIL))
                        cast((*itp)->tp, &(*itp)->exp);
                }
                else if (ispointer((*itp)->tp))
                {

                    if (Optimizer::architecture == ARCHITECTURE_MSIL &&
                        ((*itp)->exp->type != ExpressionNode::auto_ || !(*itp)->exp->v.sp->sb->va_typeof) && !params->vararg)
                        cast(&stdint, &(*itp)->exp);
                }
                if (dest && itp != itpe && (*itp)->tp && basetype(dest)->type != BasicType::memberptr_ && !comparetypes(dest, (*itp)->tp, true))
                {
                    if ((Optimizer::architecture != ARCHITECTURE_MSIL) ||
                        (!isstructured(dest) && (!isarray(dest) || !basetype(dest)->msil)))
                        cast(basetype(dest), &(*itp)->exp);
                    (*itp)->tp = dest;
                }
                else if (dest && itp != itpe && basetype(dest)->type == BasicType::enum_)
                {
                    // this needs to be revisited to get proper typing in C++
                    cast(&stdint, &(*itp)->exp);
                    (*itp)->tp = &stdint;
                }
            }
            if (itp != itpe)
            {
                bool ivararg = vararg;
                if ((*itp)->exp->type == ExpressionNode::auto_ && (*itp)->exp->v.sp->sb && (*itp)->exp->v.sp->sb->va_typeof)
                    ivararg = false;
                (*itp)->vararg = ivararg;
                if (it != itend)
                {
                    TYPE* tp = (*it)->tp;
                    while (tp && tp->type != BasicType::pointer_)
                    {
                        if (tp->type == BasicType::va_list_)
                        {
                            (*itp)->valist = true;
                            break;
                        }
                        tp = tp->btp;
                    }
                }
                ++itp;
            }
            if (it != itend)
                ++it;
        }
    if (noproto)
        errorsym(ERR_CALL_FUNCTION_NO_PROTO, params->sp);
    else if (toolong)
        errorsym(ERR_PARAMETER_LIST_TOO_LONG, params->sp);
    else if (tooshort)
        errorsym(ERR_PARAMETER_LIST_TOO_SHORT, params->sp);
}
static LEXLIST* getInitInternal(LEXLIST* lex, SYMBOL* funcsp, std::list<INITLIST*>** lptr, Keyword finish, bool allowNesting, bool allowPack,
                                bool toErr, int flags)
{
    *lptr = initListListFactory.CreateList();
    lex = getsym(); /* past ( */
    while (!MATCHKW(lex, finish))
    {
        INITLIST* p = Allocate<INITLIST>();
        if (finish == Keyword::end_)
            p->initializer_list = true;
        if (MATCHKW(lex, Keyword::begin_))
        {
            lex = getInitInternal(lex, funcsp, &p->nested, Keyword::end_, true, false, false, flags);
            (*lptr)->push_back(p);
            if (!allowNesting)
                error(ERR_CANNOT_USE_INIT_LIST);
        }
        else
        {
            LEXLIST* start = lex;
            lex = expression_assign(lex, funcsp, nullptr, &p->tp, &p->exp, nullptr,
                                    _F_PACKABLE | (finish == Keyword::closepa_ ? _F_INARGS : 0) | (flags & _F_SIZEOF));
            if (p->tp && isvoid(p->tp) && finish != Keyword::closepa_)
                error(ERR_NOT_AN_ALLOWED_TYPE);
            if (!p->exp)
                p->exp = intNode(ExpressionNode::c_i_, 0);
            optimize_for_constants(&p->exp);
            if ((!templateNestingCount || instantiatingTemplate) && p->tp && p->tp->type == BasicType::templateselector_)
                p->tp = LookupTypeFromExpression(p->exp, nullptr, false);
            if (finish != Keyword::closepa_)
            {
                assignmentUsages(p->exp, false);
            }
            else if (p->tp && isstructured(p->tp))
            {
                auto exp3 = p->exp;
                while (exp3->type == ExpressionNode::void_)
                    exp3 = exp3->right;
                if (exp3->type == ExpressionNode::thisref_)
                {
                    p->tp = CopyType(p->tp);
                    p->tp->lref = false;
                    p->tp->rref = true;
                }
            }
            if (p->tp)
            {
                if (p->exp && p->exp->type == ExpressionNode::func_ && p->exp->v.func->sp->sb->parentClass && !p->exp->v.func->ascall &&
                    !p->exp->v.func->asaddress && p->exp->v.func->functp)
                {
                    for (auto sym: *basetype(p->exp->v.func->functp)->syms)
                    {
                        if (sym->sb->storage_class == StorageClass::member_ || sym->sb->storage_class == StorageClass::mutable_)
                        {
                            error(ERR_NO_IMPLICIT_MEMBER_FUNCTION_ADDRESS);
                            break;
                        }
                    }
                }
                if (allowPack && Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::ellipse_))
                {
                    // lose p
                    lex = getsym();
                    if (templateNestingCount)
                    {
                        (*lptr)->push_back(p);
                    }
                    else if (p->exp && p->exp->type != ExpressionNode::packedempty_ && p->tp->type != BasicType::any_)
                    {
                        if (!isstructured(p->tp) && !p->tp->templateParam)
                            checkPackedExpression(p->exp);
                        // this is going to presume that the expression involved
                        // is not too long to be cached by the LEXLIST mechanism.
                        expandPackedInitList(lptr, funcsp, start, p->exp);
                    }
                }
                else
                {
                    if (toErr && argumentNesting <= 1)
                        checkUnpackedExpression(p->exp);
                    (*lptr)->push_back(p);
                }
            }
            else
            {
                lex = backupsym();
                error(ERR_IDENTIFIER_EXPECTED);
                errskim(&lex, finish == Keyword::closepa_ ? skim_closepa : skim_end);
                return lex;
            }
        }
        if (!MATCHKW(lex, Keyword::comma_))
        {
            break;
        }
        lex = getsym();
        if (MATCHKW(lex, finish))
            break;
    }
    if (!needkw(&lex, finish))
    {
        errskim(&lex, finish == Keyword::closepa_ ? skim_closepa : skim_end);
        skip(&lex, finish);
    }
    DestructParams(*lptr);
    return lex;
}
LEXLIST* getInitList(LEXLIST* lex, SYMBOL* funcsp, std::list<INITLIST*>** owner)
{
    argumentNesting++;
    auto rv = getInitInternal(lex, funcsp, owner, Keyword::end_, false, true, true, 0);
    argumentNesting--;
    return rv;
}
LEXLIST* getArgs(LEXLIST* lex, SYMBOL* funcsp, FUNCTIONCALL* funcparams, Keyword finish, bool allowPack, int flags)
{
    LEXLIST* rv;
    argumentNesting++;
    rv = getInitInternal(lex, funcsp, &funcparams->arguments, finish, true, allowPack, argumentNesting == 1, flags);
    argumentNesting--;
    return rv;
}
LEXLIST* getMemberInitializers(LEXLIST* lex, SYMBOL* funcsp, FUNCTIONCALL* funcparams, Keyword finish, bool allowPack)
{
    LEXLIST* rv;
    argumentNesting++;
    rv = getInitInternal(lex, funcsp, &funcparams->arguments, finish, true, allowPack, false, 0);
    argumentNesting--;
    return rv;
}
static int simpleDerivation(EXPRESSION* exp)
{
    int rv = 0;
    while (castvalue(exp))
        exp = exp->left;
    if (exp->left)
        rv |= simpleDerivation(exp->left);
    if (exp->right)
        rv |= simpleDerivation(exp->right);
    switch (exp->type)
    {
        case ExpressionNode::func_:
        case ExpressionNode::thisref_:
        case ExpressionNode::stmt_:
            rv |= 1;
            break;
        case ExpressionNode::labcon_:
        case ExpressionNode::global_:
        case ExpressionNode::auto_:
        case ExpressionNode::absolute_:
        case ExpressionNode::pc_:
        case ExpressionNode::threadlocal_:
            rv |= 2;
            break;
        default:
            if (lvalue(exp))
                rv |= 1;
            break;
    }
    return rv;
}
EXPRESSION* DerivedToBase(TYPE* tpn, TYPE* tpo, EXPRESSION* exp, int flags)
{
    if (isref(tpn))
        tpn = basetype(tpn)->btp;
    if (isref(tpo))
        tpo = basetype(tpo)->btp;
    if (isstructured(tpn) && isstructured(tpo))
    {
        SYMBOL* spn = basetype(tpn)->sp;
        SYMBOL* spo = basetype(tpo)->sp;
        if (spo != spn && !sameTemplate(tpn, tpo))
        {
            int n = classRefCount(spn, spo);
            if (n == 1)
            {
                // derived to base
                EXPRESSION q = {}, *v = &q;
                v->type = ExpressionNode::c_i_;
                v = baseClassOffset(spn, spo, v);

                optimize_for_constants(&v);
                if (v->type != ExpressionNode::c_i_ && (flags & _F_NOVIRTUALBASE))
                    return nullptr;

                v = baseClassOffset(spn, spo, exp);
                if ((flags & _F_VALIDPOINTER) || v->type != ExpressionNode::c_i_ || simpleDerivation(exp) == 2)
                {
                    optimize_for_constants(&v);
                }
                else
                {
                    EXPRESSION* varsp = anonymousVar(StorageClass::auto_, &stdpointer);
                    EXPRESSION* var = exprNode(ExpressionNode::l_p_, varsp, nullptr);
                    EXPRESSION* asn = exprNode(ExpressionNode::assign_, var, exp);
                    EXPRESSION* left = exprNode(ExpressionNode::add_, var, v);
                    EXPRESSION* right = var;
                    v = exprNode(ExpressionNode::cond_, var, exprNode(ExpressionNode::void_, left, right));
                    v = exprNode(ExpressionNode::void_, asn, v);
                }
                return v;
            }
        }
    }
    return exp;
}
static bool cloneTempExpr(EXPRESSION** expr, SYMBOL** found, SYMBOL** replace);
bool cloneTempStmt(std::list<STATEMENT*>** blocks, SYMBOL** found, SYMBOL** replace)
{
    bool rv = false;
    auto p = *blocks;
    *blocks = stmtListFactory.CreateList();
    for (auto block : *p)
    {
        if (found)
        {
            STATEMENT* b = Allocate<STATEMENT>();
            *b = *block;
            block = b;
            (*blocks)->push_back(b);
        }
        switch (block->type)
        {
            case StatementNode::genword_:
                break;
            case StatementNode::try_:
            case StatementNode::catch_:
            case StatementNode::seh_try_:
            case StatementNode::seh_catch_:
            case StatementNode::seh_finally_:
            case StatementNode::seh_fault_:
                rv |= cloneTempStmt(&block->lower, found, replace);
                rv |= cloneTempStmt(&block->blockTail, found, replace);
                break;
            case StatementNode::return_:
            case StatementNode::expr_:
            case StatementNode::declare_:
                rv |= cloneTempExpr(&block->select, found, replace);
                break;
            case StatementNode::goto_:
            case StatementNode::label_:
                break;
            case StatementNode::select_:
            case StatementNode::notselect_:
                rv |= cloneTempExpr(&block->select, found, replace);
                break;
            case StatementNode::switch_:
                rv |= cloneTempExpr(&block->select, found, replace);
                rv |= cloneTempStmt(&block->lower, found, replace);
                break;
            case StatementNode::block_:
                rv |= cloneTempStmt(&block->lower, found, replace);
                rv |= cloneTempStmt(&block->blockTail, found, replace);
                break;
            case StatementNode::passthrough_:
                break;
            case StatementNode::datapassthrough_:
                break;
            case StatementNode::nop_:
                break;
            case StatementNode::line_:
            case StatementNode::varstart_:
            case StatementNode::dbgblock_:
                break;
            default:
                diag("Invalid block type in cloneTempStmt");
                break;
        }
    }
    return rv;
}
#define CLONED_SYMS 20
static bool cloneTempExpr(EXPRESSION** expr, SYMBOL** found, SYMBOL** replace)
{
    bool rv = false;
    if (!*expr)
        return rv;
    if (found)
    {
        EXPRESSION* newval = Allocate<EXPRESSION>();
        *newval = **expr;
        *expr = newval;
    }
    if ((*expr)->type == ExpressionNode::thisref_)
    {
        rv = cloneTempExpr(&(*expr)->left, found, replace);
    }
    else if ((*expr)->type == ExpressionNode::stmt_)
    {
        rv = cloneTempStmt(&(*expr)->v.stmt, found, replace);
    }
    else if ((*expr)->type == ExpressionNode::func_)
    {
        if ((*expr)->v.func->sp->sb->isConstructor || (*expr)->v.func->sp->sb->isDestructor)
        {
            INITLIST** args;
            if (found)
            {
                FUNCTIONCALL* newval = Allocate<FUNCTIONCALL>();
                *newval = *(*expr)->v.func;
                (*expr)->v.func = newval;
            }
            if ((*expr)->v.func->arguments)
            {
                if (found)
                {
                    std::list<INITLIST*>* newList = initListListFactory.CreateList();
                    for (auto arg : *(*expr)->v.func->arguments)
                    {
                        INITLIST* newval = Allocate<INITLIST>();
                        *newval = *arg;
                        rv |= cloneTempExpr(&newval->exp, found, replace);
                        newList->push_back(newval);
                    }
                    (*expr)->v.func->arguments = newList;
                }
                else
                {
                    for (auto arg : *(*expr)->v.func->arguments)
                    {
                        rv |= cloneTempExpr(&arg->exp, found, replace);
                    }
                }
            }
            rv |= cloneTempExpr(&(*expr)->v.func->thisptr, found, replace);
        }
    }
    else
    {
        rv |= cloneTempExpr(&(*expr)->left, found, replace);
        rv |= cloneTempExpr(&(*expr)->right, found, replace);
        if ((*expr)->type == ExpressionNode::auto_)
        {
            SYMBOL* sym = (*expr)->v.sp;
            if (sym->sb->anonymous && !sym->sb->stackblock)
            {
                if (found)
                {
                    int i = 0;
                    EXPRESSION* newexp = Allocate<EXPRESSION>();
                    *newexp = **expr;
                    *expr = newexp;
                    for (i = 0; i < CLONED_SYMS && found[i]; i++)
                    {
                        if (found[i] == sym)
                            break;
                    }
                    if (i >= CLONED_SYMS)
                    {
                        diag("cloneTempExpr - too many syms");
                    }
                    else
                    {
                        if (!found[i])
                        {
                            found[i] = sym;
                            replace[i] = CopySymbol(sym);
                            if (theCurrentFunc)
                                replace[i]->sb->value.i = theCurrentFunc->sb->value.i;
                            replace[i]->name = AnonymousName();
                        }
                        (*expr)->v.sp = replace[i];
                    }
                }
                rv = true;
            }
        }
    }
    return rv;
}
TYPE* InitializerListType(TYPE* arg)
{
    SYMBOL* sym = namespacesearch("std", globalNameSpace, false, false);
    if (sym && sym->sb->storage_class == StorageClass::namespace_)
    {
        sym = namespacesearch("initializer_list", sym->sb->nameSpaceValues, true, false);
        if (sym)
        {
            std::list<TEMPLATEPARAMPAIR>* tplp = templateParamPairListFactory.CreateList();
            auto tpl = Allocate<TEMPLATEPARAM>();
            tpl->type = Keyword::typename_;
            tpl->byClass.dflt = arg;
            tplp->push_back(TEMPLATEPARAMPAIR{ nullptr, tpl });
            auto sym1 = GetClassTemplate(sym, tplp, true);
            if (sym1)
            {
                sym1 = TemplateClassInstantiate(sym1, tplp, false, StorageClass::auto_);
                if (sym1)
                    sym = sym1;
            }
        }
    }
    TYPE* rtp;
    if (sym)
    {
        rtp = sym->tp;
    }
    else
    {
        rtp = MakeType(BasicType::struct_);
        rtp->sp = makeID(StorageClass::type_, rtp, nullptr, "initializer_list");
        rtp->sp->sb->initializer_list = true;
    }
    return rtp;
}
EXPRESSION* getFunc(EXPRESSION* exp)
{
    EXPRESSION* rv = nullptr;
    while (exp->type == ExpressionNode::void_ && exp->right)
    {
        rv = getFunc(exp->left);
        if (rv)
            return rv;
        exp = exp->right;
    }
    if (exp->type == ExpressionNode::thisref_)
        exp = exp->left;
    if (exp->type == ExpressionNode::add_)
    {
        rv = getFunc(exp->left);
        if (!rv)
            rv = getFunc(exp->right);
    }
    else if (exp->type == ExpressionNode::func_)
    {
        return exp;
    }
    return rv;
}
void CreateInitializerList(SYMBOL* func, TYPE* initializerListTemplate, TYPE* initializerListType, std::list<INITLIST*>** lptr, bool operands,
                           bool asref)
{
 
    std::list<INITLIST*>* initial = *lptr;
    EXPRESSION *rv = nullptr, **pos = &rv;
    int count = initial->size(), i;
    EXPRESSION *data, *initList;
    SYMBOL *begin, *size;
    EXPRESSION* dest;
    begin = search(basetype(initializerListTemplate)->syms, "__begin_");
    size = search(basetype(initializerListTemplate)->syms, "__size_");
    if (!begin || !size)
        Utils::Fatal("Invalid definition of initializer-list");
    if (!initial->front()->nested && comparetypes(initial->front()->tp, initializerListTemplate, true))
    {
        auto exp = initial->front()->exp;
        initial->clear();
        initial->push_back(Allocate<INITLIST>());
        if (asref)
        {
            initial->front()->tp = MakeType(BasicType::lref_, initializerListTemplate);
            initial->front()->exp = exp;
        }
        else
        {
            initial->front()->tp = initializerListTemplate;
            initial->front()->exp = exprNode(ExpressionNode::stackblock_, exp, nullptr);
            initial->front()->exp->size = initializerListTemplate;
        }
    }
    else
    {
        auto itl = initial->begin();
        auto itle = initial->end();
        TYPE* tp = MakeType(BasicType::pointer_, initializerListType);
        tp->array = true;
        tp->size = count * (initializerListType->size);
        tp->esize = intNode(ExpressionNode::c_i_, count);
        data = anonymousVar(StorageClass::auto_, tp);
        if ((*itl)->nested)
        {
            bool found = false;
            for (auto l : *(*itl)->nested)
                if (!IsConstantExpression(l->exp, false, false))
                {
                    found = true;
                    break;
                }
            if (!found)
                data->v.sp->sb->constexpression = true;
        }
        if (isstructured(initializerListType))
        {
            EXPRESSION* exp = data;
            EXPRESSION* elms = intNode(ExpressionNode::c_i_, count);
            callDestructor(basetype(initializerListType)->sp, nullptr, &exp, elms, true, false, false, true);
            initInsert(&data->v.sp->sb->dest, tp, exp, 0, false);
        }
        std::deque<EXPRESSION*> listOfScalars;
        for (i = 0; i < count; i++, ++itl)
        {
            EXPRESSION* node;
            dest = exprNode(ExpressionNode::add_, data, intNode(ExpressionNode::c_i_, i * (initializerListType->size)));
            if (isstructured(initializerListType))
            {
                initializerListType = basetype(initializerListType);
                if (initializerListType->sp->sb->trivialCons)
                {
                    node = nullptr;
                    auto list = &node;
                    auto ita = (*itl)->nested ? (*itl)->nested->begin() : itl;
                    auto itae = (*itl)->nested ? (*itl)->nested->end() : initial->end();

                    SymbolTable<SYMBOL>::iterator it;
                    for (it = initializerListType->syms->begin(); ita != itae && it != initializerListType->syms->end(); ++it, ++ita)
                    {
                        auto sym = *it;
                        if (ismemberdata(sym))
                        {
                            auto pos = exprNode(ExpressionNode::structadd_, dest, intNode(ExpressionNode::c_i_, sym->sb->offset));
                            deref(sym->tp, &pos);
                            auto node1 = exprNode(ExpressionNode::assign_, pos, (*ita)->exp);
                            if (node)
                            {
                                *list = exprNode(ExpressionNode::void_, *list, node1);
                                list = &(*list)->right;
                            }
                            else
                            {
                                node = node1;
                            }
                        }
                    }
                    for (; it != initializerListType->syms->end(); ++it)
                    {
                        auto sym = *it;
                        if (ismemberdata(sym))
                        {
                            auto pos = exprNode(ExpressionNode::structadd_, dest, intNode(ExpressionNode::c_i_, sym->sb->offset));
                            deref(sym->tp, &pos);
                            auto node1 = exprNode(ExpressionNode::assign_, pos, intNode(ExpressionNode::c_i_, 0));
                            if (node)
                            {
                                *list = exprNode(ExpressionNode::void_, *list, node1);
                                list = &(*list)->right;
                            }
                            else
                            {
                                node = node1;
                            }
                        }
                    }
                }
                else if (*itl && !(*itl)->initializer_list && (*itl)->nested)
                {
                    auto ita = (*itl)->nested->begin();
                    auto itae = (*itl)->nested->end();
                    node = nullptr;
                    auto list = &node;
                    int count1 = 0;
                    for (; ita != itae; ++ita)
                    {
                        TYPE* ctype = initializerListType;
                        auto pos = copy_expression(exprNode(ExpressionNode::add_, dest, intNode(ExpressionNode::c_i_, count1++ * initializerListType->size)));
                        FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                        params->ascall = true;
                        params->thisptr = pos;
                        INITLIST* next = Allocate<INITLIST>();
                        *next = **ita;
                        params->arguments = initListListFactory.CreateList();
                        params->arguments->push_back(next);
                        callConstructor(&ctype, &pos, params, false, nullptr, true, false, false, false, _F_INITLIST, false, true);
                        if (node)
                        {
                            *list = exprNode(ExpressionNode::void_, *list, pos);
                            list = &(*list)->right;
                        }
                        else
                        {
                            node = pos;
                        }
                    }
                    tp->size += (count1 - 1) * (initializerListType->size);
                    tp->esize->v.i += count1 - 1;
                }
                else if (!(*itl)->nested && isstructured(initializerListType) && (comparetypes(initializerListType, (*itl)->tp, true) || sameTemplate(initializerListType, (*itl)->tp)))
                {
                    node = (*itl)->exp;
                    int offs;                    
                    auto exp = getFunc((*itl)->exp);
                    if (exp)
                    {
                        if (exp->v.func->sp->sb->isConstructor)
                        {
                            if (exp->v.func->thisptr)
                            {
                                int offs = 0;
                                auto exp1 = relptr(exp->v.func->thisptr, offs);
                                if (exp1)
                                    exp1->v.sp->sb->allocate = false;
                                exp->v.func->thisptr = dest;
                            }
                        }
                        else if (isref(basetype(exp->v.func->sp->tp)->btp))
                        {
                            TYPE* ctype = initializerListType;
                            EXPRESSION* cdest = dest;
                            FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                            params->arguments = initListListFactory.CreateList();
                            params->arguments->push_back(*itl);

                            callConstructor(&ctype, &cdest, params, false, nullptr, true, false, false, false, _F_INITLIST, false,
                                            true);
                            node = cdest;

                        }
                    }
                    else
                    {
                        TYPE* ctype = initializerListType;
                        EXPRESSION* cdest = dest;
                        FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                        params->arguments = initListListFactory.CreateList();
                        params->arguments->push_back(*itl);

                        callConstructor(&ctype, &cdest, params, false, nullptr, true, false, false, false, _F_INITLIST, false,
                                        true);
                        node = cdest;
                    }
                }
                else
                {
                    TYPE* ctype = initializerListType;
                    EXPRESSION* cdest = dest;
                    FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                    if ((*itl)->nested)
                    {
                        params->arguments = (*itl)->nested;
                    }
                    else
                    {
                        params->arguments = initListListFactory.CreateList();
                        params->arguments->push_back(*itl);
                    }

                    callConstructor(&ctype, &cdest, params, false, nullptr, true, false, false, false, _F_INITLIST, false, true);
                    node = cdest;
                }
            }
            else
            {
                auto ita = (*itl)->nested ? (*itl)->nested->begin() : itl;
                auto itae = (*itl)->nested ? (*itl)->nested->end() : initial->end();
                if ((*itl)->nested)
                {
                    node = nullptr;
                    auto list = &node;
                    int count1 = 0;
                    for ( ; ita != itae ; ++ita)
                    {
                        auto pos = exprNode(ExpressionNode::structadd_, dest, intNode(ExpressionNode::c_i_, count1++ * initializerListType->size));
                        deref(initializerListType, &pos);
                        auto node1 = exprNode(ExpressionNode::assign_, pos, (*ita)->exp);
                        listOfScalars.push_back((*ita)->exp);
                        if (node)
                        {
                            *list = exprNode(ExpressionNode::void_, *list, node1);
                            list = &(*list)->right;
                        }
                        else
                        {
                            node = node1;
                        }
                    }
                    tp->size += (count1 - 1) * (initializerListType->size);
                    tp->esize->v.i += count1 - 1;
                }
                else
                {
                    deref(initializerListType, &dest);
                    node = exprNode(ExpressionNode::assign_, dest, (*ita)->exp);
                }
            }
            if (rv)
            {
                *pos = exprNode(ExpressionNode::void_, *pos, node);
                pos = &(*pos)->right;
            }
            else
            {
                rv = node;
            }
        }
        initList = anonymousVar(StorageClass::auto_, initializerListTemplate);
        initList->v.sp->sb->constexpression = true;
        if (func && func->sb->constexpression && !listOfScalars.empty())
        {
            if (!initList->v.sp->sb->init)
                initList->v.sp->sb->init = initListFactory.CreateList();
            for (auto t : listOfScalars)
            {
                auto init = Allocate<INITIALIZER>();
                init->exp = t;
                initList->v.sp->sb->init->push_back(init);
            }
        }
        dest = exprNode(ExpressionNode::structadd_, initList, intNode(ExpressionNode::c_i_, begin->sb->offset));
        deref(&stdpointer, &dest);
        dest = exprNode(ExpressionNode::assign_, dest, data);
        if (rv)
        {
            *pos = exprNode(ExpressionNode::void_, *pos, dest);
            pos = &(*pos)->right;
        }
        else
        {
            rv = dest;
        }
        dest = exprNode(ExpressionNode::structadd_, initList, intNode(ExpressionNode::c_i_, size->sb->offset));
        deref(&stdpointer, &dest);
        dest = exprNode(ExpressionNode::assign_, dest, intNode(ExpressionNode::c_i_, tp->size / initializerListType->size));
        if (rv)
        {
            *pos = exprNode(ExpressionNode::void_, *pos, dest);
            pos = &(*pos)->right;
        }
        else
        {
            rv = dest;
        }
        initial = (*lptr) = initListListFactory.CreateList();
        initial->push_back(Allocate<INITLIST>());
        if (asref)
        {
            initial->front()->tp = MakeType(BasicType::lref_, initializerListTemplate);
            initial->front()->exp = exprNode(ExpressionNode::void_, rv, initList);
        }
        else
        {
            initial->front()->tp = initializerListTemplate;
            initial->front()->exp = exprNode(ExpressionNode::stackblock_, exprNode(ExpressionNode::void_, rv, initList), nullptr);
            initial->front()->exp->size = initializerListTemplate;
        }
    }
}
void AdjustParams(SYMBOL* func, SymbolTable<SYMBOL>::iterator it, SymbolTable<SYMBOL>::iterator itend, std::list<INITLIST*>** lptr, bool operands, bool implicit)
{
    std::list<INITLIST*>::iterator itl, itle;
    if (*lptr)
    {
        itl = (*lptr)->begin();
        itle = (*lptr)->end();
    }
    auto old = argFriend;
    argFriend = func;
    (void)operands;
    if (func->sb->storage_class == StorageClass::overloads_)
        return;	
    if (it != itend && (*it)->sb->thisPtr)
        ++it;
    while (it != itend  && (itl != itle || (*it)->sb->init != nullptr ||
                  ((*it)->sb->deferredCompile != nullptr && (!templateNestingCount || instantiatingTemplate))))
    {
        SYMBOL* sym = *it;
        INITLIST* p;

        if (itl == itle)
        {
            if (!*lptr)
            {
                *lptr = initListListFactory.CreateList();
                itl = (*lptr)->begin();
                itle = (*lptr)->end();
            }
            deferredInitializeDefaultArg(sym, func);
            EXPRESSION* q;
            if (sym->sb->init && sym->sb->init->front()->exp)
                q = sym->sb->init->front()->exp;
            else
                q = intNode(ExpressionNode::c_i_, sym->tp->size);
            optimize_for_constants(&q);
            auto il = Allocate<INITLIST>();
            il->exp = q;
            il->tp = basetype(sym->tp);
            (*lptr)->push_back(il);
            itle = (*lptr)->end();
            itl = itle;
            --itl;
            if (cloneTempExpr(&il->exp, nullptr, nullptr))
            {
                int i;
                SYMBOL *ths[CLONED_SYMS], *newval[CLONED_SYMS];
                memset(ths, 0, sizeof(ths));
                memset(newval, 0, sizeof(newval));
                cloneTempExpr(&il->exp, &ths[0], &newval[0]);
            }
            if (isstructured(sym->tp))
            {
                il->exp = exprNode(ExpressionNode::stackblock_, il->exp, nullptr);
                il->exp->size = sym->tp;
                ++it;
                ++itl;
                continue;
            }
        }
        p = *itl;
        if (p && p->exp && (p->exp->type == ExpressionNode::pc_ || p->exp->type == ExpressionNode::func_))
        {
            if (Optimizer::architecture == ARCHITECTURE_MSIL)
            {
                ValidateMSILFuncPtr(func->tp, p->tp, &p->exp);
            }
            thunkForImportTable(&p->exp);
        }
        if (Optimizer::cparams.prm_cplusplus)
        {
            auto exp2 = p->exp;
            if (exp2 && exp2->type == ExpressionNode::thisref_)
                exp2 = exp2->left;

            if (!isstructured(sym->tp) && p->tp)
            {
                if (isstructured(p->tp))
                {
                    TYPE* btp = basetype(func->tp)->btp;
                    if (btp && (!isref(btp) || !isstructured(basetype(btp)->btp)))
                    {
                        SYMBOL* sym = nullptr;
                        auto exp2 = p->exp;
                        if (exp2 && exp2->type == ExpressionNode::thisref_)
                            exp2 = exp2->left;
                        if (exp2 && exp2->type == ExpressionNode::func_)
                            sym = exp2->v.func->returnSP;
                        if (sym && !sym->sb->destructed && sym->sb->dest && sym->sb->dest->front()->exp && !basetype(sym->tp)->sp->sb->trivialCons )
                        {
                            exp2->v.func->returnSP->sb->destructed = true;
                            if (!p->destructors)
                                p->destructors = exprListFactory.CreateList();
                            p->destructors->push_front(exp2->v.func->returnSP->sb->dest->front()->exp);
                        }
                    }
                }
                else
                {
                    GetLogicalDestructors(&p->destructors, p->exp);
                }
            }
            bool done = false;
            if (!done && !p->tp && isstructured(sym->tp) && basetype(sym->tp)->sp->sb->initializer_list)
            {
                // initlist
                std::list<INITLIST*>::iterator itpinit, itpinite = itpinit;
                if (p->nested)
                {
                    itpinit = p->nested->begin();
                    itpinite = p->nested->end();
                }
                if (isstructured(sym->tp) || (isref(sym->tp) && isstructured(basetype(sym->tp)->btp)))
                {
                    TYPE* stype = sym->tp;
                    SYMBOL* sp;
                    EXPRESSION* thisptr;
                    if (isref(stype))
                        stype = basetype(stype)->btp;
                    thisptr = anonymousVar(theCurrentFunc || !isref(sym->tp) ? StorageClass::auto_ : StorageClass::localstatic_, stype);
                    sp = thisptr->v.sp;
                    if (!theCurrentFunc)
                    {
                        sp->sb->label = Optimizer::nextLabel++;
                        insertInitSym(sp);
                    }
                    if (basetype(stype)->sp->sb->trivialCons)
                    {
                        std::list<INITIALIZER*> init1;
                        std::list<INITIALIZER*>* init = { &init1 };
                        for (auto shr : *basetype(stype)->syms)
                        {
                            if (itpinit == itpinite)
                                break;
                            if (ismemberdata(shr))
                            {
                                initInsert(&init, (*itpinit)->tp, (*itpinit)->exp, shr->sb->offset, false);
                                ++itpinit;
                            }
                        }
                        p->exp = convertInitToExpression(stype, nullptr, nullptr, theCurrentFunc, init, thisptr, false);
                        if (!isref(sym->tp))
                            sp->sb->stackblock = true;
                        done = true;
                    }
                    else
                    {
                        FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                        params->ascall = true;
                        TYPE* ctype = sp->tp;
                        EXPRESSION* dexp = thisptr;
                        params->thisptr = thisptr;
                        auto itx = itpinit;
                        if (itpinit != itpinite)
                            ++itx;
                        params->arguments = initListListFactory.CreateList();
                        if ((itpinit != itpinite && itx != itpinite) || (itpinit == itpinite && !p->tp && !p->exp))  // empty initializer list)
                        {
                            TYPE* tp;
                            if (itpinit == itpinite)
                            {
                                params->arguments->push_back(p);
                                tp = &stdint;
                            }
                            else
                            {
                                params->arguments->insert(params->arguments->begin(), itl, itle);
                                tp = (*itpinit)->tp;
                            }
                            p->tp = InitializerListType(tp);
                            p->exp = intNode(ExpressionNode::c_i_, 0);
                            CreateInitializerList(nullptr, p->tp, tp, &params->arguments, true, isref(sym->tp));
                            **itl = *params->arguments->front();
                            (*itl)->tp = sym->tp;
                            if (itpinit == itpinite)
                            {
                                auto itl1 = itl;
                                ++itl1;
                                params->arguments->insert(params->arguments->end(), itl1, itle);
                            }
                        }
                        else
                        {
                            params->arguments->push_back(p);
                            p->exp = thisptr;
                            callConstructor(&ctype, &p->exp, params, false, nullptr, true, false, true, false, true, false, true);
                        }
                
                        if (!isref(sym->tp))
                        {
                            sp->sb->stackblock = true;
                        }
                        else
                        {
                            callDestructor(stype->sp, nullptr, &dexp, nullptr, true, false, false, true);
                            if (dexp)
                            {
                                if (!p->destructors)
                                    p->destructors = exprListFactory.CreateList();
                                p->destructors->push_front(dexp);
                            }
                        }
                        done = true;
                    }
                    p->tp = sym->tp;
                }
                else if (ispointer(sym->tp))
                {
                    EXPRESSION* thisptr = anonymousVar(theCurrentFunc ? StorageClass::auto_ : StorageClass::localstatic_, sym->tp);
                    SYMBOL* sp = thisptr->v.sp;
                    int n = 0;
                    TYPE* btp = sym->tp;
                    while (isarray(btp))
                        btp = basetype(btp)->btp;
                    if (!theCurrentFunc)
                    {
                        sp->sb->label = Optimizer::nextLabel++;
                        insertInitSym(sp);
                    }
                    if (!isarray(sym->tp))
                    {
                        auto itx = itpinit;
                        while (itx != itpinite)

                        {
                            n++;
                            ++itx;
                        }
                        sym->tp = CopyType(sym->tp);
                        sym->tp->array = true;
                        sym->tp->esize = intNode(ExpressionNode::c_i_, n);
                        UpdateRootTypes(sym->tp);
                        sym->tp->size = btp->size * n;
                    }
                    n = 0;
                    std::list<INITIALIZER*> init1;
                    std::list<INITIALIZER*>* init = &init1;
                    for ( ; itpinit != itpinite ; ++itpinit)
                    {
                        initInsert(&init, (*itpinit)->tp, (*itpinit)->exp, n, false);
                        n += btp->size;
                    }
                    p->exp = convertInitToExpression(sym->tp, nullptr, nullptr, theCurrentFunc, init, thisptr, false);
                    p->tp = sym->tp;
                    done = true;
                }
                else
                {
                    // defer to below processing
                    if (itpinit != itpinite)
                    {
                        p->exp = (*itpinit)->exp;
                        p->tp = (*itpinit)->tp;
                    }
                    else
                    {
                        p->exp = intNode(ExpressionNode::c_i_, 0);
                        p->tp = &stdint;
                    }
                }
            }
            if (!done && (p->exp || p->nested))
            {
                if (sym->tp->type == BasicType::ellipse_)
                {
                    if (!isstructured(p->tp) && (p->tp->lref || p->tp->rref))
                        if (isfunction(p->tp))
                            p->exp = exprNode(ExpressionNode::l_ref_, p->exp, nullptr);
                }
                else if (isstructured(sym->tp))
                {
                    bool sameType = false;
                    EXPRESSION* temp = p->exp;
                    TYPE* tpx = p->tp;
                    sym->tp = PerformDeferredInitialization(sym->tp, nullptr);
                    if (!tpx)
                        tpx = sym->tp;
                    if (isref(tpx))
                        tpx = basetype(tpx)->btp;
                    // result of a nested constructor
                    if (temp && temp->type == ExpressionNode::thisref_)
                    {
                        temp = p->exp->left;
                    }
                    if (p->nested)
                    {
                        FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                        params->ascall = true;
                        if (p->initializer_list)
                        {
                            params->arguments = p->nested;
                        }
                        else
                        {
                            params->arguments = initListListFactory.CreateList();
                            params->arguments->push_back(p);
                        }
                        TYPE* ctype = basetype(sym->tp);
                        EXPRESSION* consexp = anonymousVar(StorageClass::auto_, ctype);  // StorageClass::parameter_ to push it...
                        SYMBOL* esp = consexp->v.sp;
                        p->exp = consexp;
                        esp->sb->stackblock = true;
                        callConstructor(&ctype, &p->exp, params, false, nullptr, true, false, false, false, 0, false, true);
                        if (p->exp->type == ExpressionNode::thisref_)
                        {
                            TYPE* tpx = basetype(p->exp->left->v.func->sp->tp);
                            if (isfunction(tpx) && tpx->sp && tpx->sp->sb->castoperator)
                            {
                                p->tp = tpx->btp;
                            }
                        }
                    }
                    // use constructor or conversion function and push on stack ( no destructor)
                    else if (temp->type == ExpressionNode::func_ && basetype(temp->v.func->sp->tp)->btp &&
                             !isref(basetype(temp->v.func->sp->tp)->btp) &&
                             ((sameType = comparetypes(sym->tp, tpx, true)) ||
                              classRefCount(basetype(sym->tp)->sp, basetype(tpx)->sp) == 1))
                    {
                        SYMBOL* esp;
                        EXPRESSION* consexp;
                        // copy constructor...
                        TYPE* ctype = sym->tp;
                        EXPRESSION* paramexp;
                        consexp = anonymousVar(StorageClass::auto_, sym->tp);  // StorageClass::parameter_ to push it...
                        esp = consexp->v.sp;
                        esp->sb->stackblock = true;
                        consexp = varNode(ExpressionNode::auto_, esp);
                        paramexp = p->exp;
                        paramexp = DerivedToBase(sym->tp, tpx, paramexp, _F_VALIDPOINTER);
                        auto exp = consexp;
                        callConstructorParam(&ctype, &exp, sym->tp, paramexp, true, true, implicit, false, true);
                        if (exp->type == ExpressionNode::auto_)  // recursive call to constructor A<U>(A<U>)
                        {
                            if (temp->v.func->returnEXP)
                            {
                                temp->v.func->returnEXP = consexp;
                            }
                            else
                            {
                                if (p->exp->type == ExpressionNode::thisref_ && p->exp->left->v.func->thisptr)
                                    p->exp->left->v.func->thisptr = consexp;
                                p->exp = paramexp;
                            }
                        }
                        else
                        {
                            p->exp = exp;
                        }
                        p->exp = exprNode(ExpressionNode::void_, p->exp, consexp);
                    }
                    else if (basetype(sym->tp)->sp->sb->trivialCons)
                    {
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = p->tp;
                    }
                    else
                    {
                        TYPE* ctype = basetype(sym->tp = PerformDeferredInitialization(sym->tp, nullptr));
                        EXPRESSION* consexp = anonymousVar(StorageClass::auto_, ctype);  // StorageClass::parameter_ to push it...
                        SYMBOL* esp = consexp->v.sp;
                        EXPRESSION* paramexp = p->exp;
                        esp->sb->stackblock = true;
                        esp->sb->constexpression = true;
                        callConstructorParam(&ctype, &consexp, p->tp, paramexp, true, true, implicit, false, true);
                        if (consexp->type == ExpressionNode::auto_)  // recursive call to constructor A<U>(A<U>)
                        {
                            p->exp = paramexp;
                        }
                        else
                        {
                            if (consexp->type == ExpressionNode::thisref_)
                                esp->sb->constexpression = false;
                            p->exp = consexp;
                        }
                    }
                    p->tp = sym->tp;
                }
                else if (isref(sym->tp))
                {
                    bool nested = false;
                    TYPE* tpx1 = p->tp;
                    if (!tpx1)
                    {
                        tpx1 = sym->tp;
                    }
                    if (isstructured(basetype(sym->tp)->btp))
                    {
                        TYPE* tpx = tpx1;
                        if (isref(tpx))
                            tpx = basetype(tpx)->btp;
                        if (tpx->type == BasicType::templateparam_)
                        {
                            tpx = tpx->templateParam->second->byClass.val;
                            if (!tpx)
                                tpx = basetype(sym->tp)->btp;
                        }
                        if (p->nested)
                        {
                            nested = true;
                            FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                            params->ascall = true;
                            if (p->initializer_list)
                            {
                                params->arguments = p->nested;
                            }
                            else
                            {
                                params->arguments = initListListFactory.CreateList();
                                params->arguments->push_back(p);
                            }
                            TYPE* ctype = basetype(sym->tp)->btp;
                            EXPRESSION* consexp = anonymousVar(StorageClass::auto_, basetype(sym->tp)->btp);  // StorageClass::parameter_ to push it...
                            SYMBOL* esp = consexp->v.sp;
                            p->exp = consexp;
                            callConstructor(&ctype, &p->exp, params, false, nullptr, true, false, false, false, 0, false, true);
                            if (p->exp->type == ExpressionNode::thisref_)
                            {
                                TYPE* tpx = basetype(p->exp->left->v.func->sp->tp);
                                if (isfunction(tpx) && tpx->sp && tpx->sp->sb->castoperator)
                                {
                                    p->tp = tpx->btp;
                                }
                            }
                            EXPRESSION* dexp = consexp;
                            callDestructor(basetype(esp->tp)->sp, nullptr, &dexp, nullptr, true, false, false, true);
                            initInsert(&esp->sb->dest, basetype(sym->tp)->btp, dexp, 0, true);
                        }
                        else if ((!isconst(basetype(sym->tp)->btp) && !isconst(sym->tp) &&
                                 (sym->tp->type != BasicType::rref_ &&
                                   (!func->sb->templateLevel &&
                                   (!func->sb->parentClass || !func->sb->parentClass->sb->templateLevel) /*forward*/)) &&
                                  isconst(tpx)) ||
                                 (!comparetypes(sym->tp, tpx, true) && !sameTemplate(sym->tp, tpx) &&
                                  !classRefCount(basetype(basetype(sym->tp)->btp)->sp, basetype(tpx)->sp)))
                        {
                            // make temp via constructor or conversion function
                            EXPRESSION* consexp = anonymousVar(StorageClass::auto_, basetype(sym->tp)->btp);  // StorageClass::parameter_ to push it...
                            SYMBOL* esp = consexp->v.sp;
                            TYPE* ctype = basetype(sym->tp)->btp;
                            EXPRESSION* paramexp = p->exp;
                            p->exp = consexp;
                            callConstructorParam(&ctype, &p->exp, basetype(p->tp), paramexp, true, true, false, false, true);
                            if (p->exp->type == ExpressionNode::thisref_)
                            {
                                TYPE* tpx = basetype(p->exp->left->v.func->sp->tp);
                                if (isfunction(tpx) && tpx->sp && tpx->sp->sb->castoperator)
                                {
                                    p->tp = tpx->btp;
                                }
                            }
                            EXPRESSION* dexp = consexp;
                            callDestructor(basetype(esp->tp)->sp, nullptr, &dexp, nullptr, true, false, false, true);
                            initInsert(&esp->sb->dest, basetype(sym->tp)->btp, dexp, 0, true);
                        }
                        else
                        {
                            if (!comparetypes(sym->tp, tpx, true))
                                p->exp = DerivedToBase(sym->tp, tpx, p->exp, 0);
                        }
                        ConstExprPromote(p->exp, isconst(basetype(sym->tp)->btp));
                    }
                    else if (basetype(basetype(sym->tp)->btp)->type == BasicType::memberptr_)
                    {
                        TYPE* tp2 = basetype(sym->tp)->btp;
                        if (p->exp->type == ExpressionNode::memberptr_)
                        {
                            int lbl = dumpMemberPtr(p->exp->v.sp, tp2, true);
                            p->exp = intNode(ExpressionNode::labcon_, lbl);
                        }
                        else if (isconstzero(p->tp, p->exp) || p->exp->type == ExpressionNode::nullptr_)
                        {
                            EXPRESSION* dest = createTemporary(tp2, nullptr);
                            p->exp = exprNode(ExpressionNode::blockclear_, dest, nullptr);
                            p->exp->size = tp2;
                            p->exp = exprNode(ExpressionNode::void_, p->exp, dest);
                        }
                        else if (p->exp->type == ExpressionNode::func_ && p->exp->v.func->returnSP)
                        {
                            int lbl = dumpMemberPtr(p->exp->v.sp, tp2, true);
                            p->exp = intNode(ExpressionNode::labcon_, lbl);
                        }
                        else if (p->exp->type == ExpressionNode::pc_)
                        {
                            int lbl = dumpMemberPtr(p->exp->v.sp, tp2, true);
                            p->exp = intNode(ExpressionNode::labcon_, lbl);
                        }
                        p->tp = sym->tp;
                    }
                    else if (comparetypes(sym->tp, p->tp ? p->tp : p->nested->front()->tp, true))
                    {
                        INITLIST* p1;
                        if (p->tp)
                                p1 = p;
                        else
                            p1 = p->nested->front();
                        if (isarithmeticconst(p1->exp) ||
                            (basetype(sym->tp)->type != BasicType::rref_ && !isconst(basetype(sym->tp)->btp) && isconst(p1->tp)))
                        {
                            // make numeric temp and perform cast
                            p->exp = createTemporary(sym->tp, p1->exp);
                        }
                        else
                        {
                            // pass address
                            EXPRESSION* exp = p1->exp;
                            while (castvalue(exp) || exp->type == ExpressionNode::not__lvalue_)
                                exp = exp->left;
                            if (exp->type != ExpressionNode::l_ref_)
                            {
                                if (!isref(sym->tp) || !isfunction(basetype(sym->tp)->btp))
                                {
                                    if (!lvalue(exp))
                                    {
                                        TYPE* tp1 = basetype(sym->tp)->btp;
                                        // make numeric temp and perform cast
                                        if (isarray(tp1))
                                        {
                                            TYPE tp2 = {};
                                            MakeType(tp2, BasicType::lref_, &stdpointer);
                                            exp = createTemporary(&tp2, exp);
                                        }
                                        else if (!isref(sym->tp) || exp->type != ExpressionNode::func_ || (p1->tp->type == BasicType::aggregate_ || (isfunction(exp->v.func->sp->tp) && !isref(basetype(exp->v.func->sp->tp)->btp))))
                                        {
                                            exp = createTemporary(sym->tp, exp);
                                        }
                                    }
                                    else if (exp->type == ExpressionNode::lvalue_)
                                    {
                                        exp = createTemporary(sym->tp, exp);
                                    }
                                    else
                                    {
                                        exp = exp->left;  // take address
                                        if (exp->type == ExpressionNode::auto_)
                                        {
                                            exp->v.sp->sb->addressTaken = true;
                                        }
                                    }
                                }
                                p->exp = exp;
                            }
                            else if (ispointer(p->tp) && isstructured(basetype(p->tp)->btp))
                            {
                                // make numeric temp and perform cast
                                p->exp = createTemporary(sym->tp, exp);
                            }
                            else if (basetype(basetype(sym->tp)->btp)->byRefArray)
                            {
                                 p->exp->left = p->exp->left->left;
                            }
                        }
                    }
                    else if (isstructured(p->tp))
                    {
                        // arithmetic or pointer
                        TYPE* etp = basetype(sym->tp)->btp;
                        if (cppCast(p->tp, &etp, &p->exp))
                            p->tp = etp;
                        if (lvalue(p->exp))
                            p->exp = p->exp->left;
                        else
                            p->exp = createTemporary(sym->tp, p->exp);
                    }
                    else
                    {
                        // make numeric temp and perform cast
                        p->exp = createTemporary(sym->tp, p->exp);
                    }
                    if (!isref(tpx1) && ((isconst(tpx1) && !isconst(basetype(sym->tp)->btp)) ||
                                         (isvolatile(tpx1) && !isvolatile(basetype(sym->tp)->btp))))
                        if (basetype(sym->tp)->type != BasicType::rref_)         // converting const lref to rref is ok...
                            if (!isstructured(basetype(sym->tp)->btp))  // structure constructor is ok
                                if (!isarray(tpx1))
                                    error(ERR_REF_INITIALIZATION_DISCARDS_QUALIFIERS);
                    p->tp = sym->tp;
                }
                else if (isstructured(p->tp))
                {
                    if (sym->tp->type == BasicType::ellipse_)
                    {
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = p->tp;
                    }
                    else
                    {
                        // arithmetic or pointer
                        TYPE* etp = sym->tp;
                        if (cppCast(p->tp, &etp, &p->exp))
                            p->tp = etp;
                    }
                }
                else if (isvoidptr(sym->tp) && p->tp->type == BasicType::aggregate_)
                {
                    LookupSingleAggregate(p->tp, &p->exp);
                }
                else if (ispointer(sym->tp) && ispointer(p->tp))
                {
                    // handle vla to pointer conversion
                    if (p->tp->vla && !sym->tp->vla)
                    {
                        TYPE* tpd1 = Allocate<TYPE>();
                        *tpd1 = *p->tp;
                        tpd1->vla = false;
                        tpd1->array = false;
                        tpd1->size = getSize(BasicType::pointer_);
                        p->tp = tpd1;
                        deref(p->tp, &p->exp);
                    }
                    // handle base class conversion
                    TYPE* tpb = basetype(sym->tp)->btp;
                    TYPE* tpd = basetype(p->tp)->btp;

                    if (Optimizer::cparams.prm_cplusplus && basetype(p->tp)->stringconst && !isconst(basetype(sym->tp)->btp))
                        error(ERR_INVALID_CHARACTER_STRING_CONVERSION);
                    if (!comparetypes(basetype(tpb), basetype(tpd), true))
                    {
                        if (isstructured(tpb) && isstructured(tpd))
                        {
                            p->exp = DerivedToBase(tpb, tpd, p->exp, 0);
                        }
                        p->tp = sym->tp;
                    }
                }
                else if (basetype(sym->tp)->type == BasicType::memberptr_)
                {
                    if (p->exp->type == ExpressionNode::memberptr_)
                    {
                        int lbl = dumpMemberPtr(p->exp->v.sp, sym->tp, true);
                        p->exp = intNode(ExpressionNode::labcon_, lbl);
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = sym->tp;
                    }
                    else if (isconstzero(p->tp, p->exp) || p->exp->type == ExpressionNode::nullptr_)
                    {
                        EXPRESSION* dest = createTemporary(sym->tp, nullptr);
                        p->exp = exprNode(ExpressionNode::blockclear_, dest, nullptr);
                        p->exp->size = sym->tp;
                        p->exp = exprNode(ExpressionNode::void_, p->exp, dest);
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = sym->tp;
                    }
                    else if (p->exp->type == ExpressionNode::func_ && p->exp->v.func->returnSP)
                    {
                        EXPRESSION* dest = anonymousVar(StorageClass::auto_, sym->tp);
                        SYMBOL* esp = dest->v.sp;
                        int lbl = dumpMemberPtr(p->exp->v.sp, sym->tp, true);
                        esp->sb->stackblock = true;
                        p->exp = intNode(ExpressionNode::labcon_, lbl);
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = sym->tp;
                    }
                    else if (p->exp->type == ExpressionNode::pc_)
                    {
                        int lbl = dumpMemberPtr(p->exp->v.sp, sym->tp, true);
                        p->exp = intNode(ExpressionNode::labcon_, lbl);
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = sym->tp;
                    }
                    else
                    {
                        p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                        p->exp->size = sym->tp;
                    }
                    p->tp = sym->tp;
                }
                else if (isarithmetic(sym->tp) && isarithmetic(p->tp))
                    if (basetype(sym->tp)->type != basetype(p->tp)->type)
                    {
                        p->tp = sym->tp;
                        cast(p->tp, &p->exp);
                    }
            }
        }
        else if (Optimizer::architecture == ARCHITECTURE_MSIL)
        {
            if (isref(sym->tp))
            {
                if (comparetypes(sym->tp, p->tp, true))
                {
                    if (isarithmeticconst(p->exp) ||
                        (basetype(sym->tp)->type != BasicType::rref_ && !isconst(basetype(sym->tp)->btp) && isconst(p->tp)))
                    {
                        // make numeric temp and perform cast
                        p->exp = createTemporary(sym->tp, p->exp);
                    }
                    else
                    {
                        // pass address
                        EXPRESSION* exp = p->exp;
                        while (castvalue(exp) || exp->type == ExpressionNode::not__lvalue_)
                            exp = exp->left;
                        if (exp->type != ExpressionNode::l_ref_)
                        {
                            if (!isref(sym->tp) || !isfunction(basetype(sym->tp)->btp))
                            {
                                if (!lvalue(exp))
                                {
                                    // make numeric temp and perform cast
                                    exp = createTemporary(sym->tp, exp);
                                }
                                else
                                {
                                    exp = exp->left;  // take address
                                }
                            }
                            p->exp = exp;
                        }
                        else if (ispointer(p->tp) && isstructured(basetype(p->tp)->btp))
                        {
                            // make numeric temp and perform cast
                            p->exp = createTemporary(sym->tp, exp);
                        }
                    }
                }
                else
                {
                    // make numeric temp and perform cast
                    p->exp = createTemporary(sym->tp, p->exp);
                }
                p->tp = sym->tp;
            }
            else
            {
                if (basetype(sym->tp)->type == BasicType::string_)
                {
                    if ((basetype(p->tp)->type == BasicType::string_) || (p->exp->type == ExpressionNode::labcon_ && p->exp->string))
                    {
                        if (p->exp->type == ExpressionNode::labcon_)
                            p->exp->type = ExpressionNode::c_string_;
                    }
                    else if ((isarray(p->tp) || ispointer(p->tp)) && !basetype(p->tp)->msil &&
                             basetype(basetype(p->tp)->btp)->type == BasicType::char_)
                    {
                        // make a 'string' object and initialize it with the string
                        TYPE* ctype = find_boxed_type(basetype(sym->tp));
                        EXPRESSION *exp1, *exp2;
                        exp1 = exp2 = anonymousVar(StorageClass::auto_, &std__string);
                        callConstructorParam(&ctype, &exp2, p->tp, p->exp, true, true, false, false, true);
                        exp2 = exprNode(ExpressionNode::l_string_, exp2, nullptr);
                        p->exp = exp2;
                        p->tp = &std__string;
                    }
                }
                else if (basetype(sym->tp)->type == BasicType::object_)
                {
                    if (basetype(p->tp)->type != BasicType::object_ && !isstructured(p->tp) && (!isarray(p->tp) || !basetype(p->tp)->msil))
                    {
                        if ((isarray(p->tp) || ispointer(p->tp)) && !basetype(p->tp)->msil &&
                            basetype(basetype(p->tp)->btp)->type == BasicType::char_)
                        {
                            // make a 'string' object and initialize it with the string
                            TYPE* ctype = find_boxed_type(&std__string);
                            EXPRESSION *exp1, *exp2;
                            exp1 = exp2 = anonymousVar(StorageClass::auto_, &std__string);
                            callConstructorParam(&ctype, &exp2, p->tp, p->exp, true, true, false, false, true);
                            exp2 = exprNode(ExpressionNode::l_string_, exp2, nullptr);
                            p->exp = exp2;
                            p->tp = &std__string;
                        }
                        else
                            p->exp = exprNode(ExpressionNode::x_object_, p->exp, nullptr);
                    }
                }
                else if (ismsil(p->tp))
                    ;  // error
                // legacy c language support
                else if (p && p->tp && isstructured(p->tp) && (!basetype(p->tp)->sp->sb->msil || !isconstzero(p->tp, p->exp)))
                {
                    p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                    p->exp->size = p->tp;
                }
            }
        }
        else
        {
            // legacy c language support
            if (p && p->tp)
            {
                if (isstructured(p->tp))
                {
                    p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
                    p->exp->size = p->tp;
                }
                else if (isfloat(sym->tp) || isimaginary(sym->tp) || iscomplex(sym->tp))
                {
                    cast(sym->tp, &p->exp);
                    optimize_for_constants(&p->exp);
                }
            }
        }
        ++it;
        ++itl;
    }
    while (itl != itle)  // take care of elliptical arguments and arguments without a prototype
    {
        INITLIST* p = *itl;
        if (!p->tp || !p->exp)
        {
            p->tp = &stdint;
            p->exp = intNode(ExpressionNode::c_i_, 0);
        }
        if (func->sb->msil && p->exp->type == ExpressionNode::labcon_ && p->exp->string)
        {
            p->exp->type = ExpressionNode::c_string_;
            p->tp = &(std__string);
        }
        if (isstructured(p->tp))
        {
            p->exp = exprNode(ExpressionNode::stackblock_, p->exp, nullptr);
            p->exp->size = p->tp;
            if (Optimizer::cparams.prm_cplusplus)
            {
                auto dexp = p->exp;
                callDestructor(basetype(p->tp)->sp, nullptr, &dexp, nullptr, true, false, false, true);
                if (dexp)
                {
                    if (!p->destructors)
                        p->destructors = exprListFactory.CreateList();
                    p->destructors->push_front(dexp);
                }
            }
        }
        else if (p->tp->type == BasicType::float_)
        {
            cast(&stddouble, &p->exp);
            GetLogicalDestructors(&p->destructors, p->exp);
        }
        else
        {
            GetLogicalDestructors(&p->destructors, p->exp);
        }
        ++itl;
    }
    argFriend = old;
}
static std::list<TEMPLATEPARAMPAIR>* LiftTemplateParams(std::list<TEMPLATEPARAMPAIR>* tplx)
{
    std::list<TEMPLATEPARAMPAIR>* rv = nullptr;
    if (tplx)
    {
        rv = templateParamPairListFactory.CreateList();
        for (auto tpl :*tplx)
        {
            auto second = Allocate<TEMPLATEPARAM>();
            *second = *tpl.second;
            rv->push_back(TEMPLATEPARAMPAIR{ tpl.first, second });
            if (tpl.second->type == Keyword::typename_ && !tpl.second->packed && tpl.second->byClass.dflt &&
                basetype(tpl.second->byClass.dflt)->type == BasicType::templateparam_)
            {
                *rv->back().second = *basetype(tpl.second->byClass.dflt)->templateParam->second;
                rv->back().first = basetype(tpl.second->byClass.dflt)->templateParam->first;
            }
            else if (tpl.second->type == Keyword::int_ && tpl.second->byNonType.dflt && tpl.second->byNonType.dflt->type == ExpressionNode::templateparam_)
            {
                *rv->back().second = *tpl.second->byNonType.dflt->v.sp->tp->templateParam->second;
                rv->back().first = tpl.second->byNonType.dflt->v.sp->tp->templateParam->first;
            }
            if (rv->back().first)
            {
                TEMPLATEPARAMPAIR* rv1 = nullptr;
                for (auto&& s : structSyms)
                {
                    if (s.tmpl)
                    {
                        for (auto&& tpl1 : *s.tmpl)
                        {
                            if (tpl1.first && !strcmp(tpl1.first->name, rv->back().first->name))
                            {
                                rv1 = &tpl1;
                                if (rv1)
                                    break;
                            }
                        }
                    }
                }
                if (rv1)
                {
                    if (rv->back().second->packed)
                    {
                        rv->back().second->byPack.pack = nullptr;
                        if (rv1->second->byPack.pack)
                        {
                            rv->back().second->byPack.pack = templateParamPairListFactory.CreateList();
                            for (auto tpl2 : *rv->front().second->byPack.pack)
                            {
                                auto second = Allocate<TEMPLATEPARAM>();
                                second->type = rv->back().second->type;
                                second->byClass.dflt = tpl2.second->byClass.val;
                                rv->back().second->byPack.pack->push_back(TEMPLATEPARAMPAIR{ nullptr, second });
                            }
                        }
                    }
                    else
                    {
                        rv->back().second->byClass.dflt = rv->front().second->byClass.val;
                    }
                }
            }
        }
    }
    return rv;
}
LEXLIST* expression_arguments(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    TYPE* tp_cpp = *tp;
    EXPRESSION* exp_cpp = *exp;
    FUNCTIONCALL* funcparams;
    EXPRESSION* exp_in = *exp;
    bool operands = false;
    bool hasThisPtr = false;
    TYPE* initializerListType = nullptr;
    TYPE* initializerListTemplate = nullptr;
    bool initializerRef = false;
    bool addedThisPointer = false;
    bool memberPtr = false;
    if (exp_in->type != ExpressionNode::func_ || isfuncptr(*tp) || isstructured(*tp))
    {
        TYPE* tpx = *tp;
        SYMBOL* sym;
        funcparams = Allocate<FUNCTIONCALL>();
        if (exp_in->type == ExpressionNode::templateselector_)
        {
            auto tsl = exp_in->v.templateSelector;
            SYMBOL* ts = (*tsl)[1].sp;
            SYMBOL* sp = ts;
            if ((!sp->sb || !sp->sb->instantiated) && (*tsl)[1].isTemplate)
            {
                std::list<TEMPLATEPARAMPAIR>* current = (*tsl)[1].templateParams;
                sp = GetClassTemplate(ts, current, true);
            }
            if (sp && sp->tp->type == BasicType::templateselector_)
            {
                TYPE* tp = sp->tp;
                tp = SynthesizeType(tp, nullptr, false);
                if (tp && isstructured(tp))
                    sp = basetype(tp)->sp;
            }
            sym = nullptr;
            if (sp)
            {
                sp = basetype(PerformDeferredInitialization(sp->tp, nullptr))->sp;
                auto find = (*tsl).begin();
                ++find;
                ++find;
                for (; find != (*tsl).end() && sp ; )
                {
                    SYMBOL* spo = sp;
                    if (!isstructured(spo->tp))
                        break;

                    sp = search(spo->tp->syms, find->name);
                    if (!sp)
                    {
                        sp = classdata(find->name, spo, nullptr, false, false);
                        if (sp == (SYMBOL*)-1)
                            sp = nullptr;
                    }
                    if (sp && sp->sb->access != AccessLevel::public_)
                    {
                        sp = nullptr;
                        break;
                    }
                    auto it1 = find;
                    ++it1;
                    if (it1 == (*tsl).end() && sp)
                    {
                        funcparams->astemplate = find->isTemplate;
                        funcparams->templateParams = LiftTemplateParams(find->templateParams);
                    }
                    ++find;
                }
                if (find == (*tsl).end())
                    sym = sp;
            }
        }
        else
        {
            if (tpx->type == BasicType::templateparam_)
            {
                if (tpx->templateParam->second->type == Keyword::typename_ && tpx->templateParam->second->byClass.val)
                {
                    tpx = tpx->templateParam->second->byClass.val;
                    if (tpx->sp && tpx->sp->sb->storage_class == StorageClass::typedef_)
                        tpx = tpx->btp;
                }
            }
            if (ispointer(tpx))
                tpx = basetype(tpx)->btp;
            sym = basetype(tpx)->sp;
        }
        if (sym)
        {
            funcparams->sp = sym;
            funcparams->functp = sym->tp;
            funcparams->fcall = *exp;
            *exp = varNode(ExpressionNode::func_, nullptr);
            (*exp)->v.func = funcparams;
        }
        else if (!templateNestingCount)
            error(ERR_CALL_OF_NONFUNCTION);
    }
    else
    {
        if (isfunction(*tp) && (*exp)->type == ExpressionNode::func_ && (*exp)->v.func->resolvedCall)
        {
            funcparams = Allocate<FUNCTIONCALL>();
            funcparams->sp = basetype(*tp)->sp;
            funcparams->functp = basetype(*tp);
            funcparams->fcall = *exp;
            if ((*tp)->lref || (*tp)->rref)
                funcparams->fcall = exprNode(ExpressionNode::l_ref_, *exp, nullptr);
            *exp = varNode(ExpressionNode::func_, nullptr);
            (*exp)->v.func = funcparams;
        }
        else
        {
            SYMBOL* ss = getStructureDeclaration();
            funcparams = exp_in->v.func;
            hasThisPtr = funcparams->thisptr != nullptr;
            if (basetype(*tp)->sp)
                funcparams->sp = basetype(*tp)->sp;
            if (ss)
            {
                funcparams->functp = ss->tp;
            }
        }
    }

    if (/*(!templateNestingCount || instantiatingTemplate) &&*/ funcparams->sp && funcparams->sp->name[0] == '_' &&
        parseBuiltInTypelistFunc(&lex, funcsp, funcparams->sp, tp, exp))
        return lex;

    if (lex)
    {
        lex = getArgs(lex, funcsp, funcparams, Keyword::closepa_, true, flags);
    }
    if (funcparams->astemplate && argumentNesting)
    {
        // if we hit a packed template param here, then this is going to be a candidate
        // for some other function's packed expression
        if (funcparams->templateParams)
            for (auto tl : *funcparams->templateParams)
                if (tl.second->packed && !tl.second->ellipsis)
                    return lex;
    }
    if (*tp)
        getFunctionSP(tp);
    if ((*exp)->type == ExpressionNode::funcret_)
    {
        (*exp)->v.func = funcparams;
        *exp = exprNode(ExpressionNode::funcret_, *exp, nullptr);
        return lex;
    }
    if ((Optimizer::architecture == ARCHITECTURE_MSIL) && funcparams->sp)
    {
        if (funcparams->sp->sb->storage_class == StorageClass::overloads_)
        {
            // note at this pointer the arglist does NOT have the this pointer,
            // it will be added after we select a member function that needs it.
            funcparams->ascall = true;
            SYMBOL* sym =
                GetOverloadedFunction(tp, &funcparams->fcall, funcparams->sp, funcparams, nullptr, true, false, flags);
            if (sym)
            {
                sym->sb->attribs.inheritable.used = true;
                funcparams->sp = sym;
                *tp = sym->tp;
            }
        }
    }
    else if ((Optimizer::cparams.prm_cplusplus || (Optimizer::architecture == ARCHITECTURE_MSIL)) && funcparams->sp)
    {
        SYMBOL* sym = nullptr;
        // add this ptr
        if (!funcparams->thisptr && funcparams->sp->sb->parentClass && !isfuncptr(funcparams->sp->tp))
        {
            funcparams->thisptr = getMemberBase(funcparams->sp, nullptr, funcsp, false);
            TYPE* tp = MakeType(BasicType::pointer_, funcparams->sp->sb->parentClass->tp);
            if (funcsp)
            {
                if (isconst(funcsp->tp))
                {
                    tp->btp = MakeType(BasicType::const_, tp->btp);
                }
                if (isvolatile(funcsp->tp))
                {
                    tp->btp = MakeType(BasicType::volatile_, tp->btp);
                }
            }
            funcparams->thistp = tp;
            UpdateRootTypes(tp);
            addedThisPointer = true;
        }
        // we may get here with the overload resolution already done, e.g.
        // for operator or cast function calls...
        // also if in a trailing return we want to defer the lookup until later if there are going to be multiple choices...
        if (funcparams->sp->sb->storage_class == StorageClass::overloads_ &&
            (!parsingTrailingReturnOrUsing || funcparams->sp->tp->type != BasicType::aggregate_ ||
             funcparams->sp->tp->syms->size() < 2))
        {
            TYPE* tp1;
            // note at this pointer the arglist does NOT have the this pointer,
            // it will be added after we select a member function that needs it.
            funcparams->ascall = true;
            sym = GetOverloadedFunction(tp, &funcparams->fcall, funcparams->sp, funcparams, nullptr, true, false, flags);
            if (isfunction(*tp))
            {
                if (isstructured(basetype(*tp)->btp))
                {
                    // get rid of any invalid sizing
                    TYPE* tpx = basetype(basetype(*tp)->btp);
                    tpx->size = tpx->sp->tp->size;
                }
            }
            tp1 = *tp;
            while (tp1->btp && tp1->type != BasicType::bit_)
                tp1 = tp1->btp;
            if (sym)
            {
                sym->sb->attribs.inheritable.used = true;
                if (sym->sb->decoratedName[0] == '@' && lex)
                    browse_usage(sym, lex->data->linedata->fileindex);
                if (funcparams->astemplate && sym->sb->templateLevel && !sym->sb->specialized)
                {
                    std::list<TEMPLATEPARAMPAIR>::iterator tpln, tplne;
                    if (funcparams->templateParams)
                    {
                        tpln = funcparams->templateParams->begin();
                        tplne = funcparams->templateParams->end();
                    }
                    auto tplo = sym->sb->parentTemplate->templateParams->begin();
                    ++tplo;
                    auto tploe = sym->sb->parentTemplate->templateParams->end();
                    bool found = false;
                    for ( ; tpln != tplne && tplo != tploe; ++ tpln, ++tplo)
                    {
                        if (tplo->second->packed)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found && tpln != tplne)
                        errorsym(ERR_TOO_MANY_ARGS_PASSED_TO_TEMPLATE, sym);
                }
                sym->sb->throughClass = funcparams->sp->sb->throughClass;
                funcparams->sp = sym;
                if (ismember(sym))
                {
                    if (funcparams->noobject)
                    {
                        if (!funcsp->sb->parentClass || classRefCount(sym->sb->parentClass, funcsp->sb->parentClass) == 0)
                        {
                            errorsym(ERR_USE_DOT_OR_POINTSTO_TO_CALL, sym);
                        }
                        else if (classRefCount(sym->sb->parentClass, funcsp->sb->parentClass) != 1)
                        {
                            errorsym2(ERR_NOT_UNAMBIGUOUS_BASE, sym->sb->parentClass, funcsp->sb->parentClass);
                        }
                        else if (funcsp->sb->storage_class == StorageClass::member_ || funcsp->sb->storage_class == StorageClass::virtual_)
                        {
                            funcparams->thisptr = varNode(ExpressionNode::auto_, (SYMBOL*)basetype(funcsp->tp)->syms->front());
                            deref(&stdpointer, &funcparams->thisptr);
                            funcparams->thisptr =
                                DerivedToBase(sym->sb->parentClass->tp, basetype(funcparams->thisptr->left->v.sp->tp)->btp,
                                              funcparams->thisptr, _F_VALIDPOINTER);
                            funcparams->thistp = MakeType(BasicType::pointer_, sym->sb->parentClass->tp);
                            if (isconst(sym->tp))
                            {
                                funcparams->thistp->btp = MakeType(BasicType::const_, funcparams->thistp->btp);
                            }
                            if (isvolatile(sym->tp))
                            {
                                funcparams->thistp->btp = MakeType(BasicType::volatile_, funcparams->thistp->btp);
                            }
                            UpdateRootTypes(funcparams->thistp->btp);
                            cppCast(((SYMBOL*)basetype(funcsp->tp)->syms->front())->tp, &funcparams->thistp,
                                    &funcparams->thisptr);
                        }
                    }
                }
            }
        }
        else
        {
            /*
            operands = !ismember(funcparams->sp) && funcparams->thisptr && !addedThisPointer;
            if (!isExpressionAccessible(funcsp ? funcsp->sb->parentClass : nullptr, funcparams->sp, funcsp, funcparams->thisptr,
                                        false))
                errorsym(ERR_CANNOT_ACCESS, funcparams->sp);
             */
        }
        if (sym)
        {
            bool test;
            *tp = sym->tp;
            if (hasThisPtr)
            {
                test = isExpressionAccessible(funcsp ? funcsp->sb->parentClass : nullptr, sym, funcsp, funcparams->thisptr, false);
            }
            else
            {
                test = isExpressionAccessible(funcsp ? funcsp->sb->parentClass : nullptr, sym, funcsp, funcparams->thisptr, false);
            }
            if (!test)
            {
                errorsym(ERR_CANNOT_ACCESS, sym);
            }
            if (ismember(funcparams->sp))
            {
                if (funcparams->thistp && isconst(basetype(funcparams->thistp)->btp))
                    if (!isconst(*tp))
                        errorsym(ERR_NON_CONST_FUNCTION_CALLED_FOR_CONST_OBJECT, funcparams->sp);
            }
        }
    }
    if (basetype(*tp)->type == BasicType::memberptr_)
    {
        memberPtr = true;
        *tp = basetype(*tp)->btp;
    }
    if (!isfunction(*tp))
    {
        // might be operator ()
        if (Optimizer::cparams.prm_cplusplus && isstructured(*tp))
        {
            if (insertOperatorParams(funcsp, &tp_cpp, &exp_cpp, funcparams, flags))
            {
                hasThisPtr = funcparams->thisptr != nullptr;
                *tp = tp_cpp;
                *exp = exp_cpp;
            }
            else
            {
                if (!templateNestingCount && !(flags & _F_INDECLTYPE))
                    errortype(ERR_UNABLE_TO_FIND_SUITABLE_OPERATOR_CALL, *tp, nullptr);
            }
        }
        else
        {
            bool doit = true;

            // if we are in an argument list and there is an empty packed argument
            // don't generate an error on the theory there will be an ellipsis...
            if ((flags & (_F_INARGS | _F_INCONSTRUCTOR)) && funcparams->arguments)
            {
                for (auto arg : *funcparams->arguments)
                {
                    if (arg->tp && arg->tp->type == BasicType::templateparam_ && arg->tp->templateParam->second->packed)
                        doit = !!arg->tp->templateParam->second->byPack.pack;
                }
            }
            if (doit && !templateNestingCount && !(flags & _F_INDECLTYPE))
                error(ERR_CALL_OF_NONFUNCTION);
        }
    }
    {
        SymbolTable<SYMBOL>* temp = basetype(*tp)->syms;
        if (temp)
        {
            auto it = temp->begin();

            if (funcparams->sp && !ismember(funcparams->sp) && !memberPtr)
            {
                if (operands)
                {
                    INITLIST* al = Allocate<INITLIST>();
                    al->exp = funcparams->thisptr;
                    al->tp = funcparams->thistp;
                    if (!funcparams->arguments)
                        funcparams->arguments = initListListFactory.CreateList();
                    funcparams->arguments->push_front(al);
                }
                funcparams->thisptr = nullptr;
            }
            else
            {
                if (!getStructureDeclaration() && !ispointer(tp_cpp) && !hasThisPtr)
                    errorsym(ERR_ACCESS_MEMBER_NO_OBJECT, funcparams->sp);
                operands = false;
            }
            std::list<INITLIST*>::iterator itl, itle = itl;
            if (funcparams->arguments)
            {
                itl = funcparams->arguments->begin();
                itle = funcparams->arguments->end();
            }
            if (funcparams->sp)
            {
                if (funcparams->sp->sb->parentClass == nullptr && funcparams->sp->sb->parentNameSpace == nullptr)
                {
                    if (!strcmp(funcparams->sp->name, "setjmp"))
                        Optimizer::setjmp_used = true;
                }
                if (funcparams->sp->sb->attribs.inheritable.linkage3 == Linkage::noreturn_)
                    isCallNoreturnFunction = true;
                TYPE* tp1 = funcparams->sp->tp;
                if (ispointer(tp1))
                    tp1 = basetype(tp1)->btp;
                if (isfunction(tp1))
                {
                    auto itq = basetype(tp1)->syms->begin();
                    if (itq != basetype(tp1)->syms->end())
                    {
                        if ((*itq)->sb->thisPtr)
                            ++itq;
                        TYPE* tp = (*itq)->tp;
                        if (isref(tp))
                        {
                            initializerRef = true;
                            tp = basetype(tp)->btp;
                        }
                        if (isstructured(tp))
                        {
                            SYMBOL* sym = (basetype(tp)->sp);
                            if (sym->sb->initializer_list && sym->sb->templateLevel)
                            {
                                auto itr = sym->templateParams->begin();
                                ++itr;
                                initializerListTemplate = sym->tp;
                                initializerListType = itr->second->byClass.val;
                            }
                        }
                    }
                }
            }
            if (!(flags & _F_SIZEOF))
            {
                std::list<INITLIST*> temp1;
                std::list<INITLIST*>* temp2 = &temp1;
                if (initializerListType)
                {
                    if (funcparams->sp->sb->constexpression)
                    {
                        EXPRESSION* node = Allocate<EXPRESSION>();
                        node->type = ExpressionNode::func_;
                        node->v.func = funcparams;
                        if (EvaluateConstexprFunction(node))
                        {
                            *tp = basetype(funcparams->sp->tp)->btp;
                            *exp = node;
                            return lex;
                        }
                    }
                    bool isnested = false;
                    if (funcparams->arguments->front()->nested && funcparams->arguments->front()->nested->front()->nested && !funcparams->arguments->front()->initializer_list)
                    {
                        funcparams->arguments = funcparams->arguments->front()->nested;
                        itl = funcparams->arguments->begin();
                        itle = funcparams->arguments->end();
                        temp1.insert(temp1.begin(), itl, itle);
                        isnested = true;
                    }
                    else if ((*itl)->nested)
                    {
                        temp1.push_back(*itl);
                    }
                    else
                    {
                        temp1.insert(temp1.begin(), itl, itle);
                    }

                    CreateInitializerList(funcparams->sp, initializerListTemplate, initializerListType, &temp2, operands,
                                          initializerRef);
                    if (isnested)
                    {
                        funcparams->arguments = temp2;
                        itl = funcparams->arguments->begin();
                        itle = funcparams->arguments->end();
                    }
                    else
                    {
                        **itl = *temp2->front();
                    }
                    if (it != temp->end())
                    {
                        auto itx = itl;
                        ++itx;
                        temp1.clear();
                        temp1.insert(temp1.begin(), itx, itle);
                        temp2 = &temp1;
                        auto itxn = it;
                        ++itxn;
                        AdjustParams(funcparams->sp, itxn, temp->end(), &temp2, operands, true);
                        itx = itl;
                        ++itx;
                        for (auto t : temp1)
                        {
                            *itx = t;
                        }
                    }
                }
                else
                {
                    temp1.insert(temp1.begin(), itl, itle);
                    int n = temp1.size();
                    AdjustParams(funcparams->sp, it, temp->end(), &temp2, operands, true);
                    auto itt = temp1.begin();
                    auto itte = temp1.end();
                    for (auto i = 0; i < n; i++)
                        ++itt;
                    if (!funcparams->arguments)
                        funcparams->arguments = initListListFactory.CreateList();
                    for (; itt != itte; ++itt)
                    {
                        funcparams->arguments->push_back(*itt);
                    }
                }
            }
            if (funcparams->sp->sb->xcMode != xc_unspecified && funcparams->sp->sb->xcMode != xc_none)
                hasFuncCall = true;
            CheckCalledException(funcparams->sp, funcparams->thisptr);
            if (Optimizer::cparams.prm_cplusplus && funcparams->arguments)
            {
                for (auto il : *funcparams->arguments)
                {
                    if (il->tp && isstructured(il->tp) && il->exp)
                    {
                        EXPRESSION* exp = il->exp;
                        if (exp->type == ExpressionNode::not__lvalue_)
                            exp = exp->left;
                        if (exp->type == ExpressionNode::auto_)
                        {
                            if (!funcparams->callLab)
                                funcparams->callLab = -1;
                        }
                        else if (exp->type == ExpressionNode::thisref_)
                        {
                            if (!funcparams->callLab)
                                funcparams->callLab = -1;
                        }
                    }
                }
            }
            if (isfunction(*tp))
            {
                (*exp)->v.func->resolvedCall = true;
                if (funcparams->thisptr && !memberPtr)
                {
                    SYMBOL* base = funcparams->sp->sb->parentClass;
                    SYMBOL* derived = basetype(basetype(funcparams->thistp)->btp)->sp;
                    if (base != derived)
                    {
                        funcparams->thisptr = DerivedToBase(base->tp, derived->tp, funcparams->thisptr, _F_VALIDPOINTER);
                    }
                }
                if (isstructured(basetype(*tp)->btp) || basetype(basetype(*tp)->btp)->type == BasicType::memberptr_)
                {
                    if (!(flags & _F_SIZEOF) && !basetype(basetype(*tp)->btp)->sp->sb->structuredAliasType)
                    {
                        funcparams->returnEXP = anonymousVar(StorageClass::auto_, basetype(*tp)->btp);
                        funcparams->returnSP = funcparams->returnEXP->v.sp;
                        if (theCurrentFunc && theCurrentFunc->sb->constexpression)
                            funcparams->returnEXP->v.sp->sb->constexpression = true;
                    }
                }
                else if (isbitint(basetype(basetype(*tp)->btp)))
                {
                    if (!(flags & _F_SIZEOF))
                    {
                        funcparams->returnEXP = anonymousVar(StorageClass::auto_, basetype(*tp)->btp);
                        funcparams->returnSP = funcparams->returnEXP->v.sp;
                        if (theCurrentFunc && theCurrentFunc->sb->constexpression)
                            funcparams->returnEXP->v.sp->sb->constexpression = true;
                    }

                }
                funcparams->ascall = true;
                funcparams->functp = *tp;

                *tp = ResolveTemplateSelectors(basetype(*tp)->sp, basetype(*tp)->btp);
                if (isref(*tp))
                {
                    auto reftype = (*tp)->type;
                    *tp = CopyType((*tp)->btp);
                    UpdateRootTypes(*tp);
                    if (reftype == BasicType::rref_)
                    {
                        (*tp)->rref = true;
                        (*tp)->lref = false;
                    }
                    else
                    {
                        (*tp)->lref = true;
                        (*tp)->rref = false;
                    }
                }
                else if (ispointer(*tp) && (*tp)->array)
                {
                    *tp = CopyType(*tp);
                    UpdateRootTypes(*tp);
                    (*tp)->lref = true;
                    (*tp)->rref = false;
                }
                auto tp1 = tp;
                while (ispointer(*tp1) || basetype(*tp1)->type == BasicType::memberptr_)
                    tp1 = &basetype(*tp1)->btp;
                while ((*tp1)->btp)
                    tp1 = &(*tp1)->btp;
                if (isstructured(*tp1))
                {
                    *tp1 = (*tp1)->sp->tp;
                }
                if (!(flags & _F_SIZEOF))
                {
                    checkArgs(funcparams, funcsp);
                }
                if (funcparams->returnSP && isstructured(funcparams->returnSP->tp))
                {
                    SYMBOL* sym = basetype(funcparams->returnSP->tp)->sp;
                    if (sym->sb->templateLevel && sym->templateParams && !sym->sb->instantiated)
                    {
                        if (!allTemplateArgsSpecified(sym, sym->templateParams))
                            sym = GetClassTemplate(sym, sym->templateParams, false);
                        if (sym && allTemplateArgsSpecified(sym, sym->templateParams))
                            funcparams->returnSP->tp = TemplateClassInstantiate(sym, sym->templateParams, false, StorageClass::global_)->tp;
                    }
                }
                if ((!funcparams->novtab || (funcparams->sp && funcparams->sp->sb->ispure)) && funcparams->sp &&
                    funcparams->sp->sb->storage_class == StorageClass::virtual_)
                {
                    exp_in = funcparams->thisptr;
                    deref(&stdpointer, &exp_in);
                    exp_in = exprNode(ExpressionNode::add_, exp_in, intNode(ExpressionNode::c_i_, funcparams->sp->sb->vtaboffset));
                    deref(&stdpointer, &exp_in);
                    funcparams->fcall = exp_in;
                }
                else
                {
                    exp_in = varNode(ExpressionNode::func_, nullptr);
                    exp_in->v.func = funcparams;
                    if (exp_in && Optimizer::cparams.prm_cplusplus && funcparams->returnEXP)
                    {
                        if (!basetype(funcparams->returnSP->tp)->sp->sb->trivialCons)
                        {
                            exp_in = exprNode(ExpressionNode::thisref_, exp_in, nullptr);
                            exp_in->v.t.thisptr = funcparams->returnEXP;
                            exp_in->v.t.tp = funcparams->returnSP->tp;

                            if (isstructured(funcparams->returnSP->tp))
                            {
                                EXPRESSION* expx = funcparams->returnEXP;
                                callDestructor(basetype(funcparams->returnSP->tp)->sp, nullptr, &expx, nullptr, true, false, true,
                                               true);
                                initInsert(&funcparams->returnSP->sb->dest, funcparams->returnSP->tp, expx, 0, true);
                            }
                        }
                    }
                    if (exp_in)
                    {
                        TYPE* tp = basetype(funcparams->sp->tp)->btp;
                        if ((flags & _F_AMPERSAND) && isarithmetic(tp))
                        {
                            EXPRESSION* rv = anonymousVar(StorageClass::auto_, tp);
                            deref(tp, &rv);
                            exp_in = exprNode(ExpressionNode::void_, exprNode(ExpressionNode::assign_, rv, exp_in), rv);
                            errortype(ERR_CREATE_TEMPORARY, tp, tp);
                        }
                    }
                    if (exp_in)
                        *exp = exp_in;
                }
                if (funcparams->sp && isfunction(funcparams->sp->tp) && isref(basetype(funcparams->sp->tp)->btp))
                {
                    TYPE** tp1;
                    deref(basetype(basetype(funcparams->sp->tp)->btp)->btp, exp);
                    tp1 = &basetype(funcparams->sp->tp)->btp;
                    *tp = CopyType(basetype(*tp1)->btp);
                    UpdateRootTypes(*tp);
                    if (basetype(*tp1)->type == BasicType::rref_)
                    {
                        (*tp)->rref = true;
                        (*tp)->lref = false;
                    }
                    else
                    {
                        (*tp)->lref = true;
                        (*tp)->rref = false;
                    }
                    while (isref(*tp))
                        *tp = basetype(*tp)->btp;
                }
            }
            else if (templateNestingCount && !instantiatingTemplate && (*tp)->type == BasicType::aggregate_)
            {
                *exp = exprNode(ExpressionNode::funcret_, *exp, nullptr);
                *tp = MakeType(BasicType::templatedecltype_);
                (*tp)->templateDeclType = *exp;
            }
            else
            {
                // we may get here for a packed argument with an ellipsis
                // if that is the case this won't matter and if it isn't the error will ripple...
                *tp = &stdany;
            }
        }
        else
        {
            *tp = &stdint;
        }
    }
    return lex;
}
static LEXLIST* expression_alloca(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    lex = getsym();
    if (needkw(&lex, Keyword::openpa_))
    {
        lex = expression_comma(lex, funcsp, nullptr, tp, exp, nullptr, flags);
        if (*tp)
        {
            ResolveTemplateVariable(tp, exp, &stdint, nullptr);
            if (!isint(*tp))
                error(ERR_NEED_INTEGER_EXPRESSION);
            optimize_for_constants(exp);

            funcsp->sb->allocaUsed = true;
            *exp = exprNode(ExpressionNode::alloca_, *exp, 0);
            needkw(&lex, Keyword::closepa_);
            *tp = &stdpointer;
        }
        else
        {
            error(ERR_EXPRESSION_SYNTAX);
            *tp = nullptr;
        }
    }
    else
    {
        errskim(&lex, skim_closepa);
        skip(&lex, Keyword::closepa_);
        *tp = nullptr;
    }
    return lex;
}
static LEXLIST* expression_offsetof(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    *exp = intNode(ExpressionNode::c_i_, 0);
    lex = getsym();
    if (needkw(&lex, Keyword::openpa_))
    {
        // this is naive, not checking the actual type of the sym when these are present
        if (MATCHKW(lex, Keyword::struct_) || MATCHKW(lex, Keyword::class_))
            lex = getsym();
        if (ISID(lex))
        {
            char name[512];
            SYMBOL* sym;
            SymbolTable<SYMBOL>* table;
            SYMBOL* strSym;
            std::list<NAMESPACEVALUEDATA*>* nsv;
            strcpy(name, lex->data->value.s.a);
            lex = tagsearch(lex, name, &sym, &table, &strSym, &nsv, StorageClass::global_);
            if (!sym)  // might be a typedef
            {

                sym = namespacesearch(name, globalNameSpace, false, false);
                if (sym && sym->tp->type == BasicType::typedef_)
                {
                    sym = basetype(sym->tp->btp)->sp;
                }
                else
                {
                    sym = nullptr;
                }
            }
            if (sym && isstructured(sym->tp))
            {
                if (needkw(&lex, Keyword::comma_))
                {
                    *tp = sym->tp;
                    do
                    {
                        lex = expression_member(lex, funcsp, tp, exp, ismutable, flags | _F_AMPERSAND);
                        if (lvalue(*exp))
                            *exp = (*exp)->left;
                    } while (MATCHKW(lex, Keyword::dot_));
                }
            }
            else
            {
                error(ERR_CLASS_TYPE_EXPECTED);
            }
        }
        else
        {
            error(ERR_IDENTIFIER_EXPECTED);
        }
        needkw(&lex, Keyword::closepa_);
    }
    *tp = &stdunsigned;

    return lex;
}
static LEXLIST* expression_msilfunc(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    Keyword kw = lex->data->kw->key;
    lex = getsym();
    if (MATCHKW(lex, Keyword::openpa_))
    {
        FUNCTIONCALL funcparams;
        memset(&funcparams, 0, sizeof(funcparams));
        lex = getArgs(lex, funcsp, &funcparams, Keyword::closepa_, true, flags);
        int n = funcparams.arguments ? funcparams.arguments->size() : 0;
        if (n > 3)
        {
            *exp = intNode(ExpressionNode::c_i_, 0);
            errorstr(ERR_PARAMETER_LIST_TOO_LONG, "__cpblk/__initblk");
        }
        else if (n < 3)
        {
            *exp = intNode(ExpressionNode::c_i_, 0);
            errorstr(ERR_PARAMETER_LIST_TOO_SHORT, "__cpblk/__initblk");
        }
        else {
            auto it = funcparams.arguments->begin();
            auto arg1 = *it;
            ++it;
            auto arg2 = *it;
            ++it;
            auto arg3 = *it;
            switch (kw)
            {
            case Keyword::cpblk_:
                if (!ispointer(arg1->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__dest", "__cpblk");
                }
                else if (!ispointer(arg2->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__src", "__cpblk");
                }
                else if (!isint(arg3->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__size", "__cpblk");
                }
                else
                {
                    *exp = exprNode(ExpressionNode::void_, arg1->exp, arg2->exp);
                    *exp = exprNode(ExpressionNode::cpblk_, *exp, arg3->exp);
                }
                break;
            case Keyword::initblk_:
                if (!ispointer(arg1->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__dest", "__initblk");
                }
                else if (!isint(arg2->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__value", "__initblk");
                }
                else if (!isint(arg3->tp))
                {
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    errorstr2(ERR_TYPE_MISMATCH_IN_ARGUMENT, "__size", "__initblk");
                }
                else
                {
                    *exp = exprNode(ExpressionNode::void_, arg1->exp,  arg2->exp);
                    *exp = exprNode(ExpressionNode::initblk_, *exp, arg3->exp);
                }
                break;
            default:
                *exp = intNode(ExpressionNode::c_i_, 0);
                break;
            }
        }
        *tp = &stdpointer;
    }
    else
    {
        errskim(&lex, skim_closepa);
        skip(&lex, Keyword::closepa_);
        *tp = nullptr;
    }
    return lex;
}
static LEXLIST* expression_string(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    int elems = 0;
    STRING* data;
    (void)funcsp;

    lex = concatStringsInternal(lex, &data, &elems);
    *exp = stringlit(data);
    if (data->suffix)
    {
        enum BasicType tpb;
        SYMBOL* sym;
        char name[512];
        switch (data->strtype)
        {
            default:
            case l_astr:
                tpb = stdcharptr.btp->type;
                break;
            case l_wstr:
                tpb = stdwcharptr.btp->type;
                break;
            case l_msilstr:
                tpb = std__string.type;
                break;
            case l_ustr:
                tpb = stdchar16tptr.btp->type;
                break;
            case l_Ustr:
                tpb = stdchar32tptr.btp->type;
                break;
        }
        Optimizer::my_sprintf(name, "%s@%s", overloadNameTab[CI_LIT], data->suffix);
        sym = LookupSym(name);
        if (sym)
        {
            SYMBOL *sym1 = nullptr, *sym2 = nullptr;
            bool found = false;
            for (auto sym3 : *sym->tp->syms)
            {
                sym2 = sym3->tp->syms->front();
                if (sym3->tp->syms->size() > 1 && ispointer(sym2->tp))
                    if (isconst(sym2->tp->btp) && basetype(sym2->tp->btp)->type == tpb)
                    {
                        sym1 = sym3;
                        found = true;
                        break;
                    }
            }
            if (found)
            {
                FUNCTIONCALL* f = Allocate<FUNCTIONCALL>();
                f->sp = sym1;
                f->functp = sym1->tp;
                f->fcall = varNode(ExpressionNode::pc_, sym1);
                f->arguments = initListListFactory.CreateList();
                auto arg = Allocate<INITLIST>();
                arg->tp = sym2->tp;
                arg->exp = *exp;
                f->arguments->push_back(arg);
                arg = Allocate<INITLIST>();
                arg->tp = &stdunsigned;
                arg->exp = intNode(ExpressionNode::c_i_, elems);
                f->arguments->push_back(arg);
                *exp = intNode(ExpressionNode::func_, 0);
                (*exp)->v.func = f;
                *tp = sym1->tp;
                expression_arguments(nullptr, funcsp, tp, exp, false);
                return lex;
            }
        }
        errorstr(ERR_COULD_NOT_FIND_A_MATCH_FOR_LITERAL_SUFFIX, data->suffix);
    }
    if (data->strtype == l_msilstr)
    {
        *tp = CopyType(&std__string);
        (*tp)->rootType = *tp;
    }
    else
    {
        *tp = MakeType(BasicType::pointer_);
        (*tp)->array = true;
        (*tp)->stringconst = true;
        (*tp)->esize = intNode(ExpressionNode::c_i_, elems + 1);
        switch (data->strtype)
        {
            default:
            case l_astr:
                (*tp)->btp = stdcharptr.btp;
                break;
            case l_wstr:
                (*tp)->btp = stdwcharptr.btp;
                break;
            case l_ustr:
                (*tp)->btp = stdchar16tptr.btp;
                break;
            case l_Ustr:
                (*tp)->btp = stdchar32tptr.btp;
                break;
        }
    }
    if ((*tp)->type == BasicType::string_)
        (*tp)->size = 1;
    else
        (*tp)->size = (elems + 1) * (*tp)->btp->size;
    return lex;
}
static bool matchesAttributes(TYPE *tp1, TYPE *tp2)
{
    while (tp1 && tp2)
    {
       if (isconst(tp1) != isconst(tp2) ||
           isvolatile(tp1) != isvolatile(tp2) ||
           isrestrict(tp1) != isrestrict(tp2))
           return false;
       tp1 = basetype(tp1)->btp;
       tp2 = basetype(tp2)->btp;
    }
    return true;
}
static bool sameTypedef(TYPE* tp1, TYPE* tp2)
{
    while (tp1->btp && !tp1->typedefType)
       tp1 = tp1->btp;
    while (tp2->btp && !tp2->typedefType)
       tp2 = tp2->btp;
    return tp1->typedefType == tp2->typedefType;
}
static LEXLIST* expression_generic(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    lex = getsym();
    if (!needkw(&lex, Keyword::openpa_))
    {
        errskim(&lex, skim_closepa);
        skip(&lex, Keyword::closepa_);
        *exp = intNode(ExpressionNode::c_i_, 0);
        *tp = &stdint;
    }
    else
    {
        TYPE* selectType = nullptr;
        EXPRESSION* throwawayExpression = nullptr;
        lex = expression_assign(lex, funcsp, nullptr, &selectType, &throwawayExpression, nullptr, flags);
        if (MATCHKW(lex, Keyword::comma_))
        {
            bool dflt = false;
            struct genericHold
            {
                struct genericHold* next;
                TYPE* selector;
                TYPE* type;
                EXPRESSION* exp;
            };
            struct genericHold *list = nullptr, **pos = &list;
            struct genericHold* selectedGeneric = nullptr;
            while (MATCHKW(lex, Keyword::comma_))
            {
                struct genericHold *next = Allocate<genericHold>(), *scan;
                lex = getsym();
                if (MATCHKW(lex, Keyword::default_))
                {
                    lex = getsym();
                    if (dflt)
                    {
                        error(ERR_GENERIC_TOO_MANY_DEFAULTS);
                    }
                    dflt = true;
                    next->selector = nullptr;
                }
                else
                {
                    lex = get_type_id(lex, &next->selector, funcsp, StorageClass::cast_, false, true, false);
                    if (!next->selector)
                    {
                        error(ERR_GENERIC_MISSING_TYPE);
                        break;
                    }
                }
                if (MATCHKW(lex, Keyword::colon_))
                {
                    lex = getsym();
                    lex = expression_assign(lex, funcsp, nullptr, &next->type, &next->exp, nullptr, flags);
                    if (!next->type)
                    {
                        error(ERR_GENERIC_MISSING_EXPRESSION);
                        break;
                    }
                    scan = list;
                    while (scan)
                    {
                        if (scan->selector && next->selector && comparetypes(next->selector, scan->selector, true))
                        {
                            if (matchesAttributes(next->selector, scan->selector) && sameTypedef(next->selector, scan->selector))
                            {
                                error(ERR_DUPLICATE_TYPE_IN_GENERIC);
                            }
                        }
                        scan = scan->next;
                    }
                    if (!selectedGeneric && !next->selector)
                        selectedGeneric = next;
                    else
                    {
                        if (selectType && next->selector && comparetypes(next->selector, selectType, true))
                        {
                            if (matchesAttributes(next->selector, selectType) && sameTypedef(next->selector, selectType))
                            {
                                if (selectedGeneric && selectedGeneric->selector)
                                    error(ERR_DUPLICATE_TYPE_IN_GENERIC);
                                selectedGeneric = next;
                            }
                        }
                    }
                    *pos = next;
                    pos = &(*pos)->next;
                }
                else
                {
                    error(ERR_GENERIC_MISSING_EXPRESSION);
                    break;
                }
            }
            if (selectedGeneric)
            {
                *tp = selectedGeneric->type;
                *exp = selectedGeneric->exp;
                optimize_for_constants(exp);
            }
            else
            {
                error(ERR_GENERIC_NOTHING_SELECTED);
                *tp = &stdint;
                *exp = intNode(ExpressionNode::c_i_, 0);
            }
            if (!needkw(&lex, Keyword::closepa_))
            {
                errskim(&lex, skim_closepa);
                skip(&lex, Keyword::closepa_);
            }
        }
        else
        {
            errskim(&lex, skim_closepa);
            skip(&lex, Keyword::closepa_);
        }
    }
    return lex;
}
static bool getSuffixedChar(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    char name[512];
    enum BasicType tpb = (*tp)->type;
    SYMBOL* sym;
    Optimizer::my_sprintf(name, "%s@%s", overloadNameTab[CI_LIT], lex->data->suffix);
    sym = LookupSym(name);
    if (sym)
    {
        bool found = false;
        SYMBOL *sym1 = nullptr, *sym2;
        for (auto sp : *sym->tp->syms)
        {
            sym1 = sp;
            sym2 = sym1->tp->syms->front();
            if (sym1->tp->syms->size() == 1 && sym2->tp->type == tpb)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            FUNCTIONCALL* f = Allocate<FUNCTIONCALL>();
            f->sp = sym1;
            f->functp = sym1->tp;
            f->fcall = varNode(ExpressionNode::pc_, sym1);
            f->arguments = initListListFactory.CreateList();
            auto arg = Allocate<INITLIST>();
            f->arguments->push_back(arg);
            arg->tp = *tp;
            arg->exp = *exp;
            *exp = intNode(ExpressionNode::func_, 0);
            (*exp)->v.func = f;
            *tp = sym1->tp;
            expression_arguments(nullptr, funcsp, tp, exp, false);
            return true;
        }
    }
    errorstr(ERR_COULD_NOT_FIND_A_MATCH_FOR_LITERAL_SUFFIX, lex->data->suffix);
    return false;
}
static bool getSuffixedNumber(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    char name[512];
    enum BasicType tpb;
    SYMBOL* sym;
    if (lex->data->type == l_ull)
        tpb = BasicType::unsigned_long_long_;
    else
        tpb = BasicType::long_double_;
    Optimizer::my_sprintf(name, "%s@%s", overloadNameTab[CI_LIT], lex->data->suffix);
    sym = LookupSym(name);
    if (sym)
    {
        // look for parameter of type unsigned long long or long double
        SYMBOL *sym1 = nullptr, *sym2 = nullptr;
        auto found = false;
        for (auto sp : *sym->tp->syms)
        {
            sym1 = sp;
            sym2 = sym2->tp->syms->front();
            if (sym1->tp->syms->size() == 1 && sym2->tp->type == tpb)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            FUNCTIONCALL* f = Allocate<FUNCTIONCALL>();
            f->sp = sym1;
            f->functp = sym1->tp;
            f->fcall = varNode(ExpressionNode::pc_, sym1);
            f->arguments = initListListFactory.CreateList();
            auto arg = Allocate<INITLIST>();
            f->arguments->push_back(arg);
            if (lex->data->type == l_ull)
            {
                arg->tp = &stdunsignedlonglong;
                arg->exp = intNode(ExpressionNode::c_ull_, lex->data->value.i);
            }
            else
            {
                arg->tp = &stdlongdouble;
                arg->exp = intNode(ExpressionNode::c_ld_, 0);
                arg->exp->v.f = Allocate<FPF>();
                *arg->exp->v.f = *lex->data->value.f;
            }
            *exp = intNode(ExpressionNode::func_, 0);
            (*exp)->v.func = f;
            *tp = sym1->tp;
            expression_arguments(nullptr, funcsp, tp, exp, false);
            return true;
        }
        else
        {
            bool found = false;
            // not found, look for parameter of type const char *
            for (auto sp : *sym->tp->syms)
            {
                TYPE* tpx;
                sym1 = sp;
                sym2 = sym1->tp->syms->front();
                tpx = sym2->tp;
                if (sym1->tp->syms->size() == 1 && ispointer(tpx))
                {
                    tpx = basetype(tpx)->btp;
                    if (isconst(tpx) && basetype(tpx)->type == BasicType::char_)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (found)
            {
                FUNCTIONCALL* f = Allocate<FUNCTIONCALL>();
                STRING* data;
                int i;
                f->sp = sym1;
                f->functp = sym1->tp;
                f->fcall = varNode(ExpressionNode::pc_, sym1);
                f->arguments = initListListFactory.CreateList();
                auto arg = Allocate<INITLIST>();
                f->arguments->push_back(arg);
                arg->tp = &stdcharptr;
                arg->tp->size = (strlen(lex->data->litaslit) + 1) * arg->tp->btp->size;
                data = Allocate<STRING>();
                data->strtype = l_astr;
                data->size = 1;
                data->pointers = Allocate<Optimizer::SLCHAR*>();
                data->pointers[0] = Allocate<Optimizer::SLCHAR>();
                data->pointers[0]->count = strlen(lex->data->litaslit);
                data->pointers[0]->str = Allocate<LCHAR>(data->pointers[0]->count + 1);
                for (i = 0; i < data->pointers[0]->count; i++)
                    data->pointers[0]->str[i] = lex->data->litaslit[i];
                arg->exp = stringlit(data);
                *exp = intNode(ExpressionNode::func_, 0);
                (*exp)->v.func = f;
                *tp = sym1->tp;
                expression_arguments(nullptr, funcsp, tp, exp, false);
                return true;
            }
        }
    }
    errorstr(ERR_COULD_NOT_FIND_A_MATCH_FOR_LITERAL_SUFFIX, lex->data->suffix);
    return false;
}
static Parser::LEXLIST* atomic_modify_specific_op(Parser::LEXLIST* lex, Parser::SYMBOL* funcsp, Parser::TYPE** tp,
                                                  Parser::TYPE** typf, Parser::ATOMICDATA* d, int flags, Parser::Keyword function,
                                                  bool fetch_first)
{
    Parser::TYPE* tpf = *typf;
    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->address, nullptr, flags);
    if (tpf)
    {
        if (!ispointer(tpf))
        {
            error(ERR_DEREF);
            d->tp = *tp = &stdint;
        }
        else
        {
            d->tp = *tp = basetype(tpf)->btp;
        }
    }
    d->third = intNode(ExpressionNode::c_i_, (int)function);
    if (needkw(&lex, Keyword::comma_))
    {
        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->value, nullptr, flags);
        if (!comparetypes(tpf, *tp, false))
        {
            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
        }
        if (ispointer(*tp) && (function == Keyword::asplus_ || function == Keyword::asminus_))
        {
            d->value = exprNode(ExpressionNode::mul_, d->value, intNode(ExpressionNode::c_i_, (*tp)->size));
        }
    }
    else
    {
        *tp = &stdint;
        d->value = intNode(ExpressionNode::c_i_, 0);
    }
    if (needkw(&lex, Keyword::comma_))
    {
        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
    }
    else
    {
        tpf = &stdint;
        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
    }
    d->atomicOp = fetch_first ? Optimizer::e_ao::ao_fetch_modify : Optimizer::e_ao::ao_modify_fetch;
    if (!d->memoryOrder2)
        d->memoryOrder2 = d->memoryOrder1;
    return lex;
}
static LEXLIST* expression_atomic_func(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    Keyword kw = KW(lex);
    lex = getsym();
    if (needkw(&lex, Keyword::openpa_))
    {
        if (kw == Keyword::c11_atomic_is_lock_free_)
        {
            lex = optimized_expression(lex, funcsp, nullptr, tp, exp, false);
            if (!isint(*tp) || !isintconst(*exp))
                error(ERR_NEED_INTEGER_TYPE);
            *tp = &stdint;
            bool found = (*exp)->v.i > 0 && (*exp)->v.i <= Optimizer::chosenAssembler->arch->isLockFreeSize &&
                         ((*exp)->v.i & ((*exp)->v.i - 1)) == 0;
            *exp = intNode(ExpressionNode::c_i_, found);
            needkw(&lex, Keyword::closepa_);
        }
        else if (kw == Keyword::c11_atomic_init_)
        {
            lex = expression_assign(lex, funcsp, nullptr, tp, exp, nullptr, flags);
            if (!*tp)
                error(ERR_EXPRESSION_SYNTAX);
            if (MATCHKW(lex, Keyword::comma_))  // atomic_init
            {
                TYPE* tp1;
                EXPRESSION* exp1;
                lex = getsym();
                lex = expression_assign(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
                if (*tp && tp1)
                {
                    ATOMICDATA* d;
                    d = Allocate<ATOMICDATA>();
                    if (!ispointer(*tp))
                    {
                        error(ERR_DEREF);
                        d->tp = *tp = &stdint;
                    }
                    else
                    {
                        TYPE* tp2 = *tp;
                        d->tp = *tp = basetype(*tp)->btp;
                        if (!comparetypes(*tp, tp1, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                        tp1 = tp2;
                    }
                    d->address = *exp;
                    d->value = exp1;
                    d->atomicOp = Optimizer::ao_init;
                    *exp = exprNode(ExpressionNode::atomic_, nullptr, nullptr);
                    (*exp)->v.ad = d;
                }
                else
                {
                    error(ERR_EXPRESSION_SYNTAX);
                }
            }
            else  // ATOMIC_VAR_INIT
            {
                if (*exp)
                    (*exp)->atomicinit = true;
            }
            needkw(&lex, Keyword::closepa_);
        }
        else
        {
            TYPE *tpf = nullptr, *tpf1;
            ATOMICDATA* d;
            d = Allocate<ATOMICDATA>();
            switch (kw)
            {
                case Keyword::atomic_kill_dependency_:
                    lex = expression_assign(lex, funcsp, nullptr, &d->tp, &d->address, nullptr, flags);
                    *tp = d->tp;
                    if (!*tp)
                        error(ERR_EXPRESSION_SYNTAX);
                    d->atomicOp = Optimizer::ao_kill_dependency;
                    break;
                case Keyword::atomic_flag_test_set_:

                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->flg, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                        }
                        deref(&stdint, &d->flg);
                        d->tp = *tp = &stdint;
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = Optimizer::ao_flag_set_test;
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    break;
                case Keyword::atomic_flag_clear_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->flg, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                        }
                        deref(&stdint, &d->flg);
                        d->tp = *tp = &stdint;
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = Optimizer::ao_flag_clear;
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    *tp = &stdvoid;
                    break;
                case Keyword::c11_atomic_thread_fence_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    d->memoryOrder1 = exprNode(ExpressionNode::add_, d->memoryOrder1, intNode(ExpressionNode::c_i_, 0x80));
                    d->atomicOp = Optimizer::ao_thread_fence;
                    *tp = &stdvoid;
                    break;
                case Keyword::c11_atomic_signal_fence_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    d->memoryOrder1 = exprNode(ExpressionNode::add_, d->memoryOrder1, intNode(ExpressionNode::c_i_, 0x80));
                    d->atomicOp = Optimizer::ao_signal_fence;
                    *tp = &stdvoid;
                    break;
                case Keyword::c11_atomic_load_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->address, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                            d->tp = *tp = &stdint;
                        }
                        else
                        {
                            d->tp = *tp = basetype(tpf)->btp;
                        }
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = Optimizer::ao_load;
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    break;
                case Keyword::c11_atomic_store_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->address, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                            d->tp = *tp = &stdint;
                        }
                        else
                        {
                            d->tp = *tp = basetype(tpf)->btp;
                        }
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        TYPE* tpf1;
                        lex = expression_assign(lex, funcsp, nullptr, &tpf1, &d->value, nullptr, flags);
                        if (!comparetypes(d->tp, tpf1, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                    }
                    else
                    {
                        *tp = &stdint;
                        d->value = intNode(ExpressionNode::c_i_, 0);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = Optimizer::ao_store;
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    d->memoryOrder1 = exprNode(ExpressionNode::add_, d->memoryOrder1, intNode(ExpressionNode::c_i_, 0x80));
                    break;
                    // there's extremely likely a better way to do this, maybe a lookup table for these and using that...
                case Keyword::c11_atomic_xchg_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::assign_, true);
                    break;
                case Keyword::c11_atomic_ftchadd_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asplus_, true);
                    break;
                case Keyword::c11_atomic_ftchsub_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asminus_, true);
                    break;
                case Keyword::c11_atomic_ftchand_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asand_, true);
                    break;
                case Keyword::c11_atomic_ftchor_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asor_, true);
                    break;
                case Keyword::c11_atomic_ftchxor_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asxor_, true);
                    break;
                case Keyword::atomic_addftch_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asplus_, false);
                    break;
                case Keyword::atomic_subftch_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asminus_, false);
                    break;
                case Keyword::atomic_andftch_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asand_, false);
                    break;
                case Keyword::atomic_orftch_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asor_, false);
                    break;
                case Keyword::atomic_xorftch_:
                    lex = atomic_modify_specific_op(lex, funcsp, tp, &tpf, d, flags, Keyword::asxor_, false);
                    break;
                case Keyword::atomic_cmpxchg_n_: {
                    bool weak = false;
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->address, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                            d->tp = *tp = &stdint;
                        }
                        else
                        {
                            d->tp = *tp = basetype(tpf)->btp;
                        }
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf1, &d->third, nullptr, flags);
                        if (!comparetypes(tpf, tpf1, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                    }
                    else
                    {
                        *tp = &stdint;
                        d->third = intNode(ExpressionNode::c_i_, 0);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->value, nullptr, flags);
                        if (!comparetypes(tpf, *tp, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                        if (!isstructured(*tp))
                        {
                            cast(*tp, &d->value);
                        }
                    }
                    else
                    {
                        *tp = &stdint;
                        d->value = intNode(ExpressionNode::c_i_, 0);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        TYPE* weak_type = nullptr;
                        EXPRESSION* weak_expr = nullptr;

                        lex = optimized_expression(lex, funcsp, nullptr, &weak_type, &weak_expr, false);
                        if (!isintconst(weak_expr))
                        {
                            error(ERR_NEED_INTEGER_TYPE);
                        }
                        weak = (bool)weak_expr->v.i;
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf1, &d->memoryOrder2, nullptr, flags);
                    }
                    else
                    {
                        tpf1 = &stdint;
                        d->memoryOrder2 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = weak ? Optimizer::ao_cmpxchgweak : Optimizer::ao_cmpxchgstrong;
                    if (!d->memoryOrder2)
                    {
                        d->memoryOrder2 = d->memoryOrder1;
                    }
                    *tp = &stdint;
                    break;
                }
                case Keyword::c11_atomic_cmpxchg_weak_:
                case Keyword::c11_atomic_cmpxchg_strong_:
                    lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->address, nullptr, flags);
                    if (tpf)
                    {
                        if (!ispointer(tpf))
                        {
                            error(ERR_DEREF);
                            d->tp = *tp = &stdint;
                        }
                        else
                        {
                            d->tp = *tp = basetype(tpf)->btp;
                        }
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf1, &d->third, nullptr, flags);
                        if (!comparetypes(tpf, tpf1, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                    }
                    else
                    {
                        *tp = &stdint;
                        d->third = intNode(ExpressionNode::c_i_, 0);
                    }

                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->value, nullptr, flags);
                        if (!comparetypes(tpf, *tp, false))
                        {
                            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                        }
                        if (!isstructured(*tp))
                            cast(*tp, &d->value);
                    }
                    else
                    {
                        *tp = &stdint;
                        d->value = intNode(ExpressionNode::c_i_, 0);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf, &d->memoryOrder1, nullptr, flags);
                    }
                    else
                    {
                        tpf = &stdint;
                        d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    if (needkw(&lex, Keyword::comma_))
                    {
                        lex = expression_assign(lex, funcsp, nullptr, &tpf1, &d->memoryOrder2, nullptr, flags);
                    }
                    else
                    {
                        tpf1 = &stdint;
                        d->memoryOrder2 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
                    }
                    d->atomicOp = kw == Keyword::c11_atomic_cmpxchg_weak_ ? Optimizer::ao_cmpxchgweak : Optimizer::ao_cmpxchgstrong;
                    if (!d->memoryOrder2)
                        d->memoryOrder2 = d->memoryOrder1;
                    *tp = &stdint;
                    break;
                default:
                    break;
            }
            if (tpf && !isint(tpf))
            {
                error(ERR_NEED_INTEGER_TYPE);
                d->memoryOrder1 = *exp = intNode(ExpressionNode::c_i_, Optimizer::mo_relaxed);
            }
            if (d->memoryOrder1)
                optimize_for_constants(&d->memoryOrder1);
            if (!needkw(&lex, Keyword::closepa_))
            {
                errskim(&lex, skim_closepa);
                skip(&lex, Keyword::closepa_);
            }
#ifdef NEED_CONST_MO
            if (d->memoryOrder1 && !isintconst(d->memoryOrder1))
                d->memoryOrder1 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
            if (d->memoryOrder2 && !isintconst(d->memoryOrder2))
                d->memoryOrder2 = intNode(ExpressionNode::c_i_, Optimizer::mo_seq_cst);
#endif
            *exp = exprNode(ExpressionNode::atomic_, nullptr, nullptr);
            (*exp)->v.ad = d;
        }
    }
    else
    {
        *tp = &stdint;
        *exp = intNode(ExpressionNode::c_i_, Optimizer::mo_relaxed);
        errskim(&lex, skim_closepa);
        skip(&lex, Keyword::closepa_);
    }
    return lex;
}
static bool validate_checked_args(std::list<INITLIST*>* args)
{

    if (!args || args->size() < 3)
    {
        errorstr(ERR_PARAMETER_LIST_TOO_SHORT, "<checked math>");
        return false;
    }
    else if (args->size() > 3)
    {
        errorstr(ERR_PARAMETER_LIST_TOO_LONG, "<checked math>");
        return false;
    }
    else for (auto&& arg : *args)
    {
        TYPE* tp = basetype(arg->tp);        
        if (&arg == &args->front())
        {
             if (!ispointer(tp) || isconst(basetype(tp)->btp))
             {
                 error(ERR_EXPECTED_POINTER_FOR_CHECKED_MATH_RESULT);
                 return false;
             }
             else
             {
                 tp = basetype(tp->btp);
             }
        }
        if (!isint(tp) || tp->type == BasicType::char_ || tp->type == BasicType::bitint_ || tp->type == BasicType::unsigned_bitint_ || tp->type == BasicType::bool_)
        {
             error(ERR_EXPECTED_VALID_CHECKED_MATH_TYPE);
             return false;
        }
    }
    return true;
}
static int ___typeid_val(TYPE *tp);
static std::list<INITLIST*>* checked_arguments(std::list<INITLIST*>* args)
{
    INITLIST* arr[3];
    int i = 0;
    // already been validated at three arguments
    for (auto entry : *args)
        arr[i++] = entry;
    for (int i=1; i < 3; i++)
    {
        arr[i]->tp = MakeType(BasicType::pointer_, arr[i]->tp);
        auto exp = arr[i]->exp;
        while (castvalue(exp)) exp = exp->left;
        if (lvalue(exp)) // should always be true
            exp = exp->left;
        arr[i]->exp = exp;
    }
    args->clear();
    for (int i=0; i < 3; i++)
    {
        args->push_back(arr[i]);
        INITLIST* typeval = Allocate<INITLIST>();
        typeval->tp = &stdint;
        typeval->exp = intNode(ExpressionNode::c_i_, ___typeid_val(basetype(basetype(arr[i]->tp)->btp)));
        args->push_back(typeval);
    }    
    return args;
}
static LEXLIST* expression_checked_int(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    Keyword kw = KW(lex);
    lex = getsym();
    if (MATCHKW(lex, Keyword::openpa_))
    {
        FUNCTIONCALL* funcparams = Allocate<FUNCTIONCALL>();
        lex = getArgs(lex, funcsp, funcparams, Keyword::closepa_, true, flags);
        if (validate_checked_args(funcparams->arguments))
        {
             const char *name = nullptr;
             switch(kw)
             {
                 case Keyword::ckdadd_:
                     name = "___ckdadd";
                     break;
                 case Keyword::ckdmul_:
                     name = "___ckdmul";
                     break;
                 case Keyword::ckdsub_:
                     name = "___ckdsub";
                     break;
                 default:
                     break;
             }
             if (name)
             {
                 auto sym = gsearch(name);
                 if (!sym)
                 {
                     diag("expression_checked_int unknown func");
                     *exp = intNode(ExpressionNode::c_i_, 0);
                 }
                 else
                 {
                     sym = sym->tp->syms->front();
                     funcparams->sp = sym;
                     funcparams->functp = sym->tp;
                     funcparams->fcall = varNode(ExpressionNode::pc_, sym);                     
                     funcparams->ascall = true;
                     funcparams->arguments = checked_arguments(funcparams->arguments);
                     *exp = exprNode(ExpressionNode::func_, nullptr, nullptr);
                     (*exp)->v.func = funcparams;
                 }
             }
             else
             {
                 diag("expression_checked_int unknown func2");
                 *exp = intNode(ExpressionNode::c_i_, 0);
             }
        }
        else
        {
            *exp = intNode(ExpressionNode::c_i_, 0);
        }

        *tp = &stdbool;
    }
    else
    {
        needkw(&lex, Keyword::openpa_);
        *tp = &stdint;
        *exp = intNode(ExpressionNode::c_i_, 0);
        errskim(&lex, skim_closepa);
    }
    return lex;
}
static int ___typeid_val(TYPE *tp)
{
    int id;
    switch (tp->type)
    {
        case BasicType::bool_:
        case BasicType::char_:
        case BasicType::signed_char_:
            id = -1;
            break;
        case BasicType::short_:
            id = -2;
            break;
        case BasicType::int_:
            id = -3;
            break;
        case BasicType::long_:
            id = -4;
            break;
        case BasicType::long_long_:
            id = -5;
            break;
        case BasicType::bitint_:
            id = -(0x40000000 | tp->bitintbits);
            break;
        case BasicType::unsigned_bitint_:
            id = 0x40000000 | tp->bitintbits;
            break;
        case BasicType::unsigned_char_:
            id = 1;
            break;
        case BasicType::unsigned_short_:
        case BasicType::wchar_t_:
            id = 2;
            break;
        case BasicType::unsigned_:
            id = 3;
            break;
        case BasicType::unsigned_long_:
            id = 4;
            break;
        case BasicType::unsigned_long_long_:
            id = 5;
            break;
        case BasicType::float_:
            id = 7;
            break;
        case BasicType::double_:
            id = 8;
            break;
        case BasicType::long_double_:
            id = 10;
            break;
        case BasicType::float__imaginary_:
            id = 15;
            break;
        case BasicType::double__imaginary_:
            id = 16;
            break;
        case BasicType::long_double_imaginary_:
            id = 17;
            break;
        case BasicType::float__complex_:
            id = 20;
            break;
        case BasicType::double__complex_:
            id = 21;
            break;
        case BasicType::long_double_complex_:
            id = 22;
            break;
        default:
            id = 100000;
            break;
    }
    return id;
}
static LEXLIST* expression___typeid(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    lex = getsym();
    if (needkw(&lex, Keyword::openpa_))
    {
        lex = expression_comma(lex, funcsp, nullptr, tp, exp, nullptr, _F_SIZEOF);
        if (!*tp)
        {
            error(ERR_TYPE_NAME_EXPECTED);
            *exp = intNode(ExpressionNode::c_i_, 0);
        }
        else
        {
            TYPE* tp1 = *tp;
            if (isref(tp1))
                *tp = basetype(tp1)->btp;  // DAL fixed
            *exp = intNode(ExpressionNode::c_i_, ___typeid_val(tp1));
        }
        *tp = &stdint;
        needkw(&lex, Keyword::closepa_);
    }
    return lex;
}
static LEXLIST* expression_statement(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
	// gcc extension
     if (expressionStatements.size())
     {
         expressionReturns.push_back(std::pair<EXPRESSION*, TYPE*>(nullptr, nullptr));
         lex = compound(lex, funcsp, *expressionStatements.top(), false);
         std::pair<EXPRESSION*, TYPE*> rv = std::move(expressionReturns.back());
         expressionReturns.pop_back();
         *tp = rv.second;
         *exp = rv.first;
         if (!*tp)
             *tp = &stdvoid;
         if (!*exp)
             *exp = intNode(ExpressionNode::c_i_, 0);
     }
     else
     {
         errskim(&lex, skim_end);
         if (lex)
             needkw(&lex, Keyword::end_);
         *tp = &stdvoid;
         *exp = intNode(ExpressionNode::c_i_, 0);
     }
     return lex;
}

static LEXLIST* expression_primary(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    switch (lex ? lex->data->type : l_none)
    {
        case l_id:
            lex = variableName(lex, funcsp, atp, tp, exp, ismutable, flags);
            break;
        case l_kw:
            switch (KW(lex))
            {
                SYMBOL* sym;
                case Keyword::openbr_:
                    if (Optimizer::cparams.prm_cplusplus)
                        lex = expression_lambda(lex, funcsp, atp, tp, exp, flags);
                    break;
                case Keyword::classsel_:
                case Keyword::operator_:
                case Keyword::decltype_:
                    lex = variableName(lex, funcsp, atp, tp, exp, ismutable, flags);
                    break;
                case Keyword::nullptr_:
                    *exp = intNode(ExpressionNode::nullptr_, 0);
                    *tp = &stdnullpointer;
                    lex = getsym();
                    break;
                case Keyword::this_:
                    if (lambdas.size())
                    {
                        lambda_capture(nullptr, cmThis, false);
                        if (lambdas.front()->captureThis)
                        {
                            SYMBOL* ths = search(lambdas.front()->cls->tp->syms, "$this");
                            if (ths)
                            {
                                *tp = MakeType(BasicType::pointer_, basetype(lambdas.front()->lthis->tp)->btp);
                                *exp = varNode(ExpressionNode::auto_, (SYMBOL*)basetype(funcsp->tp)->syms->front());  // this ptr
                                deref(&stdpointer, exp);
                                *exp = exprNode(ExpressionNode::structadd_, *exp, intNode(ExpressionNode::c_i_, ths->sb->offset));
                                deref(&stdpointer, exp);
                            }
                            else
                            {
                                diag("expression_primary: missing lambda this");
                            }
                        }
                        else
                        {
                            *exp = intNode(ExpressionNode::c_i_, 0);
                            *tp = &stdint;
                        }
                    }
                    else if (getStructureDeclaration() && funcsp && funcsp->sb->parentClass)
                    {
                        getThisType(funcsp, tp);
                        *exp = varNode(ExpressionNode::auto_, (SYMBOL*)basetype(funcsp->tp)->syms->front());  // this ptr
                        deref(&stdpointer, exp);
                    }
                    else
                    {
                        *exp = intNode(ExpressionNode::c_i_, 0);
                        *tp = &stdint;
                        error(ERR_THIS_MEMBER_FUNC);
                    }
                    lex = getsym();
                    break;
                case Keyword::I_:
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    (*exp)->type = ExpressionNode::c_fi_;
                    (*exp)->v.f = Allocate<FPF>();
                    *(*exp)->v.f = (long long)1;
                    *tp = &stdfloatimaginary;
                    (*exp)->pragmas = preProcessor->GetStdPragmas();
                    lex = getsym();
                    break;
                case Keyword::offsetof_:
                    lex = expression_offsetof(lex, funcsp, tp, exp, ismutable, flags);
                    break;
                case Keyword::volatile__:
                    lex = getsym();
                    lex = expression_no_comma(lex, funcsp, nullptr, tp, exp, nullptr, 0);
                    *exp = GetSymRef(*exp);
                    if (!*exp)
                    {
                        error(ERR_IDENTIFIER_EXPECTED);
                        *exp = intNode(ExpressionNode::c_i_, 0);
                    }
                    else
                    {
                        if (!isvolatile((*exp)->v.sp->tp))
                        {
                            (*exp)->v.sp->tp = MakeType(BasicType::volatile_, (*exp)->v.sp->tp);
                        }
                    }
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    *tp = &stdvoid;
                    break;
                case Keyword::land_:
                    // computed goto: take the address of a label
                    lex = getsym();
                    if (!ISID(lex))
                    {
                        *exp = intNode(ExpressionNode::c_i_, 0);
                        error(ERR_IDENTIFIER_EXPECTED);
                    }
                    else
                    {
                        *exp = intNode(ExpressionNode::labcon_, GetLabelValue(lex, nullptr, nullptr));
                        (*exp)->computedLabel = true;
                        lex = getsym();
                    }
                    *tp = &stdpointer;
                    break;
                case Keyword::true_:
                    lex = getsym();
                    *exp = intNode(ExpressionNode::c_i_, 1);
                    (*exp)->type = ExpressionNode::c_bool_;
                    *tp = &stdbool;
                    break;
                case Keyword::false_:
                    lex = getsym();
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    (*exp)->type = ExpressionNode::c_bool_;
                    *tp = &stdbool;
                    return lex;
                case Keyword::alloca_:
                    lex = expression_alloca(lex, funcsp, tp, exp, flags);
                    return lex;
                case Keyword::initblk_:
                case Keyword::cpblk_:
                    lex = expression_msilfunc(lex, funcsp, tp, exp, flags);
                    break;
                case Keyword::openpa_: {
                    lex = getsym();
                    if (MATCHKW(lex, Keyword::begin_))
                    {
                        lex = expression_statement(lex, funcsp, nullptr, tp, exp, ismutable, flags);
                        needkw(&lex, Keyword::closepa_);
                        break;
                    }
                    else
                    {
                        LEXLIST* start = lex;
                        lex = expression_comma(lex, funcsp, nullptr, tp, exp, ismutable, flags & ~(_F_INTEMPLATEPARAMS | _F_SELECTOR));
                        if (!*tp)
                            error(ERR_EXPRESSION_SYNTAX);
                        if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::ellipse_))
                        {
                            // lose p
                            lex = getsym();
                            if (!templateNestingCount && *exp && (*exp)->type != ExpressionNode::packedempty_)
                            {
                                if (!isstructured(*tp) && !(*tp)->templateParam)
                                    checkPackedExpression(*exp);
                                // this is going to presume that the expression involved
                                // is not too long to be cached by the LEXLIST mechanism.
                                int oldPack = packIndex;
                                int count = 0;
                                SYMBOL* arg[200];
                                GatherPackedVars(&count, arg, *exp);
                                expandingParams++;
                                if (count)
                                {
                                    *exp = nullptr;
                                    EXPRESSION** next = exp;
                                    int i;
                                    int n = CountPacks(arg[0]->tp->templateParam->second->byPack.pack);
                                    for (i = 0; i < n; i++)
                                    {
                                        INITLIST* p = Allocate<INITLIST>();
                                        LEXLIST* lex = SetAlternateLex(start);
                                        TYPE* tp1;
                                        EXPRESSION* exp1;
                                        packIndex = i;
                                        expression_comma(lex, funcsp, nullptr, &tp1, &exp1, nullptr, _F_PACKABLE);
                                        SetAlternateLex(nullptr);
                                        if (tp1)
                                        {
                                            if (!*next)
                                            {
                                                *next = exp1;
                                            }
                                            else
                                            {
                                                *next = exprNode(ExpressionNode::void_, *next, exp1);
                                                next = &(*next)->right;
                                            }
                                        }
                                    }
                                }
                                expandingParams--;
                                packIndex = oldPack;
                            }
                        }
                        else
                        {
                            if (argumentNesting <= 1)
                                checkUnpackedExpression(*exp);
                        }
                        needkw(&lex, Keyword::closepa_);
                        break;
                    }
                }  
                case Keyword::func_:
                    *tp = &std__func__;
                    if (!funcsp->sb->__func__label || Optimizer::msilstrings)
                    {
                        LCHAR buf[256], *q = buf;
                        const char* p = funcsp->name;
                        STRING* string;
                        string = Allocate<STRING>();
                        string->strtype = l_astr;
                        string->size = 1;
                        string->pointers = Allocate<Optimizer::SLCHAR*>();
                        while (*p)
                            *q++ = *p++;
                        *q = 0;
                        string->pointers[0] = Allocate<Optimizer::SLCHAR>();
                        string->pointers[0]->str = wlitlate(buf);
                        string->pointers[0]->count = q - buf;
                        *exp = stringlit(string);
                        funcsp->sb->__func__label = string->label;
                    }
                    else
                    {
                        *exp = intNode(ExpressionNode::labcon_, funcsp->sb->__func__label);
                        (*exp)->altdata = intNode(ExpressionNode::c_i_, l_astr);
                    }
                    lex = getsym();
                    break;
                case Keyword::uuidof_:
                    lex = getsym();
                    needkw(&lex, Keyword::openpa_);
                    if (ISID(lex))
                    {
                        sym = tsearch(lex->data->value.s.a);
                        lex = getsym();
                    }
                    else
                    {
                        TYPE* tp1;
                        EXPRESSION* exp1;
                        lex = expression_no_comma(lex, funcsp, nullptr, &tp1, &exp1, nullptr, 0);
                        if (tp1 && isstructured(tp1))
                        {
                            sym = basetype(tp1)->sp;
                        }
                        else
                        {
                            sym = nullptr;
                        }
                    }
                    needkw(&lex, Keyword::closepa_);
                    *exp = GetUUIDData(sym);
                    *tp = &stdpointer;
                    break;
                case Keyword::va_typeof_:
                    lex = getsym();
                    if (MATCHKW(lex, Keyword::openpa_))
                    {
                        lex = getsym();
                        if (startOfType(lex, nullptr, false))
                        {
                            SYMBOL* sym;
                            lex = get_type_id(lex, tp, funcsp, StorageClass::cast_, false, true, false);
                            (*tp)->used = true;
                            needkw(&lex, Keyword::closepa_);
                            // don't enter in table, this is purely so we can cache the type info
                            sym = makeID(StorageClass::auto_, *tp, nullptr, AnonymousName());
                            sym->sb->va_typeof = true;
                            *exp = varNode(ExpressionNode::auto_, sym);
                            break;
                        }
                    }
                    error(ERR_TYPE_NAME_EXPECTED);
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    *tp = &stdint;
                    break;
                case Keyword::typeid__:
                    lex = expression___typeid(lex, funcsp, tp, exp);
                    break;
                case Keyword::has_c_attribute_:
                     lex = getsym();
                     *tp = &stdint;
                     *exp = intNode(ExpressionNode::c_i_, 0);
                     if (needkw(&lex, Keyword::openpa_))
                     {
                         if (ISID(lex))
                         {
                             (*exp)->v.i = cattributes[lex->data->value.s.a];
                             lex = getsym();
                         }
                         else
                         {
                             error(ERR_IDENTIFIER_EXPECTED);
                         }
                         needkw(&lex, Keyword::closepa_);
                     }
                     break;
                case Keyword::D0_:
                case Keyword::D1_:
                case Keyword::D2_:
                case Keyword::D3_:
                case Keyword::D4_:
                case Keyword::D5_:
                case Keyword::D6_:
                case Keyword::D7_:
                case Keyword::A0_:
                case Keyword::A1_:
                case Keyword::A2_:
                case Keyword::A3_:
                case Keyword::A4_:
                case Keyword::A5_:
                case Keyword::A6_:
                case Keyword::A7_:
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    *tp = &stdint;
                    lex = getsym();
                    break;
                case Keyword::NAN_:
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    (*exp)->type = ExpressionNode::c_f_;
                    (*exp)->v.f = Allocate<FPF>();
                    (*exp)->v.f->SetNaN();
                    lex = getsym();
                    *tp = &stdfloat;
                    break;
                case Keyword::INF_:
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    (*exp)->type = ExpressionNode::c_f_;
                    (*exp)->v.f = Allocate<FPF>();
                    (*exp)->v.f->SetInfinity(0);
                    lex = getsym();
                    *tp = &stdfloat;
                    break;
                case Keyword::generic_:
                    lex = expression_generic(lex, funcsp, tp, exp, flags);
                    break;
                case Keyword::atomic_addftch_:
                case Keyword::atomic_subftch_:
                case Keyword::atomic_andftch_:
                case Keyword::atomic_xorftch_:
                case Keyword::atomic_orftch_:
                case Keyword::atomic_cmpxchg_n_:
                case Keyword::atomic_flag_test_set_:
                case Keyword::atomic_flag_clear_:
                case Keyword::atomic_kill_dependency_:
                case Keyword::c11_atomic_is_lock_free_:
                case Keyword::c11_atomic_load_:
                case Keyword::c11_atomic_store_:
                case Keyword::c11_atomic_cmpxchg_strong_:
                case Keyword::c11_atomic_cmpxchg_weak_:
                case Keyword::c11_atomic_ftchadd_:
                case Keyword::c11_atomic_ftchsub_:
                case Keyword::c11_atomic_xchg_:
                case Keyword::c11_atomic_ftchand_:
                case Keyword::c11_atomic_ftchor_:
                case Keyword::c11_atomic_ftchxor_:
                case Keyword::c11_atomic_init_:
                case Keyword::c11_atomic_signal_fence_:
                case Keyword::c11_atomic_thread_fence_:
                    lex = expression_atomic_func(lex, funcsp, tp, exp, flags);
                    break;
                case Keyword::ckdadd_:
                case Keyword::ckdsub_:
                case Keyword::ckdmul_:
                    lex = expression_checked_int(lex, funcsp, tp, exp, flags);
                    break;
                case Keyword::typename_:
                    *tp = nullptr;
                    lex = expression_func_type_cast(lex, funcsp, tp, exp, flags);
                    break;
                default:
                    /*					errorstr(ERR_UNEXPECTED_KEYWORD, lex->data->kw->name); */
                    *tp = nullptr;
                    *exp = intNode(ExpressionNode::c_i_, 0);
                    lex = getsym();
                    break;
            }
            break;
        case l_i:
            *exp = intNode(ExpressionNode::c_i_, lex->data->value.i);
            *tp = &stdint;
            lex = getsym();
            break;
        case l_ui:
            *exp = intNode(ExpressionNode::c_ui_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_ui_;
            *tp = &stdunsigned;
            lex = getsym();
            break;
        case l_l:
            *exp = intNode(ExpressionNode::c_l_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_l_;
            *tp = &stdlong;
            lex = getsym();
            break;
        case l_ul:
            *exp = intNode(ExpressionNode::c_ul_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_ul_;
            *tp = &stdunsignedlong;
            lex = getsym();
            break;
        case l_ll:
            *exp = intNode(ExpressionNode::c_ll_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_ll_;
            *tp = &stdlonglong;
            lex = getsym();
            break;
        case l_ull:
            if (!lex->data->suffix || !getSuffixedNumber(lex, funcsp, tp, exp))
            {
                *exp = intNode(ExpressionNode::c_ull_, lex->data->value.i);
                (*exp)->type = ExpressionNode::c_ull_;
                *tp = &stdunsignedlonglong;
            }
            lex = getsym();
            break;
        case l_bitint:
            *exp = exprNode(ExpressionNode::c_bitint_, nullptr, nullptr);
            (*exp)->v.b.bits = lex->data->value.b.bits;
            (*exp)->v.b.value = lex->data->value.b.value;
            *tp = MakeType(BasicType::bitint_);
            (*tp)->bitintbits = lex->data->value.b.bits;
            (*tp)->size = lex->data->value.b.bits + Optimizer::chosenAssembler->arch->bitintunderlying - 1;
            (*tp)->size /= Optimizer::chosenAssembler->arch->bitintunderlying;
            (*tp)->size *= Optimizer::chosenAssembler->arch->bitintunderlying;
            (*tp)->size /= CHAR_BIT;
            lex = getsym();
            break;
        case l_ubitint:
            *exp = exprNode(ExpressionNode::c_ubitint_, nullptr, nullptr);
            (*exp)->v.b.bits = lex->data->value.b.bits;
            (*exp)->v.b.value = lex->data->value.b.value;
            *tp = MakeType(BasicType::unsigned_bitint_);
            (*tp)->bitintbits = lex->data->value.b.bits;
            (*tp)->size = lex->data->value.b.bits + Optimizer::chosenAssembler->arch->bitintunderlying - 1;
            (*tp)->size /= Optimizer::chosenAssembler->arch->bitintunderlying;
            (*tp)->size *= Optimizer::chosenAssembler->arch->bitintunderlying;
            (*tp)->size /= CHAR_BIT;
            lex = getsym();
            break;
        case l_f:
            *exp = intNode(ExpressionNode::c_f_, 0);
            (*exp)->v.f = Allocate<FPF>();
            *(*exp)->v.f = *lex->data->value.f;
            (*exp)->pragmas = preProcessor->GetStdPragmas();
            *tp = &stdfloat;
            lex = getsym();
            break;
        case l_d:
            *exp = intNode(ExpressionNode::c_d_, 0);
            (*exp)->v.f = Allocate<FPF>();
            *(*exp)->v.f = *lex->data->value.f;
            (*exp)->pragmas = preProcessor->GetStdPragmas();
            *tp = &stddouble;
            lex = getsym();
            break;
        case l_ld:
            if (!lex->data->suffix || !getSuffixedNumber(lex, funcsp, tp, exp))
            {
                *exp = intNode(ExpressionNode::c_ld_, 0);
                (*exp)->v.f = Allocate<FPF>();
                *(*exp)->v.f = *lex->data->value.f;
                (*exp)->pragmas = preProcessor->GetStdPragmas();
                *tp = &stdlongdouble;
            }
            lex = getsym();
            break;
        case l_astr:
        case l_wstr:
        case l_ustr:
        case l_Ustr:
        case l_msilstr:
            lex = expression_string(lex, funcsp, tp, exp);
            break;
        case l_wchr:
            *exp = intNode(ExpressionNode::c_wc_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_wc_;
            *tp = &stdwidechar;
            if (lex->data->suffix)
                getSuffixedChar(lex, funcsp, tp, exp);
            lex = getsym();
            break;
        case l_achr:
            *exp = intNode(ExpressionNode::c_c_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_c_;
            *tp = &stdchar;
            if (lex->data->suffix)
                getSuffixedChar(lex, funcsp, tp, exp);
            lex = getsym();
            break;
        case l_uchr:
            *exp = intNode(ExpressionNode::c_u16_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_u16_;
            *tp = stdchar16tptr.btp;
            if (lex->data->suffix)
                getSuffixedChar(lex, funcsp, tp, exp);
            lex = getsym();
            break;
        case l_Uchr:
            *exp = intNode(ExpressionNode::c_u32_, lex->data->value.i);
            (*exp)->type = ExpressionNode::c_u32_;
            *tp = stdchar32tptr.btp;
            if (lex->data->suffix)
                getSuffixedChar(lex, funcsp, tp, exp);
            lex = getsym();
            break;
        default:
            *tp = nullptr;
            *exp = intNode(ExpressionNode::c_i_, 0);
            break;
    }
    return lex;
}
static EXPRESSION* nodeSizeof(TYPE* tp, EXPRESSION* exp, int flags)
{
    EXPRESSION* exp_in = exp;
    tp = PerformDeferredInitialization(basetype(tp), nullptr);
    if (isstructured(tp))
        tp = basetype(tp)->sp->tp;
    if (isref(tp))
        tp = basetype(tp)->btp;
    tp = basetype(tp);
    if (exp)
    {
        while (castvalue(exp))
            exp = exp->left;
        if (lvalue(exp) && exp->left->type == ExpressionNode::bits_)
            error(ERR_SIZEOF_BITFIELD);
    }
    if (Optimizer::cparams.prm_cplusplus && tp->type == BasicType::enum_ && !tp->fixed)
        error(ERR_SIZEOF_UNFIXED_ENUMERATION);
    if (isfunction(tp))
        error(ERR_SIZEOF_NO_FUNCTION);
    if (Optimizer::cparams.prm_cplusplus && tp->size == 0 && !templateNestingCount)
        errortype(ERR_UNSIZED_TYPE, tp, tp); /* second will be ignored in this case */
    /* this tosses exp...  sizeof expressions don't get evaluated at run time */
    /* unless they are size of a vla... */
    if (tp->vla)
    {
        exp = tp->esize;
        tp = basetype(tp)->btp;

        while (tp->type == BasicType::pointer_)
        {
            exp = exprNode(ExpressionNode::mul_, exp, tp->esize);
            tp = basetype(tp)->btp;
        }
        exp = exprNode(ExpressionNode::mul_, exp, intNode(ExpressionNode::c_i_, tp->size));
    }
    else if (!isarray(tp) && Optimizer::architecture == ARCHITECTURE_MSIL)
    {
        exp = exprNode(ExpressionNode::sizeof_, typeNode(tp), nullptr);
    }
    else
    {
        exp = nullptr;
        if (isstructured(tp))
        {
            if (basetype(tp)->size == 0 && !templateNestingCount)
                errorsym(ERR_UNSIZED_TYPE, basetype(tp)->sp);
            if (basetype(tp)->syms)
            {
                SYMBOL* cache = nullptr;
                TYPE* tpx;
                for (auto sym : *basetype(tp)->syms)
                {
                    if (ismemberdata(sym))
                        cache = sym;
                }
                if (cache)
                {
                    tpx = basetype(cache->tp);
                    if (tpx->size == 0) /* if the last element of a structure is unsized */
                                        /* sizeof doesn't add the size of the padding element */
                        exp = intNode(ExpressionNode::c_i_, cache->sb->offset);
                }
            }
        }
    }
    if (!exp)
    {
        // array which is an argument has different sizeof requirements
        if ((flags & _F_SIZEOF) && tp->type == BasicType::pointer_ && tp->array && exp_in && exp_in->type == ExpressionNode::l_p_ &&
            exp_in->left->type == ExpressionNode::auto_ && exp_in->left->v.sp->sb->storage_class == StorageClass::parameter_)
            exp = intNode(ExpressionNode::c_i_, getSize(BasicType::pointer_));
        else
            exp = intNode(ExpressionNode::c_i_, tp->size);
    }
    return exp;
}
static int widelen(LCHAR* s)
{
    int rv = 0;
    while (*s++)
        rv++;
    return rv;
}
static LEXLIST* expression_sizeof(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    int paren = false;
    *exp = nullptr;
    lex = getsym();
    if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::ellipse_))
    {
        lex = getsym();
        if (MATCHKW(lex, Keyword::openpa_))
        {
            paren = true;
            lex = getsym();
        }
        if (!ISID(lex))
        {
            error(ERR_SIZEOFELLIPSE_NEEDS_TEMPLATE_PACK);
            errskim(&lex, skim_closepa);
        }
        else
        {
            EXPRESSION* exp1 = nullptr;
            lex = variableName(lex, funcsp, nullptr, tp, &exp1, nullptr, _F_PACKABLE | _F_SIZEOF);
            if (!exp1 || !exp1->v.sp->tp->templateParam || !exp1->v.sp->tp->templateParam->second->packed)
            {
                *tp = &stdunsigned;
                *exp = intNode(ExpressionNode::c_i_, 1);
            }
            else if (templateNestingCount)
            {
                *exp = intNode(ExpressionNode::sizeofellipse_, 0);
                (*exp)->v.templateParam = (*tp)->templateParam;
                *tp = &stdunsigned;
            }
            else if (!(*tp)->templateParam->second->packed)
            {
                *tp = &stdunsigned;
                *exp = intNode(ExpressionNode::c_i_, 1);
            }
            else
            {
                int n = 0;
                if ((*tp)->templateParam->second->byPack.pack)
                    n = (*tp)->templateParam->second->byPack.pack->size();
                *tp = &stdunsigned;
                *exp = intNode(ExpressionNode::c_i_, n);
            }
        }
        if (paren)
            needkw(&lex, Keyword::closepa_);
    }
    else
    {
        if (MATCHKW(lex, Keyword::openpa_))
        {
            paren = true;
            lex = getsym();
        }
        if (!startOfType(lex, nullptr, false))
        {
            if (paren)
            {
                lex = expression_comma(lex, funcsp, nullptr, tp, exp, nullptr, _F_SIZEOF);
                needkw(&lex, Keyword::closepa_);
            }
            else
            {
                lex = expression_unary(lex, funcsp, nullptr, tp, exp, nullptr, _F_SIZEOF);
            }
            ResolveTemplateVariable(tp, exp, &stdint, nullptr);
            if (!*tp)
            {
                *exp = intNode(ExpressionNode::c_i_, 1);
                error(ERR_EXPRESSION_SYNTAX);
            }
            else
            {
                *exp = nodeSizeof(*tp, *exp, _F_SIZEOF);
            }
        }
        else
        {
            LEXLIST* prev = lex;
            lex = get_type_id(lex, tp, funcsp, StorageClass::cast_, Optimizer::cparams.prm_cplusplus, true, false);
            if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::openpa_))
            {
                lex = getsym();
                if (MATCHKW(lex, Keyword::star_) || MATCHKW(lex, Keyword::cdecl_) || MATCHKW(lex, Keyword::stdcall_))
                {
                    lex = prevsym(prev);
                    lex = get_type_id(lex, tp, funcsp, StorageClass::cast_, false, true, false);
                }
                else
                {
                    lex = prevsym(prev);
                    lex = expression_func_type_cast(lex, funcsp, tp, exp, 0);
                }
            }
            if (paren)
                needkw(&lex, Keyword::closepa_);
            if (MATCHKW(lex, Keyword::ellipse_))
            {
                lex = getsym();
                if ((*tp)->type == BasicType::templateparam_)
                {
                    if (!(*tp)->templateParam->second->packed)
                    {
                        error(ERR_PACK_SPECIFIER_REQUIRES_PACKED_TEMPLATE_PARAMETER);
                    }
                    else if (packIndex != -1 && (*tp)->templateParam->second->byPack.pack)
                    {
                        auto tpl = (*tp)->templateParam->second->byPack.pack->begin();
                        auto tple = (*tp)->templateParam->second->byPack.pack->end();
                        int i;
                        for (i = 0; tpl != tple && i < packIndex; i++, ++tpl);
                        if (tpl != tple)
                            *tp = tpl->second->byClass.val;
                    }
                }
                else
                {
                    error(ERR_PACK_SPECIFIER_NOT_ALLOWED_HERE);
                }
            }
            if (!*tp)
            {
                *exp = intNode(ExpressionNode::c_i_, 1);
            }
            else
            {
                checkauto(*tp, ERR_AUTO_NOT_ALLOWED);
                *exp = nodeSizeof(*tp, *exp, _F_SIZEOF);
            }
        }

        *tp = &stdint; /* other compilers use sizeof as a signed value so we do too... */
    }
    return lex;
}
static LEXLIST* expression_alignof(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    lex = getsym();
    if (needkw(&lex, Keyword::openpa_))
    {
        if (startOfType(lex, nullptr, false))
        {
            lex = get_type_id(lex, tp, funcsp, StorageClass::cast_, false, true, false);
        }
        else
        {
            EXPRESSION* exp = nullptr;
            lex = expression(lex, funcsp, nullptr, tp, &exp, 0);
        }
        needkw(&lex, Keyword::closepa_);
        if (MATCHKW(lex, Keyword::ellipse_))
        {
            lex = getsym();
            if ((*tp)->type == BasicType::templateparam_)
            {
                if (!(*tp)->templateParam->second->packed)
                {
                    error(ERR_PACK_SPECIFIER_REQUIRES_PACKED_TEMPLATE_PARAMETER);
                }
                else if (packIndex != -1)
                {
                    auto tpl = (*tp)->templateParam->second->byPack.pack->begin();
                    auto tple = (*tp)->templateParam->second->byPack.pack->end();
                    int i;
                    for (i = 0; tpl != tple && i < packIndex; i++, ++tpl);
                    if (tpl != tple)
                        *tp = tpl->second->byClass.val;
                }
            }
            else
            {
                error(ERR_PACK_SPECIFIER_NOT_ALLOWED_HERE);
            }
        }
        if (!*tp)
        {
            *exp = intNode(ExpressionNode::c_i_, 1);
        }
        else
        {
            TYPE* itp = *tp;
            checkauto(itp, ERR_AUTO_NOT_ALLOWED);
            if (isref(itp))
                itp = (basetype(itp)->btp);
            itp = basetype(itp);

            *exp = intNode(ExpressionNode::c_i_, getAlign(StorageClass::global_, itp));
        }
    }
    *tp = &stdint;
    return lex;
}
static LEXLIST* expression_ampersand(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, int flags)
{
    lex = getsym();
    lex = expression_cast(lex, funcsp, atp, tp, exp, nullptr, (flags) | _F_AMPERSAND);
    if (*tp)
    {
        TYPE *btp, *tp1;
        EXPRESSION *exp1 = *exp, *symRef;
        while (exp1->type == ExpressionNode::void_ && exp1->right)
            exp1 = exp1->right;
        if (exp1->type == ExpressionNode::void_)
            exp1 = exp1->left;
        symRef = (Optimizer::architecture == ARCHITECTURE_MSIL) ? GetSymRef(exp1) : nullptr;
        btp = basetype(*tp);
        tp1 = LookupSingleAggregate(btp, &exp1, true);
        if ((isstructuredmath(*tp) || Optimizer::architecture == ARCHITECTURE_MSIL) &&
            insertOperatorFunc(ovcl_unary_any, Keyword::unary_and_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
        {
            return lex;
        }
        else if (isvoid(*tp) || ismsil(*tp))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if (btp->hasbits)
            error(ERR_CANNOT_TAKE_ADDRESS_OF_BIT_FIELD);
        else if (btp->msil)
            error(ERR_MANAGED_OBJECT_NO_ADDRESS);
        else if (symRef && symRef->type != ExpressionNode::labcon_ && symRef->v.sp->sb->attribs.inheritable.linkage2 == Linkage::property_)
            errorsym(ERR_CANNOT_TAKE_ADDRESS_OF_PROPERTY, symRef->v.sp);
        else if (inreg(*exp, true))
            error(ERR_CANNOT_TAKE_ADDRESS_OF_REGISTER);
        else if ((!ispointer(btp) || !(btp)->array) && !isstructured(btp) && !isfunction(btp) && btp->type != BasicType::aggregate_ &&
                 (exp1)->type != ExpressionNode::memberptr_)
        {
            if ((exp1)->type != ExpressionNode::const_ && exp1->type != ExpressionNode::assign_)
                if (!lvalue(exp1))
                    if (Optimizer::cparams.prm_ansi || !castvalue(exp1))
                        error(ERR_MUST_TAKE_ADDRESS_OF_MEMORY_LOCATION);
            if (lvalue(exp1))
            {
                if (exp1->left->type == ExpressionNode::structadd_ && isconstzero(&stdint, exp1->left->right) && exp1->left->left->type == ExpressionNode::l_ref_ && exp1->left->left->left->type == ExpressionNode::auto_)
                {
                    if (exp1->left->left->left->v.sp->sb->storage_class == StorageClass::parameter_)
                        exp1->left->left->left->v.sp->sb->addressTaken = true;
                }
            }
        }
        else
            switch ((exp1)->type)
            {
                case ExpressionNode::pc_:
                case ExpressionNode::auto_:
                case ExpressionNode::global_:
                case ExpressionNode::absolute_:
                case ExpressionNode::threadlocal_: {
                    SYMBOL* sym = exp1->v.sp;
                    if (sym->sb->isConstructor || sym->sb->isDestructor)
                        error(ERR_CANNOT_TAKE_ADDRESS_OF_CONSTRUCTOR_OR_DESTRUCTOR);
                    break;
                }
                default:
                    break;
            }
        if (exp1->type == ExpressionNode::memberptr_)
        {
            *tp = tp1;
            *exp = exp1;
        }
        else if ((*exp)->type == ExpressionNode::const_)
        {
            /* if a variable propagated silently to an inline constant
             * this will restore it as a static variable in the const section
             * when we take its address
             * but the constant will still be used inline when possible
             */
            SYMBOL* sym = (*exp)->v.sp;
            if (!sym->sb->indecltable)
            {
                if (!sym->sb->label)
                {
                    TYPE *tp, *tpb, *tpn, **tpnp = &tpn;
                    bool done = false;
                    SYMBOL* spold = sym;
                    sym->sb->label = Optimizer::nextLabel++;
                    sym = CopySymbol(sym);
                    spold->sb->indecltable = true;
                    tp = sym->tp;
                    tpb = basetype(tp);
                    do
                    {
                        *tpnp = CopyType(tp);
                        UpdateRootTypes(*tpnp);
                        tpnp = &(*tpnp)->btp;
                        if (tp != tpb)
                            tp = tp->btp;
                        else
                            done = true;
                    } while (!done);
                    sym->tp = tpn;
                    sym->sb->storage_class = StorageClass::localstatic_;
                    SetLinkerNames(sym, Linkage::cdecl_);
                    Optimizer::SymbolManager::Get(sym)->storage_class = Optimizer::scc_localstatic;
                    insertInitSym(sym);
                }
                else
                {
                    insertInitSym(sym);
                }
                if (!sym->sb->parent)
                    sym->sb->parent = funcsp;  // this promotion of a global to local is necessary to not make it linkable
                *exp = varNode(ExpressionNode::global_, sym);
            }
            else
            {
                *exp = varNode(ExpressionNode::global_, sym);
            }
            *tp = MakeType(BasicType::pointer_, *tp);
        }
        else if (!isfunction(*tp) && (*tp)->type != BasicType::aggregate_)
        {
            EXPRESSION *expasn = nullptr, **exp2;
            while (castvalue(exp1))
                exp1 = (exp1)->left;
            if (exp1->type == ExpressionNode::assign_)
            {
                expasn = exp1;
                exp1 = exp1->left;
                while (castvalue(exp1))
                    exp1 = (exp1)->left;
            }
            if (!lvalue(exp1))
            {
                if (!btp->array && !btp->vla && !isstructured(btp) && basetype(btp)->type != BasicType::memberptr_ &&
                    basetype(btp)->type != BasicType::templateparam_)
                    error(ERR_LVALUE);
            }
            else if (!isstructured(btp) && exp1->type != ExpressionNode::l_ref_)
                exp1 = (exp1)->left;
            if (exp1->type == ExpressionNode::auto_)
                SetRuntimeData(lex, exp1, exp1->v.sp);

            switch ((exp1)->type)
            {
                case ExpressionNode::pc_:
                case ExpressionNode::auto_:
                case ExpressionNode::global_:
                case ExpressionNode::absolute_:
                case ExpressionNode::threadlocal_:
                    (exp1)->v.sp->sb->addressTaken = true;
                    break;
                default:
                    break;
            }
            exp2 = exp;
            while ((*exp2)->type == ExpressionNode::void_ && (*exp2)->right)
                exp2 = &(*exp2)->right;
            if ((*exp2)->type == ExpressionNode::void_)
                exp2 = &(*exp2)->left;
            if (basetype(btp)->type != BasicType::memberptr_)
            {
                *tp = MakeType(BasicType::pointer_, *tp);
                if (expasn)
                    *exp2 = exprNode(ExpressionNode::void_, expasn, exp1);
                else
                    *exp2 = exp1;
            }
        }
    }
    return lex;
}
static LEXLIST* expression_deref(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp, int flags)
{
    lex = getsym();
    lex = expression_cast(lex, funcsp, nullptr, tp, exp, nullptr, flags);
    if ((isstructuredmath(*tp) || Optimizer::architecture == ARCHITECTURE_MSIL) &&
        insertOperatorFunc(ovcl_unary_pointer, Keyword::unary_star_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
    {
        return lex;
    }
    if (*tp && (isvoid(*tp) || (*tp)->type == BasicType::aggregate_ || ismsil(*tp)))
    {
        error(ERR_NOT_AN_ALLOWED_TYPE);
    }
    else if (*tp && basetype(*tp)->type == BasicType::memberptr_)
        error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
    if (*tp && !isfunction(*tp) && !isfuncptr(*tp))
    {
        TYPE *btp = basetype(*tp), *btp2, *btp3;
        ;
        if (!ispointer(*tp))
        {
            if (!templateNestingCount || instantiatingTemplate)
                error(ERR_DEREF);
            deref(&stdpointer, exp);
        }
        else
        {
            btp3 = btp->btp;
            btp2 = basetype(btp->btp);
            if (btp2->type == BasicType::void_)
            {
                error(ERR_DEREF);
                deref(&stdpointer, exp);
            }
            else if (isstructured(btp2))
            {
                *tp = btp3;
                *tp = PerformDeferredInitialization(*tp, funcsp);
            }
            else
            {
                if (!isfunction(btp2))
                {
                    *tp = btp3;
                    deref(*tp, exp);
                    if (isvolatile(btp3))
                        (*exp)->isvolatile = true;
                    if (isrestrict(btp3))
                        (*exp)->isrestrict = true;
                    if (isatomic(btp3))
                    {
                        (*exp)->isatomic = true;
                        if (needsAtomicLockFromType(btp3))
                            (*exp)->lockOffset = basetype(btp3)->size;
                    }
                }
            }
        }
    }
    tagNonConst(funcsp, *tp);
    return lex;
}
static LEXLIST* expression_postfix(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    TYPE* oldType;
    bool done = false;
    bool localMutable = false;
    if (KWTYPE(lex, TT_POINTERQUAL | TT_LINKAGE | TT_BASETYPE | TT_STORAGE_CLASS))
    {
        lex = expression_func_type_cast(lex, funcsp, tp, exp, flags);
    }
    else
        switch (KW(lex))
        {
            case Keyword::dynamic_cast_:
                oldType = nullptr;
                lex = GetCastInfo(lex, funcsp, tp, &oldType, exp, (flags & _F_PACKABLE));
                if (*tp && !doDynamicCast(tp, oldType, exp, funcsp))
                    if (!typeHasTemplateArg(*tp))
                        errorConversionOrCast(false, oldType, *tp);
                if (isref(*tp))
                    *tp = basetype(*tp)->btp;
                break;
            case Keyword::static_cast_:
                oldType = nullptr;
                lex = GetCastInfo(lex, funcsp, tp, &oldType, exp, (flags & _F_PACKABLE));
                if (*tp && !doStaticCast(tp, oldType, exp, funcsp, true))
                    if (!typeHasTemplateArg(*tp))
                        errorConversionOrCast(false, oldType, *tp);
                if (isref(*tp))
                    *tp = basetype(*tp)->btp;
                break;
            case Keyword::const_cast_:
                oldType = nullptr;
                lex = GetCastInfo(lex, funcsp, tp, &oldType, exp, (flags & _F_PACKABLE));
                if (*tp && !doConstCast(tp, oldType, exp, funcsp))
                    if (!typeHasTemplateArg(*tp))
                        errorConversionOrCast(false, oldType, *tp);
                if (isref(*tp))
                    *tp = basetype(*tp)->btp;
                break;
            case Keyword::reinterpret_cast_:
                oldType = nullptr;
                lex = GetCastInfo(lex, funcsp, tp, &oldType, exp, (flags & _F_PACKABLE));
                if (*tp && !doReinterpretCast(tp, oldType, exp, funcsp, true))
                    if (!typeHasTemplateArg(*tp))
                        errorConversionOrCast(false, oldType, *tp);
                if (isref(*tp))
                    *tp = basetype(*tp)->btp;
                break;
            case Keyword::typeid_:
                lex = expression_typeid(lex, funcsp, tp, exp, (flags & _F_PACKABLE));
                break;
            default:
                lex = expression_primary(lex, funcsp, atp, tp, exp, &localMutable, flags);
                if (ismutable)
                    *ismutable = localMutable;
                break;
        }
    if (!*tp)
        return lex;
    while (!done && lex && !parsingPreprocessorConstant)
    {
        Keyword kw;
        switch (KW(lex))
        {
            case Keyword::openbr_:
                lex = expression_bracket(lex, funcsp, tp, exp, flags);
                break;
            case Keyword::openpa_:
                lex = expression_arguments(lex, funcsp, tp, exp, flags);
                break;
            case Keyword::pointsto_:
            case Keyword::dot_:
                lex = expression_member(lex, funcsp, tp, exp, ismutable, flags);
                break;
            case Keyword::autoinc_:
            case Keyword::autodec_:
                ConstExprPromote(*exp, false);
                kw = KW(lex);
                lex = getsym();
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_postfix, kw, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        continue;
                    }
                    else
                    {
                        castToArithmetic(false, tp, exp, kw, nullptr, true);
                    }
                }
                if (isconstraw(*tp) && !localMutable)
                    error(ERR_CANNOT_MODIFY_CONST_OBJECT);
                else if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (!lvalue(*exp) && basetype(*tp)->type != BasicType::templateparam_)
                {
                    error(ERR_LVALUE);
                }
                else
                {
                    EXPRESSION *exp3 = nullptr, *exp1 = nullptr;
                    if ((*exp)->left->type == ExpressionNode::func_ || (*exp)->left->type == ExpressionNode::thisref_)
                    {
                        EXPRESSION* exp2 = anonymousVar(StorageClass::auto_, *tp);
                        deref(&stdpointer, &exp2);
                        exp3 = exprNode(ExpressionNode::assign_, exp2, (*exp)->left);
                        deref(*tp, &exp2);
                        *exp = exp2;
                    }
                    if (basetype(*tp)->type == BasicType::pointer_)
                    {
                        TYPE* btp = basetype(*tp)->btp;
                        if (basetype(btp)->type == BasicType::void_)
                        {
                            if (Optimizer::cparams.prm_cplusplus)
                                error(ERR_ARITHMETIC_WITH_VOID_STAR);
                            exp1 = nodeSizeof(&stdchar, *exp);
                        }
                        else
                        {
                            exp1 = nodeSizeof(btp, *exp);
                        }
                    }
                    else
                    {
                        if (isvoid(*tp) || (*tp)->type == BasicType::aggregate_ || ismsil(*tp))
                            error(ERR_NOT_AN_ALLOWED_TYPE);
                        if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                            error(ERR_SCOPED_TYPE_MISMATCH);
                        if (basetype(*tp)->type == BasicType::memberptr_)
                            error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                        exp1 = intNode(ExpressionNode::c_i_, 1);
                    }
                    if (basetype(*tp)->type == BasicType::bool_)
                    {
                        /* Keyword::autoinc_ of a bool sets it true.  Keyword::autodec_ not allowed in C++
                            * these aren't spelled out in the C99 standard, we are
                            * doing the normal thing here...
                            */
                        if (kw == Keyword::autodec_)
                        {
                            if (Optimizer::cparams.prm_cplusplus)
                                error(ERR_CANNOT_USE_bool_HERE);
                            *exp = exprNode(ExpressionNode::assign_, *exp, intNode(ExpressionNode::c_bool_, 0));
                        }
                        else
                        {
                            *exp = exprNode(ExpressionNode::assign_, *exp, intNode(ExpressionNode::c_bool_, 1));
                        }
                    }
                    else
                    {
                        if (ispointer(*tp))
                            cast(&stdint, &exp1);
                        else
                            cast(*tp, &exp1);
                        *exp = exprNode(kw == Keyword::autoinc_ ? ExpressionNode::auto_inc_ : ExpressionNode::auto_dec_, *exp, exp1);
                    }
                    if (exp3)
                        *exp = exprNode(ExpressionNode::void_, exp3, *exp);
                    while (lvalue(exp1))
                        exp1 = exp1->left;
                    if (exp1->type == ExpressionNode::auto_)
                        exp1->v.sp->sb->altered = true;
                }
                break;
            default:
                done = true;
                break;
        }
    }
    /*	if (isfunction(*tp))
            error(ERR_FUNCTION_ARGUMENTS_EXPECTED);
    */
    return lex;
}
LEXLIST* expression_unary(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    bool localMutable = false;
    Keyword kw = KW(lex);
    /* note some of the math ops are speced to do integer promotions
     * if being stored, the proposed place to store them is not known, so e.g.
     * a ~ on a unsigned char would promote to int to be evaluated,
     * the int would be further sign
     * extended to a long long though through the assignment promotion
     */
    switch (kw)
    {
        case Keyword::plus_:
            lex = getsym();
            lex = expression_cast(lex, funcsp, atp, tp, exp, nullptr, flags);
            if (*tp)
            {
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_numeric, Keyword::unary_plus_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        break;
                    }
                    else
                    {
                        LookupSingleAggregate(*tp, exp);
                        castToArithmetic(false, tp, exp, kw, nullptr, true);
                    }
                }
                if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (isvoid(*tp) || ismsil(*tp))
                    error(ERR_NOT_AN_ALLOWED_TYPE);
                else if (basetype(*tp)->type == BasicType::memberptr_)
                    error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                else if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                    error(ERR_SCOPED_TYPE_MISMATCH);
                else if (ispointer(*tp))
                    error(ERR_ILL_POINTER_OPERATION);
                else if (atp && basetype(atp)->type < BasicType::int_)
                {
                    cast(atp, exp);
                    *tp = atp;
                }
                else if (smallint(*tp))
                {
                    cast(&stdint, exp);
                    *tp = &stdint;
                }
            }
            break;
        case Keyword::minus_:
            lex = getsym();
            lex = expression_cast(lex, funcsp, atp, tp, exp, nullptr, flags);
            if (!*tp)
            {
                error(ERR_EXPRESSION_SYNTAX);
                *tp = &stdint;
            }
            LookupSingleAggregate(*tp, exp);
            if (*tp)
            {
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_numeric, Keyword::unary_minus_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        break;
                    }
                    else
                    {
                        castToArithmetic(false, tp, exp, kw, nullptr, true);
                    }
                }
                if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (isvoid(*tp) || ismsil(*tp))
                    error(ERR_NOT_AN_ALLOWED_TYPE);
                else if (basetype(*tp)->type == BasicType::memberptr_)
                    error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                else if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                    error(ERR_SCOPED_TYPE_MISMATCH);
                else if (ispointer(*tp))
                    error(ERR_ILL_POINTER_OPERATION);
                else if (atp && basetype(atp)->type < BasicType::int_)
                {
                    *exp = exprNode(ExpressionNode::uminus_, *exp, nullptr);
                    cast(atp, exp);
                    *tp = atp;
                }
                else if (smallint(*tp))
                {
                    cast(&stdint, exp);
                    *exp = exprNode(ExpressionNode::uminus_, *exp, nullptr);
                    *tp = &stdint;
                }
                else
                {
                    *exp = exprNode(ExpressionNode::uminus_, *exp, nullptr);
                }
            }
            break;
        case Keyword::star_:
            lex = expression_deref(lex, funcsp, tp, exp, flags);
            break;
        case Keyword::and_:
            lex = expression_ampersand(lex, funcsp, atp, tp, exp, flags);
            break;
        case Keyword::not_:
            lex = getsym();
            lex = expression_cast(lex, funcsp, atp, tp, exp, nullptr, flags);
            if (!*tp)
            {
                error(ERR_EXPRESSION_SYNTAX);
                *tp = &stdint;
            }
            LookupSingleAggregate(*tp, exp);
            if (*tp)
            {
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_numericptr, Keyword::not_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        break;
                    }
                    else
                    {
                        castToArithmetic(false, tp, exp, kw, nullptr, false);
                    }
                }
                if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (isvoid(*tp) || ismsil(*tp))
                    error(ERR_NOT_AN_ALLOWED_TYPE);
                else if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                    error(ERR_SCOPED_TYPE_MISMATCH);
                if ((*tp)->type == BasicType::memberptr_)
                {
                    *exp = exprNode(ExpressionNode::mp_as_bool_, *exp, nullptr);
                    (*exp)->size = *tp;
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                }
                else
                {
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                }
                if (Optimizer::cparams.prm_cplusplus)
                    *tp = &stdbool;
                else
                    *tp = &stdint;
            }
            break;
        case Keyword::complx_:
            lex = getsym();
            lex = expression_cast(lex, funcsp, atp, tp, exp, nullptr, flags);
            if (!*tp)
            {
                error(ERR_EXPRESSION_SYNTAX);
                *tp = &stdint;
            }
            LookupSingleAggregate(*tp, exp);
            if (*tp)
            {
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_int, Keyword::complx_, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        break;
                    }
                    else
                    {
                        castToArithmetic(true, tp, exp, kw, nullptr, true);
                    }
                }
                if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (iscomplex(*tp))
                    error(ERR_ILL_USE_OF_COMPLEX);
                else if (isfloat(*tp) || isimaginary(*tp))
                    error(ERR_ILL_USE_OF_FLOATING);
                else if (ispointer(*tp))
                    error(ERR_ILL_POINTER_OPERATION);
                else if (isvoid(*tp) || ismsil(*tp))
                    error(ERR_NOT_AN_ALLOWED_TYPE);
                else if (basetype(*tp)->type == BasicType::memberptr_)
                    error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                else if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                    error(ERR_SCOPED_TYPE_MISMATCH);
                else if (atp && basetype(atp)->type < BasicType::int_)
                {
                    *exp = exprNode(ExpressionNode::compl_, *exp, nullptr);
                    cast(atp, exp);
                    *tp = atp;
                }
                else if (smallint(*tp))
                {
                    cast(&stdint, exp);
                    *exp = exprNode(ExpressionNode::compl_, *exp, nullptr);
                    *tp = &stdint;
                }
                else
                {
                    *exp = exprNode(ExpressionNode::compl_, *exp, nullptr);
                    if (!isbitint(*tp))
                        cast(basetype(*tp), exp);
                }
            }
            break;
        case Keyword::autoinc_:
        case Keyword::autodec_:
            lex = getsym();
            lex = expression_cast(lex, funcsp, atp, tp, exp, &localMutable, flags);
            ConstExprPromote(*exp, false);
            if (*tp)
            {
                if (isstructuredmath(*tp))
                {
                    if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                        insertOperatorFunc(ovcl_unary_prefix, kw, funcsp, tp, exp, nullptr, nullptr, nullptr, flags))
                    {
                        break;
                    }
                    else
                    {
                        castToArithmetic(false, tp, exp, kw, nullptr, true);
                    }
                }
                if (isstructured(*tp))
                    error(ERR_ILL_STRUCTURE_OPERATION);
                else if (iscomplex(*tp))
                    error(ERR_ILL_USE_OF_COMPLEX);
                else if (isconstraw(*tp) && !localMutable)
                    error(ERR_CANNOT_MODIFY_CONST_OBJECT);
                else if (isvoid(*tp) || (*tp)->type == BasicType::aggregate_ || ismsil(*tp))
                    error(ERR_NOT_AN_ALLOWED_TYPE);
                else if (basetype(*tp)->type == BasicType::memberptr_)
                    error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                else if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                    error(ERR_SCOPED_TYPE_MISMATCH);
                else if (!lvalue(*exp))
                {
                    if ((*exp)->type != ExpressionNode::templateparam_ && basetype(*tp)->type != BasicType::templateparam_ &&
                        basetype(*tp)->type != BasicType::templateselector_ && basetype(*tp)->type != BasicType::templatedecltype_)
                        error(ERR_LVALUE);
                }
                else
                {
                    EXPRESSION* exp3 = nullptr;
                    if ((*exp)->left->type == ExpressionNode::func_ || (*exp)->left->type == ExpressionNode::thisref_)
                    {
                        EXPRESSION* exp2 = anonymousVar(StorageClass::auto_, *tp);
                        deref(&stdpointer, &exp2);
                        exp3 = exprNode(ExpressionNode::assign_, exp2, (*exp)->left);
                        deref(*tp, &exp2);
                        *exp = exp2;
                    }
                    if (ispointer(*tp))
                    {
                        TYPE* tpx;
                        if (basetype(basetype(*tp)->btp)->type == BasicType::void_)
                        {
                            if (Optimizer::cparams.prm_cplusplus)
                                error(ERR_ARITHMETIC_WITH_VOID_STAR);
                            tpx = &stdchar;
                        }
                        else
                        {
                            tpx = basetype(*tp)->btp;
                        }
                        *exp =
                            exprNode(ExpressionNode::assign_, *exp, exprNode(kw == Keyword::autoinc_ ? ExpressionNode::add_ : ExpressionNode::sub_, *exp, nodeSizeof(tpx, *exp)));
                        (*exp)->preincdec = true;
                    }
                    else if (kw == Keyword::autoinc_ && basetype(*tp)->type == BasicType::bool_)
                    {
                        *exp = exprNode(ExpressionNode::assign_, *exp, intNode(ExpressionNode::c_i_, 1));  // set to true as per C++
                    }
                    else if (kw == Keyword::autodec_ && basetype(*tp)->type == BasicType::bool_)
                    {
                        if (Optimizer::cparams.prm_cplusplus)
                            error(ERR_CANNOT_USE_bool_HERE);

                        *exp = exprNode(ExpressionNode::assign_, *exp, intNode(ExpressionNode::c_i_, 0));  // c++ doesn't allow it, set it to true for C.
                    }
                    else
                    {
                        EXPRESSION *dest = *exp, *exp1 = intNode(ExpressionNode::c_i_, 1);
                        *exp = RemoveAutoIncDec(*exp);
                        cast(*tp, &exp1);
                        *exp = exprNode(ExpressionNode::assign_, dest, exprNode(kw == Keyword::autoinc_ ? ExpressionNode::add_ : ExpressionNode::sub_, *exp, exp1));
                        (*exp)->preincdec = true;
                    }
                    if (exp3)
                        *exp = exprNode(ExpressionNode::void_, exp3, *exp);
                }
            }
            break;
        case Keyword::sizeof_:
            lex = expression_sizeof(lex, funcsp, tp, exp);
            break;
        case Keyword::alignof_:
            lex = expression_alignof(lex, funcsp, tp, exp);
            break;
        case Keyword::new_:
            lex = expression_new(lex, funcsp, tp, exp, false, flags);
            break;
        case Keyword::delete_:
            lex = expression_delete(lex, funcsp, tp, exp, false, flags);
            break;
        case Keyword::noexcept_:
            lex = expression_noexcept(lex, funcsp, tp, exp);
            break;
        case Keyword::classsel_: {
            LEXLIST* placeholder = lex;
            lex = getsym();
            switch (KW(lex))
            {
                case Keyword::new_:
                    return expression_new(lex, funcsp, tp, exp, true, flags);
                case Keyword::delete_:
                    return expression_delete(lex, funcsp, tp, exp, true, flags);
                default:
                    break;
            }
            lex = prevsym(placeholder);
        }
            // fallthrough
        default:
            lex = expression_postfix(lex, funcsp, atp, tp, exp, ismutable, flags);
            break;
    }
    return lex;
}
LEXLIST* expression_cast(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    TYPE* throwaway = nullptr;
    if (MATCHKW(lex, Keyword::openpa_))
    {
        bool loadedAttribs = false;
        attributes oldAttribs;
        LEXLIST* start = lex;
        lex = getsym();
        if (MATCHKW(lex, Keyword::attribute_))
        {
            loadedAttribs = true;
            oldAttribs = basisAttribs;

            basisAttribs = {0};
            ParseAttributeSpecifiers(&lex, funcsp, true);
        }
        bool structured = false;
        if (startOfType(lex, &structured, false))
        {
            if (!Optimizer::cparams.prm_cplusplus || resolveToDeclaration(lex, structured))
            {
                bool done = false;
                lex = get_type_id(lex, tp, funcsp, StorageClass::cast_, false, true, false);
                if (!*tp)
                {
                    error(ERR_TYPE_NAME_EXPECTED);
                    *tp = &stdint;
                    *exp = intNode(ExpressionNode::c_i_, 0);
                }
                else
                {
                    (*tp)->used = true;
                    needkw(&lex, Keyword::closepa_);
                    checkauto(*tp, ERR_AUTO_NOT_ALLOWED);
                    if (MATCHKW(lex, Keyword::begin_))
                    {
                        std::list<INITIALIZER*>* temp = nullptr;
                        INITIALIZER* init = nullptr;
                        SYMBOL* sym = nullptr;
                        if (Optimizer::cparams.c_dialect < Dialect::c99 && !Optimizer::cparams.prm_cplusplus)
                            error(ERR_C99_STYLE_INITIALIZATION_USED);
                        if (Optimizer::cparams.prm_cplusplus)
                        {
                            sym = makeID(StorageClass::auto_, *tp, nullptr, AnonymousName());
                            localNameSpace->front()->syms->Add(sym);
                        }
                        lex = initType(lex, funcsp, 0, StorageClass::auto_, &temp, nullptr, *tp, sym, false, flags);
                        *exp = convertInitToExpression(*tp, nullptr, nullptr, funcsp, temp, *exp, false);
                        while (!done && lex)
                        {
                            Keyword kw;
                            switch (KW(lex))
                            {
                                case Keyword::openbr_:
                                    lex = expression_bracket(lex, funcsp, tp, exp, flags);
                                    break;
                                case Keyword::openpa_:
                                    lex = expression_arguments(lex, funcsp, tp, exp, flags);
                                    break;
                                case Keyword::pointsto_:
                                case Keyword::dot_:
                                    lex = expression_member(lex, funcsp, tp, exp, ismutable, flags);
                                    break;
                                case Keyword::autoinc_:
                                case Keyword::autodec_:
                                    ConstExprPromote(*exp, false);

                                    kw = KW(lex);
                                    lex = getsym();
                                    if (isstructuredmath(*tp))
                                    {
                                        if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                                            insertOperatorFunc(ovcl_unary_postfix, kw, funcsp, tp, exp, nullptr, nullptr, nullptr,
                                                flags))
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            castToArithmetic(false, tp, exp, kw, nullptr, true);
                                        }
                                    }
                                    if (isstructured(*tp))
                                        error(ERR_ILL_STRUCTURE_OPERATION);
                                    else if (!lvalue(*exp) && basetype(*tp)->type != BasicType::templateparam_)
                                        error(ERR_LVALUE);
                                    else
                                    {
                                        EXPRESSION* exp1 = nullptr;
                                        if (basetype(*tp)->type == BasicType::pointer_)
                                        {
                                            TYPE* btp = basetype(*tp)->btp;
                                            exp1 = nodeSizeof(btp, *exp);
                                        }
                                        else
                                        {
                                            if (isvoid(*tp) || (*tp)->type == BasicType::aggregate_ || ismsil(*tp))
                                                error(ERR_NOT_AN_ALLOWED_TYPE);
                                            if (basetype(*tp)->scoped && !(flags & _F_SCOPEDENUM))
                                                error(ERR_SCOPED_TYPE_MISMATCH);
                                            if (basetype(*tp)->type == BasicType::memberptr_)
                                                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                                            exp1 = intNode(ExpressionNode::c_i_, 1);
                                        }
                                        if (basetype(*tp)->type == BasicType::bool_)
                                        {
                                            /* Keyword::autoinc_ of a bool sets it true.  Keyword::autodec_ not allowed
                                                * these aren't spelled out in the C99 standard, we are
                                                * following the C++ standard here
                                                */
                                            if (kw == Keyword::autodec_)
                                                error(ERR_CANNOT_USE_bool_HERE);
                                            *exp = exprNode(ExpressionNode::assign_, *exp, intNode(ExpressionNode::c_bool_, 1));
                                        }
                                        else
                                        {
                                            cast(*tp, &exp1);
                                            *exp = exprNode(kw == Keyword::autoinc_ ? ExpressionNode::auto_inc_ : ExpressionNode::auto_dec_, *exp, exp1);
                                        }
                                        while (lvalue(exp1))
                                            exp1 = exp1->left;
                                        if (exp1->type == ExpressionNode::auto_)
                                            exp1->v.sp->sb->altered = true;
                                    }
                                    break;
                                default:
                                    done = true;
                                    break;
                            }
                        }
                    }
                    else
                    {
                        LEXLIST* lastSym = lex;
                        lex = expression_cast(lex, funcsp, nullptr, &throwaway, exp, ismutable, flags);
                        if (throwaway)
                        {
                            if ((*exp)->type == ExpressionNode::pc_ || ((*exp)->type == ExpressionNode::func_ && !(*exp)->v.func->ascall))
                                thunkForImportTable(exp);
                            if (basetype(*tp)->type == BasicType::string_)
                            {
                                if ((*exp)->type == ExpressionNode::labcon_ && (*exp)->string)
                                    (*exp)->type = ExpressionNode::c_string_;
                                else if (basetype(throwaway)->type != BasicType::string_)
                                    *exp = exprNode(ExpressionNode::x_string_, *exp, nullptr);
                            }
                            else if (basetype(*tp)->type == BasicType::object_)
                            {
                                if (basetype(throwaway)->type != BasicType::object_)
                                    if (!isstructured(throwaway) && (!isarray(throwaway) || !basetype(throwaway)->msil))
                                        *exp = exprNode(ExpressionNode::x_object_, *exp, nullptr);
                            }
                            else if ((isvoid(throwaway) && !isvoid(*tp)) || ismsil(*tp))
                            {
                                error(ERR_NOT_AN_ALLOWED_TYPE);
                            }
                            else if (!Optimizer::cparams.prm_cplusplus &&
                                     ((isstructured(throwaway) && !isvoid(*tp)) || basetype(throwaway)->type == BasicType::memberptr_ ||
                                      basetype(*tp)->type == BasicType::memberptr_) &&
                                     !comparetypes(throwaway, *tp, true))
                            {
                                error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                            }
                            else if (Optimizer::cparams.prm_cplusplus)
                            {
                                if (!doStaticCast(tp, throwaway, exp, funcsp, false) &&
                                    !doReinterpretCast(tp, throwaway, exp, funcsp, false))
                                {
                                    if (isstructured(throwaway) || isstructured(*tp))
                                    {
                                        errorConversionOrCast(false, throwaway, *tp);
                                    }
                                    cast(*tp, exp);
                                }
                            }
                            else if (!isstructured(*tp) && (!isarray(*tp) || !basetype(*tp)->msil))
                            {
                                cast(*tp, exp);
                            }
                            else
                            {
                                error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
                            }
                        }
                        else
                        {
                            *exp = intNode(ExpressionNode::c_i_, 0);
                            *tp = &stdint;
                            prevsym(lastSym);
                            error(ERR_EXPRESSION_SYNTAX);
                        }
                    }
                }
            }
            else  // expression in parenthesis
            {
                lex = prevsym(start);
                lex = expression_unary(lex, funcsp, atp, tp, exp, ismutable, flags);
            }
        }
        else
        {
            lex = prevsym(start);
            lex = expression_unary(lex, funcsp, atp, tp, exp, ismutable, flags);
        }
        if (loadedAttribs)
            basisAttribs = oldAttribs;
    }
    else
    {
        lex = expression_unary(lex, funcsp, atp, tp, exp, ismutable, flags);
    }
    return lex;
}
static LEXLIST* expression_pm(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_cast(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, Keyword::dotstar_) || MATCHKW(lex, Keyword::pointstar_))
    {
        bool points = false;
        Keyword kw = KW(lex);
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        lex = getsym();
        lex = expression_cast(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
        if (isstructuredmath(*tp, tp1) && Optimizer::cparams.prm_cplusplus && kw == Keyword::pointstar_ &&
            insertOperatorFunc(ovcl_binary_any, Keyword::pointstar_, funcsp, tp, exp, tp1, exp1, nullptr, flags))
        {
            continue;
        }
        if (kw == Keyword::pointstar_)
        {
            points = true;
            if (ispointer(*tp))
            {
                *tp = basetype(*tp);
                *tp = (*tp)->btp;
                if (!isstructured(*tp))
                {
                    errorstr(ERR_POINTER_TO_STRUCTURE_EXPECTED, lex->data->kw->name);
                }
            }
            else
                errorstr(ERR_POINTER_TO_STRUCTURE_EXPECTED, lex->data->kw->name);
        }
        else if (!isstructured(*tp))
        {
            errorstr(ERR_STRUCTURED_TYPE_EXPECTED, lex->data->kw->name);
        }
        if (isfunction(tp1) && isstructured(*tp))
        {
            exp1 = getMemberPtr(basetype(tp1)->sp, basetype(*tp)->sp, &tp1, funcsp);
        }
        if (basetype(tp1)->type != BasicType::memberptr_)
        {
            error(ERR_INCOMPATIBLE_TYPE_CONVERSION);
        }
        else
        {
            if (isstructured(*tp) && basetype(tp1)->type == BasicType::memberptr_)
            {
                if (basetype(*tp)->sp != basetype(tp1)->sp && basetype(*tp)->sp->sb->mainsym != basetype(tp1)->sp &&
                    basetype(*tp)->sp != basetype(tp1)->sp->sb->mainsym)
                {
                    if (classRefCount(basetype(tp1)->sp, (*tp)->sp) != 1)
                    {
                        errorsym2(ERR_NOT_UNAMBIGUOUS_BASE, basetype(tp1)->sp, (*tp)->sp);
                    }
                    else
                    {
                        *exp = baseClassOffset(basetype(tp1)->sp, (*tp)->sp, *exp);
                    }
                }
                if (exp1->type == ExpressionNode::memberptr_)
                {
                    int lab = dumpMemberPtr(exp1->v.sp, *tp, true);
                    exp1 = intNode(ExpressionNode::labcon_, lab);
                }
                if (isfunction(basetype(tp1)->btp))
                {
                    FUNCTIONCALL* funcparams = Allocate<FUNCTIONCALL>();
                    if ((basetype(*tp))->sp->sb->vbaseEntries)
                    {
                        EXPRESSION* ec = exprNode(ExpressionNode::add_, exp1, intNode(ExpressionNode::c_i_, getSize(BasicType::pointer_) + getSize(BasicType::int_)));
                        EXPRESSION* ec1;
                        deref(&stdint, &ec);
                        ec1 = exprNode(ExpressionNode::sub_, ec, intNode(ExpressionNode::c_i_, 1));
                        deref(&stdint, &ec1);
                        ec = exprNode(ExpressionNode::cond_, ec, exprNode(ExpressionNode::void_, ec1, intNode(ExpressionNode::c_i_, 0)));
                        *exp = exprNode(ExpressionNode::add_, *exp, ec);
                    }
                    else
                    {
                        EXPRESSION* ec = exprNode(ExpressionNode::add_, exp1, intNode(ExpressionNode::c_i_, getSize(BasicType::pointer_)));
                        deref(&stdpointer, &ec);
                        *exp = exprNode(ExpressionNode::add_, *exp, ec);
                    }
                    funcparams->sp = tp1->btp->sp;
                    funcparams->fcall = exp1;
                    deref(&stdpointer, &funcparams->fcall);
                    funcparams->thisptr = *exp;
                    funcparams->thistp = MakeType(BasicType::pointer_, *tp);
                    *exp = varNode(ExpressionNode::func_, nullptr);
                    (*exp)->v.func = funcparams;
                    *tp = basetype(tp1);
                }
                else
                {
                    if ((*tp)->sp->sb->vbaseEntries)
                    {
                        EXPRESSION* ec = exprNode(ExpressionNode::add_, exp1, intNode(ExpressionNode::c_i_, getSize(BasicType::int_)));
                        EXPRESSION* ec1;
                        deref(&stdint, &ec);
                        ec1 = exprNode(ExpressionNode::add_, ec, intNode(ExpressionNode::c_i_, -1));
                        ec1 = exprNode(ExpressionNode::add_, *exp, ec1);
                        deref(&stdint, &ec1);
                        *exp = exprNode(ExpressionNode::cond_, ec, exprNode(ExpressionNode::void_, ec1, *exp));
                    }
                    deref(&stdint, &exp1);
                    *exp = exprNode(ExpressionNode::add_, *exp, exp1);
                    *exp = exprNode(ExpressionNode::add_, *exp, intNode(ExpressionNode::c_i_, -1));
                    if (!isstructured(basetype(tp1)->btp))
                    {
                        deref(basetype(tp1)->btp, exp);
                    }
                    *tp = basetype(tp1)->btp;
                }
            }
        }
    }
    return lex;
}
static LEXLIST* expression_times(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_pm(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, Keyword::star_) || MATCHKW(lex, Keyword::divide_) || MATCHKW(lex, Keyword::mod_))
    {
        Keyword kw = KW(lex);
        enum ExpressionNode type;
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        lex = getsym();
        lex = expression_pm(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            *tp = nullptr;
            return lex;
        }
        ResolveTemplateVariable(tp, exp, tp1, atp);
        ResolveTemplateVariable(&tp1, &exp1, *tp, atp);
        if (isstructuredmath(*tp, tp1))
        {

            if (Optimizer::cparams.prm_cplusplus &&
                insertOperatorFunc(kw == Keyword::mod_ ? ovcl_binary_int : ovcl_binary_numeric, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
            {
                continue;
            }
            else
            {
                castToArithmetic(kw == Keyword::mod_, tp, exp, kw, tp1, true);
                castToArithmetic(kw == Keyword::mod_, &tp1, &exp1, (Keyword)-1, *tp, true);
                LookupSingleAggregate(*tp, exp);
                LookupSingleAggregate(tp1, &exp1);
            }
        }
        if (isstructured(*tp) || isstructured(tp1))
            error(ERR_ILL_STRUCTURE_OPERATION);
        else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
            error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
        else if ((basetype(*tp)->scoped || basetype(tp1)->scoped) && !(flags & _F_SCOPEDENUM))
            error(ERR_SCOPED_TYPE_MISMATCH);
        else if (ispointer(*tp) || ispointer(tp1))
            error(ERR_ILL_POINTER_OPERATION);
        else
        {
            int m1 = -1, m2 = -1;
            if (isimaginary(*tp) && isimaginary(tp1))
            {
                if (kw == Keyword::star_)
                    *exp = exprNode(ExpressionNode::uminus_, *exp, nullptr);
                m1 = (int)(*tp)->type - (int)BasicType::float__imaginary_;
                m2 = (int)(int)tp1->type - (int)BasicType::float__imaginary_;
                m1 = m1 > m2 ? m1 : m2;
                switch (m1)
                {
                    case 0:
                        *tp = &stdfloat;
                        tp1 = &stdfloatimaginary;
                        break;
                    case 1:
                        *tp = &stddouble;
                        tp1 = &stddoubleimaginary;
                        break;
                    case 2:
                        *tp = &stdlongdouble;
                        tp1 = &stdlongdoubleimaginary;
                        break;
                }
                cast(tp1, exp);
                cast(tp1, &exp1);
            }
            else
            {
                if (isimaginary(*tp) && (isfloat(tp1) || isint(tp1)))
                {
                    m1 = (int)(*tp)->type - (int)BasicType::float__imaginary_;
                    m2 = isfloat(tp1) ? (int)(int)tp1->type - (int)BasicType::float_ : m1;
                }
                else if ((isfloat(*tp) || isint(*tp)) && isimaginary(tp1))
                {
                    m1 = (int)(int)tp1->type - (int)BasicType::float__imaginary_;
                    m2 = isfloat(*tp) ? (int)(*tp)->type - (int)BasicType::float_ : m1;
                }
                if (m1 >= 0)
                {
                    bool b = isimaginary(*tp);
                    m1 = m1 > m2 ? m1 : m2;
                    switch (m1)
                    {
                        case 0:
                            *tp = &stdfloatimaginary;
                            tp1 = &stdfloat;
                            break;
                        case 1:
                            *tp = &stddoubleimaginary;
                            tp1 = &stddouble;
                            break;
                        default:
                            *tp = &stdlongdoubleimaginary;
                            tp1 = &stdlongdouble;
                            break;
                    }
                    if (b)
                    {
                        cast(*tp, exp);
                        cast(tp1, &exp1);
                    }
                    else
                    {
                        cast(tp1, exp);
                        cast(*tp, &exp1);
                    }
                }
                else
                {
                    *tp = destSize(*tp, tp1, exp, &exp1, false, nullptr);
                }
            }
            switch (kw)
            {
                case Keyword::star_:
                    type = ExpressionNode::mul_;
                    break;
                case Keyword::divide_:
                    type = isunsigned(*tp) ? ExpressionNode::udiv_ : ExpressionNode::div_;
                    break;
                case Keyword::mod_:
                    type = isunsigned(*tp) ? ExpressionNode::umod_ : ExpressionNode::mod_;
                    break;
                default:
                    break;
            }
            *exp = exprNode(type, *exp, exp1);
        }
    }
    return lex;
}
static LEXLIST* expression_add(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    /* fixme add vlas */
    lex = expression_times(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, Keyword::plus_) || MATCHKW(lex, Keyword::minus_))
    {
        bool msil = false;
        Keyword kw = KW(lex);
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        lex = getsym();
        lex = expression_times(lex, funcsp, atp, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            *tp = nullptr;
            return lex;
        }
        ResolveTemplateVariable(tp, exp, tp1, atp);
        ResolveTemplateVariable(&tp1, &exp1, *tp, atp);
        if (isstructuredmath(*tp, tp1))
        {

            if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                insertOperatorFunc(ovcl_binary_numericptr, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
            {
                continue;
            }
            else
            {
                if (Optimizer::cparams.prm_cplusplus && *tp && isstructured(*tp))
                {
                    if (!castToArithmeticInternal(false, tp, exp, kw, isstructured(tp1) ? &stdint : tp1, false))
                        castToPointer(tp, exp, kw, &stdpointer);
                }
                if (Optimizer::cparams.prm_cplusplus && tp1 && isstructured(tp1))
                {
                    if (!castToArithmeticInternal(false, &tp1, &exp1, (Keyword)-1, isstructured(*tp) ? &stdint : *tp, false))
                        castToPointer(&tp1, &exp1, kw, &stdpointer);
                }
                LookupSingleAggregate(*tp, exp);
                LookupSingleAggregate(tp1, &exp1);
            }
        }
        if ((Optimizer::architecture == ARCHITECTURE_MSIL) && Optimizer::cparams.msilAllowExtensions && kw == Keyword::plus_ &&
            (basetype(*tp)->type == BasicType::string_ || basetype(tp1)->type == BasicType::string_ ||
                (atp && basetype(atp)->type == BasicType::string_)))
        {
            msil = true;
            if ((*exp)->type == ExpressionNode::labcon_ && (*exp)->string)
                (*exp)->type = ExpressionNode::c_string_;
            else if (!ismsil(*tp))
                *exp = exprNode(ExpressionNode::x_object_, *exp, nullptr);
            if (exp1->type == ExpressionNode::labcon_ && exp1->string)
                exp1->type = ExpressionNode::c_string_;
            else if (!ismsil(tp1))
                exp1 = exprNode(ExpressionNode::x_object_, exp1, nullptr);
        }
        else if (kw == Keyword::plus_ && ispointer(*tp) && ispointer(tp1))
            error(ERR_ILL_POINTER_ADDITION);
        else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
            error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
        else if ((basetype(*tp)->scoped || basetype(tp1)->scoped) && !(flags & _F_SCOPEDENUM))
            error(ERR_SCOPED_TYPE_MISMATCH);
        else if (kw != Keyword::plus_ && !ispointer(*tp) && ispointer(tp1))
            error(ERR_ILL_POINTER_SUBTRACTION);
        else if (isstructured(*tp) || isstructured(tp1))
            error(ERR_ILL_STRUCTURE_OPERATION);
        else if (ispointer(*tp))
        {
            if (isarray(*tp) && (*tp)->msil)
                error(ERR_MANAGED_OBJECT_NO_ADDRESS);
            else if (ispointer(tp1) && !comparetypes(*tp, tp1, true) && !comparetypes(tp1, *tp, true))
                error(ERR_NONPORTABLE_POINTER_CONVERSION);
            else if (iscomplex(tp1))
                error(ERR_ILL_USE_OF_COMPLEX);
            else if (isfloat(tp1) || isimaginary(tp1))
                error(ERR_ILL_USE_OF_FLOATING);
        }
        else if (ispointer(tp1))
        {
            if (iscomplex(*tp))
                error(ERR_ILL_USE_OF_COMPLEX);
            else if (isfloat(*tp) || isimaginary(*tp))
                error(ERR_ILL_USE_OF_FLOATING);
            else if (isarray(tp1) && (tp1)->msil)
                error(ERR_MANAGED_OBJECT_NO_ADDRESS);
        }
        if (msil)
        {
            // MSIL back Keyword::end_ will take care of figuring out what function to call
            // to perform the concatenation
            *exp = exprNode(ExpressionNode::add_, *exp, exp1);
            *tp = &std__string;
        }
        else if (ispointer(*tp))
        {
            EXPRESSION* ns;
            if (basetype(basetype(*tp)->btp)->type == BasicType::void_)
            {
                if (Optimizer::cparams.prm_cplusplus)
                    error(ERR_ARITHMETIC_WITH_VOID_STAR);
                ns = nodeSizeof(&stdchar, *exp);
            }
            else
            {
                ns = nodeSizeof(basetype(*tp)->btp, *exp);
            }
            if (ispointer(tp1))
            {
                if (Optimizer::architecture == ARCHITECTURE_MSIL)
                {
                    cast(&stdinative, exp);
                    cast(&stdinative, &exp1);
                }
                *exp = exprNode(ExpressionNode::sub_, *exp, exp1);
                *exp = exprNode(ExpressionNode::arraydiv_, *exp, ns);
                *tp = &stdint; /* ptrdiff_t */
            }
            else
            {
                if (Optimizer::architecture == ARCHITECTURE_MSIL)
                {
                    cast(&stdinative, exp);
                }
                /*				*tp = tp1 = destSize(*tp, tp1, exp, &exp1, false, nullptr); */
                if (smallint(tp1))
                    cast(&stdint, &exp1);
                exp1 = exprNode(ExpressionNode::umul_, exp1, ns);
                *exp = exprNode(kw == Keyword::plus_ ? ExpressionNode::add_ : ExpressionNode::sub_, *exp, exp1);
            }
        }
        else if (ispointer(tp1))
        {
            EXPRESSION* ns;
            if (basetype(basetype(tp1)->btp)->type == BasicType::void_)
            {
                if (Optimizer::cparams.prm_cplusplus)
                    error(ERR_ARITHMETIC_WITH_VOID_STAR);
                ns = nodeSizeof(&stdchar, *exp);
            }
            else
            {
                ns = nodeSizeof(basetype(tp1)->btp, *exp);
            }
            /*			*tp = tp1 = destSize(*tp, tp1, exp, &exp1, false, nullptr); */
            if (Optimizer::architecture == ARCHITECTURE_MSIL)
            {
                cast(&stdinative, &exp1);
            }
            if (smallint(*tp))
                cast(&stdint, exp);
            *exp = exprNode(ExpressionNode::umul_, *exp, ns);
            *exp = exprNode(ExpressionNode::add_, *exp, exp1);
            *tp = tp1;
        }
        else
        {
            int m1 = -1, m2 = -1;
            if (isimaginary(*tp) && (isfloat(tp1) || isint(tp1)))
            {
                m1 = (int)(*tp)->type - (int)BasicType::float__imaginary_;
                m2 = isfloat(tp1) ? (int)(int)tp1->type - (int)BasicType::float_ : m1;
            }
            else if ((isfloat(*tp) || isint(*tp)) && isimaginary(tp1))
            {
                m1 = (int)(int)tp1->type - (int)BasicType::float__imaginary_;
                m2 = isfloat(*tp) ? (int)(*tp)->type - (int)BasicType::float_ : m1;
            }
            if (m1 >= 0)
            {
                m1 = m1 > m2 ? m1 : m2;
                TYPE *tpa, *tpb;
                switch (m1)
                {
                    case 0:
                        *tp = &stdfloatcomplex;
                        tpa = &stdfloat;
                        tpb = &stdfloatimaginary;
                        break;
                    case 1:
                        *tp = &stddoublecomplex;
                        tpa = &stddouble;
                        tpb = &stddoubleimaginary;
                        break;
                    default:
                        *tp = &stdlongdoublecomplex;
                        tpa = &stdlongdouble;
                        tpb = &stdlongdoubleimaginary;
                        break;
                }
                if (isimaginary(tp1))
                {
                    cast(tpa, exp);
                    cast(tpb, &exp1);
                }
                else
                {
                    cast(tpb, exp);
                    cast(tpa, &exp1);
                }
                if (kw != Keyword::plus_)
                    exp1 = exprNode(ExpressionNode::uminus_, exp1, nullptr);
                *exp = exprNode(ExpressionNode::add_, *exp, exp1);
                cast(*tp, exp);
            }
            else
            {
                *tp = destSize(*tp, tp1, exp, &exp1, true, atp);
                *exp = exprNode(kw == Keyword::plus_ ? ExpressionNode::add_ : ExpressionNode::sub_, *exp, exp1);
            }
        }
    }
    return lex;
}
static LEXLIST* expression_shift(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_add(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while ((MATCHKW(lex, Keyword::rightshift_) && !(flags & _F_INTEMPLATEPARAMS)) || MATCHKW(lex, Keyword::leftshift_))
    {
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        enum ExpressionNode type;
        Keyword kw = KW(lex);
        lex = getsym();
        lex = expression_add(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            *tp = nullptr;
            return lex;
        }
        ResolveTemplateVariable(tp, exp, tp1, atp);
        ResolveTemplateVariable(&tp1, &exp1, *tp, atp);
        if (isstructuredmath(*tp, tp1))
        {
            LookupSingleAggregate(*tp, exp);
            LookupSingleAggregate(tp1, &exp1);
            if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                insertOperatorFunc(ovcl_binary_int, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
            {
                continue;
            }
            else
            {
                castToArithmetic(true, tp, exp, kw, tp1, true);
                castToArithmetic(true, &tp1, &exp1, (Keyword)-1, *tp, true);
            }
        }
        if (isstructured(*tp) || isstructured(tp1))
            error(ERR_ILL_STRUCTURE_OPERATION);
        else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
            error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
        else if ((basetype(*tp)->scoped || basetype(tp1)->scoped) && !(flags & _F_SCOPEDENUM))
            error(ERR_SCOPED_TYPE_MISMATCH);
        else if (ispointer(*tp) || ispointer(tp1))
            error(ERR_ILL_POINTER_OPERATION);
        else if (isfloat(*tp) || isfloat(tp1) || isimaginary(*tp) || isimaginary(tp1))
            error(ERR_ILL_USE_OF_FLOATING);
        else if (iscomplex(*tp) || iscomplex(tp1))
            error(ERR_ILL_USE_OF_COMPLEX);
        else
        {
            if (kw == Keyword::rightshift_)
                if (isunsigned(*tp))
                    type = ExpressionNode::ursh_;
                else
                    type = ExpressionNode::rsh_;
            else
                type = ExpressionNode::lsh_;
            if (smallint(*tp))
            {
                *tp = &stdint;
                cast(*tp, exp);
            }
            if (Optimizer::architecture == ARCHITECTURE_MSIL && tp1->size != stdint.size)
            {
                cast(&stdint, &exp1);
            } 
            else if (isbitint(tp1))
            {
                cast(&stdint, &exp1);
            }
            *exp = exprNode(type, *exp, exprNode(ExpressionNode::shiftby_, exp1, 0));
        }
    }
    return lex;
}
static LEXLIST* expression_inequality(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable,
                                      int flags)
{
    bool done = false;
    lex = expression_shift(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (!done && lex)
    {
        Keyword kw = KW(lex);
        enum ExpressionNode type;
        const char* opname = "";
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        switch (kw)
        {
            case Keyword::gt_:
                opname = lex->data->kw->name;
                done = (flags & _F_INTEMPLATEPARAMS);
                break;
            case Keyword::geq_:
            case Keyword::lt_:
            case Keyword::leq_:
                opname = lex->data->kw->name;
                break;
            default:
                done = true;
                break;
        }
        if (!done)
        {
            lex = getsym();

            lex = expression_shift(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
            if (!tp1)
            {
                *tp = nullptr;
                return lex;
            }
            ResolveTemplateVariable(tp, exp, tp1, atp);
            ResolveTemplateVariable(&tp1, &exp1, *tp, atp);
            if (isstructuredmath(*tp, tp1))
            {
                if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                    insertOperatorFunc(ovcl_binary_numericptr, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
                {
                    continue;
                }
                else
                {
                    checkscope(*tp, tp1);
                    if (Optimizer::cparams.prm_cplusplus && *tp && isstructured(*tp))
                    {
                        if (!castToArithmeticInternal(false, tp, exp, kw, tp1, true))
                            castToPointer(tp, exp, kw, ispointer(tp1) ? tp1 : &stdpointer);
                    }
                    if (Optimizer::cparams.prm_cplusplus && tp1 && isstructured(tp1))
                    {
                        if (!castToArithmeticInternal(false, &tp1, &exp1, (Keyword) - 1, *tp, true))
                            castToPointer(&tp1, &exp1, (Keyword) -1, ispointer(*tp) ? *tp : &stdpointer);
                    }
                }
                LookupSingleAggregate(*tp, exp);
                LookupSingleAggregate(tp1, &exp1);
            }

            if (Optimizer::cparams.prm_cplusplus)
            {
                SYMBOL* funcsp = nullptr;
                if ((ispointer(*tp) || basetype(*tp)->type == BasicType::memberptr_) && tp1->type == BasicType::aggregate_)
                {
                    if (tp1->syms->size() > 1)
                        errorstr(ERR_OVERLOADED_FUNCTION_AMBIGUOUS, ((SYMBOL*)tp1->syms->front())->name);
                    exp1 = varNode(ExpressionNode::pc_, tp1->syms->front());
                    tp1 = ((SYMBOL*)tp1->syms->front())->tp;
                }
                else if ((ispointer(tp1) || basetype(tp1)->type == BasicType::memberptr_) && (*tp)->type == BasicType::aggregate_)
                {
                    if ((*tp)->syms->size() > 1)
                        errorstr(ERR_OVERLOADED_FUNCTION_AMBIGUOUS, ((SYMBOL*)(*tp)->syms->front())->name);
                    (*exp) = varNode(ExpressionNode::pc_, (*tp)->syms->front());
                    (*tp) = ((SYMBOL*)(*tp)->syms->front())->tp;
                }
            }
            if ((*exp)->type == ExpressionNode::pc_ || ((*exp)->type == ExpressionNode::func_ && !(*exp)->v.func->ascall))
                thunkForImportTable(exp);
            if (exp1->type == ExpressionNode::pc_ || (exp1->type == ExpressionNode::func_ && !exp1->v.func->ascall))
                thunkForImportTable(&exp1);
            if (isstructured(*tp) || isstructured(tp1))
                error(ERR_ILL_STRUCTURE_OPERATION);
            else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
                error(ERR_NOT_AN_ALLOWED_TYPE);
            else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
            else if (ispointer(*tp))
            {
                if (isintconst(exp1))
                {
                    if (!isconstzero(tp1, exp1))
                        error(ERR_NONPORTABLE_POINTER_COMPARISON);
                }
                else if (isint(tp1))
                    error(ERR_NONPORTABLE_POINTER_COMPARISON);
                else if (isfloat(tp1) || isimaginary(tp1))
                    error(ERR_ILL_USE_OF_FLOATING);
                else if (iscomplex(tp1))
                    error(ERR_ILL_USE_OF_COMPLEX);
                else if (ispointer(tp1) && !isvoidptr(*tp) && !isvoidptr(tp1) && !comparetypes(*tp, tp1, false))
                    error(ERR_NONPORTABLE_POINTER_COMPARISON);
            }
            else if (ispointer(tp1))
            {
                if (isintconst(*exp))
                {
                    if (!isconstzero(*tp, *exp))
                        error(ERR_NONPORTABLE_POINTER_COMPARISON);
                }
                else if (isint(*tp))
                    error(ERR_NONPORTABLE_POINTER_COMPARISON);
                else if (isfloat(*tp) || isimaginary(*tp))
                    error(ERR_ILL_USE_OF_FLOATING);
                else if (iscomplex(*tp))
                    error(ERR_ILL_USE_OF_COMPLEX);
            }
            else if (isint(*tp) && isint(tp1))
            {
                if ((isunsigned(*tp) && !isunsigned(tp1)) || (isunsigned(tp1) && !isunsigned(*tp)))
                    errorstr(ERR_SIGNED_UNSIGNED_MISMATCH_RELAT, opname);
            }
            *tp = destSize(*tp, tp1, exp, &exp1, true, nullptr);
            switch (kw)
            {
                case Keyword::gt_:
                    type = isunsigned(*tp) ? ExpressionNode::ugt_ : ExpressionNode::gt_;
                    break;
                case Keyword::geq_:
                    type = isunsigned(*tp) ? ExpressionNode::uge_ : ExpressionNode::ge_;
                    break;
                case Keyword::lt_:
                    type = isunsigned(*tp) ? ExpressionNode::ult_ : ExpressionNode::lt_;
                    break;
                case Keyword::leq_:
                    type = isunsigned(*tp) ? ExpressionNode::ule_ : ExpressionNode::le_;
                    break;
                default:
                    done = true;
                    break;
            }
            *exp = exprNode(type, *exp, exp1);
            if (Optimizer::cparams.prm_cplusplus)
                *tp = &stdbool;
            else
                *tp = &stdint;
        }
    }
    return lex;
}
static LEXLIST* expression_equality(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable,
                                    int flags)
{
    lex = expression_inequality(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, Keyword::eq_) || MATCHKW(lex, Keyword::neq_))
    {
        bool done = false;
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        Keyword kw = KW(lex);
        lex = getsym();
        lex = expression_inequality(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            *tp = nullptr;
            return lex;
        }
        ResolveTemplateVariable(tp, exp, tp1, atp);
        ResolveTemplateVariable(&tp1, &exp1, *tp, atp);
        if (isstructuredmath(*tp, tp1))
        {
            if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                insertOperatorFunc(ovcl_binary_numericptr, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
            {
                continue;
            }
            else
            {
                if (Optimizer::cparams.prm_cplusplus && *tp && isstructured(*tp))
                {
                    if (!castToArithmeticInternal(false, tp, exp, kw, tp1, true))
                        castToPointer(tp, exp, kw, ispointer(tp1) ? tp1 : &stdpointer);
                }
                if (Optimizer::cparams.prm_cplusplus && tp1 && isstructured(tp1))
                {
                    if (!castToArithmeticInternal(false, &tp1, &exp1, (Keyword) - 1, *tp, true))
                        castToPointer(&tp1, &exp1, (Keyword) -1, ispointer(*tp) ? *tp : &stdpointer);
                }
            }
        }
        checkscope(*tp, tp1);
        if (Optimizer::cparams.prm_cplusplus)
        {
            SYMBOL* funcsp = nullptr;
            if ((ispointer(*tp) || basetype(*tp)->type == BasicType::memberptr_) && tp1->type == BasicType::aggregate_)
            {
                if (tp1->syms->size() > 1)
                    errorstr(ERR_OVERLOADED_FUNCTION_AMBIGUOUS, ((SYMBOL*)tp1->syms->front())->name);
                exp1 = varNode(ExpressionNode::pc_, tp1->syms->front());
                tp1 = ((SYMBOL*)tp1->syms->front())->tp;
            }
            else if ((ispointer(tp1) || basetype(tp1)->type == BasicType::memberptr_) && (*tp)->type == BasicType::aggregate_)
            {
                if (tp1->syms->size() > 1)
                    errorstr(ERR_OVERLOADED_FUNCTION_AMBIGUOUS, ((SYMBOL*)(*tp)->syms->front())->name);
                (*exp) = varNode(ExpressionNode::pc_, (*tp)->syms->front());
                (*tp) = ((SYMBOL*)(*tp)->syms->front())->tp;
            }
        }
        if ((*exp)->type == ExpressionNode::pc_ || ((*exp)->type == ExpressionNode::func_ && !(*exp)->v.func->ascall))
            thunkForImportTable(exp);
        if (exp1->type == ExpressionNode::pc_ || (exp1->type == ExpressionNode::func_ && !exp1->v.func->ascall))
            thunkForImportTable(&exp1);
        if (isstructured(*tp) || isstructured(tp1))
        {
            if ((Optimizer::architecture != ARCHITECTURE_MSIL) || (!isconstzero(*tp, *exp) && !isconstzero(tp1, exp1)))
                error(ERR_ILL_STRUCTURE_OPERATION);
        }
        else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        if (ispointer(*tp))
        {
            if (isintconst(exp1))
            {
                if (!isconstzero(tp1, exp1))
                    error(ERR_NONPORTABLE_POINTER_COMPARISON);
            }
            else if (isint(tp1))
                error(ERR_NONPORTABLE_POINTER_COMPARISON);
            else if (isfloat(tp1) || isimaginary(tp1))
                error(ERR_ILL_USE_OF_FLOATING);
            else if (iscomplex(tp1))
                error(ERR_ILL_USE_OF_COMPLEX);
            else if (ispointer(tp1) && !isvoidptr(*tp) && !isvoidptr(tp1) && !comparetypes(*tp, tp1, false))
                error(ERR_NONPORTABLE_POINTER_COMPARISON);
        }
        else if (ispointer(tp1))
        {
            if (isintconst(*exp))
            {
                if (!isconstzero(*tp, *exp))
                    error(ERR_NONPORTABLE_POINTER_COMPARISON);
            }
            else if (isint(*tp))
                error(ERR_NONPORTABLE_POINTER_COMPARISON);
            else if (isfloat(*tp) || isimaginary(*tp))
                error(ERR_ILL_USE_OF_FLOATING);
            else if (iscomplex(*tp))
                error(ERR_ILL_USE_OF_COMPLEX);
        }
        else if (basetype(*tp)->type == BasicType::memberptr_)
        {
            if (basetype(tp1)->type == BasicType::memberptr_)
            {
                if (!comparetypes(basetype(*tp)->btp, basetype(tp1)->btp, true))
                {
                    error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                }
                *exp = exprNode(ExpressionNode::mp_compare_, *exp, exp1);
                (*exp)->size = *tp;
                if (kw == Keyword::neq_)
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                done = true;
            }
            else if (isconstzero(tp1, exp1))
            {
                *exp = exprNode(ExpressionNode::mp_as_bool_, *exp, nullptr);
                (*exp)->size = *tp;
                if (kw == Keyword::eq_)
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                done = true;
            }
            else if (comparetypes(basetype(*tp)->btp, tp1, true))
            {
                int lbl = dumpMemberPtr(exp1->v.sp, *tp, true);
                exp1 = intNode(ExpressionNode::labcon_, lbl);
                *exp = exprNode(ExpressionNode::mp_compare_, *exp, exp1);
                (*exp)->size = *tp;
                if (kw == Keyword::neq_)
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                done = true;
            }
            else
            {
                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
            }
        }
        else if (basetype(tp1)->type == BasicType::memberptr_)
        {
            if (isconstzero(*tp, *exp))
            {
                *exp = exprNode(ExpressionNode::mp_as_bool_, exp1, nullptr);
                (*exp)->size = tp1;
                if (kw == Keyword::eq_)
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                done = true;
            }
            else if (comparetypes(*tp, basetype(tp1)->btp, true))
            {
                int lbl = dumpMemberPtr((*exp)->v.sp, tp1, true);
                *(exp) = intNode(ExpressionNode::labcon_, lbl);
                *exp = exprNode(ExpressionNode::mp_compare_, *exp, exp1);
                (*exp)->size = tp1;
                if (kw == Keyword::neq_)
                    *exp = exprNode(ExpressionNode::not_, *exp, nullptr);
                done = true;
            }
            else
            {
                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
            }
        }
        if (!done)
        {
            if (!(Optimizer::chosenAssembler->arch->preferopts & OPT_BYTECOMPARE) ||
                (!fittedConst(*tp, *exp) && !fittedConst(tp1, exp1)) || !isint(*tp) || !isint(tp1))
                if (!isstructured(*tp) && !isstructured(tp1))
                    destSize(*tp, tp1, exp, &exp1, true, nullptr);
            *exp = exprNode(kw == Keyword::eq_ ? ExpressionNode::eq_ : ExpressionNode::ne_, *exp, exp1);
        }
        if (Optimizer::cparams.prm_cplusplus)
            *tp = &stdbool;
        else
            *tp = &stdint;
    }
    return lex;
}
void GetLogicalDestructors(std::list<struct expr*>** rv, EXPRESSION* cur)
{

    if (!cur || cur->type == ExpressionNode::land_ || cur->type == ExpressionNode::lor_ || cur->type == ExpressionNode::cond_)
        return;
    // nots are genned with a branch, so, get past them...
    while (cur->type == ExpressionNode::not_)
        cur = cur->left;
    if (cur->type == ExpressionNode::func_)
    {
        if (cur->v.func->arguments)
            for (auto t : *cur->v.func->arguments)
                if (isref(t->tp))
                    GetLogicalDestructors(rv, t->exp);
        if (cur->v.func->thisptr)
            GetLogicalDestructors(rv, cur->v.func->thisptr);
        if (cur->v.func->returnSP)
        {
            SYMBOL* sym = cur->v.func->returnSP;
            if (!sym->sb->destructed && sym->sb->dest && sym->sb->dest->front()->exp && !basetype(sym->tp)->sp->sb->trivialCons)
            {
                Optimizer::LIST* listitem;
                sym->sb->destructed = true;
                if (!*rv)
                    *rv = exprListFactory.CreateList();
                (*rv)->push_front(sym->sb->dest->front()->exp);
            }
        }
        else if (cur->v.func->sp->sb->isConstructor)
        {
            // it is going to be a local symbol if we get here...
            EXPRESSION* exp = cur->v.func->thisptr;
            int offset;
            SYMBOL* sym = nullptr;
            exp = relptr(exp, offset, true);
            if (exp)
                sym = exp->v.sp;
            if (sym && !sym->sb->destructed && sym->sb->dest && sym->sb->dest->front()->exp)
            {
                Optimizer::LIST* listitem;
                sym->sb->destructed = true;
                if (!*rv)
                    *rv = exprListFactory.CreateList();
                (*rv)->push_front(sym->sb->dest->front()->exp);
            }
        }
    }
    if (cur->left)
    {
        GetLogicalDestructors(rv, cur->left);
    }
    if (cur->right)
    {
        GetLogicalDestructors(rv, cur->right);
    }
}
static void GetAssignDestructors (std::list<EXPRESSION*> **rv, EXPRESSION *exp)
{
    while (castvalue(exp) || lvalue(exp) || exp->type == ExpressionNode::thisref_)
        exp = exp->left;
    if (exp->type == ExpressionNode::assign_)
    {
        GetAssignDestructors(rv, exp->right);
        GetAssignDestructors(rv, exp->left);
    }
    else if (exp->type == ExpressionNode::func_ && exp->v.func->arguments)
    {
        for (auto t : *exp->v.func->arguments)
        {
            GetAssignDestructors(rv, t->exp); 
            if (t->destructors)
            {
                if (!*rv)
                    *rv = exprListFactory.CreateList();
                (*rv)->insert((*rv)->end(), t->destructors->begin(), t->destructors->end());
                t->destructors = nullptr;
            }
        }
    }
}

static LEXLIST* binop(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, Keyword kw, ExpressionNode type,
                      LEXLIST*(nextFunc)(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable,
                                         int flags),
                      bool* ismutable, int flags)
{
    bool first = true;
    lex = (*nextFunc)(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, kw))
    {
        TYPE* tp1 = nullptr;
        EXPRESSION* exp1 = nullptr;
        std::list<struct expr*>* logicaldestructorsleft = nullptr;
        std::list<struct expr*>* logicaldestructorsright = nullptr;
        if (first && (kw == Keyword::land_ || kw == Keyword::lor_))
        {
            first = false;
            GetLogicalDestructors(&logicaldestructorsleft, *exp);
            if (Optimizer::cparams.prm_cplusplus && *tp && isstructured(*tp))
            {
                if (!castToArithmeticInternal(false, tp, exp, (Keyword) - 1, &stdbool, false))
                    if (!castToArithmeticInternal(false, tp, exp, (Keyword) - 1, &stdint, false))
                        if (!castToPointer(tp, exp, (Keyword) - 1, &stdpointer))
                            errorConversionOrCast(true, *tp, &stdint);
            }
        }
        lex = getsym();
        lex = (*nextFunc)(lex, funcsp, atp, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            *tp = nullptr;
            break;
        }
        if (kw == Keyword::land_ || kw == Keyword::lor_)
        {
            GetLogicalDestructors(&logicaldestructorsright, exp1);
            if (Optimizer::cparams.prm_cplusplus && tp1 && isstructured(tp1))
            {
                if (!castToArithmeticInternal(false, &tp1, &exp1, (Keyword) - 1, &stdbool, false))
                    if (!castToArithmeticInternal(false, &tp1, &exp1, (Keyword) - 1, &stdint, false))
                        if (!castToPointer(&tp1, &exp1, (Keyword) - 1, &stdpointer))
                            errorConversionOrCast(true, tp1, &stdint);
            }
        }
        if (isstructuredmath(*tp, tp1))
        {
            if (Optimizer::cparams.prm_cplusplus &&
                insertOperatorFunc(kw == Keyword::lor_ || kw == Keyword::land_ ? ovcl_binary_numericptr : ovcl_binary_int, kw, funcsp, tp, exp, tp1, exp1,
                    nullptr, flags))
            {
                continue;
            }
            if (kw == Keyword::land_ || kw == Keyword::lor_)
            {
                castToArithmetic(kw != Keyword::land_ && kw != Keyword::lor_, tp, exp, kw, &stdbool, kw != Keyword::land_ && kw != Keyword::lor_);
                castToArithmetic(kw != Keyword::land_ && kw != Keyword::lor_, &tp1, &exp1, (Keyword)-1, &stdbool, kw != Keyword::land_ && kw != Keyword::lor_);
            }
            else
            {
                castToArithmetic(kw != Keyword::land_ && kw != Keyword::lor_, tp, exp, kw, tp1, kw != Keyword::land_ && kw != Keyword::lor_);
                castToArithmetic(kw != Keyword::land_ && kw != Keyword::lor_, &tp1, &exp1, (Keyword)-1, *tp, kw != Keyword::land_ && kw != Keyword::lor_);
            }
            LookupSingleAggregate(*tp, exp);
            LookupSingleAggregate(tp1, &exp1);
        }
        if (isstructured(*tp) || isstructured(tp1))
            error(ERR_ILL_STRUCTURE_OPERATION);
        else if (isvoid(*tp) || isvoid(tp1) || ismsil(*tp) || ismsil(tp1))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if ((basetype(*tp)->scoped || basetype(tp1)->scoped) && !(flags & _F_SCOPEDENUM) &&
                 ((Optimizer::architecture != ARCHITECTURE_MSIL) || Optimizer::cparams.msilAllowExtensions))
            error(ERR_SCOPED_TYPE_MISMATCH);
        if (type != ExpressionNode::lor_ && type != ExpressionNode::land_)
        {
            if (ispointer(*tp) || ispointer(tp1))
                error(ERR_ILL_POINTER_OPERATION);
            else if (isfloat(*tp) || isfloat(tp1) || isimaginary(*tp) || isimaginary(tp1))
                error(ERR_ILL_USE_OF_FLOATING);
            else if (iscomplex(*tp) || iscomplex(tp1))
                error(ERR_ILL_USE_OF_COMPLEX);
            else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
                error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
            *tp = destSize(*tp, tp1, exp, &exp1, true, atp);
        }
        else
        {
            if (Optimizer::cparams.prm_cplusplus)
                *tp = &stdbool;
            else
                *tp = &stdint;
        }
        if (basetype(*tp)->type == BasicType::memberptr_)
        {
            *exp = exprNode(ExpressionNode::mp_as_bool_, *exp, nullptr);
            (*exp)->size = *tp;
        }
        if (basetype(tp1)->type == BasicType::memberptr_)
        {
            exp1 = exprNode(ExpressionNode::mp_as_bool_, exp1, nullptr);
            exp1->size = tp1;
        }
        *exp = exprNode(type, *exp, exp1);
        (*exp)->v.logicaldestructors.left = logicaldestructorsleft;
        (*exp)->v.logicaldestructors.right = logicaldestructorsright;
    }
    return lex;
}
static LEXLIST* expression_and(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    return binop(lex, funcsp, atp, tp, exp, Keyword::and_, ExpressionNode::and_, expression_equality, ismutable, flags);
}
static LEXLIST* expression_xor(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    return binop(lex, funcsp, atp, tp, exp, Keyword::uparrow_, ExpressionNode::xor_, expression_and, ismutable, flags);
}
static LEXLIST* expression_or(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    return binop(lex, funcsp, atp, tp, exp, Keyword::or_, ExpressionNode::or_, expression_xor, ismutable, flags);
}
static LEXLIST* expression_land(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    return binop(lex, funcsp, atp, tp, exp, Keyword::land_, ExpressionNode::land_, expression_or, ismutable, flags);
}
static LEXLIST* expression_lor(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    return binop(lex, funcsp, atp, tp, exp, Keyword::lor_, ExpressionNode::lor_, expression_land, ismutable, flags);
}

static LEXLIST* expression_hook(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_lor(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    if (MATCHKW(lex, Keyword::hook_))
    {
        TYPE *tph = nullptr, *tpc = nullptr;
        EXPRESSION *eph = nullptr, *epc = nullptr;
        if (Optimizer::cparams.prm_cplusplus && *tp && isstructured(*tp))
        {
            if (!castToArithmeticInternal(false, tp, exp, (Keyword) - 1, &stdbool, false))
                if (!castToArithmeticInternal(false, tp, exp, (Keyword) - 1, &stdint, false))
                    if (!castToPointer(tp, exp, (Keyword) - 1, &stdpointer))
                        errorConversionOrCast(true, *tp, &stdint);
        }
        std::list<EXPRESSION*>* logicaldestructors = nullptr;
        GetLogicalDestructors(&logicaldestructors, *exp);
        LookupSingleAggregate(*tp, exp);

        if (isstructured(*tp))
            error(ERR_ILL_STRUCTURE_OPERATION);
        else if (isvoid(*tp) || ismsil(*tp))
            error(ERR_NOT_AN_ALLOWED_TYPE);
        lex = getsym();
        isCallNoreturnFunction = false;
        if (MATCHKW(lex, Keyword::colon_))
        {
            // replicate the selector into the 'true' value
            eph = anonymousVar(StorageClass::auto_, *tp);
            deref(*tp, &eph);
            tph = *tp;
            *exp = exprNode(ExpressionNode::assign_, eph, *exp);
        }
        else
        {
            lex = expression_comma(lex, funcsp, nullptr, &tph, &eph, nullptr, flags);
        }
        bool oldCallExit = isCallNoreturnFunction;
        isCallNoreturnFunction = false;
        if (!tph)
        {
            *tp = nullptr;
        }
        else if (MATCHKW(lex, Keyword::colon_))
        {
            lex = getsym();
            lex = expression_assign(lex, funcsp, nullptr, &tpc, &epc, nullptr, flags);
            isCallNoreturnFunction &= oldCallExit;
            if (!tpc)
            {
                *tp = nullptr;
            }
            else
            {
                ResolveTemplateVariable(tp, exp, tph, atp);
                ResolveTemplateVariable(&tph, &eph, *tp, atp);
                ResolveTemplateVariable(tp, exp, tpc, atp);
                ResolveTemplateVariable(&tpc, &epc, *tp, atp);
                if (basetype(*tp)->type == BasicType::memberptr_)
                {
                    *exp = exprNode(ExpressionNode::mp_as_bool_, *exp, nullptr);
                    (*exp)->size = *tp;
                }
                if (tph->type == BasicType::void_)
                    tph = tpc;
                else if (tpc->type == BasicType::void_)
                    tpc = tph;
                if (tpc->lref && tph->lref)
                {
                    if (!comparetypes(tpc, tph, 1) && !sameTemplate(tpc, tph))
                    {
                        tpc->lref = 0;
                        tpc->rref = 0;
                    }
                }
                if (Optimizer::cparams.prm_cplusplus && (isstructured(tpc) || isstructured(tph)) && (!tpc->lref || !tph->lref))
                {
                    if ( ! isstructured(tpc) || !isstructured(tph) || (comparetypes(tph, tpc, false) && !sameTemplate(tph, tpc, false)) || epc->type == ExpressionNode::thisref_ || epc->type == ExpressionNode::func_ || eph->type == ExpressionNode::thisref_ || eph->type == ExpressionNode::func_)
                    {
                        // structure is result of constructor or return value
                        // at this point we want to check that both sides
                        // are the same type; if not make a constructor
                        EXPRESSION* rv = nullptr;
                        if (!comparetypes(tph, tpc, false))
                        {
                            if (atp && isstructured(atp))
                            {
                                if (!comparetypes(atp, tpc, false) || !sameTemplate(atp, tpc))
                                {
                                    rv = anonymousVar(StorageClass::auto_, tph);
                                    EXPRESSION* exp = rv;
                                    TYPE* ctype = atp;
                                    callConstructorParam(&ctype, &exp, tpc, epc, true, false, false, false, true);
                                    epc = exp;
                                    tpc = atp;
                                }

                                if (!comparetypes(atp, tph, false) || !sameTemplate(atp, tph))
                                {
                                    rv = anonymousVar(StorageClass::auto_, tph);
                                    EXPRESSION* exp = rv;
                                    TYPE* ctype = atp;
                                    callConstructorParam(&ctype, &exp, tph, eph, true, false, false, false, true);
                                    eph = exp;
                                    tph = CopyType(atp);
                                    tph->lref = tph->rref = false;
                                }
                            }
                            else if (isstructured(tph))
                            {
                                rv = anonymousVar(StorageClass::auto_, tph);
                                EXPRESSION* exp = rv;
                                TYPE* ctype = tph;
                                callConstructorParam(&ctype, &exp, tpc, epc, true, false, false, false, true);
                                epc = exp;
                                tpc = tph;
                            }
                            else
                            {
                                rv = anonymousVar(StorageClass::auto_, tpc);
                                EXPRESSION* exp = rv;
                                TYPE* ctype = tpc;
                                callConstructorParam(&ctype, &exp, tph, eph, true, false, false, false, true);
                                eph = exp;
                                tph = tpc;
                                tph->lref = tph->rref = false;
                            }
                        }
                        // now check for structured alias types
                        if (isstructured(tpc) && basetype(tpc)->sp->sb->structuredAliasType)
                        {
                            auto srp = basetype(tpc)->sp->sb->structuredAliasType;
                            rv = anonymousVar(StorageClass::auto_, srp);
                            epc = exprNode(ExpressionNode::blockassign_, rv, epc);
                            epc->size = tpc;
                            epc->altdata = (void*)(tpc);
                            eph = exprNode(ExpressionNode::blockassign_, rv, eph);
                            eph->size = tph;
                            eph->altdata = (void*)(tph);
                        }
                        else
                        {
                            // now make sure both sides are in an anonymous variable
                            if (tph->lref || tph->rref || (eph->type != ExpressionNode::func_ && eph->type != ExpressionNode::thisref_))
                            {
                                if (!rv)
                                    rv = anonymousVar(StorageClass::auto_, tph);
                                EXPRESSION* exp = rv;
                                TYPE* ctype = tph;
                                callConstructorParam(&ctype, &exp, tph, eph, true, false, false, false, true);
                                eph = exp;
                            }
                            if (tpc->lref || tpc->rref || (epc->type != ExpressionNode::func_ && epc->type != ExpressionNode::thisref_))
                            {
                                //if (!rv)
                                    rv = anonymousVar(StorageClass::auto_, tph);
                                EXPRESSION* exp = rv;
                                TYPE* ctype = tpc;
                                callConstructorParam(&ctype, &exp, tpc, epc, true, false, false, false, true);
                                epc = exp;
                            }
                            // now make sure both sides are using the same anonymous variable
                            if (!rv)
                                rv = anonymousVar(StorageClass::auto_, tph);
                            EXPRESSION* dexp = rv;
                            callDestructor(basetype(tph)->sp, nullptr, &dexp, nullptr, true, false, false, true);
                            initInsert(&rv->v.sp->sb->dest, rv->v.sp->tp, dexp, 0, true);

                            EXPRESSION* exp = eph;
                            if (exp->type == ExpressionNode::thisref_)
                                exp = exp->left;
                            if (exp->v.func->returnSP)
                            {
                                if (exp->v.func->returnEXP->type == ExpressionNode::auto_ && exp->v.func->returnEXP->v.sp != rv->v.sp)
                                    exp->v.func->returnEXP->v.sp->sb->allocate = false;
                                exp->v.func->returnEXP = rv;
                            }
                            else if (exp->v.func->thisptr)
                            {
                                if (exp->v.func->thisptr->type == ExpressionNode::auto_ && exp->v.func->thisptr->v.sp != rv->v.sp)
                                    exp->v.func->thisptr->v.sp->sb->allocate = false;
                                exp->v.func->thisptr = rv;
                            }
                            exp = epc;
                            if (exp->type == ExpressionNode::thisref_)
                                exp = exp->left;
                            if (exp->v.func->returnSP)
                            {
                                if (exp->v.func->returnEXP->type == ExpressionNode::auto_ && exp->v.func->returnEXP->v.sp != rv->v.sp)
                                    exp->v.func->returnEXP->v.sp->sb->allocate = false;
                                exp->v.func->returnEXP = rv;
                            }
                            else if (exp->v.func->thisptr)
                            {
                                if (exp->v.func->thisptr->type == ExpressionNode::auto_ && exp->v.func->thisptr->v.sp != rv->v.sp)
                                    exp->v.func->thisptr->v.sp->sb->allocate = false;
                                exp->v.func->thisptr = rv;
                            }
                        }
                    }
                    else
                    {
                        tpc = tph;
                    }
                }
                if (ispointer(tph) || ispointer(tpc))
                    if (!comparetypes(tph, tpc, false))
                        if (!isconstzero(tph, eph) && !isconstzero(tpc, epc))
                            error(ERR_NONPORTABLE_POINTER_CONVERSION);
                if (isfunction(tph) || isfunction(tpc))
                    if (!comparetypes(tph, tpc, true))
                        if (!isconstzero(tph, eph) && !isconstzero(tpc, epc))
                            error(ERR_NONPORTABLE_POINTER_CONVERSION);
                if (tph != tpc && (ispointer(tph) || isfunction(tph) || ispointer(tpc) || isfunction(tpc)))
                {
                    if (!comparetypes(tpc, tph, true))
                    {
                        if ((isvoidptr(tpc) && ispointer(tph)) || (isvoidptr(tph) && ispointer(tpc)))
                        {
                            if (tph->nullptrType)
                                tph = tpc;
                            else if (tpc->nullptrType)
                                tpc = tph;
                            else
                                tpc = tph = &stdpointer;
                        }
                        else if (!((ispointer(tph) || isfunction(tph))) && !((ispointer(tpc) || isfunction(tpc))))
                        {
                            if (!comparetypes(tpc, tph, false))
                                if (!(isarithmetic(tpc) && isarithmetic(tph)))
                                    errortype(ERR_TWO_OPERANDS_SAME_TYPE, tpc, tph);
                        }
                        else if ((isfunction(tph) || isfunction(tpc)) && !comparetypes(tpc, tph, true))
                            if (!isarithmetic(tph) && !isarithmetic(tpc))
                                errortype(ERR_TWO_OPERANDS_SAME_TYPE, tpc, tph);
                    }
                }
                if (isfunction(tph))
                {
                    *tp = tph;
                }
                else if (isfunction(tpc))
                {
                    *tp = tpc;
                }
                else if (tpc->lref && tph->lref)
                {
                    *tp = tpc;
                }
                else if (!isvoid(tpc))
                {
                    if (tpc->scoped)
                    {
                        *tp = tpc;
                    }
                    else
                    {
                        *tp = destSize(tpc, tph, &epc, &eph, false, nullptr);
                    }
                }
                else
                {
                    if (Optimizer::architecture == ARCHITECTURE_MSIL)
                    {
                        EXPRESSION* exp1 = eph;
                        while (castvalue(exp1))
                            exp1 = exp1->left;
                        if (exp1->type == ExpressionNode::thisref_)
                            exp1 = exp1->left;
                        if (exp1->type == ExpressionNode::func_)
                            if (isvoid(basetype(exp1->v.func->sp->tp)->btp))
                                eph = exprNode(ExpressionNode::void_, eph, intNode(ExpressionNode::c_i_, 0));
                        exp1 = epc;
                        while (castvalue(exp1))
                            exp1 = exp1->left;
                        if (exp1->type == ExpressionNode::thisref_)
                            exp1 = exp1->left;
                        if (exp1->type == ExpressionNode::func_)
                            if (isvoid(basetype(exp1->v.func->sp->tp)->btp))
                                epc = exprNode(ExpressionNode::void_, epc, intNode(ExpressionNode::c_i_, 0));
                    }
                    *tp = tpc;
                }
                *exp = exprNode(ExpressionNode::cond_, *exp, exprNode(ExpressionNode::void_, eph, epc));
                (*exp)->v.logicaldestructors.left = logicaldestructors;
                // when assigning a structure to itself, need an intermediate copy
                // this always puts it in...
                if (Optimizer::cparams.prm_cplusplus && isstructured(*tp) && !(*tp)->lref && atp && !basetype(*tp)->sp->sb->structuredAliasType)
                {
                    EXPRESSION* rv = anonymousVar(StorageClass::auto_, *tp);
                    TYPE* ctype = *tp;
                    callConstructorParam(&ctype, &rv, *tp, *exp, true, false, false, false, true);
                    *exp = rv;
                }
            }
        }
        else
        {
            error(ERR_HOOK_NEEDS_COLON);
            *tp = nullptr;
        }
    }
    return lex;
}
static bool isTemplatedPointer(TYPE* tp)
{
    TYPE* tpb = basetype(tp)->btp;
    while (tp != tpb)
    {
        if (tp->templateTop)
            return true;
        tp = tp->btp;
    }
    return false;
}
LEXLIST* expression_throw(LEXLIST* lex, SYMBOL* funcsp, TYPE** tp, EXPRESSION** exp)
{
    TYPE* tp1 = nullptr;
    EXPRESSION* exp1 = nullptr;
    *tp = &stdvoid;
    hasXCInfo = true;
    lex = getsym();
    functionCanThrow = true;
    if (!MATCHKW(lex, Keyword::semicolon_))
    {
        SYMBOL* sym = namespacesearch("_ThrowException", globalNameSpace, false, false);
        makeXCTab(funcsp);
        lex = expression_assign(lex, funcsp, nullptr, &tp1, &exp1, nullptr, 0);
        if (!tp1)
        {
            error(ERR_EXPRESSION_SYNTAX);
        }
        else if (sym)
        {
            FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
            INITLIST* arg1 = Allocate<INITLIST>();  // exception table
            INITLIST* arg2 = Allocate<INITLIST>();  // instance
            INITLIST* arg3 = Allocate<INITLIST>();  // array size
            INITLIST* arg4 = Allocate<INITLIST>();  // constructor
            INITLIST* arg5 = Allocate<INITLIST>();  // exception block
            SYMBOL* rtti = RTTIDumpType(tp1, true);
            SYMBOL* cons = nullptr;
            if (isstructured(tp1))
            {
                cons = getCopyCons(basetype(tp1)->sp, false);
                if (cons && !cons->sb->inlineFunc.stmt)
                {
                    if (cons->sb->defaulted)
                        createConstructor(basetype(tp1)->sp, cons);
                }
            }
            sym = (SYMBOL*)basetype(sym->tp)->syms->front();
            arg1->exp = varNode(ExpressionNode::auto_, funcsp->sb->xc->xctab);
            arg1->tp = &stdpointer;
            if (isstructured(tp1))
            {
                arg2->exp = exp1;
            }
            else
            {
                EXPRESSION* exp3 = anonymousVar(StorageClass::auto_, tp1);
                arg2->exp = exp3;
                deref(tp1->type == BasicType::pointer_ ? &stdpointer : tp1, &exp3);
                exp3 = exprNode(ExpressionNode::assign_, exp3, exp1);
                arg2->exp = exprNode(ExpressionNode::void_, exp3, arg2->exp);
            }
            arg2->tp = &stdpointer;
            arg3->exp = isarray(tp1) ? intNode(ExpressionNode::c_i_, tp1->size / (basetype(tp1)->btp->size)) : intNode(ExpressionNode::c_i_, 1);
            arg3->tp = &stdint;
            arg4->exp = cons ? varNode(ExpressionNode::pc_, cons) : intNode(ExpressionNode::c_i_, 0);
            arg4->tp = &stdpointer;
            if (cons && cons->sb->attribs.inheritable.linkage2 == Linkage::import_)
            {
                arg4->exp = exprNode(ExpressionNode::l_p_, arg4->exp, nullptr);
            }
            arg5->exp = rtti ? varNode(ExpressionNode::global_, rtti) : intNode(ExpressionNode::c_i_, 0);
            arg5->tp = &stdpointer;
            params->arguments = initListListFactory.CreateList();
            params->arguments->push_back(arg1);
            params->arguments->push_back(arg2);
            params->arguments->push_back(arg3);
            params->arguments->push_back(arg4);
            params->arguments->push_back(arg5);
            params->ascall = true;
            params->sp = sym;
            params->functp = sym->tp;
            params->fcall = varNode(ExpressionNode::pc_, sym);
            params->rttiType = rtti;
            *exp = exprNode(ExpressionNode::func_, nullptr, nullptr);
            (*exp)->v.func = params;
        }
    }
    else
    {
        SYMBOL* sym = namespacesearch("_RethrowException", globalNameSpace, false, false);
        if (sym)
        {
            FUNCTIONCALL* parms = Allocate<FUNCTIONCALL>();
            INITLIST* arg1 = Allocate<INITLIST>();  // exception table
            makeXCTab(funcsp);
            sym = (SYMBOL*)basetype(sym->tp)->syms->front();
            parms->ascall = true;
            parms->sp = sym;
            parms->functp = sym->tp;
            parms->fcall = varNode(ExpressionNode::pc_, sym);
            parms->arguments = initListListFactory.CreateList();
            parms->arguments->push_back(arg1);
            arg1->exp = varNode(ExpressionNode::auto_, funcsp->sb->xc->xctab);
            arg1->tp = &stdpointer;
            *exp = exprNode(ExpressionNode::func_, nullptr, nullptr);
            (*exp)->v.func = parms;
        }
    }
    isCallNoreturnFunction = true;
    return lex;
}
static void ReplaceThisAssign(EXPRESSION** init, SYMBOL* sym, EXPRESSION* exp)
{
    if (*init)
    {
        if ((*init)->type == ExpressionNode::global_)
        {
            if ((*init)->v.sp == sym)
                *init = exp;
        }
        else
        {
            ReplaceThisAssign(&(*init)->left, sym, exp);
            ReplaceThisAssign(&(*init)->right, sym, exp);
        }
    }
}
LEXLIST* expression_assign(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    bool done = false;
    EXPRESSION *exp1 = nullptr, **exp2;
    EXPRESSION* asndest = nullptr;

    bool localMutable = false;
    TYPE* tp2;
    if (MATCHKW(lex, Keyword::throw_))
    {
        return expression_throw(lex, funcsp, tp, exp);
    }
    lex = expression_hook(lex, funcsp, atp, tp, exp, &localMutable, flags);
    if (*tp == nullptr)
        return lex;
    while (!done && lex)
    {
        Keyword kw = KW(lex);
        enum ExpressionNode op;
        enum ovcl selovcl;
        TYPE* tp1 = nullptr;
        EXPRESSION* symRef;
        switch (kw)
        {
            case Keyword::asand_:
            case Keyword::asor_:
            case Keyword::asxor_:
            case Keyword::asmod_:
            case Keyword::asleftshift_:
            case Keyword::asrightshift_:
                selovcl = ovcl_assign_int;
                break;
            case Keyword::astimes_:
            case Keyword::asdivide_:
                selovcl = ovcl_assign_numeric;
                break;
            case Keyword::assign_:
                selovcl = ovcl_assign_any;
                break;
            case Keyword::asplus_:
            case Keyword::asminus_:
                selovcl = ovcl_assign_numericptr;
                break;
            default:
                if (ismutable)
                    *ismutable = localMutable;
                done = true;
                continue;
        }
        lex = getsym();
        switch (kw)
        {
            case Keyword::assign_:
                if (Optimizer::cparams.prm_cplusplus && MATCHKW(lex, Keyword::begin_))
                {
                    if (isstructured(*tp))
                    {
                        std::list<INITIALIZER*>* init = nullptr;
                        SYMBOL* spinit = nullptr;
                        tp1 = *tp;
                        spinit = anonymousVar(StorageClass::localstatic_, tp1)->v.sp;
                        localNameSpace->front()->syms->Add(spinit);
                        lex = initType(lex, funcsp, 0, StorageClass::auto_, &init, nullptr, tp1, spinit, false, flags | _F_ASSIGNINIT);
                        EXPRESSION* exp2 = nullptr;
                        if (init && init->front()->exp->type == ExpressionNode::thisref_)
                        {
                            ReplaceThisAssign(&init->front()->exp->left->v.func->thisptr, spinit, *exp);
                            spinit->sb->allocate = false;
                        }
                        else
                        {
                            exp2 = *exp;
                        }
                        *exp = convertInitToExpression(tp1, nullptr, exp2, funcsp, init, exp1, false);

                        return lex;
                    }
                    else
                    {
                        lex = getsym();
                        ++inAssignRHS;
                        lex = expression_assign(lex, funcsp, *tp, &tp1, &exp1, nullptr, flags);
                        --inAssignRHS;
                        if (!needkw(&lex, Keyword::end_))
                        {
                            errskim(&lex, skim_end);
                            skip(&lex, Keyword::end_);
                        }
                    }
                }
                else
                {
                    ++inAssignRHS;
                    lex = expression_assign(lex, funcsp, *tp, &tp1, &exp1, nullptr, flags);
                    --inAssignRHS;
                }
                break;
            case Keyword::asplus_:
            case Keyword::asminus_:
            case Keyword::asand_:
            case Keyword::asor_:
            case Keyword::asxor_:
                ++inAssignRHS;
                lex = expression_assign(lex, funcsp, *tp, &tp1, &exp1, nullptr, flags);
                --inAssignRHS;
                break;
            default:
                ++inAssignRHS;
                lex = expression_assign(lex, funcsp, nullptr, &tp1, &exp1, nullptr, flags);
                --inAssignRHS;
                break;
        }
        if (!tp1)
        {
            *tp = nullptr;
            return lex;
        }
        ResolveTemplateVariable(tp, exp, tp1, nullptr);
        ResolveTemplateVariable(&tp1, &exp1, *tp, nullptr);
        ConstExprPromote(*exp, false);
        if (isstructuredmath(*tp, tp1))
        {
            if ((Optimizer::cparams.prm_cplusplus || Optimizer::architecture == ARCHITECTURE_MSIL) &&
                insertOperatorFunc(selovcl, kw, funcsp, tp, exp, tp1, exp1, nullptr, flags))
            {
                // unallocated var for destructor
                if (!inAssignRHS)
                {
                    GetAssignDestructors(&(*exp)->v.func->destructors, *exp);
                }
                if (asndest)
                {
                    SYMBOL* sym = anonymousVar(StorageClass::auto_, tp1)->v.sp;
                    callDestructor(basetype(tp1)->sp, nullptr, &asndest, nullptr, true, false, false, true);
                    initInsert(&sym->sb->dest, tp1, asndest, 0, true);
                }

                continue;
            }
        }
        if (kw == Keyword::assign_ && (flags & _F_SELECTOR))
        {
            /* a little naive... */
            error(ERR_POSSIBLE_INCORRECT_ASSIGNMENT);
            flags &= ~_F_SELECTOR;
        }
        checkscope(*tp, tp1);
        if (Optimizer::cparams.prm_cplusplus)
        {
            if (isarithmetic(*tp))
            {
                castToArithmetic(false, &tp1, &exp1, (Keyword) - 1, *tp, true);
            }
            else if (isstructured(tp1))
            {
                cppCast(*tp, &tp1, &exp1);
            }
        }
        exp2 = &exp1;
        while (castvalue(*exp2))
            exp2 = &(*exp2)->left;
        if ((*exp2)->type == ExpressionNode::func_ && (*exp2)->v.func->sp->sb->storage_class == StorageClass::overloads_)
        {
            TYPE* tp2 = nullptr;
            SYMBOL* funcsp;
            if ((*exp2)->v.func->sp->sb->parentClass && !(*exp2)->v.func->asaddress)
                if ((*exp2)->v.func->sp->sb->storage_class == StorageClass::member_ || (*exp2)->v.func->sp->sb->storage_class == StorageClass::mutable_)
                    error(ERR_NO_IMPLICIT_MEMBER_FUNCTION_ADDRESS);
            funcsp = MatchOverloadedFunction((*tp), isfuncptr(*tp) || basetype(*tp)->type == BasicType::memberptr_ ? &tp1 : &tp2,
                                             (*exp2)->v.func->sp, exp2, flags);
            if (funcsp && basetype(*tp)->type == BasicType::memberptr_)
            {
                int lbl = dumpMemberPtr(funcsp, *tp, true);
                exp1 = intNode(ExpressionNode::labcon_, lbl);
            }
            if (funcsp)
            {
                tp1 = funcsp->tp;
                if (exp1->type == ExpressionNode::pc_ || (exp1->type == ExpressionNode::func_ && !exp1->v.func->ascall))
                {
                    thunkForImportTable(&exp1);
                }
            }
            if (basetype(*tp)->btp && !comparetypes(basetype(*tp)->btp, tp1, true))
            {
                if (!isvoidptr(*tp))
                    errorConversionOrCast(true, tp1, *tp);
            }
        }
        if (exp1->type == ExpressionNode::pc_ || (exp1->type == ExpressionNode::func_ && !exp1->v.func->ascall))
        {
            if (Optimizer::architecture == ARCHITECTURE_MSIL)
            {
                ValidateMSILFuncPtr(*tp, tp1, &exp1);
            }
        }
        EXPRESSION* temp = GetSymRef(*exp);
        symRef = (Optimizer::architecture == ARCHITECTURE_MSIL) ? temp : nullptr;
        LookupSingleAggregate(tp1, &exp1);

        if (isstructured(tp1))
        {
            SYMBOL* conv = lookupNonspecificCast(basetype(tp1)->sp, *tp);
            if (conv)
            {
                FUNCTIONCALL* params = Allocate<FUNCTIONCALL>();
                params->thisptr = exp1;
                params->thistp = tp1;
                params->ascall = true;
                params->functp = conv->tp;
                params->fcall = varNode(ExpressionNode::pc_, conv);
                params->sp = conv;
                exp1 = exprNode(ExpressionNode::func_, nullptr, nullptr);
                exp1->v.func = params;
                tp1 = basetype(conv->tp)->btp;
            }
        }
        if (((*exp)->type == ExpressionNode::const_ || isconstraw(*tp)) && !localMutable &&
            (!temp || temp->v.sp->sb->storage_class != StorageClass::parameter_ || !isarray(*tp)) &&
            ((*exp)->type != ExpressionNode::func_ || !isconstraw(basetype((*exp)->v.func->sp->tp)->btp)))
            error(ERR_CANNOT_MODIFY_CONST_OBJECT);
        else if (isvoid(*tp) || isvoid(tp1) || (*tp)->type == BasicType::aggregate_)
            error(ERR_NOT_AN_ALLOWED_TYPE);
        else if (!isstructured(*tp) && ((*tp)->btp && !ispointer((*tp)->btp)) && (!isarray(*tp) || !basetype(*tp)->msil) &&
                 basetype(*tp)->type != BasicType::memberptr_ && basetype(*tp)->type != BasicType::templateparam_ &&
                 basetype(*tp)->type != BasicType::templateselector_ && !lvalue(*exp) && (*exp)->type != ExpressionNode::msil_array_access_)
            error(ERR_LVALUE);
        else if (symRef && symRef->v.sp->sb->attribs.inheritable.linkage2 == Linkage::property_ && !symRef->v.sp->sb->has_property_setter)
            errorsym(ERR_CANNOT_MODIFY_PROPERTY_WITHOUT_SETTER, symRef->v.sp);
        else if (isstructured(*tp) && !basetype(*tp)->sp->sb->trivialCons)
        {
            errorsym(ERR_NO_ASSIGNMENT_OPERATOR, basetype(*tp)->sp);
        }
        else
            switch (kw)
            {
                case Keyword::asand_:
                case Keyword::asor_:
                case Keyword::asxor_:
                case Keyword::asleftshift_:
                case Keyword::asrightshift_:
                    if (ismsil(*tp) || ismsil(tp1))
                        error(ERR_NOT_AN_ALLOWED_TYPE);
                    if (iscomplex(*tp) || iscomplex(tp1))
                        error(ERR_ILL_USE_OF_COMPLEX);
                    if (isfloat(*tp) || isfloat(tp1) || isimaginary(*tp) || isimaginary(tp1))
                        error(ERR_ILL_USE_OF_FLOATING);
                    /* fall through */
                case Keyword::astimes_:
                case Keyword::asdivide_:
                case Keyword::asmod_:
                    if (ismsil(*tp) || ismsil(tp1))
                        error(ERR_NOT_AN_ALLOWED_TYPE);
                    if (ispointer(*tp) || ispointer(tp1))
                        error(ERR_ILL_POINTER_OPERATION);
                    if (isstructured(*tp) || isstructured(tp1))
                        error(ERR_ILL_STRUCTURE_OPERATION);
                    if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
                        error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                    break;
                case Keyword::asplus_:
                    if ((*tp)->type == BasicType::string_)
                    {
                        if (exp1->type == ExpressionNode::labcon_ && exp1->string)
                            exp1->type = ExpressionNode::c_string_;
                        else if (!ismsil(tp1))
                            exp1 = exprNode(ExpressionNode::x_object_, exp1, nullptr);
                    }
                    else if (ismsil(*tp) || ismsil(tp1))
                        error(ERR_NOT_AN_ALLOWED_TYPE);
                    else if (ispointer(*tp))
                    {
                        if (ispointer(tp1))
                            error(ERR_ILL_POINTER_ADDITION);
                        else
                        {
                            EXPRESSION* ns;
                            if (iscomplex(tp1))
                                error(ERR_ILL_USE_OF_COMPLEX);
                            else if (isfloat(tp1) || isimaginary(tp1))
                                error(ERR_ILL_USE_OF_FLOATING);
                            else if (isstructured(tp1))
                                error(ERR_ILL_STRUCTURE_OPERATION);
                            if (basetype(basetype(*tp)->btp)->type == BasicType::void_)
                            {
                                if (Optimizer::cparams.prm_cplusplus)
                                    error(ERR_ARITHMETIC_WITH_VOID_STAR);
                                cast(&stdint, &exp1);
                                ns = nodeSizeof(&stdchar, exp1);
                            }
                            else
                            {
                                cast(&stdint, &exp1);
                                ns = nodeSizeof(basetype(*tp)->btp, exp1);
                            }
                            exp1 = exprNode(ExpressionNode::umul_, exp1, ns);
                        }
                    }
                    else if (ispointer(tp1))
                    {
                        error(ERR_ILL_POINTER_OPERATION);
                    }
                    else if (isstructured(*tp) || isstructured(tp1))
                        error(ERR_ILL_STRUCTURE_OPERATION);
                    else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
                        error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                    break;
                case Keyword::asminus_:
                    if (ismsil(*tp) || ismsil(tp1))
                        error(ERR_NOT_AN_ALLOWED_TYPE);
                    if (isstructured(*tp) || isstructured(tp1))
                        error(ERR_ILL_STRUCTURE_OPERATION);
                    else if (ispointer(tp1))
                    {
                        error(ERR_ILL_POINTER_SUBTRACTION);
                    }
                    else if (basetype(*tp)->type == BasicType::memberptr_ || basetype(tp1)->type == BasicType::memberptr_)
                        error(ERR_ILLEGAL_USE_OF_MEMBER_PTR);
                    else if (ispointer(*tp))
                    {
                        EXPRESSION* ns;
                        if (basetype(basetype(*tp)->btp)->type == BasicType::void_)
                        {
                            if (Optimizer::cparams.prm_cplusplus)
                                error(ERR_ARITHMETIC_WITH_VOID_STAR);
                            cast(&stdint, &exp1);
                            ns = nodeSizeof(&stdchar, exp1);
                        }
                        else
                        {
                            cast(&stdint, &exp1);
                            ns = nodeSizeof(basetype(*tp)->btp, exp1);
                        }
                        exp1 = exprNode(ExpressionNode::umul_, exp1, ns);
                    }
                    break;
                case Keyword::assign_:
                    if (basetype(*tp)->type == BasicType::string_)
                    {
                        if (exp1->type == ExpressionNode::labcon_ && exp1->string)
                            exp1->type = ExpressionNode::c_string_;
                    }
                    else if (basetype(*tp)->type == BasicType::object_)
                    {
                        if (tp1->type != BasicType::object_ && !isstructured(tp1) && (!isarray(tp1) || !basetype(tp1)->msil))
                            exp1 = exprNode(ExpressionNode::x_object_, exp1, nullptr);
                    }
                    else if (ismsil(*tp) || ismsil(tp1))
                        error(ERR_NOT_AN_ALLOWED_TYPE);
                    if (ispointer(*tp))
                    {
                        if (isarray(*tp) && (*tp)->msil && !comparetypes(*tp, tp1, true) && natural_size(*exp) != ISZ_OBJECT)
                        {
                            *exp = exprNode(ExpressionNode::l_object_, *exp, nullptr);
                            (*exp)->v.tp = tp1;
                        }
                        if (isarithmetic(tp1))
                        {
                            if (iscomplex(tp1))
                                error(ERR_ILL_USE_OF_COMPLEX);
                            else if (isfloat(tp1) || isimaginary(tp1))
                                error(ERR_ILL_USE_OF_FLOATING);
                            else if (isarithmeticconst(exp1))
                            {
                                if (!isintconst(exp1) || !isconstzero(*tp, exp1))
                                    error(ERR_NONPORTABLE_POINTER_CONVERSION);
                            }
                            else if (Optimizer::cparams.prm_cplusplus)
                            {
                                errorConversionOrCast(true, tp1, *tp);
                            }
                            else
                            {
                                error(ERR_NONPORTABLE_POINTER_CONVERSION);
                            }
                        }
                        else if (ispointer(basetype(tp1)) || tp1->type == BasicType::any_)
                        {
                            if (Optimizer::cparams.prm_cplusplus && basetype(tp1)->stringconst && !isconst(basetype(*tp)->btp))
                                error(ERR_INVALID_CHARACTER_STRING_CONVERSION);

                            while (tp1->type == BasicType::any_ && tp1->btp)
                                tp1 = tp1->btp;
                            if (!ispointer(basetype(tp1)))
                                goto end;
                            if (!comparetypes(*tp, tp1, true))
                            {
                                bool found = false;
                                if (ispointer(tp1))
                                {
                                    TYPE* tpo = basetype(basetype(tp1)->btp);
                                    TYPE* tpn = basetype(basetype(*tp)->btp);
                                    if (isstructured(tpo) && isstructured(tpn))
                                    {
                                        if (classRefCount(tpo->sp, tpn->sp) == 1)
                                        {
                                            EXPRESSION* v = Allocate<EXPRESSION>();
                                            v->type = ExpressionNode::c_i_;
                                            v = baseClassOffset(tpo->sp, tpn->sp, v);
                                            optimize_for_constants(&v);
                                            if (v->type == ExpressionNode::c_i_)  // check for no virtual base
                                            {
                                                if (isAccessible(tpn->sp, tpn->sp, tpo->sp, funcsp, AccessLevel::public_, false))
                                                {
                                                    exp1 = exprNode(ExpressionNode::sub_, exp1, v);
                                                    found = true;
                                                }
                                            }
                                        }
                                        else if (classRefCount(tpn->sp, tpo->sp) == 1)
                                        {
                                            EXPRESSION* v = Allocate<EXPRESSION>();
                                            v->type = ExpressionNode::c_i_;
                                            v = baseClassOffset(tpn->sp, tpo->sp, v);
                                            optimize_for_constants(&v);
                                            if (v->type == ExpressionNode::c_i_)  // check for no virtual base
                                            {
                                                if (isAccessible(tpo->sp, tpo->sp, tpn->sp, funcsp, AccessLevel::public_, false))
                                                {
                                                    exp1 = exprNode(ExpressionNode::add_, exp1, v);
                                                    found = true;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (!found)
                                {
                                    if (Optimizer::cparams.prm_cplusplus)
                                    {
                                        if ((!isvoidptr(*tp) || !ispointer(tp1)) && !tp1->nullptrType)
                                            if (!isTemplatedPointer(*tp))
                                            {
                                                errorConversionOrCast(true, tp1, *tp);
                                            }
                                    }
                                    else if (!isvoidptr(*tp) && !isvoidptr(tp1))
                                    {
                                        if (!matchingCharTypes(*tp, tp1))
                                            error(ERR_SUSPICIOUS_POINTER_CONVERSION);
                                    }
                                    else if (Optimizer::cparams.prm_cplusplus && !isvoidptr(*tp) && isvoidptr(tp1) &&
                                             exp1->type != ExpressionNode::nullptr_)
                                    {
                                        error(ERR_ANSI_FORBIDS_IMPLICIT_CONVERSION_FROM_VOID);
                                    }
                                }
                            }
                        }
                        else if (isfunction(tp1))
                        {
                            if (!isvoidptr(*tp) && (!isfunction(basetype(*tp)->btp) ||
                                                    !comparetypes(basetype(basetype(*tp)->btp)->btp, basetype(tp1)->btp, true)))
                                error(ERR_SUSPICIOUS_POINTER_CONVERSION);
                        }
                        else
                        {
                            if (tp1->type == BasicType::memberptr_)
                            {
                                errorConversionOrCast(true, tp1, *tp);
                            }
                            else
                                error(ERR_INVALID_POINTER_CONVERSION);
                        }
                    end:;
                    }
                    else if (ispointer(tp1))
                    {
                        if (iscomplex(*tp))
                        {
                            error(ERR_ILL_USE_OF_COMPLEX);
                        }
                        else if (isfloat(*tp) || isimaginary(*tp))
                        {
                            error(ERR_ILL_USE_OF_FLOATING);
                        }
                        else if (isint(*tp))
                        {
                            if (basetype(*tp)->type != BasicType::bool_)
                                error(ERR_NONPORTABLE_POINTER_CONVERSION);
                        }
                        else if (isarray(tp1) && (tp1)->msil)
                        {
                            error(ERR_MANAGED_OBJECT_NO_ADDRESS);
                        }
                    }
                    if (isstructured(*tp) && (!isstructured(tp1) || (!comparetypes(*tp, tp1, true) && !sameTemplate(tp1, *tp))))
                    {
                        if (!((Optimizer::architecture == ARCHITECTURE_MSIL) && basetype(*tp)->sp->sb->msil &&
                              (isconstzero(tp1, exp1) || basetype(tp1)->nullptrType)))
                        {
                            error(ERR_ILL_STRUCTURE_ASSIGNMENT);
                        }
                    }
                    else if (isstructured(*tp) && !(*tp)->size)
                    {
                        if (!(flags & _F_SIZEOF))
                            errorsym(ERR_STRUCT_NOT_DEFINED, basetype(*tp)->sp);
                    }
                    else if (!isstructured(*tp) && isstructured(tp1))
                    {
                        error(ERR_ILL_STRUCTURE_ASSIGNMENT);
                    }
                    else if (basetype(*tp)->type == BasicType::memberptr_)
                    {
                        if (exp1->type == ExpressionNode::memberptr_)
                        {
                            if (exp1->v.sp != basetype(*tp)->sp && exp1->v.sp != basetype(*tp)->sp->sb->mainsym &&
                                !sameTemplate(exp1->v.sp->tp, basetype(*tp)->sp->tp))  // DAL FIXED
                                errorConversionOrCast(true, tp1, *tp);
                        }
                        else if ((!isfunction(basetype(*tp)->btp) || !comparetypes(basetype(*tp)->btp, tp1, true)) &&
                                 !isconstzero(tp1, *exp) && !comparetypes(*tp, tp1, true))
                        {
                            errorConversionOrCast(true, tp1, *tp);
                        }
                    }
                    else if (basetype(tp1)->type == BasicType::memberptr_)
                        errorConversionOrCast(true, tp1, *tp);
                    break;
                default:
                    done = true;
                    break;
            }
        switch (kw)
        {
            case Keyword::asand_:
                op = ExpressionNode::and_;
                break;
            case Keyword::asor_:
                op = ExpressionNode::or_;
                break;
            case Keyword::asxor_:
                op = ExpressionNode::xor_;
                break;

            case Keyword::astimes_:
                op = ExpressionNode::mul_;
                break;
            case Keyword::asdivide_:
                tp2 = destSize(*tp, tp1, nullptr, nullptr, false, nullptr);
                op = isunsigned(*tp) && largenum(*tp) ? ExpressionNode::udiv_ : ExpressionNode::div_;
                break;
            case Keyword::asmod_:
                tp2 = destSize(*tp, tp1, nullptr, nullptr, false, nullptr);
                op = isunsigned(*tp) && largenum(*tp) ? ExpressionNode::umod_ : ExpressionNode::mod_;
                break;
            case Keyword::assign_:
                op = ExpressionNode::assign_;
                break;

            case Keyword::asplus_:
                op = ExpressionNode::add_;
                break;
            case Keyword::asminus_:
                op = ExpressionNode::sub_;
                break;

            case Keyword::asleftshift_:
                op = ExpressionNode::lsh_;
                break;
            case Keyword::asrightshift_:
                tp2 = destSize(*tp, tp1, nullptr, nullptr, false, nullptr);
                op = isunsigned(*tp) ? ExpressionNode::ursh_ : ExpressionNode::rsh_;
                break;
            default:
                break;
        }
        if (ismsil(*tp))
        {
            if (exp1->type == ExpressionNode::labcon_ && exp1->string)
                exp1->type = ExpressionNode::c_string_;
            else if (!ismsil(tp1) && !isstructured(tp1) && (!isarray(tp1) || !basetype(tp1)->msil))
                exp1 = exprNode(ExpressionNode::x_object_, exp1, nullptr);
            if (op == ExpressionNode::assign_)
            {
                *exp = exprNode(op, *exp, exp1);
            }
            else
            {
                EXPRESSION* dest = *exp;
                *exp = exprNode(op, *exp, exp1);
                *exp = exprNode(ExpressionNode::assign_, dest, *exp);
            }
        }
        else if (basetype(*tp)->type == BasicType::memberptr_)
        {
            if ((*exp)->type == ExpressionNode::not__lvalue_ || ((*exp)->type == ExpressionNode::func_ && !(*exp)->v.func->ascall) || (*exp)->type == ExpressionNode::void_ ||
                (*exp)->type == ExpressionNode::memberptr_)
            {
                if (basetype(*tp)->type != BasicType::templateparam_)
                    error(ERR_LVALUE);
            }
            else if (exp1->type == ExpressionNode::memberptr_)
            {
                int lbl = dumpMemberPtr(exp1->v.sp, *tp, true);
                exp1 = intNode(ExpressionNode::labcon_, lbl);
                *exp = exprNode(ExpressionNode::blockassign_, *exp, exp1);
                (*exp)->size = *tp;
                (*exp)->altdata = (void*)(*tp);
            }
            else if (isconstzero(tp1, exp1) || exp1->type == ExpressionNode::nullptr_)
            {
                *exp = exprNode(ExpressionNode::blockclear_, *exp, nullptr);
                (*exp)->size = *tp;
            }
            else if (exp1->type == ExpressionNode::func_ && exp1->v.func->returnSP)
            {
                exp1->v.func->returnSP->sb->allocate = false;
                exp1->v.func->returnEXP = *exp;
                *exp = exp1;
            }
            else
            {
                *exp = exprNode(ExpressionNode::blockassign_, *exp, exp1);
                (*exp)->size = *tp;
                (*exp)->altdata = (void*)(*tp);
            }
        }
        else if (isstructured(*tp))
        {
            EXPRESSION* exp2 = exp1;
            if (((*exp)->type == ExpressionNode::not__lvalue_ || ((*exp)->type == ExpressionNode::func_ && (!(*exp)->v.func->ascall || (*exp)->v.func->returnSP)) || ((*exp)->type == ExpressionNode::void_) && !(flags & _F_SIZEOF)))
                error(ERR_LVALUE);
            if (lvalue(exp2))
                exp2 = exp2->left;
            if (exp2->type == ExpressionNode::func_ && exp2->v.func->returnSP)
            {
                exp2->v.func->returnSP->sb->allocate = false;
                exp2->v.func->returnEXP = *exp;
                *exp = exp1;
            }
            else
            {
                *exp = exprNode(ExpressionNode::blockassign_, *exp, exp1);
                (*exp)->size = *tp;
                (*exp)->altdata = (void*)(*tp);
                if (isatomic(*tp))
                {
                    (*exp)->size = CopyType((*exp)->size);
                    (*exp)->size->size -= ATOMIC_FLAG_SPACE;
                }
            }
            *exp = exprNode(ExpressionNode::not__lvalue_, *exp, nullptr);
        }
        else
        {
            if (kw == Keyword::assign_)
            {
                if ((*exp)->type != ExpressionNode::msil_array_access_ && exp1->type != ExpressionNode::msil_array_access_)
                {
                    if (isarithmetic(*tp) || isarithmetic(tp1) ||
                        ((ispointer(*tp) && (!isarray(*tp) || !basetype(*tp)->msil)) ||
                         (ispointer(tp1) && (!isarray(tp1) || !basetype(tp1)->msil))))
                    {
                        int n = natural_size(*exp);
                        if (natural_size(exp1) != n)
                            cast((*tp), &exp1);
                        else if (abs(n) == ISZ_BITINT && !comparetypes(*tp, tp1, false))
                            cast((*tp), &exp1);
                    }
                    else if (isarray(*tp) && basetype(*tp)->msil)
                    {
                        *exp = exprNode(ExpressionNode::l_object_, *exp, nullptr);
                        (*exp)->v.tp = *tp;
                    }
                }

                *exp = exprNode(op, *exp, exp1);
            }
            else
            {
                EXPRESSION* src, *dest;
                dest = EvaluateDest(*exp, *tp);
                if (dest)
                {
                    src = dest->left;
                    dest = dest->right;
                }
                else
                {
                    dest = RemoveAutoIncDec(*exp);
                    src = *exp;
                }
                // we want to optimize the as* operations for the backend
                // but can't do the optimization for divisions
                // otherwise it is fine for the processor we are on
                if (isbitint(*tp))
                {
                    if (kw == Keyword::asleftshift_ || kw == Keyword::asrightshift_)
                    {
                        cast(&stdint, &exp1);
                        *exp = exprNode(op, src, exp1);
                    }
                    else
                    {
                        auto tp2 = destSize(*tp, tp1, &src, &exp1, false, nullptr);
                        *exp = exprNode(op, src, exp1);
                        if (!comparetypes(tp2, *tp, 0))
                            cast(*tp, exp);
                    }
                    *exp = exprNode(ExpressionNode::assign_, dest, *exp);

                }
                else if (kw == Keyword::asmod_ || kw == Keyword::asdivide_ || basetype(*tp)->type == BasicType::bool_)
                {
                    int n = natural_size(*exp);
                    destSize(*tp, tp1, &src, &exp1, false, nullptr);
                    *exp = exprNode(op, src, exp1);
                    if (natural_size(exp1) != n)
                        cast((*tp), exp);
                    *exp = exprNode(ExpressionNode::assign_, dest, *exp);
                }
                else if (kw == Keyword::asleftshift_ || kw == Keyword::asrightshift_)
                {
                    int n = natural_size(*exp);
                    if (natural_size(exp1) != n)
                        cast(&stdint, &exp1);
                    *exp = exprNode(op, src, exp1);
                    *exp = exprNode(ExpressionNode::assign_, dest, *exp);
                }
                else
                {
                    int n = natural_size(*exp);
                    if (natural_size(exp1) != n)
                        destSize(*tp, tp1, &src, &exp1, false, nullptr);
                    *exp = exprNode(op, src, exp1);
                    if (natural_size(*exp) != n)
                        cast(*tp, exp);
                    *exp = exprNode(ExpressionNode::assign_, dest, *exp);
                }
            }
        }
        /* now if there is a cast on the LHS move it to the RHS */
        EXPRESSION* expl = (*exp)->left;
        if (castvalue(expl))
        {
            auto exp2 = expl;
            auto exp3 = expl;
            while (castvalue(exp2))
            {
                /* cast on the lhs isn't sign-extended */
                switch (exp2->type)
                {
                    case ExpressionNode::x_c_:
                        exp2->type = ExpressionNode::x_uc_;
                        break;
                    case ExpressionNode::x_s_:
                        exp2->type = ExpressionNode::x_us_;
                        break;
                    case ExpressionNode::x_i_:
                        exp2->type = ExpressionNode::x_ui_;
                        break;
                    case ExpressionNode::x_l_:
                        exp2->type = ExpressionNode::x_ul_;
                        break;
                    case ExpressionNode::x_ll_:
                        exp2->type = ExpressionNode::x_ull_;
                        break;
                    case ExpressionNode::x_bitint_:
                        exp2->type = ExpressionNode::x_ubitint_;
                        break;
                }
                if (castvalue(exp2->left))
                    exp3 = exp2->left;
                exp2 = exp2->left;
            }
            (*exp)->left = exp2;
            exp3->left = (*exp)->right;
            (*exp)->right = exp3;
        }
    }
    if ((*exp)->type == ExpressionNode::pc_ || ((*exp)->type == ExpressionNode::func_ && !(*exp)->v.func->ascall))
        thunkForImportTable(exp);
    return lex;
}
LEXLIST* expression_comma(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_assign(lex, funcsp, atp, tp, exp, ismutable, flags);
    if (*tp == nullptr)
        return lex;
    while (MATCHKW(lex, Keyword::comma_))
    {
        EXPRESSION* exp1 = nullptr;
        TYPE* tp1 = nullptr;
        lex = getsym();
        lex = expression_assign(lex, funcsp, atp, &tp1, &exp1, nullptr, flags);
        if (!tp1)
        {
            break;
        }
        if ((Optimizer::cparams.prm_cplusplus || (Optimizer::architecture == ARCHITECTURE_MSIL)) &&
            insertOperatorFunc(ovcl_comma, Keyword::comma_, funcsp, tp, exp, tp1, exp1, nullptr, flags))
        {
            continue;
        }
        else
        {
            *tp = tp1;
            *exp = exprNode(ExpressionNode::void_, *exp, exp1);
            exp = &(*exp)->right;
        }
    }
    return lex;
}
LEXLIST* expression_no_comma(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, bool* ismutable, int flags)
{
    lex = expression_assign(lex, funcsp, atp, tp, exp, ismutable, flags);
    assignmentUsages(*exp, false);
    return lex;
}
LEXLIST* expression_no_check(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, int flags)
{
    if (flags & _F_TYPETEST)
    {
        anonymousNotAlloc++;
        lex = expression_no_comma(lex, funcsp, atp, tp, exp, nullptr, flags);        
        anonymousNotAlloc--;
    }
    else
    {
        lex = expression_comma(lex, funcsp, atp, tp, exp, nullptr, flags);
    }
    return lex;
}

LEXLIST* expression(LEXLIST* lex, SYMBOL* funcsp, TYPE* atp, TYPE** tp, EXPRESSION** exp, int flags)
{
    lex = expression_comma(lex, funcsp, atp, tp, exp, nullptr, flags);
    assignmentUsages(*exp, false);
    return lex;
}
}  // namespace Parser