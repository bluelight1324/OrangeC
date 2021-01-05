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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "be.h"
#include <stack>
#include "ioptimizer.h"
#include "config.h"
#include "ildata.h"
#include "OptUtils.h"
#include "occ.h"
#include "peep.h"
#include "outasm.h"
#include "memory.h"
#include "symfuncs.h"
#include "gen.h"
#include "ioptutil.h"

#define MAX_ALIGNS 50
namespace occx86
{
int pushlevel = 0;
int funcstackheight = 0;

AMODE *singleLabel, *doubleLabel, *zerolabel;

static int fconstcleanup = -1;
static int floatpop;
static int fstackid;
static int inframe;
static int switch_deflab;
static long long switch_range, switch_case_count, switch_case_max;
static Optimizer::IMODE* switch_ip;
static enum { swm_enumerate, swm_compactstart, swm_compact, swm_tree } switch_mode;
static int switch_lastcase;
static AMODE *switch_apl, *switch_aph;
static int switch_live;
static int *switchTreeLabels, *switchTreeBranchLabels;
static long long* switchTreeCases;
static int switchTreeLabelCount;
static int switchTreePos;
/*static int floatArea; */

/* map the icode version of the regs to the processor version */
bool BackendIntrinsic(Optimizer::QUAD* q);

Optimizer::SimpleExpression* copy_expression(Optimizer::SimpleExpression* node)
{
    Optimizer::SimpleExpression* rv = Allocate<Optimizer::SimpleExpression>();
    memcpy(rv, node, sizeof(*rv));
    if (rv->left)
        rv->left = copy_expression(rv->left);
    if (rv->right)
        rv->right = copy_expression(rv->right);
    return rv;
}
//-------------------------------------------------------------------------

AMODE* make_muldivval(AMODE* ap)
{
    AMODE* ap1 = make_label(queue_muldivval(ap->offset->i));
    ap1->mode = am_direct;
    return (ap1);
}

/*-------------------------------------------------------------------------*/

AMODE* make_label(int lab)
/*
 *      construct a reference node for an internal label number.
 */
{
    Optimizer::SimpleExpression* lnode;
    AMODE* ap;
    lnode = beLocalAllocate<Optimizer::SimpleExpression>();
    lnode->type = Optimizer::se_labcon;
    lnode->i = lab;
    ap = beLocalAllocate<AMODE>();
    ap->mode = am_immed;
    ap->offset = lnode;
    ap->length = ISZ_UINT;
    return ap;
}

/*-------------------------------------------------------------------------*/

AMODE* makesegreg(int seg)
{
    AMODE* ap = beLocalAllocate<AMODE>();
    ap->mode = am_seg;
    ap->seg = seg;
    ap->length = ISZ_SEG;
    return ap;
}

AMODE* makeSSE(int reg)
{
    AMODE* ap = beLocalAllocate<AMODE>();
    ap->mode = am_xmmreg;
    ap->preg = reg;
    ap->length = 0;
    return ap;
}
void make_floatconst(AMODE* ap)
{
    int size = ap->length;
    if (isintconst(ap->offset))
    {
        ap->offset->f = (long long)ap->offset->i;
        ap->offset->type = Optimizer::se_f;
        size = ISZ_DOUBLE;
    }
    else
    {
        size = ap->offset->sizeFromType;
    }
    AMODE* ap1 = make_label(queue_floatval(&ap->offset->f, size));
    ap->mode = am_direct;
    ap->length = 0;
    ap->offset = ap1->offset;
}
AMODE* moveFP(AMODE* apa, int sza, AMODE* apl, int szl)
{
    if (apa->mode != am_xmmreg)
    {
        if (sza < ISZ_FLOAT)
            gen_code_sse(op_cvtsd2si, op_cvtss2si, szl, apa, apl);
        else if (szl < ISZ_FLOAT)
            gen_code_sse(op_cvtsi2ss, op_cvtsi2sd, sza, apa, apl);
        else
        {
            int m1 = (sza - ISZ_FLOAT) % 3;
            int m2 = (szl - ISZ_FLOAT) % 3;
            if ((m1 == 0 && m2 != 0) || (m2 == 0 && m1 != 0))
                gen_code_sse(op_cvtsd2ss, op_cvtss2sd, sza, apa, apl);
            else
                gen_code_sse(op_movss, op_movsd, sza, apa, apl);
        }
    }
    else if (apl->mode == am_xmmreg)
    {
        int m1 = (sza - ISZ_FLOAT) % 3;
        int m2 = (szl - ISZ_FLOAT) % 3;
        if ((m1 && !m2) || (m2 && !m1))
        {
            gen_code_sse(op_cvtsd2ss, op_cvtss2sd, sza, apa, apl);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, sza, apa, apl);
        }
    }
    else
    {
        if (apl->mode == am_immed)
        {
            make_floatconst(apl);
        }
        if (szl < ISZ_FLOAT)
        {
            gen_code_sse(op_cvtsi2ss, op_cvtsi2sd, sza, apa, apl);
            apl->length = szl;
        }
        else
        {
            int m1 = (sza - ISZ_FLOAT) % 3;
            int m2 = (szl - ISZ_FLOAT) % 3;
            if ((m1 == 0 && m2 != 0) || (m2 == 0 && m1 != 0))
                gen_code_sse(op_cvtsd2ss, op_cvtss2sd, sza, apa, apl);
            else
                gen_code_sse(op_movss, op_movsd, sza, apa, apl);
        }
    }
    return apa;
}
/*-------------------------------------------------------------------------*/

AMODE* aimmed(unsigned long long i)
/*
 *      make a node to reference an immediate value i.
 */
{
    AMODE* ap;
    Optimizer::SimpleExpression* ep;
    i &= 0xffffffffU;
    ep = beLocalAllocate<Optimizer::SimpleExpression>();
    ep->type = Optimizer::se_i;
    ep->i = i;
    ap = beLocalAllocate<AMODE>();
    ap->mode = am_immed;
    ap->offset = ep;
    ap->length = ISZ_ADDR;
    return ap;
}

/*-------------------------------------------------------------------------*/

AMODE* aimmedt(long i, int size)
/*
 *      make a node to reference an immediate value i.
 */
{
    AMODE* ap;
    switch (size)
    {
        case ISZ_UCHAR:
        case -ISZ_UCHAR:
            i &= 0xff;
            break;
        case ISZ_U16:
        case ISZ_USHORT:
        case -ISZ_USHORT:
            i &= 0xffff;
            break;
        case ISZ_ADDR:
        case ISZ_UINT:
        case ISZ_ULONG:
        case ISZ_U32:
        case -ISZ_ADDR:
        case -ISZ_UINT:
        case -ISZ_ULONG:
            i &= 0xffffffffU;
            break;
    }
    ap = aimmed(i);
    ap->length = size;
    return ap;
}

/*-------------------------------------------------------------------------*/
bool isauto(Optimizer::SimpleExpression* ep)
{
    if (ep->type == Optimizer::se_auto)
        return true;
    if (ep->type == Optimizer::se_add)
        return isauto(ep->left) || isauto(ep->right);
    if (ep->type == Optimizer::se_sub)
        return isauto(ep->left);
    return false;
}
AMODE* make_offset(Optimizer::SimpleExpression* node)
/*
 *      make a direct reference to a node.
 */
{
    AMODE* ap;
    ap = beLocalAllocate<AMODE>();
    if (node->type == Optimizer::se_tempref)
    {
        diag("make_offset: orignode");
    }
    if (isauto(node))
    {
        ap->preg = usingEsp ? ESP : EBP;
        ap->mode = am_indisp;
    }
    else
        ap->mode = am_direct;
    ap->offset = copy_expression(node);
    ap->seg = 0;
    return ap;
}
/*-------------------------------------------------------------------------*/

AMODE* make_stack(int number)
{
    AMODE* ap = beLocalAllocate<AMODE>();
    Optimizer::SimpleExpression* ep =
        beLocalAllocate<Optimizer::SimpleExpression>();
    ep->type = Optimizer::se_i;
    ep->i = -number;
    ap->mode = am_indisp;
    ap->preg = ESP;
    ap->offset = ep;
    ap->length = ISZ_UINT;
    ap->keepesp = true;
    return (ap);
}
AMODE* setSymbol(const char* name)
/*
 *      generate a call to a library routine.S
 */
{
    AMODE* result;
    Optimizer::SimpleSymbol* sym = Optimizer::SymbolManager::Get(name);
    if (sym == 0)
    {
        Optimizer::LIST* l1;
        sym = Allocate<Optimizer::SimpleSymbol>();
        sym->storage_class = Optimizer::scc_external;
        sym->name = sym->outputName = litlate(name);
        sym->tp = Allocate<Optimizer::SimpleType>();
        sym->tp->type = Optimizer::st_func;
        //        Optimizer::SymbolManager::Add(name, sym);
        Optimizer::externals.push_back(sym);
    }
    result = Allocate<AMODE>();
    result->offset = Allocate<Optimizer::SimpleExpression>();
    result->offset->type = Optimizer::se_global;
    result->offset->sp = sym;
    result->offset->type = Optimizer::se_pc;
    if (Optimizer::chosenAssembler->arch->libsasimports)
        result->mode = am_direct;
    else
        result->mode = am_immed;
    return result;
}
static void callLibrary(const char* name, int size)
{
    /* so it will get in the external list */
    AMODE* an = setSymbol(name);
    gen_code(op_call, an, 0);
    if (size)
        gen_code(op_add, makedreg(ESP), aimmed(size));
}
void oa_gen_vtt(int dataOffset, Optimizer::SimpleSymbol* func)
{
    Optimizer::SimpleExpression* n = Allocate<Optimizer::SimpleExpression>();
    n->type = Optimizer::se_pc;
    n->sp = func;
    AMODE* ofs = make_offset(n);
    ofs->mode = am_immed;
    gen_code(op_add, make_stack(-4), aimmedt(dataOffset, ISZ_ADDR));
    gen_code(op_jmp, ofs, NULL);
    flush_peep(NULL, NULL);
}
void oa_gen_vc1(Optimizer::SimpleSymbol* func)
{
    AMODE* ofs = makedreg(EAX);
    ofs->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
    ofs->mode = am_indisp;
    gen_code(op_mov, makedreg(EAX), makedreg(ECX));
    gen_code(op_mov, makedreg(EAX), ofs);
    ofs->offset = Optimizer::simpleIntNode(Optimizer::se_i, func->offset);
    gen_code(op_jmp, ofs, NULL);
    flush_peep(NULL, NULL);
}
void oa_gen_importThunk(Optimizer::SimpleSymbol* func)
{
    AMODE* ofs = Allocate<AMODE>();
    ofs->mode = am_direct;
    ofs->offset = Allocate<Optimizer::SimpleExpression>();
    ofs->offset->type = Optimizer::se_pc;
    ofs->offset->sp = func;  // was func->mainsym
    gen_code(op_jmp, ofs, NULL);
    flush_peep(NULL, NULL);
}

void make_complexconst(AMODE* ap, AMODE* api)
{
    AMODE* apt;
    if (isintconst(ap->offset))
    {
        api->offset = Optimizer::simpleExpressionNode(Optimizer::se_f, 0, 0); /* defaults to zero. 0 */
        api->offset->sp->sizeFromType = ISZ_DOUBLE;
    }
    else
    {
        switch (ap->offset->sizeFromType)
        {
            case ISZ_FLOAT:
            case ISZ_DOUBLE:
            case ISZ_LDOUBLE:
                api->offset = Optimizer::simpleExpressionNode(Optimizer::se_f, 0, 0); /* defaults to zero. 0 */
                api->offset->sizeFromType = ap->offset->sizeFromType;
                break;
            case ISZ_IFLOAT:
            case ISZ_IDOUBLE:
            case ISZ_ILDOUBLE:
                api->offset = Optimizer::simpleExpressionNode(Optimizer::se_fi, 0, 0); /* defaults to zero. 0 */
                api->offset->sizeFromType = ap->offset->sizeFromType;
                apt = api;
                api = ap;
                ap = apt;
                break;
            case ISZ_CFLOAT:
            case ISZ_CDOUBLE:
            case ISZ_CLDOUBLE:
                api->offset = Optimizer::simpleExpressionNode(Optimizer::se_fi, 0, 0); /* defaults to zero. 0 */
                api->offset->f = ap->offset->c.i;
                api->offset->sizeFromType = ap->offset->sizeFromType;
                ap->offset->type = Optimizer::se_f;
                ap->offset->f = ap->offset->c.r;
                break;
            default:
                break;
        }
    }
    make_floatconst(ap);
    make_floatconst(api);
}
void floatchs(AMODE* ap, int sz)
{
    AMODE* lbl;
    e_opcode op;
    if (sz == ISZ_FLOAT || sz == ISZ_IFLOAT || sz == ISZ_CFLOAT)
    {
        if (!singleLabel)
        {
            singleLabel = setSymbol("__fschsmask");
            singleLabel->mode = am_direct;
        }
        lbl = singleLabel;
        op = op_xorps;
    }
    else
    {
        if (!doubleLabel)
        {
            doubleLabel = setSymbol("__fdchsmask");
            doubleLabel->mode = am_direct;
        }
        lbl = doubleLabel;
        op = op_xorpd;
    }
    int reg = -1;
    if (Optimizer::cparams.prm_lscrtdll)
    {
        bool pushed = false;
        if (!(ap->liveRegs & (1 << EAX)))
        {
            reg = EAX;
        }
        else if (!(ap->liveRegs & (1 << ECX)))
        {
            reg = ECX;
        }
        else if (!(ap->liveRegs & (1 << EDX)))
        {
            reg = EDX;
        }
        else
        {
            gen_code(op_push, makedreg(EAX), NULL);
            pushlevel += 4;
            pushed = true;
            reg = EAX;
        }
        AMODE* ap1 = Allocate<AMODE>();
        *ap1 = *lbl;
        ap1->mode = am_direct;
        lbl = makedreg(reg);
        gen_code(op_mov, lbl, ap1);
        lbl->mode = am_indisp;
        lbl->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        lbl->length = 0;
        if (!pushed)
            reg = -1;
    }
    gen_code(op, ap, lbl);
    if (reg != -1)
    {
        pushlevel -= 4;
        gen_code(op_pop, makedreg(reg), NULL);
    }
}

void zerocleanup(void)
{
    if (fconstcleanup >= 0)
    {
        gen_code(op_pop, makedreg(fconstcleanup), NULL);
        pushlevel -= 4;
    }
    fconstcleanup = -1;
}
AMODE* floatzero(AMODE* ap)
{
    if (!zerolabel)
    {
        zerolabel = setSymbol("__fzero");
        zerolabel->mode = am_direct;
    }
    if (Optimizer::cparams.prm_lscrtdll)
    {
        int reg;
        if (!(ap->liveRegs & (1 << EAX)))
        {
            reg = EAX;
        }
        else if (!(ap->liveRegs & (1 << ECX)))
        {
            reg = ECX;
        }
        else if (!(ap->liveRegs & (1 << EDX)))
        {
            reg = EDX;
        }
        else
        {
            gen_code(op_push, makedreg(EAX), NULL);
            pushlevel += 4;
            reg = EAX;
        }
        AMODE* ap1 = Allocate<AMODE>();
        *ap1 = *zerolabel;
        ap1->mode = am_direct;
        zerolabel = makedreg(reg);
        gen_code(op_mov, zerolabel, ap1);
        zerolabel->mode = am_indisp;
        zerolabel->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
    }
    return zerolabel;
}

bool sameTemp(Optimizer::QUAD* head);
int beRegFromTempInd(Optimizer::QUAD* q, Optimizer::IMODE* im, int which)
{
    if (which)
    {
        return (q->scaleColor < 0) ? 0 : q->scaleColor;
    }
    if (im == q->ans)
    {
        return (q->ansColor < 0) ? 0 : q->ansColor;
    }
    else if (im == q->dc.left)
    {
        return (q->leftColor < 0) ? 0 : q->leftColor;
    }
    else
    {
        return (q->rightColor < 0) ? 0 : q->rightColor;
    }
}
int beRegFromTemp(Optimizer::QUAD* q, Optimizer::IMODE* im) { return beRegFromTempInd(q, im, 0); }
bool sameTemp(Optimizer::QUAD* head)
{
    if ((head->temps & (TEMP_LEFT | TEMP_RIGHT)) == (TEMP_LEFT | TEMP_RIGHT))
    {
        if (head->dc.left->mode == Optimizer::i_direct && head->dc.right->mode == Optimizer::i_direct)
        {
            if (head->dc.left->offset->sp->i == head->dc.right->offset->sp->i)
            {
                return true;
            }
        }
    }
    return false;
}
int imaginary_offset(int sz)
{
    int size;
    switch (sz)
    {
        case ISZ_CFLOAT:
            size = 4;
            break;
        case ISZ_CDOUBLE:
        case ISZ_CLDOUBLE:
        default:
            size = 8;
            break;
    }
    return size;
}

int samereg(AMODE* ap1, AMODE* ap2)
{
    switch (ap1->mode)
    {
        case am_indisp:
        case am_dreg:
            switch (ap2->mode)
            {
                case am_indisp:
                case am_dreg:
                    return ap1->preg == ap2->preg;
                case am_indispscale:
                    return ap1->preg == ap2->preg || ap1->preg == ap2->sreg;
                default:
                    break;
            }
            break;
        case am_indispscale:
            switch (ap2->mode)
            {
                case am_indispscale:
                    if (ap2->preg == ap1->preg || ap2->preg == ap1->sreg)
                        return true;
                    /* fall through */
                case am_indisp:
                case am_dreg:
                    return ap1->preg == ap2->preg || ap1->preg == ap2->sreg;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return false;
}
void getAmodes(Optimizer::QUAD* q, enum e_opcode* op, Optimizer::IMODE* im, AMODE** apl, AMODE** aph)
{
    *op = op_mov;
    *aph = 0;
    if (im->offset && im->offset->type == Optimizer::se_threadlocal)
    {
        AMODE* temp = setSymbol("__TLSINITSTART");
        temp->mode = am_immed;
        temp->offset = Optimizer::simpleExpressionNode(Optimizer::se_sub, im->offset, temp->offset);
        gen_code(op_push, temp, 0);
        callLibrary("___tlsaddr", 0);
        *apl = beLocalAllocate<AMODE>();
        (*apl)->preg = Optimizer::chosenAssembler->arch->regMap[beRegFromTemp(q, q->ans)][0];
        (*apl)->mode = am_dreg;
        gen_codes(op_pop, ISZ_ADDR, (*apl), 0);
    }
    else if (im->mode == Optimizer::i_ind)
    {
        enum e_am mode;
        if ((im->offset && im->offset2) || (im->offset2 && im->scale))
            mode = am_indispscale;
        else if (im->offset || im->offset2)
            mode = am_indisp;
        else
            mode = am_direct;
        *apl = beLocalAllocate<AMODE>();
        {
            int reg = Optimizer::chosenAssembler->arch->regMap[beRegFromTempInd(q, im, 1)][0];
            if (im->offset)
            {
                (*apl)->preg = Optimizer::chosenAssembler->arch->regMap[beRegFromTemp(q, im)][0];
                (*apl)->sreg = im->offset2 ? reg : -1;
            }
            else if (mode == am_indisp && im->offset2)
            {
                (*apl)->preg = reg;
                (*apl)->sreg = -1;
            }
            else
            {
                (*apl)->preg = -1;
                (*apl)->sreg = im->offset2 ? reg : -1;
            }
        }
        (*apl)->scale = im->scale;
        (*apl)->offset = im->offset3 ? im->offset3 : Optimizer::simpleIntNode(Optimizer::se_i, 0);
        if (im->size < ISZ_FLOAT)
            (*apl)->length = im->size;
        if (im->offset3)
        {
            if (isauto(im->offset3))
            {
                if ((*apl)->preg == -1)
                {
                    if ((*apl)->sreg == -1)
                        mode = am_indisp;
                    else
                        mode = am_indispscale;
                    if (usingEsp)
                    {
                        (*apl)->preg = ESP;
                    }
                    else
                    {
                        (*apl)->preg = EBP;
                    }
                }
                else if ((*apl)->sreg == -1)
                {
                    mode = am_indispscale;
                    if (usingEsp)
                    {
                        (*apl)->sreg = (*apl)->preg;
                        (*apl)->preg = ESP;
                    }
                    else
                    {
                        (*apl)->sreg = EBP;
                    }
                }
                else
                    diag("getAmodes: no room for EBP in ind processing");
            }
        }
        (*apl)->mode = mode;
        if (im->size >= ISZ_CFLOAT)
        {
            *aph = beLocalAllocate<AMODE>();
            **aph = **apl;
            (*aph)->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, (*apl)->offset,
                                                             Optimizer::simpleIntNode(Optimizer::se_i, imaginary_offset(im->size)));
            if ((*apl)->preg >= 0)
                (*apl)->liveRegs |= 1 << (*apl)->preg;
            if ((*apl)->sreg >= 0)
                (*apl)->liveRegs |= 1 << (*apl)->sreg;
        }
        else if (im->size == ISZ_ULONGLONG || im->size == -ISZ_ULONGLONG)
        {
            *aph = beLocalAllocate<AMODE>();
            **aph = **apl;
            (*aph)->offset =
                Optimizer::simpleExpressionNode(Optimizer::se_add, (*apl)->offset, Optimizer::simpleIntNode(Optimizer::se_i, 4));
            if ((*apl)->preg >= 0)
                (*apl)->liveRegs |= 1 << (*apl)->preg;
            if ((*apl)->sreg >= 0)
                (*apl)->liveRegs |= 1 << (*apl)->sreg;
        }
    }
    else if (im->mode == Optimizer::i_immed)
    {
        if (im->size >= ISZ_CFLOAT)
        {
            *apl = beLocalAllocate<AMODE>();
            *aph = beLocalAllocate<AMODE>();
            (*apl)->offset = im->offset;
            make_complexconst(*apl, *aph);
        }
        else if (im->size >= ISZ_FLOAT)
        {
            *apl = beLocalAllocate<AMODE>();
            (*apl)->offset = im->offset;
            make_floatconst(*apl);
        }
        else if (im->size == ISZ_ULONGLONG || im->size == -ISZ_ULONGLONG)
        {
            *apl = aimmed(im->offset->i);
#ifdef USE_LONGLONG
            *aph = aimmed((im->offset->i >> 32));
#else
            if (im->size < 0 && im->offset->i < 0)
                *aph = aimmed(-1);
            else
                *aph = aimmed(0);
#endif
        }
        else
        {
            if (iscomplexconst(im->offset))
            {
                *apl = beLocalAllocate<AMODE>();
                (*apl)->offset = im->offset;
                *aph = beLocalAllocate<AMODE>();
                make_complexconst(*apl, *aph);
            }
            else if (isfloatconst(im->offset) || isimaginaryconst(im->offset))
            {
                *apl = beLocalAllocate<AMODE>();
                (*apl)->offset = im->offset;
                make_floatconst(*apl);
            }
            else
            {
                if (!isintconst(im->offset))
                {
                    *apl = make_offset(im->offset);
                    if ((*apl)->mode == am_indisp)
                        *op = op_lea;
                    else
                        (*apl)->mode = am_immed;
                    if (im->size < ISZ_FLOAT)
                        (*apl)->length = im->size;
                }
                else if (im->offset->sizeFromType == ISZ_ULONGLONG || im->offset->sizeFromType == -ISZ_ULONGLONG)
                {
                    *apl = aimmed(im->offset->i);
#ifdef USE_LONGLONG
                    *aph = aimmed((im->offset->i >> 32));
#else
                    if (im->size < 0 && im->offset->i < 0)
                        *aph = aimmed(-1);
                    else
                        *aph = aimmed(0);
#endif
                }
                else
                {
                    *apl = make_offset(im->offset);
                    if ((*apl)->mode == am_indisp)
                        *op = op_lea;
                    else
                    {
                        (*apl)->offset->i &= 0xffffffffU;
                        (*apl)->mode = am_immed;
                    }
                    // in case of a int const being used with a long long
#ifdef USE_LONGLONG
                    *aph = aimmed((im->offset->i >> 32));
#else
                    if (im->size < 0 && im->offset->i < 0)
                        *aph = aimmed(-1);
                    else
                        *aph = aimmed(0);
#endif
                }
            }
        }
    }
    else if (im->mode == Optimizer::i_direct)
    {
        /*
            if (im->offset->type == Optimizer::se_reg)
            {
                *apl = makedreg(im->offset->i);
            }
            else
        */
        if (im->size >= ISZ_FLOAT)
        {
            if (im->size >= ISZ_CFLOAT)
            {
                if (im->offset->type == Optimizer::se_tempref && !im->offset->right)
                {
                    int clr = beRegFromTemp(q, im);
                    int reg1, reg2;
                    if (clr == 0)
                    {
                        reg1 = 1;
                        reg2 = 0;
                    }
                    else
                    {
                        reg1 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][1];
                        reg2 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][0];
                    }
                    *apl = makeSSE(reg2);
                    *aph = makeSSE(reg1);
                    //                    (*apl)->liveRegs = (*aph)->liveRegs = (1 << reg1) | (1 << reg2);
                }
                else
                {
                    *apl = make_offset(im->offset);
                    *aph = make_offset(im->offset);
                    (*aph)->offset = Optimizer::simpleExpressionNode(
                        Optimizer::se_add, (*aph)->offset, Optimizer::simpleIntNode(Optimizer::se_i, imaginary_offset(im->size)));
                }
            }
            else if (im->offset->type == Optimizer::se_tempref && !im->offset->right)
            {
                int clr = beRegFromTemp(q, im);
                int reg;
                reg = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][0];
                *apl = makeSSE(reg);
            }
            else
                *apl = make_offset(im->offset);
        }
        else if (im->size == ISZ_ULONGLONG || im->size == -ISZ_ULONGLONG)
        {
            if (im->offset->type == Optimizer::se_tempref)
            {
                int clr = beRegFromTemp(q, im);
                int reg1, reg2;
                reg1 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][1];
                reg2 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][0];
                *apl = makedreg(reg2);
                *aph = makedreg(reg1);
                (*apl)->liveRegs = (*aph)->liveRegs = (1 << reg1) | (1 << reg2);
            }
            else
            {
                *apl = make_offset(im->offset);
                *aph = copy_addr(*apl);
                (*aph)->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, (*aph)->offset,
                                                                 Optimizer::simpleIntNode(Optimizer::se_i, 4));
            }
        }
        else if (im->offset->type == Optimizer::se_tempref)
        {
            int l = Optimizer::chosenAssembler->arch->regMap[beRegFromTemp(q, im)][0];
            *apl = makedreg(l);
            if (im->size < ISZ_FLOAT)
                (*apl)->length = im->size;
        }
        else
        {
            *apl = make_offset(im->offset);
            if (im->size < ISZ_FLOAT)
                (*apl)->length = im->size;
        }
    }
    if (!(*aph))
        if ((*apl)->liveRegs == -1)
            (*apl)->liveRegs = 0;
    (*apl)->liveRegs |= q->liveRegs;
    if (*aph)
        (*aph)->liveRegs |= q->liveRegs;
}
void bit_store(AMODE* dest, AMODE* src, int size, int bits, int startbit)
{
    if (src->mode == am_immed && isintconst(src->offset))
    {
        if (bits == 1)
        {
            if (src->offset->i & 1)
                gen_codes(op_bts, ISZ_UINT, dest, aimmed(startbit));
            else
                gen_codes(op_btr, ISZ_UINT, dest, aimmed(startbit));
        }
        else
        {
            int l = dest->liveRegs;
            if (dest->mode == am_indisp && dest->preg != EBP)
                dest->liveRegs |= 1 << dest->preg;
            if (bits != 32)
            {
                gen_codes(op_and, size, dest, aimmed(~(((1 << bits) - 1) << startbit)));
            }
            dest->liveRegs = l;
            gen_codes(op_or, size, dest, aimmed((src->offset->i & ((1 << bits) - 1)) << startbit));
        }
    }
    else
    {
        if (bits != 32)
        {
            gen_codes(op_and, size, src, aimmed((1 << bits) - 1));
            gen_codes(op_and, size, dest, aimmed(~(((1 << bits) - 1) << startbit)));
            if (startbit)
                gen_codes(op_shl, size, src, aimmed(startbit));
            gen_codes(op_or, size, dest, src);
        }
        else
            gen_codes(op_mov, size, dest, src);
    }
}
void liveQualify(AMODE* reg, AMODE* left, AMODE* right)
{
    if (left)
    {
        if (left->mode == am_indisp)
        {
            reg->liveRegs |= 1 << left->preg;
        }
        else if (left->mode == am_indispscale)
        {
            reg->liveRegs |= 1 << left->preg;
            reg->liveRegs |= 1 << left->sreg;
        }
    }
    if (right)
    {
        if (right->mode == am_indisp)
        {
            reg->liveRegs |= 1 << right->preg;
        }
        else if (right->mode == am_indispscale)
        {
            reg->liveRegs |= 1 << right->preg;
            reg->liveRegs |= 1 << right->sreg;
        }
    }
}
bool overlaps(AMODE* apal, AMODE* apah, AMODE* apll, AMODE* aplh)
{
    bool overlap = false;
    if (apll->mode == am_dreg || apll->mode == am_indisp || apll->mode == am_indispscale)
        overlap = overlap || apll->preg == apal->preg || apll->preg == apah->preg;
    if (apll->mode == am_dreg)
        overlap = overlap || aplh->preg == apal->preg || aplh->preg == apah->preg;
    if (apll->mode == am_indispscale)
        overlap = overlap || apll->sreg == apal->preg || apll->sreg == apah->preg;
    return overlap;
}
void func_axdx(enum e_opcode func, AMODE* apal, AMODE* apah, AMODE* apll, AMODE* aplh)
{
    enum e_opcode func1 = func;
    liveQualify(apal, apal, apah);
    liveQualify(apah, apal, apah);
    if (apll->mode == am_immed && !aplh)
    {
        if (isintconst(apll->offset))
        {
            aplh = aimmedt(apll->offset->i >> 32, ISZ_UINT);
            apll->offset->i &= 0xffffffff;
        }
        else
            aplh = aimmedt(0, ISZ_UINT);
    }
    if (func == op_add || func == op_sub)
    {
        func1 = func == op_add ? op_adc : op_sbb;
    }
    if (apal->mode == am_dreg)
    {
        bool overlap = (apll->mode == am_indisp && apll->preg == apal->preg) ||
                       (apll->mode == am_indispscale && (apll->preg == apal->preg || apll->sreg == apal->preg));
        if (overlap)
        {
            if (func == op_add || func == op_sub)
            {
                gen_codes(func, ISZ_UINT, apah, aplh);
                gen_codes(func, ISZ_UINT, apal, apll);
                gen_codes(func1, ISZ_UINT, apah, aimmed(0));
            }
            else
            {
                gen_codes(func1, ISZ_UINT, apah, aplh);
                gen_codes(func, ISZ_UINT, apal, apll);
            }
            return;
        }
    }
    if (func == op_mov)
    {
        if (equal_address(apal, aplh))
        {
            if (equal_address(apll, apah))
            {
                /* just in case, reg allocator shouldn't do this though */
                gen_codes(op_xchg, ISZ_UINT, apal, apah);
            }
            else
            {
                gen_codes(func1, ISZ_UINT, apah, aplh);
                gen_codes(func, ISZ_UINT, apal, apll);
            }
        }
        else
        {
            gen_codes(func, ISZ_UINT, apal, apll);
            gen_codes(func1, ISZ_UINT, apah, aplh);
        }
    }
    else
    {
        gen_codes(func, ISZ_UINT, apal, apll);
        gen_codes(func1, ISZ_UINT, apah, aplh);
    }
}
void func_axdx_mov(enum e_opcode op, AMODE* apal, AMODE* apah, AMODE* apll, AMODE* aplh, AMODE* aprl, AMODE* aprh)
{
    if (overlaps(apal, apah, aprl, aprh))
    {
        func_axdx(op_mov, apal, apah, aprl, aprh);
        func_axdx(op, apal, apah, apll, aplh);
        if (op == op_sub)
        {
            gen_codes(op_neg, ISZ_UINT, apah, 0);
            gen_codes(op_neg, ISZ_UINT, apal, 0);
            gen_codes(op_sbb, ISZ_UINT, apah, aimmed(0));
        }
    }
    else
    {
        func_axdx(op_mov, apal, apah, apll, aplh);
        func_axdx(op, apal, apah, aprl, aprh);
    }
}
void gen_lshift(enum e_opcode op, AMODE* aph, AMODE* apl, AMODE* n)
{
    if (apl->mode != am_dreg || aph->mode != am_dreg)
    {
        int reg = -1;
        bool pushed = false;
        if (!(apl->liveRegs & (1 << EAX)))
        {
            reg = EAX;
        }
        else if (!(apl->liveRegs & (1 << ECX)))
        {
            reg = ECX;
        }
        else if (!(apl->liveRegs & (1 << EDX)))
        {
            reg = EDX;
        }
        else
        {
            gen_code(op_push, makedreg(EAX), NULL);
            pushlevel += 4;
            pushed = true;
            reg = EAX;
        }
        if (op == op_shrd)
        {
            gen_code(op_shr, apl, n);
            gen_code(op_mov, makedreg(reg), aph);
            gen_code(op_ror, makedreg(reg), n);
            gen_code(op_and, makedreg(reg), aimmed(-1 >> (n->offset->i)));
            gen_code(op_or, apl, makedreg(reg));
        }
        else
        {
            gen_code(op_shl, aph, n);
            gen_code(op_mov, makedreg(reg), apl);
            gen_code(op_rol, makedreg(reg), n);
            gen_code(op_and, makedreg(reg), aimmed((1 << (n->offset->i)) - 1));
            gen_code(op_or, aph, makedreg(reg));
        }
        if (pushed)
        {
            gen_code(op_pop, makedreg(EAX), NULL);
            pushlevel -= 4;
        }
    }
    else
    {
        gen_code3(op, aph, apl, n);
    }
}
void gen_xset(Optimizer::QUAD* q, enum e_opcode pos, enum e_opcode neg, enum e_opcode flt)
{
    enum e_opcode op = pos, opa;
    Optimizer::IMODE* left = q->dc.left;
    Optimizer::IMODE* right = q->dc.right;
    AMODE *apll, *aplh, *aprl, *aprh, *apal, *apah;
    bool assign = false;
    bool stacked = false;
    AMODE* altreg = nullptr;
    if (left->mode == Optimizer::i_immed && left->size < ISZ_FLOAT)
    {
        Optimizer::IMODE* t = right;
        right = left;
        left = t;
        op = neg;
    }
    if (left->bits && right->mode == Optimizer::i_immed && isintconst(right->offset))
    {
        if (left->bits != 1)
            diag("gen_xset: too many bits");
        getAmodes(q, &opa, left, &apll, &aplh);
        getAmodes(q, &opa, q->ans, &apal, &apah);
        gen_codes(op_bt, ISZ_UINT, apll, aimmed(q->dc.left->startbit));
        if (op == op_sete)
        {
            if (right->offset->i & 1)
            {
                op = op_setc;
            }
            else
            {
                op = op_setnc;
            }
        }
        else if (op == op_setne)
        {
            if (right->offset->i & 1)
            {
                op = op_setnc;
            }
            else
            {
                op = op_setc;
            }
        }
        else
            diag("gen_xset: Unknown bit type");
        if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
            gen_codes(op_mov, ISZ_UINT, apal, aimmed(0));
        gen_codes(op, ISZ_UCHAR, apal, 0);
        return;
    }
    if (left->size >= ISZ_FLOAT)
    {
        getAmodes(q, &opa, right, &aprl, &aprh);
        getAmodes(q, &opa, left, &apll, &aplh);
        if (flt == op_sete || flt == op_setne)
        {
            gen_code_sse(op_ucomiss, op_ucomisd, left->size, apll, aprl);
            getAmodes(q, &opa, q->ans, &apal, &apah);
            if (flt == op_sete)
                flt = op_setnp;
            else
                flt = op_setp;
            if ((apal->mode != am_dreg || apal->preg != EAX) && live(apll->liveRegs, EAX))
                gen_code(op_push, makedreg(EAX), nullptr);
            gen_code(op_lahf, nullptr, nullptr);
            // if the 'parity' and 'zero' flags are the same, they are not equal
            // otherwise the zero flag will be set so they are equal
            gen_codes(op_test, ISZ_UCHAR, makedreg(AH), aimmed(0x44));
            if ((apal->mode != am_dreg || apal->preg != EAX) && live(apll->liveRegs, EAX))
                gen_code(op_pop, makedreg(EAX), nullptr);
            gen_codes(flt, ISZ_UCHAR, apal, 0);
            gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
            return;
        }
        else
        {
            // have to use seta/setae throughout, because
            // it is the only way to get a valid state from the carry flag
            // (nans will set it)
            // setae/seta are selected in rewritex86 module
            gen_code_sse(op_comiss, op_comisd, left->size, apll, aprl);
            getAmodes(q, &opa, q->ans, &apal, &apah);
            gen_codes(flt, ISZ_UCHAR, apal, 0);
            gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
            return;
        }
    }
    getAmodes(q, &opa, left, &apll, &aplh);
    getAmodes(q, &opa, right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (apal->preg > EBX)
    {
        int i;
        for (i = 0; i < 4; i++)
        {
            if (!live(apal->liveRegs, i))
            {
                altreg = makedreg(i);
                assign = true;
                break;
            }
        }
        if (!assign)
        {
            altreg = make_stack(0);
            stacked = true;
        }
    }
    if (left->size == ISZ_ULONGLONG || left->size == -ISZ_ULONGLONG)
    {
        int lab = beGetLabel;
        int sop = op;
        switch (op)
        {
            case op_setg:
                sop = op_seta;
                break;
            case op_setge:
                sop = op_setnc;
                break;
            case op_setl:
                sop = op_setc;
                break;
            case op_setle:
                sop = op_setbe;
                break;
            default:
                break;
        }
        if (op == sop)

        {
            gen_codes(op_cmp, ISZ_UINT, aplh, aprh);
            peep_tail->oper1->liveRegs = -1;
            gen_branch(op_jne, lab);
            gen_codes(op_cmp, ISZ_UINT, apll, aprl);
            peep_tail->oper1->liveRegs = -1;
            oa_gen_label(lab);
            if (assign)
            {
                gen_codes(op, ISZ_UCHAR, altreg, 0);
                gen_codes(op_and, ISZ_UINT, altreg, aimmed(1));
                gen_codes(op_mov, ISZ_UINT, apal, altreg);
            }
            else if (stacked)
            {
                gen_code(op_push, aimmed(0), NULL);
                gen_codes(op, ISZ_UCHAR, altreg, 0);
                gen_code(op_pop, apal, NULL);
            }
            else
            {
                gen_codes(op, ISZ_UCHAR, apal, 0);
                gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
            }
        }
        else
        {
            int lab2 = beGetLabel;
            gen_codes(op_cmp, ISZ_UINT, aplh, aprh);
            peep_tail->oper1->liveRegs = -1;
            gen_branch(op_jne, lab);
            gen_codes(op_cmp, ISZ_UINT, apll, aprl);
            peep_tail->oper1->liveRegs = -1;
            if (assign)
            {
                gen_codes(sop, ISZ_UCHAR, altreg, 0);
                if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
                    gen_codes(op_and, ISZ_UINT, altreg, aimmed(1));
                gen_codes(op_mov, ISZ_UINT, apal, altreg);
            }
            else if (stacked)
            {
                gen_code(op_push, aimmed(0), NULL);
                gen_codes(sop, ISZ_UCHAR, altreg, 0);
                gen_code(op_pop, apal, NULL);
            }
            else
            {
                gen_codes(sop, ISZ_UCHAR, apal, 0);
                if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
                    gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
            }
            gen_branch(op_jmp, lab2);
            oa_gen_label(lab);
            if (assign)
            {
                gen_codes(op, ISZ_UCHAR, altreg, 0);
                if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
                    gen_codes(op_and, ISZ_UINT, altreg, aimmed(1));
                gen_codes(op_mov, ISZ_UINT, apal, altreg);
            }
            else if (stacked)
            {
                gen_code(op_push, aimmed(0), NULL);
                gen_codes(op, ISZ_UCHAR, altreg, 0);
                gen_code(op_pop, apal, NULL);
            }
            else
            {
                gen_codes(op, ISZ_UCHAR, apal, 0);
                if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
                    gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
            }
            oa_gen_label(lab2);
        }
        return;
    }
    else
    {
        gen_codes(op_cmp, left->size, apll, aprl);
        peep_tail->oper1->liveRegs = q->liveRegs;
    }
    if (assign)
    {
        gen_codes(op, ISZ_UCHAR, altreg, 0);
        if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
            gen_codes(op_and, ISZ_UINT, altreg, aimmed(1));
        gen_codes(op_mov, ISZ_UINT, apal, altreg);
    }
    else if (stacked)
    {
        gen_code(op_push, aimmed(0), NULL);
        gen_codes(op, ISZ_UCHAR, altreg, 0);
        gen_code(op_pop, apal, NULL);
    }
    else
    {
        gen_codes(op, ISZ_UCHAR, apal, 0);
        if (apal->length != ISZ_UCHAR && apal->length != -ISZ_UCHAR)
            gen_codes(op_and, ISZ_UINT, apal, aimmed(1));
    }
}
void gen_goto(Optimizer::QUAD* q, enum e_opcode pos, enum e_opcode neg, enum e_opcode llpos, enum e_opcode llneg,
              enum e_opcode llintermpos, enum e_opcode llintermneg, enum e_opcode flt)
{
    enum e_opcode sop = pos, sop1 = llpos, top = llneg, top1 = llintermpos, opa;
    Optimizer::IMODE* left = q->dc.left;
    Optimizer::IMODE* right = q->dc.right;
    AMODE *apll, *aplh, *aprl, *aprh;
    if (left->mode == Optimizer::i_immed && left->size < ISZ_FLOAT)
    {
        Optimizer::IMODE* t = right;
        right = left;
        left = t;
        if (pos != op_je && pos != op_jne)
        {
            sop = neg;
            sop1 = llneg;
            top = llpos;
            top1 = llintermneg;
        }
    }
    if (left->bits && right->mode == Optimizer::i_immed && isintconst(right->offset))
    {
        if (left->bits != 1)
            diag("gen_goto: too many bits");
        getAmodes(q, &opa, left, &apll, &aplh);
        gen_codes(op_bt, ISZ_UINT, apll, aimmed(q->dc.left->startbit));
        if (sop == op_je)
        {
            if (right->offset->i & 1)
            {
                sop = op_jc;
            }
            else
            {
                sop = op_jnc;
            }
        }
        else if (sop == op_jne)
        {
            if (right->offset->i & 1)
            {
                sop = op_jnc;
            }
            else
            {
                sop = op_jc;
            }
        }
        else
            diag("gen_goto: Unknown bit type");
        gen_branch(sop, q->dc.v.label);
        return;
    }
    if (left->size >= ISZ_FLOAT)
    {

        getAmodes(q, &opa, right, &aprl, &aprh);
        getAmodes(q, &opa, left, &apll, &aplh);
        if (flt == op_je || flt == op_jne)
        {
            gen_code_sse(op_ucomiss, op_ucomisd, left->size, apll, aprl);
            if (flt == op_je)
                flt = op_jnp;
            else
                flt = op_jp;
            if (live(apll->liveRegs, EAX))
                gen_code(op_push, makedreg(EAX), nullptr);
            gen_code(op_lahf, nullptr, nullptr);
            // if the 'parity' and 'zero' flags are the same, they are not equal
            // otherwise the zero flag will be set so they are equal
            gen_codes(op_test, ISZ_UCHAR, makedreg(AH), aimmed(0x44));
            if (live(apll->liveRegs, EAX))
                gen_code(op_pop, makedreg(EAX), nullptr);
            gen_branch(flt, q->dc.v.label);
            return;
        }
        else
        {
            // have to use jb/jbe throughout, because
            // it is the only way to get a valid state from the carry flag
            // (nans will set it)
            // jbe/jb are selected in rewritex86 module
            gen_code_sse(op_comiss, op_comisd, left->size, apll, aprl);
            gen_branch(flt, q->dc.v.label);
        }
    }
    else
    {
        getAmodes(q, &opa, left, &apll, &aplh);
        getAmodes(q, &opa, right, &aprl, &aprh);
        if (left->size == ISZ_ULONGLONG || left->size == -ISZ_ULONGLONG)
        {
            int lab = beGetLabel;
            gen_codes(op_cmp, ISZ_UINT, aplh, aprh);
            peep_tail->oper1->liveRegs = -1;
            if (top != op_jne)
                gen_branch(sop1, q->dc.v.label);
            if (top != op_je)
                gen_branch(top, lab);
            gen_codes(op_cmp, ISZ_UINT, apll, aprl);
            peep_tail->oper1->liveRegs = -1;
            gen_branch(top1, q->dc.v.label);
            oa_gen_label(lab);
        }
        else
        {
            int size;
            //			if (isintconst(left->offset))
            //				size = right->size;
            //			else
            size = left->size;
            gen_codes(op_cmp, size, apll, aprl);
            peep_tail->oper1->liveRegs = q->liveRegs;
            gen_branch(sop, q->dc.v.label);
        }
    }
}
static void gen_div(Optimizer::QUAD* q, enum e_opcode op) /* unsigned division */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    int mod = q->dc.opcode == Optimizer::i_umod || q->dc.opcode == Optimizer::i_smod;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_FLOAT)
    {
        if (mod)
        {
            diag("gen_div: floating point in mod operation");
        }
        if (equal_address(apal, apll))
        {
            gen_code_sse(op_divss, op_divsd, q->ans->size, apal, aprl);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            gen_code_sse(op_divss, op_divsd, q->ans->size, apal, aprl);
        }
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        diag("gen_div: long long");
    }
    else
    {
        AMODE* divby = aprl;
        if (apal->mode != am_dreg)
            diag("asm_udiv: answer not a dreg");
        if (aprl->mode == am_immed)
        {
            divby = make_muldivval(aprl);
        }
        if (op == op_div)
        {
            gen_code(op_mov, makedreg(EDX), aimmed(0));
        }
        else
        {
            gen_code(op_cdq, 0, 0);
        }
        divby->liveRegs = q->liveRegs;
        gen_codes(op, q->ans->size, divby, 0);
    }
}
static void gen_mulxh(Optimizer::QUAD* q, enum e_opcode op) /* unsigned division */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    int mod = q->dc.opcode == Optimizer::i_umod || q->dc.opcode == Optimizer::i_smod;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_FLOAT)
    {
        diag("gen_mulxh: floating point");
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        diag("gen_mulxh: long long");
    }
    else
    {
        AMODE* mulby = aprl;
        if (apal->mode != am_dreg)
            diag("asm_mulxh: answer not a dreg");
        if (apll->mode == am_immed)
        {
            gen_codes(op_mov, q->ans->size, makedreg(EAX), apll);
        }
        if (aprl->mode == am_immed)
        {
            mulby = make_muldivval(aprl);
        }
        mulby->liveRegs = q->liveRegs;
        gen_codes(op, q->ans->size, mulby, 0);
    }
}
static void gen_shift(Optimizer::QUAD* q, enum e_opcode op, AMODE* apal, AMODE* apll, AMODE* aprl)
{
    AMODE* cx = makedreg(ECX);
    cx->liveRegs = q->liveRegs;
    cx->length = ISZ_UCHAR;
    if (aprl->mode == am_immed)
    {
        gen_code(op_mov, apal, apll);
        gen_code(op, apal, aprl);
    }
    else if (equal_address(apal, aprl))
    {
        if (aprl->mode == am_dreg && aprl->preg == ECX)
        {
            if (apll->mode == am_immed)
            {
                gen_codes(op_push, ISZ_UINT, apll, NULL);
                gen_code(op, make_stack(0), cx);
                gen_codes(op_pop, ISZ_UINT, cx, NULL);
            }
            else
            {
                bool pushed = false;
                if (apll->mode != am_dreg || live(apal->liveRegs, apll->preg))
                {
                    pushed = true;
                    gen_codes(op_push, ISZ_UINT, apll, NULL);
                    pushlevel += 4;
                }
                gen_code(op, apll, cx);
                gen_codes(op_mov, ISZ_UINT, apal, apll);
                if (pushed)
                {
                    gen_codes(op_pop, ISZ_UINT, apll, NULL);
                    pushlevel -= 4;
                }
            }
        }
        else if (apll->mode == am_dreg && apll->preg == ECX)
        {
            gen_codes(op_xchg, ISZ_UINT, apll, aprl);
            gen_code(op, aprl, cx);
            gen_codes(op_xchg, ISZ_UINT, apll, aprl);
        }
        else
        {
            if (apll->mode == am_immed)
            {
                gen_codes(op_xchg, ISZ_UINT, aprl, cx);
                gen_codes(op_push, ISZ_UINT, apll, NULL);
                gen_code(op, make_stack(0), cx);
                gen_codes(op_pop, ISZ_UINT, cx, NULL);
                gen_codes(op_xchg, ISZ_UINT, aprl, cx);
            }
            else
            {
                bool pushed = false;
                if (apll->mode != am_dreg || live(apal->liveRegs, apll->preg))
                {
                    pushed = true;
                    gen_codes(op_push, ISZ_UINT, apll, NULL);
                    pushlevel += 4;
                }
                gen_codes(op_xchg, ISZ_UINT, aprl, cx);
                gen_code(op, apll, cx);
                gen_codes(op_xchg, ISZ_UINT, aprl, cx);
                gen_codes(op_mov, ISZ_UINT, apal, apll);
                if (pushed)
                {
                    gen_codes(op_pop, ISZ_UINT, apll, NULL);
                    pushlevel -= 4;
                }
            }
        }
    }
    else
    {
        if (aprl->mode == am_dreg && aprl->preg == ECX)
        {
            if (apal->mode == am_dreg && apal->preg == ECX)
            {
                if (apll->mode != am_dreg || (apll->liveRegs & (1 << apll->preg)))
                {
                    gen_code(op_push, apll, NULL);
                    pushlevel += 4;
                    gen_code(op, apll, cx);
                    gen_codes(op_mov, ISZ_UINT, apal, apll);
                    gen_code(op_pop, apll, NULL);
                    pushlevel -= 4;
                }
                else
                {
                    gen_code(op, apll, cx);
                    gen_codes(op_mov, ISZ_UINT, apal, apll);
                }
            }
            else
            {
                gen_codes(op_mov, ISZ_UINT, apal, apll);
                gen_code(op, apal, cx);
            }
        }
        else if (apal->mode == am_dreg && apal->preg == ECX)
        {
            gen_codes(op_push, ISZ_UINT, apll, NULL);
            pushlevel += 4;
            gen_codes(op_mov, ISZ_UINT, cx, aprl);
            gen_code(op, make_stack(0), cx);
            gen_codes(op_pop, ISZ_UINT, cx, NULL);
            pushlevel -= 4;
        }
        else
        {
            gen_codes(op_xchg, ISZ_UINT, aprl, cx);
            gen_codes(op_mov, ISZ_UINT, apal, apll);
            gen_code(op, apal, cx);
            gen_codes(op_xchg, ISZ_UINT, aprl, cx);
        }
    }
}
static void do_movzx(int dsize, int ssize, AMODE* dest, AMODE* src)
{
    gen_code2(op_movzx, dsize, ssize, dest, src);
    return;

    if (!equal_address(dest, src))
    {
        gen_code2(op_movzx, dsize, ssize, dest, src);
        //		gen_code(op_xor, dest, dest);
        //		gen_codes(op_mov, ssize, dest, src);
    }
    else
    {
        AMODE* am;
        if (ssize == ISZ_UCHAR)
            am = aimmed(0xff);
        else
            am = aimmed(0xffff);
        gen_codes(op_and, ISZ_UINT, dest, am);
    }
}
static void do_movsx(int dsize, int ssize, AMODE* dest, AMODE* src)
{
    if (dest->preg == EAX)
    {
        if (!equal_address(dest, src))
        {
            gen_codes(op_mov, ssize, dest, src);
        }
        if (ssize == ISZ_UCHAR)
            gen_code(op_cbw, NULL, NULL);
        gen_code(op_cwde, NULL, NULL);
    }
    else
    {
        gen_code2(op_movsx, dsize, ssize, dest, src);
    }
}
static void compactSwitchHeader(long long bottom)
{
    int tablab, size;
    AMODE* ap;
    Optimizer::SimpleExpression* lnode;
    tablab = beGetLabel;
    size = switch_ip->size;
    if (size == ISZ_ULONGLONG || size == -ISZ_ULONGLONG)
    {
        if (bottom)
        {
            int golab = beGetLabel;
#ifdef USE_LONGLONG
            gen_codes(op_cmp, ISZ_UINT, switch_aph, aimmed(bottom >> 32));
#else
            gen_codes(op_cmp, ISZ_UINT, switch_aph, aimmed((switch_ip->size < 9 && bottom < 0) ? -1 : 0));
#endif
            if (size < 0)
            {
                gen_branch(op_jl, switch_deflab);
                gen_branch(op_jg, golab);
            }
            else
            {
                peep_tail->noopt = true;
                gen_branch(op_jb, switch_deflab);
                gen_branch(op_ja, golab);
            }
            gen_codes(op_cmp, ISZ_UINT, switch_apl, aimmed(bottom));
            if (size < 0)
                gen_branch(op_jl, switch_deflab);
            else
            {
                peep_tail->noopt = true;
                gen_branch(op_jb, switch_deflab);
            }
            oa_gen_label(golab);
        }
        else if (size < 0)
        {
            if (switch_aph->mode == am_dreg)
                gen_codes(op_and, ISZ_UINT, switch_aph, switch_aph);
            else
                gen_codes(op_test, ISZ_UINT, switch_aph, aimmed(-1));
            gen_branch(op_jl, switch_deflab);
        }
        gen_codes(op_cmp, ISZ_UINT, switch_apl, aimmed(switch_range + bottom));
    }
    else
    {
        if (bottom)
        {
            gen_codes(op_cmp, switch_ip->size, switch_apl, aimmed(bottom));
            if (size < 0)
                gen_branch(op_jl, switch_deflab);
            else
            {
                peep_tail->noopt = true;
                gen_branch(op_jb, switch_deflab);
            }
        }
        else if (size < 0)
        {
            if (switch_apl->mode == am_dreg)
                gen_codes(op_and, switch_ip->size, switch_apl, switch_apl);
            else
                gen_codes(op_test, switch_ip->size, switch_apl, aimmed(-1));
            gen_branch(op_jl, switch_deflab);
        }
        gen_codes(op_cmp, switch_ip->size, switch_apl, aimmed(switch_range + bottom));
    }
    if (size < 0)
        gen_branch(op_jge, switch_deflab);
    else
        gen_branch(op_jnc, switch_deflab);
    gen_codes(op_push, ISZ_UINT, switch_apl, 0);
    pushlevel += 4;
    switch (switch_ip->size)
    {
        case ISZ_USHORT:
            do_movzx(ISZ_UINT, ISZ_USHORT, switch_apl, switch_apl);
            break;
        case -ISZ_USHORT:
            do_movsx(ISZ_UINT, ISZ_USHORT, switch_apl, switch_apl);
            break;
        case ISZ_UCHAR:
            do_movzx(ISZ_UINT, ISZ_UCHAR, switch_apl, switch_apl);
            break;
        case ISZ_BOOLEAN:
            gen_codes(op_and, ISZ_UINT, switch_apl, aimmed(1));
            break;
        case -ISZ_UCHAR:
            do_movsx(ISZ_UINT, ISZ_UCHAR, switch_apl, switch_apl);
            break;
        default:
            break;
    }

    peep_tail->noopt = true;
    lnode = beLocalAllocate<Optimizer::SimpleExpression>();
    lnode->type = Optimizer::se_labcon;
    lnode->i = tablab;
    if (bottom)
    {
        lnode = Optimizer::simpleExpressionNode(Optimizer::se_add, lnode, Optimizer::simpleIntNode(Optimizer::se_i, -bottom * 4));
    }
    ap = beLocalAllocate<AMODE>();
    ap->mode = am_indispscale;
    ap->preg = -1;
    ap->scale = 2;
    ap->sreg = switch_apl->preg;
    ap->offset = lnode;
    gen_codes(op_mov, ISZ_UINT, switch_apl, ap);
    gen_codes(op_xchg, ISZ_UINT, switch_apl, make_stack(0));
    gen_codes(op_ret, 0, 0, 0);
    pushlevel -= 4;
    gen_code(op_align, aimmed(4), 0);
    oa_gen_label(tablab);
}
//-------------------------------------------------------------------------

void bingen(int lower, int avg, int higher)
{
    AMODE* ap2 = aimmed(switchTreeCases[avg]);
    if (switchTreeBranchLabels[avg] != 0)
        oa_gen_label(switchTreeBranchLabels[avg]);
    gen_coden(op_cmp, switch_apl->length, switch_apl, ap2);
    gen_branch(op_je, switchTreeLabels[avg]);
    peep_tail->oper1->liveRegs = switch_live;
    if (avg == lower)
    {
        gen_branch(op_jmp, switch_deflab);
        peep_tail->oper1->liveRegs = switch_live;
    }
    else
    {
        int avg1 = (lower + avg) / 2;
        int avg2 = (higher + avg + 1) / 2;
        int lab;
        if (avg + 1 < higher)
            lab = switchTreeBranchLabels[avg2] = switchTreeLabels[avg] + 1;
        else
            lab = switch_deflab;
        if (switch_apl->length < 0)
            gen_branch(op_jg, lab);
        else
            gen_branch(op_ja, lab);
        peep_tail->oper1->liveRegs = switch_live;
        bingen(lower, avg1, avg);
        if (avg + 1 < higher)
            bingen(avg + 1, avg2, higher);
    }
}
int getPushMask(int i)
{
    if (i & (1 << 8))
        i |= 3;
    if (i & (1 << 9))
        i |= 8;
    return i & 0x0b;
}
static void llongatomicmath(e_opcode low, e_opcode high, Optimizer::QUAD* q)
{
    bool pushax = false, pushcx = false, pushdx = false, pushbx = false;
    bool pushsi = false, pushdi = false, pushbp = false;
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    int reg1 = apal->preg, reg2 = apah->preg;
    if ((apal->liveRegs & (1 << EAX)) && reg1 != EAX && reg2 != EAX)
    {
        pushax = true;
        gen_code(op_push, makedreg(EAX), NULL);
        pushlevel += 4;
    }
    if ((apal->liveRegs & (1 << ECX)) && reg1 != ECX && reg2 != ECX)
    {
        pushcx = true;
        gen_code(op_push, makedreg(ECX), NULL);
        pushlevel += 4;
    }
    if ((apal->liveRegs & (1 << EDX)) && reg1 != EDX && reg2 != EDX)
    {
        pushdx = true;
        gen_code(op_push, makedreg(EDX), NULL);
        pushlevel += 4;
    }
    if ((apal->liveRegs & (1 << EBX)) && reg1 != EBX && reg2 != EBX)
    {
        pushbx = true;
        gen_code(op_push, makedreg(EBX), NULL);
        pushlevel += 4;
    }
    if (low != op_xchg && aprl->mode != am_immed && /*(apal->liveRegs & (1 << ESI)) &&*/ reg1 != ESI && reg2 != ESI)
    {
        pushsi = true;
        gen_code(op_push, makedreg(ESI), NULL);
        pushlevel += 4;
    }
    if (low != op_xchg && aprl->mode != am_immed && /*(apal->liveRegs & (1 << EDI)) &&*/ reg1 != EDI && reg2 != EDI)
    {
        pushdi = true;
        gen_code(op_push, makedreg(EDI), NULL);
        pushlevel += 4;
    }
    if (/*(apal->liveRegs & (1 << EBP)) &&*/ (apll->mode == am_indispscale ||
                                              (apll->mode == am_indisp && apll->preg != EBP && apll->preg != ESP)))
    {
        pushbp = true;
        gen_code(op_push, makedreg(EBP), NULL);
        pushlevel += 4;
    }
    if (aprl->mode == am_indispscale || (aprl->mode == am_indisp && aprl->preg != ESP && aprl->preg != EBP))
    {
        gen_code(op_lea, makedreg(EAX), aprl);
        aprl = makedreg(EAX);
        aprl->mode = am_indisp;
        aprl->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        aprh = makedreg(EAX);
        aprh->mode = am_indisp;
        aprh->offset = Optimizer::simpleIntNode(Optimizer::se_i, 4);
    }
    if (apll->mode == am_indispscale || (apll->mode == am_indisp && apll->preg != EBP && apll->preg != ESP))
    {
        gen_code(op_lea, makedreg(EBP), apll);
        apll = makedreg(EBP);
        apll->mode = am_indisp;
        apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        aplh = makedreg(EBP);
        aplh->mode = am_indisp;
        aplh->offset = Optimizer::simpleIntNode(Optimizer::se_i, 4);
    }
    if (high == op_cmpxchg8b)
    {
        gen_codes(op_mov, ISZ_UINT, makedreg(EBX), apll);
        gen_codes(op_mov, ISZ_UINT, makedreg(ECX), aplh);
        gen_codes(op_mov, ISZ_UINT, makedreg(EAX), aprl);
        gen_codes(op_mov, ISZ_UINT, makedreg(EDX), aprh);
    }
    else
    {
        AMODE* regl = low == op_xchg ? makedreg(EBX) : makedreg(ESI);
        AMODE* regh = low == op_xchg ? makedreg(ECX) : makedreg(EDI);
        if (aprl->mode != am_immed)
        {

            if (aprl->mode == am_dreg)
            {
                if (aprh->preg == regl->preg)
                {
                    if (aprl->preg == regh->preg)
                    {
                        gen_code(op_xchg, regl, regh);
                    }
                    else
                    {
                        gen_code(op_mov, regh, aprh);
                        gen_code(op_mov, regl, aprl);
                    }
                }
                else
                {
                    gen_code(op_mov, regl, aprl);
                    gen_code(op_mov, regh, aprh);
                }
            }
            else
            {
                gen_code(op_mov, regl, aprl);
                gen_code(op_mov, regh, aprh);
            }
        }
        gen_codes(op_mov, ISZ_UINT, makedreg(EAX), apll);
        gen_codes(op_mov, ISZ_UINT, makedreg(EDX), aplh);
    }
    if (low != op_xchg && aprl->mode != am_immed)
    {
        aprl = makedreg(ESI);
        aprh = makedreg(EDI);
    }
    int labno;
    if (high != op_cmpxchg8b)
    {
        labno = beGetLabel;
        oa_gen_label(labno);
    }
    if (low != op_xchg)
    {
        gen_code(op_mov, makedreg(EBX), makedreg(EAX));
        gen_code(op_mov, makedreg(ECX), makedreg(EDX));
        gen_code(low, makedreg(EBX), aprl);
        gen_code(high, makedreg(ECX), aprh);
    }
    gen_code(op_lock, NULL, NULL);
    gen_codes(op_cmpxchg8b, ISZ_NONE, high == op_cmpxchg8b ? apal : apll, NULL);
    if (high != op_cmpxchg8b)
    {
        gen_code(op_jne, make_label(labno), NULL);

        if (reg1 != EAX || reg2 != EDX)
        {
            if (apal->preg == EDX)
            {
                gen_code(op_mov, apah, makedreg(EDX));
                gen_code(op_mov, apal, makedreg(EAX));
            }
            else
            {
                gen_code(op_mov, apal, makedreg(EAX));
                gen_code(op_mov, apah, makedreg(EDX));
            }
        }
    }
    if (pushbp)
    {
        gen_code(op_pop, makedreg(EBP), NULL);
        pushlevel -= 4;
    }
    if (pushdi)
    {
        gen_code(op_pop, makedreg(EDI), NULL);
        pushlevel -= 4;
    }
    if (pushsi)
    {
        gen_code(op_pop, makedreg(ESI), NULL);
        pushlevel -= 4;
    }

    if (pushbx)
    {
        gen_code(op_pop, makedreg(EBX), NULL);
        pushlevel -= 4;
    }
    if (pushdx)
    {
        gen_code(op_pop, makedreg(EDX), NULL);
        pushlevel -= 4;
    }
    if (pushcx)
    {
        gen_code(op_pop, makedreg(ECX), NULL);
        pushlevel -= 4;
    }
    if (pushax)
    {
        gen_code(op_pop, makedreg(EAX), NULL);
        pushlevel -= 4;
    }
}
static void addsubatomic(e_opcode op, Optimizer::QUAD* q)
{
    if (((q->ans->size <= ISZ_U32 && q->ans->size != -ISZ_ULONGLONG) || q->ans->size == ISZ_ADDR) ||
        ((q->dc.left->size <= ISZ_U32 && q->dc.left->size != -ISZ_ULONGLONG) || q->dc.left->size == ISZ_ADDR) ||
        ((q->dc.right->size <= ISZ_U32 && q->dc.right->size != -ISZ_ULONGLONG) || q->dc.right->size == ISZ_ADDR))
    {
        bool pushbp = false;
        enum e_opcode opa, opl, opr;
        AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
        getAmodes(q, &opl, q->dc.left, &apll, &aplh);
        getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
        getAmodes(q, &opa, q->ans, &apal, &apah);
        if (apll->mode == am_indispscale || (apll->mode == am_indisp && apll->preg != ESP && apll->preg != EBP))
        {
            //            if (apll->liveRegs & (1 << EBP))
            {
                pushbp = true;
                gen_code(op_push, makedreg(EBP), NULL);
                pushlevel += 4;
            }
            gen_code(op_lea, makedreg(EBP), apll);
            apll = makedreg(EBP);
            apll->mode = am_indisp;
            apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        if (apal->liveRegs & (1 << apal->preg))
        {
            if (aprl->mode == am_dreg)
            {
                if (op == op_sub)
                    gen_codes(op_neg, q->ans->size, aprl, NULL);
                gen_code(op_lock, NULL, NULL);
                gen_codes(op_xadd, q->ans->size, apll, aprl);
                gen_codes(op_mov, q->ans->size, apal, aprl);
            }
            else
            {
                gen_codes(op_mov, q->ans->size, apal, aprl);
                if (op == op_sub)
                    gen_codes(op_neg, q->ans->size, apal, NULL);
                gen_code(op_lock, NULL, NULL);
                gen_codes(op_xadd, q->ans->size, apll, apal);
            }
        }
        else
        {
            if (aprl->mode == am_dreg || aprl->mode == am_immed)
            {
                gen_code(op_lock, NULL, NULL);
                gen_codes(op, q->ans->size, apll, aprl);
            }
            else
            {
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_code(op_lock, NULL, NULL);
                gen_codes(op, q->ans->size, apll, apal);
            }
        }
        if (pushbp)
        {
            gen_code(op_pop, makedreg(EBP), NULL);
            pushlevel -= 4;
        }
    }
    else
    {
        if (op == op_add)
            llongatomicmath(op_add, op_adc, q);
        else
            llongatomicmath(op_sub, op_sbb, q);
    }
}
static void logicatomic(e_opcode op, Optimizer::QUAD* q)
{
    if (((q->ans->size <= ISZ_U32 && q->ans->size != -ISZ_ULONGLONG) || q->ans->size == ISZ_ADDR) ||
        ((q->dc.left->size <= ISZ_U32 && q->dc.left->size != -ISZ_ULONGLONG) || q->dc.left->size == ISZ_ADDR) ||
        ((q->dc.right->size <= ISZ_U32 && q->dc.right->size != -ISZ_ULONGLONG) || q->dc.right->size == ISZ_ADDR))
    {
        bool pushbp = false;
        enum e_opcode opa, opl, opr;
        AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
        getAmodes(q, &opl, q->dc.left, &apll, &aplh);
        getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
        getAmodes(q, &opa, q->ans, &apal, &apah);
        if (apll->mode == am_indispscale || (apll->mode == am_indisp && apll->preg != ESP && apll->preg != EBP))
        {
            //            if (apll->liveRegs & (1 << EBP))
            {
                pushbp = true;
                gen_code(op_push, makedreg(EBP), NULL);
                pushlevel += 4;
            }
            gen_code(op_lea, makedreg(EBP), apll);
            apll = makedreg(EBP);
            apll->mode = am_indisp;
            apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        if (apal->liveRegs & (1 << apal->preg))
        {
            bool pusheddx = false;
            bool pushedcx = false;
            bool pushedax = false;
            if ((apal->liveRegs & (1 << ECX)) && apal->preg != EDX && (aprl->mode != am_dreg || aprl->preg == EAX) &&
                aprl->mode != am_immed)
                pusheddx = true;
            if ((apal->liveRegs & (1 << ECX)) && apal->preg != ECX)
                pushedcx = true;
            if ((apal->liveRegs & (1 << EAX)) && apal->preg != EAX)
                pushedax = true;
            if (pusheddx)
            {
                gen_code(op_push, makedreg(EDX), NULL);
                pushlevel += 4;
            }
            if (pushedcx)
            {
                gen_code(op_push, makedreg(ECX), NULL);
                pushlevel += 4;
            }
            if (pushedax)
            {
                gen_code(op_push, makedreg(EAX), NULL);
                pushlevel += 4;
            }
            int lab = beGetLabel;
            if ((aprl->mode != am_dreg || aprl->preg == EAX) && aprl->mode != am_immed)
            {
                gen_codes(op_mov, q->ans->size, makedreg(EDX), aprl);
                aprl = makedreg(EDX);
            }
            gen_codes(op_mov, q->ans->size, makedreg(EAX), apll);
            oa_gen_label(lab);
            gen_codes(op_mov, q->ans->size, makedreg(ECX), makedreg(EAX));
            gen_codes(op, q->ans->size, makedreg(ECX), aprl);
            gen_codes(op_cmpxchg, q->ans->size, apll, makedreg(ECX));
            gen_code(op_jne, make_label(lab), NULL);
            gen_codes(op_mov, q->ans->size, apal, makedreg(EAX));
            if (pushedax)
            {
                gen_code(op_pop, makedreg(EAX), NULL);
                pushlevel -= 4;
            }
            if (pushedcx)
            {
                gen_code(op_pop, makedreg(ECX), NULL);
                pushlevel -= 4;
            }
            if (pusheddx)
            {
                gen_code(op_pop, makedreg(EDX), NULL);
                pushlevel -= 4;
            }
        }
        else
        {
            if (aprl->mode == am_dreg || aprl->mode == am_immed)
            {
                gen_code(op_lock, NULL, NULL);
                gen_codes(op, q->ans->size, apll, aprl);
            }
            else
            {
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_code(op_lock, NULL, NULL);
                gen_codes(op, q->ans->size, apll, apal);
            }
        }
        if (pushbp)
        {
            gen_code(op_pop, makedreg(EBP), NULL);
            pushlevel -= 4;
        }
    }
    else
    {
        llongatomicmath(op, op, q);
    }
}
void asm_line(Optimizer::QUAD* q) /* line number information and text */
{
    OCODE* newitem = beLocalAllocate<OCODE>();
    newitem->opcode = (e_opcode)op_line;
    newitem->oper1 = (AMODE*)(q->dc.left); /* line data */
    add_peep(newitem);
}
void asm_blockstart(Optimizer::QUAD* q) /* line number information and text */
{
    OCODE* newitem = beLocalAllocate<OCODE>();
    newitem->opcode = (e_opcode)op_blockstart;
    add_peep(newitem);
}
void asm_blockend(Optimizer::QUAD* q) /* line number information and text */
{
    OCODE* newitem = beLocalAllocate<OCODE>();
    newitem->opcode = (e_opcode)op_blockend;
    add_peep(newitem);
}
void asm_varstart(Optimizer::QUAD* q) /* line number information and text */
{
    OCODE* newitem = beLocalAllocate<OCODE>();
    newitem->opcode = (e_opcode)op_varstart;
    newitem->oper1 = (AMODE*)(q->dc.left->offset->sp); /* line data */
    add_peep(newitem);
}
void asm_func(Optimizer::QUAD* q) /* line number information and text */
{
    OCODE* newitem = beLocalAllocate<OCODE>();
    newitem->opcode = (e_opcode)(q->dc.v.label ? op_funcstart : op_funcend);
    newitem->oper1 = (AMODE*)(q->dc.left->offset->sp); /* line data */
    add_peep(newitem);
}
void asm_passthrough(Optimizer::QUAD* q) /* reserved */
{
    OCODE* val = (OCODE*)q->dc.left;
    if (val->oper1 && val->oper1->mode == am_indisp && val->oper1->preg == EBP)
    {
        Optimizer::SimpleSymbol* sp = varsp(val->oper1->offset);
        if (usingEsp)
            val->oper1->preg = ESP;
        if (sp && sp->regmode == 2)
        {
            int len = -(sp->offset >> 8);
            val->oper1 = makedreg(-sp->offset & 255);
            if (len == 2 || len == -2)
                len = ISZ_USHORT;
            else if (len == 1 || len == -1)
                len = ISZ_UCHAR;
            else
                len = ISZ_UINT;
            val->oper1->length = len;
        }
    }
    if (val->oper2 && val->oper2->mode == am_indisp && val->oper2->preg == EBP)
    {
        Optimizer::SimpleSymbol* sp = varsp(val->oper2->offset);
        if (usingEsp)
            val->oper2->preg = ESP;
        if (sp && sp->regmode == 2)
        {
            int len = -(sp->offset >> 8);
            val->oper2 = makedreg(-sp->offset & 255);
            if (len == 2 || len == -2)
                len = ISZ_USHORT;
            else if (len == 1 || len == -1)
                len = ISZ_UCHAR;
            else
                len = ISZ_UINT;
            val->oper2->length = len;
        }
    }
    val = gen_code(val->opcode, val->oper1, val->oper2);
    val->noopt = true;
}
void asm_datapassthrough(Optimizer::QUAD* q) /* reserved */ { (void)q; }
void asm_label(Optimizer::QUAD* q) /* put a label in the code stream */
{
    OCODE* out = beLocalAllocate<OCODE>();
    out->opcode = (e_opcode)op_label;
    out->oper1 = make_label(q->dc.v.label);
    add_peep(out);
}
void asm_goto(Optimizer::QUAD* q) /* unconditional branch */
{
    if (q->dc.opcode == Optimizer::i_goto)
        gen_branch(op_jmp, q->dc.v.label);
    else /* directbranch */
    {
        AMODE* ap;
        Optimizer::SimpleSymbol* sp = q->dc.left->offset->sp;
        if (sp->storage_class == Optimizer::scc_temp)
        {
            ap = makedreg(Optimizer::chosenAssembler->arch->regMap[q->leftColor & 0xff][0]);
            ap->liveRegs = q->liveRegs;
            if (q->dc.left->mode == Optimizer::i_ind)
            {
                ap->mode = am_indisp;
                ap->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
            }
        }
        else
        {
            ap = make_offset(q->dc.left->offset);
            if (q->dc.left->offset->type == Optimizer::se_pc)
                ap->mode = am_immed;
            ap->liveRegs = q->liveRegs;
        }
        gen_code(op_jmp, ap, 0);
    }
}
void asm_parm(Optimizer::QUAD* q) /* push a parameter*/
{
    enum e_opcode op;
    AMODE *apl, *aph;
    getAmodes(q, &op, q->dc.left, &apl, &aph);
    if (q->dc.left->mode == Optimizer::i_immed)
    {
        if (q->dc.left->size >= ISZ_CFLOAT)
        {
            int sz = 8;
            if (q->dc.left->size == ISZ_FLOAT || q->dc.left->size == ISZ_IFLOAT || q->dc.left->size == ISZ_CFLOAT)
                sz = 4;
            pushlevel += sz * 2;
            if (apl->mode == am_xmmreg)
            {
                gen_codes(op_sub, ISZ_UINT, makedreg(ESP), aimmed(sz * 2));
                gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(-sz), aph);
                gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(0), apl);
            }
            else
            {
                if (sz == 8)
                    gen_codes(op_push, ISZ_UINT,
                              make_offset(Optimizer::simpleExpressionNode(Optimizer::se_add, aph->offset,
                                                                          Optimizer::simpleIntNode(Optimizer::se_i, 4))),
                              NULL);
                gen_codes(op_push, ISZ_UINT, aph, NULL);
                if (sz == 8)
                    gen_codes(op_push, ISZ_UINT,
                              make_offset(Optimizer::simpleExpressionNode(Optimizer::se_add, apl->offset,
                                                                          Optimizer::simpleIntNode(Optimizer::se_i, 4))),
                              NULL);
                gen_codes(op_push, ISZ_UINT, apl, NULL);
            }
        }
        else if (q->dc.left->size >= ISZ_FLOAT)
        {
            int sz = 8;
            if (q->dc.left->size == ISZ_FLOAT || q->dc.left->size == ISZ_IFLOAT || q->dc.left->size == ISZ_CFLOAT)
                sz = 4;
            pushlevel += sz;
            if (apl->mode == am_xmmreg)
            {
                if (sz == 4)
                    gen_code(op_push, makedreg(ECX), NULL);
                else
                    gen_codes(op_sub, ISZ_UINT, makedreg(ESP), aimmed(sz));
                gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(0), apl);
            }
            else
            {
                if (sz == 8)
                    gen_codes(op_push, ISZ_UINT,
                              make_offset(Optimizer::simpleExpressionNode(Optimizer::se_add, apl->offset,
                                                                          Optimizer::simpleIntNode(Optimizer::se_i, 4))),
                              NULL);
                gen_codes(op_push, ISZ_UINT, apl, NULL);
            }
        }
        else
        {
            int sz = q->dc.left->size;
            if (!isintconst(apl->offset))
            {
                gen_codes(op_push, ISZ_UINT, apl, 0);
                pushlevel += 4;
            }
            else if (sz == ISZ_ULONGLONG || sz == -ISZ_ULONGLONG)
            {
                gen_codes(op_push, ISZ_UINT, aph, 0);
                pushlevel += 4;
                gen_codes(op_push, ISZ_UINT, apl, 0);
                pushlevel += 4;
            }
            else
            {
                long long i;
                i = apl->offset->i;
                switch (sz)
                {
                    case ISZ_USHORT:
                        i &= 0xffff;
                        break;
                    case -ISZ_USHORT:
                        i &= 0xffff;
                        i |= i & 0x8000 ? 0xffff0000U : 0;
                        break;
                    case ISZ_UCHAR:
                        i &= 0xff;
                        break;
                    case -ISZ_UCHAR:
                        i &= 0xff;
                        i |= i & 0x80 ? 0xffffff00U : 0;
                        break;
                    case ISZ_BOOLEAN:
                        i &= 1;
                        break;
                }
                gen_codes(op_push, ISZ_UINT, aimmed(i), 0);
                pushlevel += 4;
            }
        }
    }
    else
    {
        if (q->dc.left->size >= ISZ_CFLOAT)
        {
            int sz = 8;
            if (q->dc.left->size == ISZ_FLOAT || q->dc.left->size == ISZ_IFLOAT || q->dc.left->size == ISZ_CFLOAT)
                sz = 4;
            gen_codes(op_sub, ISZ_UINT, makedreg(ESP), aimmed(sz * 2));
            pushlevel += sz * 2;
            gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(-sz), aph);
            gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(0), apl);
        }
        else if (q->dc.left->size >= ISZ_FLOAT)
        {
            int sz = 8;
            if (q->dc.left->size == ISZ_FLOAT || q->dc.left->size == ISZ_IFLOAT || q->dc.left->size == ISZ_CFLOAT)
                sz = 4;
            if (sz == 4)
                gen_code(op_push, makedreg(ECX), NULL);
            else
                gen_codes(op_sub, ISZ_UINT, makedreg(ESP), aimmed(sz));
            pushlevel += sz;
            gen_code_sse(op_movss, op_movsd, q->dc.left->size, make_stack(0), apl);
        }
        else
        {
            if (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG)
            {
                if (q->dc.left->mode == Optimizer::i_direct && q->dc.left->offset->type == Optimizer::se_tempref)
                {
                    int clr = beRegFromTemp(q, q->dc.left);
                    int reg1, reg2;
                    reg1 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][1];
                    reg2 = Optimizer::chosenAssembler->arch->regMap[clr & 0xff][0];
                    gen_codes(op_push, ISZ_UINT, makedreg(reg1), 0);
                    pushlevel += 4;
                    gen_codes(op_push, ISZ_UINT, makedreg(reg2), 0);
                    pushlevel += 4;
                }
                else
                {
                    gen_codes(op_push, ISZ_UINT, aph, 0);
                    pushlevel += 4;
                    gen_codes(op_push, ISZ_UINT, apl, 0);
                    pushlevel += 4;
                }
            }
            else
            {
                if (q->dc.left->mode == Optimizer::i_direct && q->dc.left->offset->type == Optimizer::se_tempref)
                {
                    int l = beRegFromTemp(q, q->dc.left);
                    AMODE* pal;
                    l = Optimizer::chosenAssembler->arch->regMap[l][0];
                    pal = makedreg(l);
                    pal->liveRegs = q->liveRegs;
                    switch (q->dc.left->size)
                    {
                        case ISZ_USHORT:
                            do_movzx(ISZ_UINT, ISZ_USHORT, pal, pal);
                            break;
                        case -ISZ_USHORT:
                            do_movsx(ISZ_UINT, ISZ_USHORT, pal, pal);
                            break;
                        case ISZ_UCHAR:
                            do_movzx(ISZ_UINT, ISZ_UCHAR, pal, pal);
                            break;
                        case ISZ_BOOLEAN:
                            gen_codes(op_and, ISZ_UINT, pal, aimmed(1));
                            break;
                        case -ISZ_UCHAR:
                            do_movsx(ISZ_UINT, ISZ_UCHAR, pal, pal);
                            break;
                        default:
                            break;
                    }
                    gen_codes(op_push, ISZ_UINT, pal, 0);
                }
                else
                {
                    gen_codes(op_push, ISZ_UINT, apl, 0);
                }
                pushlevel += 4;
            }
        }
    }
}
void asm_parmblock(Optimizer::QUAD* q) /* push a block of memory */
{
    int n = q->dc.right->offset->i;
    AMODE *apl, *aph;
    enum e_opcode op;
    Optimizer::SimpleExpression* ofs;

    getAmodes(q, &op, q->dc.left, &apl, &aph);

    n += 3;
    n &= 0xfffffffcU;

    ofs = apl->offset;
    if (q->dc.left->mode == Optimizer::i_immed)
    {
        op = op_lea;
        if (ofs->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apl->preg = ESP;
            }
            else
            {
                apl->preg = EBP;
            }
            apl->mode = am_indisp;
        }
        else
            apl->mode = am_direct;
    }

    if (n <= 24 && q->dc.left->mode == Optimizer::i_immed)
    {
        while (n > 0)
        {
            n -= 4;
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n));
            gen_codes(op_push, ISZ_UINT, apl, 0);
            pushlevel += 4;
        }
    }
    else
    {
        AMODE* cx = makedreg(ECX);
        AMODE* di = makedreg(EDI);
        AMODE* si = makedreg(ESI);
        AMODE* sp = makedreg(ESP);
        cx->liveRegs = q->liveRegs;
        si->liveRegs = q->liveRegs;
        di->liveRegs = q->liveRegs;
        gen_codes(op_sub, ISZ_UINT, sp, aimmed(n));
        pushlevel += n;
        gen_codes(op_push, ISZ_UINT, di, 0);
        gen_codes(op_push, ISZ_UINT, si, 0);
        gen_codes(op_push, ISZ_UINT, cx, 0);
        pushlevel += 12;
        gen_codes(op, ISZ_UINT, si, apl);
        gen_codes(op_mov, ISZ_UINT, di, sp);
        gen_codes(op_add, ISZ_UINT, di, aimmed(12));
        gen_codes(op_mov, ISZ_UINT, cx, aimmed(n / 4));
        gen_code(op_cld, 0, 0);
        gen_code(op_rep, 0, 0);
        gen_code(op_movsd, 0, 0);
        gen_codes(op_pop, ISZ_UINT, cx, 0);
        gen_codes(op_pop, ISZ_UINT, si, 0);
        gen_codes(op_pop, ISZ_UINT, di, 0);
        pushlevel -= 12;
    }
}
void asm_parmadj(Optimizer::QUAD* q) /* adjust stack after function call */
{
    int mask;
    int i = beGetIcon(q->dc.left);
    if (i)
    {
        if (i == 4)
        {
            gen_code(op_pop, makedreg(ECX), 0);
        }
        else if (i == 8)
        {
            gen_code(op_pop, makedreg(ECX), 0);
            gen_code(op_pop, makedreg(ECX), 0);
        }
        else
        {
            gen_code(op_add, makedreg(ESP), aimmed(i));
        }
    }
    pushlevel -= beGetIcon(q->dc.right);
    mask = getPushMask(q->dc.v.i);
    for (i = 3; i >= 0; i--)
        if ((1 << i) & mask)
        {
            gen_code(op_pop, makedreg(Optimizer::chosenAssembler->arch->regMap[i][0]), 0);
            pushlevel -= 4;
        }
}
void asm_gosub(Optimizer::QUAD* q) /* normal gosub to an immediate label or through a var */
{
    Optimizer::SimpleExpression* en = NULL;
    enum e_opcode op;
    AMODE *apl, *aph;
    if (!q->dc.left->offset || q->dc.left->offset->type != Optimizer::se_pc || !BackendIntrinsic(q))
    {
        if (q->dc.left->offset)
            en = GetSymRef(q->dc.left->offset);
        if (!en && q->dc.left->offset2)
            en = GetSymRef(q->dc.left->offset2);
        if (!en && q->dc.left->offset3)
            en = GetSymRef(q->dc.left->offset3);
        getAmodes(q, &op, q->dc.left, &apl, &aph);

        if (q->dc.left->mode == Optimizer::i_immed)
        {
            if (isintconst(q->dc.left->offset))
            {
                // doing call via ret here because we really need a register and we may not have one...
                apl->length = 0;
                int lbl = beGetLabel;
                gen_code(op_push, make_label(lbl), nullptr);
                gen_code(op_push, apl, nullptr);
                gen_code(op_ret, nullptr, nullptr);
                oa_gen_label(lbl);
            }
            else
            {
                apl->length = 0;
                gen_code(op_call, apl, nullptr);
            }
        }
        else
        {
            apl->liveRegs = q->liveRegs;
            gen_code(op_call, apl, 0);
        }

        // pop the stack if returning from a function returning floating point
        // whose return value lies unused.
        if (q->novalue >= 0)
        {
            switch (q->novalue)
            {
                case ISZ_CFLOAT:
                case ISZ_CDOUBLE:
                case ISZ_CLDOUBLE:
                    gen_codes(op_fstp, ISZ_LDOUBLE, makefreg(0), 0);
                    // fall through
                case ISZ_FLOAT:
                case ISZ_DOUBLE:
                case ISZ_LDOUBLE:
                case ISZ_IFLOAT:
                case ISZ_IDOUBLE:
                case ISZ_ILDOUBLE:
                    gen_codes(op_fstp, ISZ_LDOUBLE, makefreg(0), 0);
                    break;
            }
        }
    }
}
void asm_fargosub(Optimizer::QUAD* q) /* far version of gosub */
{
    gen_code(op_push, makesegreg(CS), 0);
    asm_gosub(q);
}
void asm_trap(Optimizer::QUAD* q) /* 'trap' instruction - the arg will be an immediate # */
{
    if (q->dc.left->offset->i == 3)
        gen_code(op_int3, 0, 0);
    else
        gen_code(op_int, aimmed(q->dc.left->offset->i), 0);
}
void asm_int(Optimizer::QUAD* q) /* 'int' instruction(Optimizer::QUAD *q) calls a labeled function which is an interrupt */
{
    gen_code(op_pushf, 0, 0);
    gen_code(op_push, makesegreg(CS), 0);
    asm_gosub(q);
}
/* left will be a constant holding the number of bytes to pop
 * e.g. the parameters will be popped in stdcall or pascal type functions
 */
void asm_ret(Optimizer::QUAD* q) /* return from subroutine */
{
    if (beGetIcon(q->dc.left))
        gen_code(op_ret, aimmed(beGetIcon(q->dc.left)), 0);
    else
        gen_code(op_ret, 0, 0);
}
/* left will be a constant holding the number of bytes to pop
 * e.g. the parameters will be popped in stdcall or pascal type functions
 */
void asm_fret(Optimizer::QUAD* q) /* far return from subroutine */
{
    if (beGetIcon(q->dc.left))
        gen_code(op_retf, aimmed(beGetIcon(q->dc.left)), 0);
    else
        gen_code(op_retf, 0, 0);
}
/*
 * this can be either a fault or iret return
 * for processors that char, the 'left' member will have an integer
 * value that is true for an iret or false or a fault ret
 */
void asm_rett(Optimizer::QUAD* q) /* return from trap or int */
{
    (void)q;
    gen_code(op_iret, 0, 0);
}
void asm_add(Optimizer::QUAD* q) /* evaluate an addition */
{
    if (q->atomic)
    {
        addsubatomic(op_add, q);
        return;
    }
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_CFLOAT)
    {
        if ((q->dc.left->size < ISZ_CFLOAT && q->dc.right->size < ISZ_CFLOAT) &&
            (q->dc.left->size < ISZ_IFLOAT) != (q->dc.right->size < ISZ_IFLOAT))
        {
            AMODE* realsource;
            AMODE* imagsource;
            if (q->dc.left->size < ISZ_IFLOAT)
            {
                realsource = apll;
                imagsource = aprl;
            }
            else

            {
                realsource = aprl;
                imagsource = apll;
            }
            if (equal_address(apal, imagsource))
            {
                if (equal_address(apah, realsource))
                {
                    // poor man's swap - note for the 'single' case this alters bits 64-127 as well
                    gen_code_sse_imm(op_shufps, op_shufpd, q->dc.left->size, apah, apal,
                                     aimmed(0));  // put apal in the high word of apah
                    gen_code_sse_imm(op_shufps, op_shufpd, q->dc.left->size, apal, apah,
                                     aimmed(0));  // put apal in the high word of apah
                    gen_code_sse_imm(op_shufps, op_shufpd, q->dc.left->size, apah, apah,
                                     aimmed(q->ans->size == ISZ_CFLOAT ? 2 : 1));  // move high wored of apah to low word of apah
                    gen_code_sse_imm(op_shufps, op_shufpd, q->dc.left->size, apal, apal,
                                     aimmed(q->ans->size == ISZ_CFLOAT ? 2 : 1));  // move high wored of apah to low word of apah
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apah, imagsource);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apal, realsource);
                }
            }
            else  // apal == realsource or independent of each other
            {
                gen_code_sse(op_movss, op_movsd, q->ans->size, apal, realsource);
                gen_code_sse(op_movss, op_movsd, q->ans->size, apah, imagsource);
            }
        }

        else
        {
            bool highzero = false, lowzero = false;
            if ((q->dc.left->size < ISZ_CFLOAT && q->dc.right->size < ISZ_CFLOAT) &&
                (q->dc.left->size < ISZ_IFLOAT) == (q->dc.right->size < ISZ_IFLOAT))
            {
                if (q->dc.left->size < ISZ_IFLOAT)
                    highzero = true;
                else
                    lowzero = true;
            }
            if (equal_address(apal, apll))
            {
                if (!lowzero)
                {
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apal, aprl);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apal, floatzero(apal));
                    zerocleanup();
                }
                if (!highzero)
                {
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apah, aprh);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apah, floatzero(apah));
                    zerocleanup();
                }
            }
            else if (equal_address(apal, aprl))
            {
                if (!lowzero)
                {
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apal, apll);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apal, floatzero(apal));
                    zerocleanup();
                }
                if (!highzero)
                {
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apah, aplh);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apah, floatzero(apah));
                    zerocleanup();
                }
            }
            else
            {
                if (!lowzero)
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apal, aprl);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apal, floatzero(apal));
                    zerocleanup();
                }
                if (!highzero)
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apah, aplh);
                    gen_code_sse(op_addss, op_addsd, q->ans->size, apah, aprh);
                }
                else
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apah, floatzero(apah));
                    zerocleanup();
                }
            }
        }
    }
    else if (q->ans->size >= ISZ_FLOAT)
    {
        if (equal_address(apal, apll))
        {
            gen_code_sse(op_addss, op_addsd, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
        {
            gen_code_sse(op_addss, op_addsd, q->ans->size, apal, apll);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            gen_code_sse(op_addss, op_addsd, q->ans->size, apal, aprl);
        }
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (equal_address(apal, apll))
        {
            func_axdx(op_add, apal, apah, aprl, aprh);
        }
        else if (equal_address(apal, aprl))
        {
            func_axdx(op_add, apal, apah, apll, aplh);
        }
        else
        {
            liveQualify(apal, apal, apah);
            liveQualify(apah, apal, apah);
            func_axdx_mov(op_add, apal, apah, apll, aplh, aprl, aprh);
        }
    }
    else
    {
        if (equal_address(apal, apll))
        {
            if (opr == op_lea)
            {
                if (apal->mode != am_dreg)
                    diag("asm_add: expected dreg in add from EBP");
                aprl->mode = am_indispscale;
                aprl->sreg = apal->preg;
                gen_codes(op_lea, q->ans->size, apal, aprl);
            }
            else
            {
                gen_codes(op_add, q->ans->size, apal, aprl);
            }
        }
        else if (equal_address(apal, aprl))
        {
            if (opl == op_lea)
            {
                if (apal->mode != am_dreg)
                    diag("asm_add: expected dreg in add from EBP");
                apll->mode = am_indispscale;
                apll->sreg = apal->preg;
                gen_codes(op_lea, q->ans->size, apal, apll);
            }
            else
            {
                gen_codes(op_add, q->ans->size, apal, apll);
            }
        }
        else
        {
            if (samereg(apal, aprl) || apll->mode == am_immed)
            {
                liveQualify(apal, apal, 0);
                gen_codes(opr, q->ans->size, apal, aprl);
                if (opl == op_lea)
                {
                    if (apal->mode != am_dreg)
                        diag("asm_add: expected dreg in add from EBP");
                    apll->mode = am_indispscale;
                    apll->sreg = apal->preg;
                    gen_codes(op_lea, q->ans->size, apal, apll);
                }
                else
                {
                    gen_codes(op_add, q->ans->size, apal, apll);
                }
            }
            else
            {
                liveQualify(apal, apal, 0);
                if (opl == op_lea && aprl->mode == am_dreg)
                {
                    if (apal->mode != am_dreg)
                        diag("asm_add: expected dreg in add from EBP");
                    apll->mode = am_indispscale;
                    apll->sreg = aprl->preg;
                    gen_codes(op_lea, q->ans->size, apal, apll);
                }
                else if (opr == op_lea && apll->mode == am_dreg)
                {
                    if (apal->mode != am_dreg)
                        diag("asm_add: expected dreg in add from EBP");
                    aprl->mode = am_indispscale;
                    aprl->sreg = apll->preg;
                    gen_codes(op_lea, q->ans->size, apal, aprl);
                }
                else
                {
                    if (equal_address(apll, aprl))
                        apal->liveRegs = -1;
                    if (opl == op_lea || opr != op_lea)
                    {
                        gen_codes(opl, q->ans->size, apal, apll);
                        gen_codes(op_add, q->ans->size, apal, aprl);
                    }
                    else
                    {
                        gen_codes(opr, q->ans->size, apal, aprl);
                        gen_codes(op_add, q->ans->size, apal, apll);
                    }
                }
            }
        }
    }
}
void asm_sub(Optimizer::QUAD* q) /* evaluate a subtraction */
{
    if (q->atomic)
    {
        addsubatomic(op_sub, q);
        return;
    }
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_CFLOAT)
    {
        if (equal_address(apal, apll))
        {
            gen_code_sse(op_subss, op_subsd, q->ans->size, apal, aprl);
            gen_code_sse(op_subss, op_subsd, q->ans->size, apah, aprh);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            gen_code_sse(op_subss, op_subsd, q->ans->size, apal, aprl);
            gen_code_sse(op_movss, op_movsd, q->ans->size, apah, aplh);
            gen_code_sse(op_subss, op_subsd, q->ans->size, apah, aprh);
        }
    }
    else if (q->ans->size >= ISZ_FLOAT)
    {
        if (equal_address(apal, apll))
        {
            gen_code_sse(op_subss, op_subsd, q->ans->size, apal, aprl);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            gen_code_sse(op_subss, op_subsd, q->ans->size, apal, aprl);
        }
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (equal_address(apal, apll))
        {
            func_axdx(op_sub, apal, apah, aprl, aprh);
        }
        else if (equal_address(apal, aprl))
        {
            func_axdx(op_sub, apal, apah, apll, aplh);
            gen_codes(op_neg, ISZ_UINT, apah, 0);
            gen_codes(op_neg, ISZ_UINT, apal, 0);
            gen_codes(op_sbb, ISZ_UINT, apah, aimmed(0));
        }
        else
        {
            liveQualify(apal, apal, apah);
            liveQualify(apah, apal, apah);
            func_axdx_mov(op_sub, apal, apah, apll, aplh, aprl, aprh);
        }
    }
    else
    {
        if (equal_address(apal, apll))
        {
            gen_codes(op_sub, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
        {
            liveQualify(apal, apal, 0);
            gen_codes(op_sub, q->ans->size, apal, apll);
            gen_codes(op_neg, q->ans->size, apal, 0);
        }
        else
        {
            if (equal_address(apll, aprl))
                apal->liveRegs = -1;
            if (samereg(apal, aprl))
            {
                liveQualify(apal, apal, 0);
                gen_codes(opr, q->ans->size, apal, aprl);
                gen_codes(op_sub, q->ans->size, apal, apll);
                gen_codes(op_neg, q->ans->size, apal, 0);
            }
            else
            {
                liveQualify(apal, apal, 0);
                gen_codes(opl, q->ans->size, apal, apll);
                gen_codes(op_sub, q->ans->size, apal, aprl);
            }
        }
    }
}
void asm_udiv(Optimizer::QUAD* q) /* unsigned division */ { gen_div(q, op_div); }
void asm_umod(Optimizer::QUAD* q) /* unsigned modulous */ { gen_div(q, op_div); }
void asm_sdiv(Optimizer::QUAD* q) /* signed division */ { gen_div(q, op_idiv); }
void asm_smod(Optimizer::QUAD* q) /* signed modulous */ { gen_div(q, op_idiv); }
void asm_muluh(Optimizer::QUAD* q) { gen_mulxh(q, op_mul); }
void asm_mulsh(Optimizer::QUAD* q) { gen_mulxh(q, op_imul); }
void asm_mul(Optimizer::QUAD* q) /* signed multiply */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_FLOAT)
    {
        if (equal_address(apal, apll))
        {
            gen_code_sse(op_mulss, op_mulsd, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
        {
            gen_code_sse(op_mulss, op_mulsd, q->ans->size, apal, apll);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            gen_code_sse(op_mulss, op_mulsd, q->ans->size, apal, aprl);
        }
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        diag("asm_smul: long long");
    }
    else
    {
        if (equal_address(apal, apll))
        {
            gen_codes(op_imul, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
            gen_codes(op_imul, q->ans->size, apal, apll);
        else if (apll->mode == am_immed)
        {
            if (aprl->mode == am_immed)
            {
                apll = aimmed(apll->offset->i * aprl->offset->i);
                gen_codes(op_mov, q->ans->size, apal, apll);
            }
            else
            {
                gen_code3(op_imul, apal, aprl, apll);
            }
        }
        else if (aprl->mode == am_immed)
        {
            gen_code3(op_imul, apal, apll, aprl);
        }
        else
        {
            if (apll->mode == am_immed)
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_codes(op_imul, q->ans->size, apal, apll);
            }
            else
            {
                liveQualify(apal, apal, 0);
                if (equal_address(apll, aprl))
                {
                    gen_codes(op_mov, q->ans->size, apal, apll);
                    gen_codes(op_imul, q->ans->size, apal, apal);
                }
                else if (samereg(apal, aprl))
                {
                    gen_codes(op_mov, q->ans->size, apal, aprl);
                    gen_codes(op_imul, q->ans->size, apal, apll);
                }
                else
                {
                    gen_codes(op_mov, q->ans->size, apal, apll);
                    gen_codes(op_imul, q->ans->size, apal, aprl);
                }
            }
        }
    }
}
void asm_lsr(Optimizer::QUAD* q) /* unsigned shift right */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (aprl->mode == am_immed)
        {
            int n = aprl->offset->i;
            if (n >= 32)
            {
                n -= 32;
                liveQualify(apah, apal, apah);
                gen_codes(op_mov, ISZ_UINT, apal, aplh);
                gen_codes(op_mov, ISZ_UINT, apah, aimmed(0));
                if (n)
                    gen_codes(op_shr, ISZ_UINT, apal, aimmed(n));
            }
            else
            {
                if (!equal_address(apal, apll))
                {
                    func_axdx(op_mov, apal, apah, apll, aplh);
                }
                if (n)
                {
                    gen_lshift(op_shrd, apal, apah, aimmed(n));
                    gen_codes(op_shr, ISZ_UINT, apah, aimmed(n));
                }
            }
        }
        else
            diag("asm_lsr: long long shift by non-const");
    }
    else
    {
        gen_shift(q, op_shr, apal, apll, aprl);
    }
}
void asm_lsl(Optimizer::QUAD* q) /* signed shift left */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (aprl->mode == am_immed)
        {
            int n = aprl->offset->i;
            if (n >= 32)
            {
                n -= 32;
                liveQualify(apal, apal, apah);
                liveQualify(apah, apal, apah);
                gen_codes(op_mov, ISZ_UINT, apah, apll);
                gen_codes(op_mov, ISZ_UINT, apal, aimmed(0));
                if (n)
                    gen_code(op_shl, apah, aimmed(n));
            }
            else
            {
                liveQualify(apal, apal, apah);
                liveQualify(apah, apal, apah);
                if (!equal_address(apal, apll))
                {
                    func_axdx(op_mov, apal, apah, apll, aplh);
                }
                gen_lshift(op_shld, apah, apal, aimmed(n));
                gen_codes(op_shl, ISZ_UINT, apal, aimmed(n));
            }
        }
        else
            diag("asm_lsr: long long shift by non-const");
    }
    else
    {
        gen_shift(q, op_shl, apal, apll, aprl);
    }
}
void asm_asr(Optimizer::QUAD* q) /* signed shift right */
{
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (aprl->mode == am_immed)
        {
            int n = aprl->offset->i;
            if (n >= 32)
            {
                n -= 32;
                liveQualify(apal, apal, apah);
                liveQualify(apah, apal, apah);
                func_axdx(op_mov, apal, apah, aplh, aplh);
                gen_codes(op_sar, ISZ_UINT, apah, aimmed(31));
                if (n)
                    gen_code(op_sar, apal, aimmed(n));
            }
            else
            {
                if (!equal_address(apal, apll))
                {
                    liveQualify(apah, apal, apah);
                    func_axdx(op_mov, apal, apah, apll, aplh);
                }
                if (n)
                {
                    liveQualify(apah, apal, apah);
                    gen_lshift(op_shrd, apal, apah, aimmed(n));
                    gen_codes(op_sar, ISZ_UINT, apah, aimmed(n));
                }
            }
        }
        else
            diag("asm_asr: long long shift by non-const");
    }
    else
    {
        gen_shift(q, op_sar, apal, apll, aprl);
    }
}
void asm_neg(Optimizer::QUAD* q) /* negation */
{
    enum e_opcode opa, opl;
    AMODE *apal, *apah, *apll, *aplh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size >= ISZ_CFLOAT)
    {
        if (equal_address(apal, apll))
        {
            floatchs(apal, q->ans->size);
            floatchs(apah, q->ans->size);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            floatchs(apal, q->ans->size);
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, aplh);
            floatchs(apah, q->ans->size);
        }
    }
    else if (q->ans->size >= ISZ_FLOAT)
    {
        if (equal_address(apal, apll))
        {
            floatchs(apal, q->ans->size);
        }
        else
        {
            gen_code_sse(op_movss, op_movsd, q->ans->size, apal, apll);
            floatchs(apal, q->ans->size);
        }
    }
    else if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (!equal_address(apal, apll))
        {
            func_axdx(op_mov, apal, apah, apll, aplh);
        }
        liveQualify(apah, apah, 0);
        liveQualify(apal, apah, 0);
        gen_codes(op_neg, ISZ_UINT, apah, 0);
        gen_codes(op_neg, ISZ_UINT, apal, 0);
        gen_codes(op_sbb, ISZ_UINT, apah, aimmed(0));
    }
    else
    {
        if (!equal_address(apal, apll))
        {
            liveQualify(apal, apal, 0);
            gen_codes(op_mov, q->ans->size, apal, apll);
        }
        gen_codes(op_neg, q->ans->size, apal, 0);
    }
}
void asm_not(Optimizer::QUAD* q) /* complement */
{
    enum e_opcode opa, opl;
    AMODE *apal, *apah, *apll, *aplh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (!equal_address(apal, apll))
        {
            func_axdx(op_mov, apal, apah, apll, aplh);
        }
        gen_codes(op_not, ISZ_UINT, apah, 0);
        gen_codes(op_not, ISZ_UINT, apal, 0);
    }
    else
    {
        if (!equal_address(apal, apll))
        {
            liveQualify(apal, apal, 0);
            gen_codes(op_mov, q->ans->size, apal, apll);
        }
        gen_codes(op_not, q->ans->size, apal, 0);
    }
}
void asm_and(Optimizer::QUAD* q) /* binary and */
{
    if (q->atomic)
    {
        logicatomic(op_and, q);
        return;
    }
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (equal_address(apal, apll))
        {
            func_axdx(op_and, apal, apah, aprl, aprh);
        }
        else if (equal_address(apal, aprl))
        {
            func_axdx(op_and, apal, apah, apll, aplh);
        }
        else
        {
            liveQualify(apal, apal, apah);
            liveQualify(apah, apal, apah);
            func_axdx_mov(op_and, apal, apah, apll, aplh, aprl, aprh);
        }
    }
    else
    {
        if (equal_address(apal, apll))
        {
            gen_codes(op_and, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
            gen_codes(op_and, q->ans->size, apal, apll);
        else
        {
            if (equal_address(apll, aprl))
                apal->liveRegs = -1;
            if (samereg(apal, aprl))
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_codes(op_and, q->ans->size, apal, apll);
            }
            else
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, apll);
                gen_codes(op_and, q->ans->size, apal, aprl);
            }
        }
    }
}
void asm_or(Optimizer::QUAD* q) /* binary or */
{
    if (q->atomic)
    {
        logicatomic(op_or, q);
        return;
    }
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (equal_address(apal, apll))
        {
            func_axdx(op_or, apal, apah, aprl, aprh);
        }
        else if (equal_address(apal, aprl))
        {
            func_axdx(op_or, apal, apah, apll, aplh);
        }
        else
        {
            liveQualify(apal, apal, apah);
            liveQualify(apah, apal, apah);
            func_axdx_mov(op_or, apal, apah, apll, aplh, aprl, aprh);
        }
    }
    else
    {
        if (equal_address(apal, apll))
        {
            gen_codes(op_or, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
            gen_codes(op_or, q->ans->size, apal, apll);
        else
        {
            if (equal_address(apll, aprl))
                apal->liveRegs = -1;
            if (samereg(apal, aprl))
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_codes(op_or, q->ans->size, apal, apll);
            }
            else
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, apll);
                gen_codes(op_or, q->ans->size, apal, aprl);
            }
        }
    }
}
void asm_eor(Optimizer::QUAD* q) /* binary exclusive or */
{
    if (q->atomic)
    {
        logicatomic(op_xor, q);
        return;
    }
    enum e_opcode opa, opl, opr;
    AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &opa, q->ans, &apal, &apah);
    if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
    {
        if (equal_address(apal, apll))
        {
            func_axdx(op_xor, apal, apah, aprl, aprh);
        }
        else if (equal_address(apal, aprl))
        {
            func_axdx(op_xor, apal, apah, apll, aplh);
        }
        else
        {
            liveQualify(apal, apal, apah);
            liveQualify(apah, apal, apah);
            func_axdx_mov(op_xor, apal, apah, apll, aplh, aprl, aprh);
        }
    }
    else
    {
        if (equal_address(apal, apll))
        {
            gen_codes(op_xor, q->ans->size, apal, aprl);
        }
        else if (equal_address(apal, aprl))
            gen_codes(op_xor, q->ans->size, apal, apll);
        else
        {
            if (equal_address(apll, aprl))
                apal->liveRegs = -1;
            if (samereg(apal, aprl))
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, aprl);
                gen_codes(op_xor, q->ans->size, apal, apll);
            }
            else
            {
                liveQualify(apal, apal, 0);
                gen_codes(op_mov, q->ans->size, apal, apll);
                gen_codes(op_xor, q->ans->size, apal, aprl);
            }
        }
    }
}
void asm_setne(Optimizer::QUAD* q) /* evaluate a = b != c */ { gen_xset(q, op_setne, op_setne, op_setne); }
void asm_sete(Optimizer::QUAD* q) /* evaluate a = b == c */ { gen_xset(q, op_sete, op_sete, op_sete); }
void asm_setc(Optimizer::QUAD* q) /* evaluate a = b U< c */ { gen_xset(q, op_setc, op_seta, op_setc); }
void asm_seta(Optimizer::QUAD* q) /* evaluate a = b U> c */ { gen_xset(q, op_seta, op_setc, op_seta); }
void asm_setnc(Optimizer::QUAD* q) /* evaluate a = b U>= c */ { gen_xset(q, op_setnc, op_setbe, op_setae); }
void asm_setbe(Optimizer::QUAD* q) /* evaluate a = b U<= c */ { gen_xset(q, op_setbe, op_setnc, op_setbe); }
void asm_setl(Optimizer::QUAD* q) /* evaluate a = b S< c */ { gen_xset(q, op_setl, op_setg, op_setc); }
void asm_setg(Optimizer::QUAD* q) /* evaluate a = b s> c */ { gen_xset(q, op_setg, op_setl, op_seta); }
void asm_setle(Optimizer::QUAD* q) /* evaluate a = b S<= c */ { gen_xset(q, op_setle, op_setge, op_setbe); }
void asm_setge(Optimizer::QUAD* q) /* evaluate a = b S>= c */ { gen_xset(q, op_setge, op_setle, op_setae); }
void asm_assn(Optimizer::QUAD* q) /* assignment */
{
    AMODE *apa, *apa1, *apl = nullptr, *apl1 = nullptr;
    enum e_opcode opa, opl;
    /* when we get here, one side or the other is in a register */
    int szl;
    int sza = q->ans->size;
    int bits = q->ans->bits;
    if (sza < 0)
        sza = -sza;
    szl = sza;
    q->ans->bits = 0;
    getAmodes(q, &opa, q->ans, &apa, &apa1);
    q->ans->bits = bits;
    if (q->dc.opcode == Optimizer::i_assn)
    {
        szl = q->dc.left->size;
        if (szl < 0)
            szl = -szl;
        // next is for sized constants
        getAmodes(q, &opl, q->dc.left, &apl, &apl1);
    }
    else if (q->dc.opcode == Optimizer::i_icon)
    {
        opl = op_mov;
        if (sza == ISZ_ULONGLONG)
        {

            apl = aimmed(q->dc.v.i);
#ifdef USE_LONGLONG
            apl1 = aimmed((q->dc.v.i >> 32));
#else
            if (q->dc.i < 0)
                apl1 = aimmed(-1);
            else
                apl1 = aimmed(0);
#endif
        }
        else
        {
            apl = aimmed(q->dc.v.i);
        }
    }
    else if (q->dc.opcode == Optimizer::i_fcon)
    {
        Optimizer::SimpleExpression* node = Optimizer::simpleExpressionNode(Optimizer::se_f, 0, 0);
        node->sizeFromType = ISZ_LDOUBLE;
        node->f = q->dc.v.f;
        apl = beLocalAllocate<AMODE>();
        apl->offset = node;
        make_floatconst(apl);
    }
    else
        diag("asm_assn: unknown opcode");
    if (sza == szl || q->dc.left->mode == Optimizer::i_immed)
    {
        if (q->atomic && (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG))
        {
            bool pushed = false;
            bool pushbx = false, pushcx = false, pushbp = false;
            int reg1 = apa->preg;
            int reg2 = apa1->preg;
            if ((reg1 != EAX || reg2 != EDX) && (apa->liveRegs & ((1 << EAX) | (1 << EDX))))
            {
                pushed = true;
                gen_code(op_push, makedreg(EDX), NULL);
                gen_code(op_push, makedreg(EAX), NULL);
                pushlevel += 8;
            }
            if (apa->liveRegs & (1 << EBX) && reg1 != EBX && reg2 != EBX)
            {
                pushbx = true;
                gen_code(op_push, makedreg(EBX), NULL);
                pushlevel += 4;
            }
            if (apa->liveRegs & (1 << ECX) && reg1 != ECX && reg2 != ECX)
            {
                pushcx = true;
                gen_code(op_push, makedreg(ECX), NULL);
                pushlevel += 4;
            }
            if (apl->mode == am_indispscale || (apl->mode == am_indisp && apl->preg != ESP && apl->preg != EBP))
            {
                pushbp = true;
                gen_code(op_push, makedreg(EBP), NULL);
                pushlevel += 4;

                gen_code(op_lea, makedreg(EBP), apl);
                apl = makedreg(EBP);
                apl->mode = am_indisp;
                apl->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
                apl1 = makedreg(EBP);
                apl1->mode = am_indisp;
                apl1->offset = Optimizer::simpleIntNode(Optimizer::se_i, 4);
            }
            gen_code(op_mov, makedreg(EAX), apl);
            gen_code(op_mov, makedreg(EDX), apl1);
            int labno = beGetLabel;
            oa_gen_label(labno);
            gen_code(op_mov, makedreg(EBX), makedreg(EAX));
            gen_code(op_mov, makedreg(ECX), makedreg(EDX));
            gen_codes(op_cmpxchg8b, ISZ_NONE, apl, NULL);
            gen_code(op_jne, make_label(labno), NULL);
            gen_code(op_mov, apa, makedreg(EAX));
            gen_code(op_mov, apa1, makedreg(EDX));
            if (pushbp)
            {
                gen_code(op_pop, makedreg(EBP), NULL);
                pushlevel -= 4;
            }
            if (pushcx)
            {
                gen_code(op_pop, makedreg(ECX), NULL);
                pushlevel -= 4;
            }
            if (pushbx)
            {
                gen_code(op_pop, makedreg(EBX), NULL);
                pushlevel -= 4;
            }
            if (pushed)
            {
                gen_code(op_pop, makedreg(EAX), NULL);
                gen_code(op_pop, makedreg(EDX), NULL);
                pushlevel -= 8;
            }
        }
        else if (q->ans->retval && q->ans->size >= ISZ_FLOAT)
        {
            if (Optimizer::fltexp)
            {
                AMODE* ap = make_offset(Optimizer::fltexp);
                if (q->ans->size >= ISZ_CFLOAT)
                {
                    int sz = 8;
                    if (q->ans->size == ISZ_CFLOAT)
                        sz = 4;
                    AMODE* ap1 = make_offset(Optimizer::simpleExpressionNode(Optimizer::se_add, Optimizer::fltexp,
                                                                             Optimizer::simpleIntNode(Optimizer::se_i, sz)));
                    apa = moveFP(apa, q->ans->size, apl, q->dc.left->size);
                    apa1 = moveFP(apa1, q->ans->size, apl1, q->dc.left->size);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, ap, apa);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, ap1, apa1);
                    ap1->length = ap->length = q->ans->size - ISZ_CFLOAT + ISZ_FLOAT;
                    gen_codef(op_fld, ap1, NULL);
                    gen_codef(op_fld, ap, NULL);
                }
                else
                {
                    apa = moveFP(apa, q->ans->size, apl, q->dc.left->size);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, ap, apa);
                    ap->length = q->ans->size >= ISZ_IFLOAT ? q->ans->size - ISZ_IFLOAT + ISZ_FLOAT : q->ans->size;
                    gen_codef(op_fld, ap, NULL);
                }
            }
            else
            {
                diag("asm_assn: no float store area");
            }
        }
        else if (q->dc.left->retval && q->dc.left->size >= ISZ_FLOAT)
        {
            if (q->dc.left->altretval)
            {
                moveFP(apa, q->ans->size, makeSSE(0), q->dc.left->size);
            }
            else
            {
                if (Optimizer::fltexp)
                {
                    AMODE* ap = make_offset(Optimizer::fltexp);
                    if (q->ans->size >= ISZ_CFLOAT)
                    {
                        int sz = 8;
                        if (q->ans->size == ISZ_CFLOAT)
                            sz = 4;
                        AMODE* ap1 = make_offset(Optimizer::simpleExpressionNode(Optimizer::se_add, Optimizer::fltexp,
                                                                                 Optimizer::simpleIntNode(Optimizer::se_i, sz)));
                        ap1->length = ap->length = q->ans->size - ISZ_CFLOAT + ISZ_FLOAT;
                        gen_codef(op_fstp, ap, NULL);
                        gen_codef(op_fstp, ap1, NULL);
                        ap1->length = ap->length = 0;
                        gen_code_sse(op_movss, op_movsd, q->ans->size, apa, ap);
                        gen_code_sse(op_movss, op_movsd, q->ans->size, apa1, ap1);
                        ap1->length = ap->length = q->ans->size - ISZ_CFLOAT + ISZ_FLOAT;
                    }
                    else
                    {
                        ap->length = q->ans->size >= ISZ_IFLOAT ? q->ans->size - ISZ_IFLOAT + ISZ_FLOAT : q->ans->size;
                        gen_codef(op_fstp, ap, NULL);
                        ap->length = 0;
                        gen_code_sse(op_movss, op_movsd, q->ans->size, apa, ap);
                    }
                }
                else
                {
                    diag("asm_assn: no float store area");
                }
            }
        }
        else if (q->ans->size >= ISZ_CFLOAT)
        {
            if (q->dc.left->size >= ISZ_CFLOAT)
            {
                moveFP(apa, q->ans->size, apl, q->dc.left->size);
                moveFP(apa1, q->ans->size, apl1, q->dc.left->size);
            }
            else
            {
                if (q->dc.left->size >= ISZ_IFLOAT)
                {
                    moveFP(apa1, q->ans->size, apl, q->dc.left->size);
                    moveFP(apa, q->ans->size, floatzero(apl1), q->dc.left->size);
                }
                else
                {
                    moveFP(apa, q->ans->size, apl, q->dc.left->size);
                    moveFP(apa1, q->ans->size, floatzero(apa1), q->dc.left->size);
                }
            }
        }
        else if (q->ans->size >= ISZ_FLOAT)
        {
            moveFP(apa, q->ans->size, apl, q->dc.left->size);
        }
        else if (sza == ISZ_ULONGLONG)
        {
            func_axdx(op_mov, apa, apa1, apl, apl1);
        }
        else
        {
            if (q->ans->bits)
                bit_store(apa, apl, q->ans->size, q->ans->bits, q->ans->startbit);
            else
            {
                if (sza < szl)
                {
                    if (q->ans->size == ISZ_BOOLEAN)
                    {
                        gen_codes(opl, q->dc.left->size, apa, apl);
                        gen_codes(op_cmp, q->dc.left->size, apa, aimmed(0));
                        gen_codes(op_setne, q->ans->size, apa, nullptr);
                    }
                    else if (szl == ISZ_ULONGLONG)
                    {
                        gen_codes(opl, q->ans->size, apa, apl);
                    }
                    else
                    {
                        gen_codes(opl, q->dc.left->size, apa, apl);
                    }
                }
                else
                {
                    gen_codes(opl, q->ans->size, apa, apl);
                }
                if (q->dc.opcode == Optimizer::i_assn && q->dc.left->bits)
                {
                    int max;
                    switch (sza)
                    {
                        case ISZ_UCHAR:
                            max = 8;
                            break;
                        case ISZ_USHORT:
                        case ISZ_U16:
                            max = 16;
                            break;
                        case ISZ_UINT:
                        case ISZ_ULONG:
                        case ISZ_U32:
                        default:
                            max = 32;
                            break;
                    }
                    /* should be in a register at this point */
                    if (apa->mode != am_dreg)
                        diag("asm_assn: Expected register in bit processing");
                    if (q->dc.left->size < 0)
                    {
                        gen_codes(op_shl, q->ans->size, apa, aimmed(max - q->dc.left->startbit - q->dc.left->bits));
                        gen_codes(op_sar, q->ans->size, apa, aimmed(max - q->dc.left->bits));
                    }
                    else
                    {
                        if (q->dc.left->startbit)
                            gen_codes(op_shr, q->ans->size, apa, aimmed(q->dc.left->startbit));
                        if (q->dc.left->bits < max)
                            gen_codes(op_and, q->ans->size, apa, aimmed((1 << q->dc.left->bits) - 1));
                    }
                }
            }
        }
    }
    else if (sza < szl)
    {
        if (q->dc.left->size == ISZ_ADDR)
            if (q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG)
                goto addrupjn;
        if (q->ans->size == ISZ_BOOLEAN)

        {
            int ulbl = beGetLabel;
            if (q->dc.left->size >= ISZ_CFLOAT)
            {

                int lbl1 = beGetLabel;
                AMODE* apz;
                gen_code_sse(op_ucomiss, op_ucomisd, q->dc.left->size, apl, apz = floatzero(apl));
                zerocleanup();
                gen_code(op_jnz, make_label(lbl1), NULL);
                gen_code_sse(op_ucomiss, op_ucomisd, q->dc.left->size, apl1, apz);
                oa_gen_label(lbl1);
                gen_code(op_setne, apa, NULL);
                gen_codes(op_and, ISZ_UINT, apa, aimmed(1));
            }
            else if (q->dc.left->size >= ISZ_IFLOAT)
            {
                gen_code_sse(op_ucomiss, op_ucomisd, q->dc.left->size, apl, floatzero(apl));
                zerocleanup();
                gen_code(op_setne, apa, NULL);
                gen_codes(op_and, ISZ_UINT, apa, aimmed(1));
            }
            else if (q->dc.left->size >= ISZ_FLOAT)
            {
                gen_code_sse(op_ucomiss, op_ucomisd, q->dc.left->size, apl, floatzero(apl));
                zerocleanup();
                gen_code(op_setne, apa, NULL);
                gen_codes(op_and, ISZ_UINT, apa, aimmed(1));
            }
            else if (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG)
            {
                if (apl->mode == am_dreg)
                {
                    if (apl->preg == apa->preg)
                    {
                        gen_codes(op_or, ISZ_UINT, apa, apl1);
                    }
                    else if (apl1->preg == apa->preg)
                    {
                        gen_codes(op_or, ISZ_UINT, apa, apl);
                    }
                    else
                    {
                        gen_codes(op_mov, ISZ_UINT, apa, apl);
                        gen_codes(op_or, ISZ_UINT, apa, apl1);
                    }
                }
                else if ((apl->mode == am_indispscale && (apl->preg == apa->preg || apl->sreg == apa->preg)) ||
                         (apl->mode == am_indisp && apl->preg == apa->preg))
                {
                    gen_codes(op_push, ISZ_UINT, apl1, NULL);
                    pushlevel += 4;
                    gen_codes(op_mov, ISZ_UINT, apa, apl);
                    gen_codes(op_or, ISZ_UINT, apa, make_stack(0));
                    gen_codes(op_pop, ISZ_UINT, apa, NULL);
                    pushlevel -= 4;
                }
                else
                {
                    gen_codes(op_mov, ISZ_UINT, apa, apl);
                    gen_codes(op_or, ISZ_UINT, apa, apl1);
                }
            }
            else
            {
                gen_codes(op_cmp, q->dc.left->size, apl, aimmed(0));
            }
            gen_codes(op_setne, ISZ_UCHAR, apa, 0);
        }
        else if (q->dc.left->size >= ISZ_CFLOAT)
        {
            if (q->ans->size >= ISZ_CFLOAT)
            {
                moveFP(apa, q->ans->size, apl, q->dc.left->size);
                moveFP(apa1, q->ans->size, apl1, q->dc.left->size);
            }
            else if (q->ans->size >= ISZ_IFLOAT)
            {
                moveFP(apa, q->ans->size, apl1, q->dc.left->size);
            }
            else if (q->ans->size >= ISZ_FLOAT)
            {
                moveFP(apa, q->ans->size, apl, q->dc.left->size);
            }
            else
            {
                apa->length = ISZ_UINT;
                gen_code_sse(op_cvttss2si, op_cvttsd2si, q->dc.left->size, apa, apl);
            }
        }
        else if (q->dc.left->size >= ISZ_FLOAT)
        {
            moveFP(apa, q->ans->size, apl, q->dc.left->size);
        }
        else
        {
            int size;
            /* this next size optimization is good for code gen,
             * but it is required for example if we were to try to
             * gen mov cl,(lowbyte(edi))
             *
             * the ans will be one of the lower four registers
             * in this case because of the way the registers
             * are declared in config.c
             */

        addrdnjn:
            size = q->ans->size;
            if (apa->mode == am_dreg && apl->mode == am_dreg)
                size = ISZ_UINT;
            else if (size <= ISZ_UCHAR && apl->mode == am_dreg && apl->preg > EBX)
                diag("asm_assn: Conversion of invalid reg to byte");
            if (q->ans->bits)
                bit_store(apa, apl, q->ans->size, q->ans->bits, q->ans->startbit);
            else
            {
                if (q->dc.opcode == Optimizer::i_assn && q->dc.left->bits)
                {
                    int max;
                    switch (szl)
                    {
                        case ISZ_UCHAR:
                            max = 8;
                            break;
                        case ISZ_USHORT:
                        case ISZ_U16:
                            max = 16;
                            break;
                        case ISZ_UINT:
                        case ISZ_ULONG:
                        case ISZ_U32:
                        default:
                            max = 32;
                            break;
                    }
                    liveQualify(apa, apa, 0);
                    gen_codes(opl, size, apa, apl);
                    /* should be in a register at this point */
                    if (apa->mode != am_dreg)
                        diag("asm_assn: Expected register in bit processing");
                    if (q->dc.left->startbit)
                        gen_codes(op_shr, q->ans->size, apa, aimmed(q->dc.left->startbit));
                    if (q->dc.left->bits < max)
                        gen_codes(op_and, q->ans->size, apa, aimmed((1 << q->dc.left->bits) - 1));
                }
                else
                {
                    gen_codes(opl, size, apa, apl);
                }
            }
        }
    }
    else /* q->ans->size > q->dc.left->size */
    {
        if (q->ans->size == ISZ_ADDR)
            if (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG)
            {
                goto addrdnjn;
            }
        if (q->ans->size >= ISZ_FLOAT)
        {
            if (q->dc.left->size >= ISZ_CFLOAT)
            {
                moveFP(apa, q->ans->size, apl, q->dc.left->size);
                moveFP(apa1, q->ans->size, apl1, q->dc.left->size);
            }
            else if (q->dc.left->size >= ISZ_IFLOAT)
            {
                if (q->ans->size >= ISZ_CFLOAT)
                {
                    moveFP(apa1, q->ans->size, apl, q->dc.left->size);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apa, floatzero(apl));
                    zerocleanup();
                }
                else
                {
                    moveFP(apa, q->ans->size, apl, q->dc.left->size);
                }
            }
            else if (q->dc.left->size >= ISZ_FLOAT)
            {
                if (q->ans->size >= ISZ_CFLOAT)
                {
                    moveFP(apa, q->ans->size, apl, q->dc.left->size);
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apa1, floatzero(apl));
                    zerocleanup();
                }
                else
                {
                    moveFP(apa, q->ans->size, apl, q->dc.left->size);
                }
            }
            else if (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG || q->dc.left->size == ISZ_ULONG ||
                     q->dc.left->size == ISZ_UINT)
            {
                diag("asm_assn: untranslated conversion from long or longlong");
            }
            else
            {
                if (q->ans->size >= ISZ_IFLOAT && q->ans->size <= ISZ_ILDOUBLE)
                {
                    gen_code_sse(op_movss, op_movsd, q->ans->size, apa, floatzero(apl));
                    zerocleanup();
                }
                else
                {
                    if (q->dc.left->size != -ISZ_ULONG && q->dc.left->size != -ISZ_UINT)
                    {
                        AMODE* ap = makedreg(ECX);
                        ap->liveRegs = q->liveRegs;
                        gen_codes(op_push, ISZ_UINT, ap, 0);
                        pushlevel += 4;
                        switch (q->dc.left->size)
                        {
                            case ISZ_BOOLEAN:
                                gen_codes(op_mov, ISZ_UINT, ap, apl);
                                gen_codes(op_and, ISZ_UINT, ap, aimmed(1));
                                break;
                            case ISZ_UCHAR:
                                do_movzx(ISZ_UINT, ISZ_UCHAR, ap, apl);
                                break;
                            case -ISZ_UCHAR:
                                do_movsx(ISZ_UINT, ISZ_UCHAR, ap, apl);
                                break;
                            case ISZ_USHORT:
                                do_movzx(ISZ_UINT, ISZ_USHORT, ap, apl);
                                break;
                            case -ISZ_USHORT:
                                do_movsx(ISZ_UINT, ISZ_USHORT, ap, apl);
                                break;
                            default:
                                break;
                        }
                        gen_code_sse(op_cvtsi2ss, op_cvtsi2sd, q->ans->size, apa, ap);
                        gen_codes(op_pop, ISZ_UINT, ap, 0);
                        pushlevel -= 4;
                    }
                    else
                    {
                        gen_code_sse(op_cvtsi2ss, op_cvtsi2sd, q->ans->size, apa, apl);
                    }
                    if (q->ans->size >= ISZ_CFLOAT)
                    {
                        gen_code_sse(op_movss, op_movsd, q->ans->size, apa1, floatzero(apl));
                        zerocleanup();
                    }
                }
            }
        }
        else
        {
            AMODE* ap;
        addrupjn:
            ap = apa;
            if (apa1)
            {
                liveQualify(apa1, apa, apa1);
                liveQualify(apa, apa, apa1);
            }
            if (ap->mode != am_dreg)
            {
                ap = apl;
                if (apa1)
                    liveQualify(apl, apa, apa1);
            }
            if (szl <= ISZ_ULONG || szl == ISZ_ADDR)
            {
                switch (q->dc.left->size)
                {
                    case ISZ_BOOLEAN:
                        if (ap->preg <= EBX)
                        {
                            gen_codes(op_mov, ISZ_UCHAR, ap, apl);
                            gen_codes(op_and, ISZ_UINT, ap, aimmed(1));
                        }
                        else
                        {
                            gen_codes(op_mov, ISZ_USHORT, ap, apl);
                            gen_codes(op_and, ISZ_UINT, ap, aimmed(1));
                        }
                        break;
                    case ISZ_UCHAR:
                        do_movzx(ISZ_UINT, ISZ_UCHAR, ap, apl);
                        break;
                    case -ISZ_UCHAR:
                        do_movsx(ISZ_UINT, ISZ_UCHAR, ap, apl);
                        break;
                    case ISZ_USHORT:
                    case ISZ_U16:
                        do_movzx(ISZ_UINT, ISZ_USHORT, ap, apl);
                        break;
                    case -ISZ_USHORT:
                        do_movsx(ISZ_UINT, ISZ_USHORT, ap, apl);
                        break;
                    case ISZ_U32:
                    case ISZ_UINT:
                    case -ISZ_UINT:
                    case ISZ_ULONG:
                    case -ISZ_ULONG:
                    case ISZ_ADDR:
                        if (ap == apa)
                        {
                            if (q->ans->bits)
                                bit_store(ap, apl, q->ans->size, q->ans->bits, q->ans->startbit);
                            else
                            {
                                gen_codes(op_mov, ISZ_UINT, ap, apl);
                                if (q->dc.opcode == Optimizer::i_assn && q->dc.left->bits)
                                {
                                    /* should be in a register at this point */
                                    if (apa->mode != am_dreg)
                                        diag("asm_assn: Expected register in bit processing");
                                    if (q->dc.left->startbit)
                                    {
                                        liveQualify(apa, apa, 0);
                                        gen_codes(op_shr, q->ans->size, apa, aimmed(q->dc.left->startbit));
                                    }
                                    if (q->dc.left->bits < 32)
                                        gen_codes(op_and, q->ans->size, apa, aimmed((1 << q->dc.left->bits) - 1));
                                }
                            }
                        }
                        break;
                }
            }
            if (ap != apa)
            {
                if (q->ans->bits)
                    bit_store(ap, apl, q->ans->size, q->ans->bits, q->ans->startbit);
                else
                {
                    gen_codes(op_mov, ISZ_UINT, apa, ap);
                    peep_tail->oper1->liveRegs = q->liveRegs;
                    if (q->dc.opcode == Optimizer::i_assn && q->dc.left->bits)
                    {
                        int max;
                        switch (szl)
                        {
                            case ISZ_UCHAR:
                                max = 8;
                                break;
                            case ISZ_USHORT:
                            case ISZ_U16:
                                max = 16;
                                break;
                            case ISZ_UINT:
                            case ISZ_ULONG:
                            case ISZ_U32:
                            case ISZ_ADDR:
                            default:
                                max = 32;
                                break;
                        }
                        /* should be in a register at this point */
                        if (apa->mode != am_dreg)
                            diag("asm_assn: Expected register in bit processing");
                        if (q->dc.left->startbit)
                        {
                            liveQualify(apa, apa, 0);
                            gen_codes(op_shr, q->ans->size, apa, aimmed(q->dc.left->startbit));
                        }
                        if (q->dc.left->bits < max)
                            gen_codes(op_and, q->ans->size, apa, aimmed((1 << q->dc.left->bits) - 1));
                    }
                }
            }
            if (sza == ISZ_ULONGLONG)
            {
                if (q->dc.left->size < 0)
                {
                    if (apa1->mode == am_dreg && apa->mode == am_dreg && apa1->preg == EDX && apa->preg == EAX)
                    {
                        gen_code(op_cdq, NULL, NULL);
                    }
                    else
                    {
                        gen_codes(op_mov, ISZ_UINT, apa1, apa);
                        gen_codes(op_sar, ISZ_UINT, apa1, aimmed(31));
                    }
                }
                else if (apa->mode == am_dreg)
                    gen_codes(op_sub, ISZ_UINT, apa1, apa1);
                else
                    gen_codes(op_mov, ISZ_UINT, apa1, aimmed(0));
            }
        }
    }
}
void asm_genword(Optimizer::QUAD* q) /* put a byte or word into the code stream */
{
    gen_code(op_genword, aimmed(q->dc.left->offset->i), 0);
}

void asm_coswitch(Optimizer::QUAD* q) /* switch characteristics */
{
    enum e_opcode op;
    switch_deflab = q->dc.v.label;
    switch_range = q->dc.right->offset->i;
    switch_case_max = switch_case_count = q->ans->offset->i;
    switch_ip = q->dc.left;
    getAmodes(q, &op, switch_ip, &switch_apl, &switch_aph);
    switch_live = 0;
    if (switch_apl->mode == am_dreg)
        switch_live |= 1 << switch_apl->preg;
    if (switch_aph && switch_aph->mode == am_dreg)
        switch_live |= 1 << switch_aph->preg;
    if (switch_ip->size == ISZ_ULONGLONG || switch_ip->size == -ISZ_ULONGLONG || switch_case_max <= 5)
    {
        switch_mode = swm_enumerate;
    }
    else if (switch_case_max * 10 / switch_range > 8)
    {
        switch_mode = swm_compactstart;
    }
    else
    {
        switch_mode = swm_tree;
        if (!switchTreeLabelCount || switchTreeLabelCount < switch_case_max)
        {
            free(switchTreeCases);
            free(switchTreeLabels);
            free(switchTreeBranchLabels);
            switchTreeLabelCount = (switch_case_max + 1024) & ~1023;
            switchTreeCases = (long long*)calloc(switchTreeLabelCount, sizeof(long long));
            switchTreeLabels = (int*)calloc(switchTreeLabelCount, sizeof(int));
            switchTreeBranchLabels = (int*)calloc(switchTreeLabelCount, sizeof(int));
        }
        switchTreePos = 0;
        memset(switchTreeBranchLabels, 0, sizeof(int) * switch_case_max);
    }
}
void asm_swbranch(Optimizer::QUAD* q) /* case characteristics */
{
    unsigned long long swcase = q->dc.left->offset->i;
    int lab = q->dc.v.label;
    if (switch_case_count == 0)
    {
        /*		diag("asm_swbranch, count mismatch"); in case only a default */
        return;
    }

    if (switch_mode == swm_compactstart)
    {
        compactSwitchHeader(swcase);
    }
    switch (switch_mode)
    {
        case swm_enumerate:
        default:
            if (switch_ip->size == ISZ_ULONGLONG || switch_ip->size == -ISZ_ULONGLONG)
            {
                int nxlab = beGetLabel;
                gen_codes(op_cmp, ISZ_UINT, switch_apl, aimmed(swcase));
                gen_branch(op_jne, nxlab);
                peep_tail->oper1->liveRegs = switch_live;
#ifdef USE_LONGLONG
                gen_codes(op_cmp, ISZ_UINT, switch_aph, aimmed(swcase >> 32));
#else
                if (switch_ip->size < 0 && swcase < 0)
                    gen_codes(op_cmp, ISZ_UINT, switch_aph, aimmed(-1));
                else
                    gen_codes(op_cmp, ISZ_UINT, switch_aph, aimmed(0));
#endif
                gen_branch(op_je, lab);
                peep_tail->oper1->liveRegs = switch_live;
                oa_gen_label(nxlab);
            }
            else
            {
                gen_codes(op_cmp, switch_ip->size, switch_apl, aimmed(swcase));
                gen_branch(op_je, lab);
                peep_tail->oper1->liveRegs = switch_live;
            }
            if (--switch_case_count == 0)
            {
                gen_branch(op_jmp, switch_deflab);
                peep_tail->oper1->liveRegs = switch_live;
            }
            break;
        case swm_compact:
            while (switch_lastcase < swcase)
            {
                gen_codes(op_dd, 0, make_label(switch_deflab), 0);
                switch_lastcase++;
            }
        case swm_compactstart:
            gen_codes(op_dd, 0, make_label(lab), 0);
            switch_lastcase = swcase + 1;
            switch_mode = swm_compact;
            --switch_case_count;
            break;
        case swm_tree:
            liveQualify(switch_apl, switch_apl, switch_aph);
            switchTreeCases[switchTreePos] = swcase;
            switchTreeLabels[switchTreePos++] = lab;
            //            switchTreeBranchLabels[switchTreePos++] = lab + 1;
            if (--switch_case_count == 0)
                bingen(0, switch_case_max / 2, switch_case_max);

            break;
    }
}
void asm_dc(Optimizer::QUAD* q) /* unused */ { (void)q; }
void asm_assnblock(Optimizer::QUAD* q) /* copy block of memory*/
{
    int n = q->ans->offset->i;
    AMODE *apl, *aph, *apal, *apah;
    enum e_opcode op, opa;
    Optimizer::SimpleExpression *ofs, *ofsa;

    getAmodes(q, &op, q->dc.right, &apl, &aph);
    getAmodes(q, &opa, q->dc.left, &apal, &apah);

    ofs = apl->offset;
    ofsa = apal->offset;
    if (q->dc.right->mode == Optimizer::i_immed)
    {
        op = op_lea;
        if (ofs->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apl->preg = ESP;
            }
            else
            {
                apl->preg = EBP;
            }
            apl->mode = am_indisp;
        }
        else
            apl->mode = am_direct;
    }
    if (q->dc.left->mode == Optimizer::i_immed)
    {
        opa = op_lea;
        if (ofsa->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apal->preg = ESP;
            }
            else
            {
                apal->preg = EBP;
            }
            apal->mode = am_indisp;
        }
        else
            apal->mode = am_direct;
    }

    if (n <= 24 && (q->dc.right->mode == Optimizer::i_immed || apl->mode == am_dreg) &&
        (q->dc.left->mode == Optimizer::i_immed || apal->mode == am_dreg))
    {
        AMODE* ax;
        int reg = -1;
        int i;
        int push = false;
        if (apl->mode == am_dreg)
        {
            apl->mode = am_indisp;
            ofs = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        if (apal->mode == am_dreg)
        {
            apal->mode = am_indisp;
            ofsa = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        for (i = 0; i < 4; i++)
        {
            if (Optimizer::chosenAssembler->arch->regMap[i][0] < 3 && !(q->liveRegs & ((unsigned long long)1 << i)))
            {
                if ((apl->mode != am_indisp || apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apl->mode != am_indispscale || (apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                     apl->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])) &&
                    (apal->mode != am_indisp || apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apal->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])))
                {
                    reg = Optimizer::chosenAssembler->arch->regMap[i][0];
                    break;
                }
            }
        }
        if (reg == -1)
        {
            for (i = 0; i < 6; i++)
            {
                if ((apl->mode != am_indisp || apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apl->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])) &&
                    (apal->mode != am_indisp || apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apal->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])))
                {
                    reg = Optimizer::chosenAssembler->arch->regMap[i][0];
                    push = true;
                    break;
                }
            }
        }
        ax = makedreg(reg);
        ax->liveRegs = q->liveRegs;
        if (push)
        {
            gen_codes(op_push, ISZ_UINT, ax, 0);
            pushlevel += 4;
        }
        if (n & 1)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 1));
            apal->offset =
                Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n - 1));
            gen_codes(op_mov, ISZ_UCHAR, ax, apl);
            gen_codes(op_mov, ISZ_UCHAR, apal, ax);
            n--;
        }
        if (n & 2)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 2));
            apal->offset =
                Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n - 2));
            gen_codes(op_mov, ISZ_USHORT, ax, apl);
            gen_codes(op_mov, ISZ_USHORT, apal, ax);
            n -= 2;
        }

        while (n > 0)
        {
            n -= 4;
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n));
            apal->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n));
            gen_codes(op_mov, ISZ_UINT, ax, apl);
            gen_codes(op_mov, ISZ_UINT, apal, ax);
        }
        if (push)
        {
            gen_codes(op_pop, ISZ_UINT, ax, 0);
            pushlevel -= 4;
        }
    }
    else
    {
        AMODE* cx = makedreg(ECX);
        AMODE* di = makedreg(EDI);
        AMODE* si = makedreg(ESI);
        cx->liveRegs = q->liveRegs;
        di->liveRegs = q->liveRegs;
        si->liveRegs = q->liveRegs;
        gen_codes(op_push, ISZ_UINT, di, 0);
        gen_codes(op_push, ISZ_UINT, si, 0);
        gen_codes(op_push, ISZ_UINT, cx, 0);
        pushlevel += 12;
        if (samereg(di, apl))
        {
            if (samereg(si, apal))
            {
                gen_codes(op, ISZ_UINT, cx, apl);
                gen_codes(opa, ISZ_UINT, di, apal);
                gen_codes(op_mov, ISZ_UINT, si, cx);
            }
            else
            {
                gen_codes(op, ISZ_UINT, si, apl);
                gen_codes(opa, ISZ_UINT, di, apal);
            }
        }
        else
        {
            gen_codes(opa, ISZ_UINT, di, apal);
            gen_codes(op, ISZ_UINT, si, apl);
        }
        gen_codes(op_mov, ISZ_UINT, cx, aimmed(n / 4));
        gen_code(op_cld, 0, 0);
        gen_code(op_rep, 0, 0);
        gen_code(op_movsd, 0, 0);
        if (n & 2)
            gen_code(op_movsw, 0, 0);
        if (n & 1)
            gen_code(op_movsb, 0, 0);
        gen_codes(op_pop, ISZ_UINT, cx, 0);
        gen_codes(op_pop, ISZ_UINT, si, 0);
        gen_codes(op_pop, ISZ_UINT, di, 0);
        pushlevel -= 12;
    }
}
void asm_clrblock(Optimizer::QUAD* q) /* clear block of memory */
{
    int n = q->dc.right->offset->i;
    AMODE *apl, *aph;
    AMODE *aprl, *aprh;
    enum e_opcode op, opr;
    Optimizer::SimpleExpression* ofs;

    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
    getAmodes(q, &op, q->dc.left, &apl, &aph);
    if (q->dc.right->mode != Optimizer::i_immed)
        n = INT_MAX;

    ofs = apl->offset;
    if (q->dc.left->mode == Optimizer::i_immed)
    {
        op = op_lea;
        if (ofs->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apl->preg = ESP;
            }
            else
            {
                apl->preg = EBP;
            }
            apl->mode = am_indisp;
        }
        else
            apl->mode = am_direct;
    }

    if (n <= 24 && (q->dc.left->mode == Optimizer::i_immed || apl->mode == am_dreg))
    {
        if (apl->mode == am_dreg)
        {
            apl->mode = am_indisp;
            ofs = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        if (n & 1)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 1));
            gen_codes(op_mov, ISZ_UCHAR, apl, aimmed(0));
            n--;
        }
        if (n & 2)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 2));
            gen_codes(op_mov, ISZ_USHORT, apl, aimmed(0));
            n -= 2;
        }
        while (n > 0)
        {
            n -= 4;
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n));
            gen_codes(op_mov, ISZ_ULONG, apl, aimmed(0));
        }
    }
    else
    {
        AMODE* ax = makedreg(EAX);
        AMODE* cx = makedreg(ECX);
        AMODE* di = makedreg(EDI);
        cx->liveRegs = q->liveRegs;
        di->liveRegs = q->liveRegs;
        gen_codes(op_push, ISZ_UINT, di, 0);
        gen_codes(op_push, ISZ_UINT, cx, 0);
        gen_codes(op_push, ISZ_UINT, ax, 0);
        pushlevel += 12;
        gen_codes(op, ISZ_UINT, di, apl);
        gen_codes(op_xor, ISZ_UINT, ax, ax);
        if (n == INT_MAX)
        {
            gen_codes(op_mov, ISZ_UINT, cx, aprl);
            gen_codes(op_shr, ISZ_UINT, cx, aimmed(2));
        }
        else
        {
            gen_codes(op_mov, ISZ_UINT, cx, aimmed(n / 4));
        }
        gen_code(op_cld, 0, 0);
        gen_code(op_rep, 0, 0);
        gen_code(op_stosd, 0, 0);
        if (n & 2)
            gen_code(op_stosw, 0, 0);
        if (n & 1)
            gen_code(op_stosb, 0, 0);
        gen_codes(op_pop, ISZ_UINT, ax, 0);
        gen_codes(op_pop, ISZ_UINT, cx, 0);
        gen_codes(op_pop, ISZ_UINT, di, 0);
        pushlevel -= 12;
    }
}
void asm_cmpblock(Optimizer::QUAD* q)
{
    int n = q->ans->offset->i;
    AMODE *apl, *aph, *apal, *apah;
    enum e_opcode op, opa;
    Optimizer::SimpleExpression *ofs, *ofsa;

    getAmodes(q, &op, q->dc.right, &apl, &aph);
    getAmodes(q, &opa, q->dc.left, &apal, &apah);

    ofs = apl->offset;
    ofsa = apal->offset;
    if (q->dc.right->mode == Optimizer::i_immed)
    {
        op = op_lea;
        if (ofs->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apl->preg = ESP;
            }
            else
            {
                apl->preg = EBP;
            }
            apl->mode = am_indisp;
        }
        else
            apl->mode = am_direct;
    }
    if (q->dc.left->mode == Optimizer::i_immed)
    {
        opa = op_lea;
        if (ofsa->type == Optimizer::se_auto)
        {
            if (usingEsp)
            {
                apal->preg = ESP;
            }
            else
            {
                apal->preg = EBP;
            }
            apal->mode = am_indisp;
        }
        else
            apal->mode = am_direct;
    }

    int labno = q->dc.v.label;

    if (n <= 24 && (q->dc.right->mode == Optimizer::i_immed || apl->mode == am_dreg) &&
        (q->dc.left->mode == Optimizer::i_immed || apal->mode == am_dreg))
    {
        AMODE* ax;
        int reg = -1;
        int i;
        int push = false;
        if (apl->mode == am_dreg)
        {
            apl->mode = am_indisp;
            ofs = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        if (apal->mode == am_dreg)
        {
            apal->mode = am_indisp;
            ofsa = Optimizer::simpleIntNode(Optimizer::se_i, 0);
        }
        for (i = 0; i < 4; i++)
        {
            if (Optimizer::chosenAssembler->arch->regMap[i][0] < 3 && !(q->liveRegs & ((unsigned long long)1 << i)))
            {
                if ((apl->mode != am_indisp || apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apl->mode != am_indispscale || (apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                     apl->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])) &&
                    (apal->mode != am_indisp || apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apal->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])))
                {
                    reg = Optimizer::chosenAssembler->arch->regMap[i][0];
                    break;
                }
            }
        }
        if (reg == -1)
        {
            for (i = 0; i < 6; i++)
            {
                if ((apl->mode != am_indisp || apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apl->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apl->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])) &&
                    (apal->mode != am_indisp || apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0]) &&
                    (apal->mode != am_indispscale || (apal->preg != Optimizer::chosenAssembler->arch->regMap[i][0] &&
                                                      apal->sreg != Optimizer::chosenAssembler->arch->regMap[i][0])))
                {
                    reg = Optimizer::chosenAssembler->arch->regMap[i][0];
                    push = true;
                    break;
                }
            }
        }
        ax = makedreg(reg);
        ax->liveRegs = q->liveRegs;
        if (push)
        {
            gen_codes(op_push, ISZ_UINT, ax, 0);
            pushlevel += 4;
        }
        if (n & 1)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 1));
            apal->offset =
                Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n - 1));
            gen_codes(op_mov, ISZ_UCHAR, ax, apl);
            gen_codes(op_cmp, ISZ_UCHAR, apal, ax);
            gen_code(op_jne, make_label(labno), NULL);
            n--;
        }
        if (n & 2)
        {
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n - 2));
            apal->offset =
                Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n - 2));
            gen_codes(op_mov, ISZ_USHORT, ax, apl);
            gen_codes(op_cmp, ISZ_USHORT, apal, ax);
            gen_code(op_jne, make_label(labno), NULL);
            n -= 2;
        }

        while (n > 0)
        {
            n -= 4;
            apl->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofs, Optimizer::simpleIntNode(Optimizer::se_i, n));
            apal->offset = Optimizer::simpleExpressionNode(Optimizer::se_add, ofsa, Optimizer::simpleIntNode(Optimizer::se_i, n));
            gen_codes(op_mov, ISZ_UINT, ax, apl);
            gen_codes(op_cmp, ISZ_UINT, apal, ax);
            gen_code(op_jne, make_label(labno), NULL);
        }
        if (push)
        {
            gen_codes(op_pop, ISZ_UINT, ax, 0);
            pushlevel -= 4;
        }
    }
    else
    {
        AMODE* cx = makedreg(ECX);
        AMODE* di = makedreg(EDI);
        AMODE* si = makedreg(ESI);
        cx->liveRegs = q->liveRegs;
        di->liveRegs = q->liveRegs;
        si->liveRegs = q->liveRegs;
        gen_codes(op_push, ISZ_UINT, di, 0);
        gen_codes(op_push, ISZ_UINT, si, 0);
        gen_codes(op_push, ISZ_UINT, cx, 0);
        pushlevel += 12;
        if (samereg(di, apl))
        {
            if (samereg(si, apal))
            {
                gen_codes(op, ISZ_UINT, cx, apl);
                gen_codes(opa, ISZ_UINT, di, apal);
                gen_codes(op_mov, ISZ_UINT, si, cx);
            }
            else
            {
                gen_codes(op, ISZ_UINT, si, apl);
                gen_codes(opa, ISZ_UINT, di, apal);
            }
        }
        else
        {
            gen_codes(opa, ISZ_UINT, di, apal);
            gen_codes(op, ISZ_UINT, si, apl);
        }
        gen_codes(op_mov, ISZ_UINT, cx, aimmed(n / 4));
        gen_code(op_cld, 0, 0);
        gen_code(op_rep, 0, 0);
        gen_code(op_cmpsd, 0, 0);
        int labno1 = beGetLabel;
        if (n & 2)
        {
            gen_code(op_jne, make_label(labno1), NULL);
            gen_code(op_cmpsw, 0, 0);
        }
        if (n & 1)
        {
            gen_code(op_jne, make_label(labno1), NULL);
            gen_code(op_cmpsb, 0, 0);
        }
        oa_gen_label(labno1);
        gen_codes(op_pop, ISZ_UINT, cx, 0);
        gen_codes(op_pop, ISZ_UINT, si, 0);
        gen_codes(op_pop, ISZ_UINT, di, 0);
        gen_code(op_jne, make_label(labno), NULL);
        pushlevel -= 12;
    }
}
void asm_jc(Optimizer::QUAD* q) /* branch if a U< b */ { gen_goto(q, op_jc, op_ja, op_jc, op_ja, op_jb, op_ja, op_jb); }
void asm_ja(Optimizer::QUAD* q) /* branch if a U> b */ { gen_goto(q, op_ja, op_jc, op_ja, op_jc, op_ja, op_jb, op_ja); }
void asm_je(Optimizer::QUAD* q) /* branch if a == b */ { gen_goto(q, op_je, op_jne, op_je, op_jne, op_je, op_jne, op_je); }
void asm_jnc(Optimizer::QUAD* q) /* branch if a U>= b */ { gen_goto(q, op_jnc, op_jbe, op_ja, op_jc, op_jae, op_jbe, op_jae); }
void asm_jbe(Optimizer::QUAD* q) /* branch if a U<= b */ { gen_goto(q, op_jbe, op_jnc, op_jc, op_ja, op_jbe, op_jnc, op_jbe); }
void asm_jne(Optimizer::QUAD* q) /* branch if a != b */ { gen_goto(q, op_jne, op_je, op_jne, op_je, op_jne, op_je, op_jne); }
void asm_jl(Optimizer::QUAD* q) /* branch if a S< b */ { gen_goto(q, op_jl, op_jg, op_jl, op_jg, op_jb, op_ja, op_jb); }
void asm_jg(Optimizer::QUAD* q) /* branch if a S> b */ { gen_goto(q, op_jg, op_jl, op_jg, op_jl, op_ja, op_jb, op_ja); }
void asm_jle(Optimizer::QUAD* q) /* branch if a S<= b */ { gen_goto(q, op_jle, op_jge, op_jl, op_jg, op_jbe, op_jae, op_jbe); }
void asm_jge(Optimizer::QUAD* q) /* branch if a S>= b */ { gen_goto(q, op_jge, op_jle, op_jg, op_jl, op_jae, op_jbe, op_jae); }
void asm_cppini(Optimizer::QUAD* q) /* cplusplus initialization (historic)*/ { (void)q; }
/*
 * function prologue.  left has a constant which is a bit mask
 * of registers to push.  It also has a flag indicating whether frames
 * are absolutely necessary
 *
 * right has the number of bytes to allocate on the stack
 */
void asm_prologue(Optimizer::QUAD* q) /* function prologue */
{
    Optimizer::chosenAssembler->arch->retblockparamadjust = usingEsp ? 0 : 4;
    inframe =
        !!(beGetIcon(q->dc.left) & FRAME_FLAG_NEEDS_FRAME) || Optimizer::cparams.prm_debug || Optimizer::cparams.prm_stackalign;
    if (inframe)
    {
        int n = beGetIcon(q->dc.right);

        if (Optimizer::cparams.prm_stackalign && beGetIcon(q->dc.left) != 0)
        {
            // adjust for pushed regs
            int cnt = 0;
            int mask = 1, compare;
            compare = (unsigned)(beGetIcon(q->dc.left) & ~(FRAME_FLAG_NEEDS_FRAME));
            while (mask <= compare)
            {
                if (mask & compare)
                    cnt += 4;
                mask <<= 1;
            }
            if (cnt % Optimizer::cparams.prm_stackalign)
                cnt = Optimizer::cparams.prm_stackalign - cnt % Optimizer::cparams.prm_stackalign;
            n += cnt;
        }
        /* enter is *really* inefficient so we will not use it */
        if (!usingEsp)
        {
            gen_code(op_push, makedreg(EBP), 0);
            gen_code(op_mov, makedreg(EBP), makedreg(ESP));
        }
        pushlevel = 0;
        if (n)
        {
            if (n < 16)
            {
                int i;
                for (i = 0; i < n; i += 4)
                    gen_code(op_push, makedreg(ECX), 0);
            }
            else if (n <= 4092)
            {
                gen_code(op_add, makedreg(ESP), aimmed(-n));
            }
            else
            {
                if (n <= 8188)
                {
                    gen_code(op_add, makedreg(ESP), aimmed(-4092));
                    gen_code(op_push, makedreg(EAX), 0);
                    gen_code(op_add, makedreg(ESP), aimmed(-(n - 4096)));
                }
                else
                {
                    n += 15;   // must match the RTL...
                    n &= -16;  //
                    gen_code(op_push, aimmed(n), 0);
                    callLibrary("___substackp", 0);
                }
            }
        }
        funcstackheight = n;
    }
    else
    {
        funcstackheight = 0;
    }
    if (beGetIcon(q->dc.left) != 0)
    {
        int cnt = 0;
        int mask = 1, compare;
        compare = (unsigned)(beGetIcon(q->dc.left) & ~(FRAME_FLAG_NEEDS_FRAME));
        if (currentFunction->anyTry)
            compare |= 0xc8;  // make sure we push everything if this function may have try/catch blocks
        while (mask <= compare)
        {
            if (mask & compare)
            {
                funcstackheight += 4;
                gen_code(op_push, makedreg(Optimizer::chosenAssembler->arch->regMap[cnt][0]), 0);
            }
            cnt++, mask <<= 1;
        }
    }
}
/*
 * function epilogue, left holds the mask of which registers were pushed
 */
void asm_epilogue(Optimizer::QUAD* q) /* function epilogue */
{
    if (pushlevel != 0 && usingEsp)
        diag("asm_epilogue: pushlevel not aligned");
    if (beGetIcon(q->dc.left) != 0)
    {
        int mask = 0x80, compare;
        int cnt = 7;
        compare = (unsigned)(beGetIcon(q->dc.left) & ~(FRAME_FLAG_NEEDS_FRAME));
        if (currentFunction->anyTry)
            compare |= 0xc8;  // make sure we push everything if this function may have try/catch blocks
        while (mask)
        {
            if (mask & compare)
            {
                gen_code(op_pop, makedreg(Optimizer::chosenAssembler->arch->regMap[cnt][0]), 0);
                funcstackheight -= 4;
            }
            cnt--, mask >>= 1;
        }
    }
    if (inframe)
    {
        if (usingEsp)
        {
            if (funcstackheight <= 16)
            {
                int i;
                for (i = 0; i < funcstackheight; i += 4)
                    gen_code(op_pop, makedreg(ECX), 0);
            }
            else
            {
                gen_code(op_add, makedreg(ESP), aimmed(funcstackheight));
            }
        }
        else
        {
            if (beGetIcon(q->dc.right))
                gen_code(op_mov, makedreg(ESP), makedreg(EBP));
            gen_code(op_pop, makedreg(EBP), 0);
        }
    }
}
/*
 * in an interrupt handler, push the current context
 */
void asm_pushcontext(Optimizer::QUAD* q) /* push register context */
{
    (void)q;
    if (Optimizer::cparams.prm_farkeyword)
    {
        gen_code(op_push, makesegreg(ES), 0);
        gen_code(op_push, makesegreg(FS), 0);
        gen_code(op_push, makesegreg(GS), 0);
    }
    gen_code(op_pushad, 0, 0);
}
/*
 * in an interrupt handler, pop the current context
 */
void asm_popcontext(Optimizer::QUAD* q) /* pop register context */
{
    (void)q;
    gen_code(op_popad, 0, 0);
    if (Optimizer::cparams.prm_farkeyword)
    {
        gen_code(op_pop, makesegreg(GS), 0);
        gen_code(op_pop, makesegreg(FS), 0);
        gen_code(op_pop, makesegreg(ES), 0);
    }
}
/*
 * loads a context, e.g. for the loadds qualifier
 */
void asm_loadcontext(Optimizer::QUAD* q) /* load register context (e.g. at interrupt level ) */
{
    (void)q;
    gen_code(op_push, makesegreg(DS), 0);
    gen_code(op_push, makesegreg(CS), 0);
    gen_codes(op_add, 2, make_stack(0), aimmed(8));
    gen_code(op_pop, makesegreg(DS), 0);
}
/*
 * unloads a context, e.g. for the loadds qualifier
 */
void asm_unloadcontext(Optimizer::QUAD* q) /* load register context (e.g. at interrupt level ) */
{
    (void)q;
    gen_code(op_pop, makesegreg(DS), 0);
}
void asm_tryblock(Optimizer::QUAD* q) /* try/catch */
{
    AMODE* ap1 = beLocalAllocate<AMODE>();
    ap1->mode = am_indisp;
    if (usingEsp)
    {
        ap1->preg = ESP;
        ap1->offset = Optimizer::simpleIntNode(Optimizer::se_i, q->dc.v.label + funcstackheight);  // ESP
    }
    else
    {
        ap1->preg = EBP;
        ap1->offset = Optimizer::simpleIntNode(Optimizer::se_i, q->dc.v.label);  // ESP
    }

    switch ((int)q->dc.left->offset->i)
    {
        case 0: /* try */
            gen_codes(op_push, ISZ_UINT, ap1, 0);
            gen_codes(op_mov, ISZ_UINT, ap1, makedreg(ESP));
            break;
        case 1: /* catch */
            gen_codes(op_mov, ISZ_UINT, makedreg(ESP), ap1);
            gen_codes(op_pop, ISZ_UINT, ap1, 0);
            break;
        case 2: /* end block */
            gen_codes(op_mov, ISZ_UINT, makedreg(ESP), ap1);
            gen_codes(op_pop, ISZ_UINT, ap1, 0);
            break;
        default:
            diag("asm_tryblock: Unknown tryblock type");
            break;
    }
}
void asm_stackalloc(
    Optimizer::QUAD* q) /* allocate stack space - positive value = allocate(Optimizer::QUAD *q) negative value deallocate */
{
    enum e_opcode op;
    AMODE *apl, *aph;
    getAmodes(q, &op, q->dc.left, &apl, &aph);
    if (apl->mode == am_immed)
    {
        int n = apl->offset->i;
        if (n)
        {
            if (n < 0)
                n = (n - 3) & ~3;
            else
                n = (n + 3) & ~3;

            if (n <= 4092)  // also gets adds to the stack pointer
                gen_code(op_add, makedreg(ESP), aimmed(-n));
            else
            {
                if (n <= 8188)
                {
                    gen_code(op_add, makedreg(ESP), aimmed(-4092));
                    gen_code(op_push, makedreg(EAX), 0);
                    gen_code(op_add, makedreg(ESP), aimmed(-(n - 4096)));
                }
                else
                {
                    if (n < 0)  // must match rtl
                        n = (n - 15) & ~15;
                    else
                        n = (n + 15) & ~15;
                    gen_code(op_push, aimmed(n), 0);
                    callLibrary("___substackp", 0);
                }
            }
            pushlevel += n;
            if (q->ans)
            {
                getAmodes(q, &op, q->ans, &apl, &aph);
                gen_codes(op_mov, ISZ_UINT, apl, makedreg(ESP));
            }
        }
    }
    else
    {
        gen_code(op_push, apl, 0);
        callLibrary("___substackp", 0);  // substack does paragraph rounding already...
        if (q->ans)
        {
            getAmodes(q, &op, q->ans, &apl, &aph);
            gen_codes(op_mov, ISZ_UINT, apl, makedreg(ESP));
        }
    }
}
void asm_loadstack(Optimizer::QUAD* q) /* load the stack pointer from a var */
{
    AMODE *apl, *aph;
    enum e_opcode op;

    getAmodes(q, &op, q->dc.left, &apl, &aph);
    apl->liveRegs = q->liveRegs;
    gen_codes(op_mov, ISZ_UINT, makedreg(ESP), apl);
}
void asm_savestack(Optimizer::QUAD* q) /* save the stack pointer to a var */
{
    AMODE *apl, *aph;
    enum e_opcode op;

    getAmodes(q, &op, q->dc.left, &apl, &aph);
    apl->liveRegs = q->liveRegs;
    gen_codes(op_mov, ISZ_UINT, apl, makedreg(ESP));
}
void asm_functail(Optimizer::QUAD* q, int begin, int size) /* functail start or end */
{
    enum e_opcode op = op_push;
    (void)q;
    if (!begin)
        op = op_pop;
    if (size == ISZ_ULONGLONG)
    {
        if (begin)
            gen_code(op, makedreg(EDX), 0);
        gen_code(op, makedreg(EAX), 0);
        if (!begin)
            gen_code(op, makedreg(EDX), 0);
    }
    else if (size < ISZ_FLOAT)
    {
        gen_code(op, makedreg(EAX), 0);
    }
}
void asm_atomic(Optimizer::QUAD* q)
{
    bool pushed;
    int needsync = q->dc.opcode != Optimizer::i_xchg ? q->dc.left->offset->i : 0;
    if (needsync < 0)
        needsync = 0;
    // direct store has bit 7 set...
    switch (q->dc.opcode)
    {
        bool pushed;
        int reg;
        enum e_opcode opa;
        enum e_opcode opl;
        enum e_opcode opr;
        AMODE *apal, *apah, *apll, *aplh, *aprl, *aprh;
        int lbl1, lbl2;
        case Optimizer::i_atomic_fence:
            if (needsync == Optimizer::mo_seq_cst + 0x80)  // in this case the value may be mo_seq_cst + 0x80 for store,
                                                           // for the x86 we are only genning these on stores:
            {
                gen_code(op_mfence, NULL, NULL);
            }
            break;
        case Optimizer::i_atomic_flag_fence:
            getAmodes(q, &opl, q->dc.right, &apll, &aplh);
            apll->mode = am_indisp;
            apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
            if (q->dc.left->offset->i > 0)
            {
                lbl1 = beGetLabel;
                lbl2 = beGetLabel;
                reg = -1;
                pushed = false;
                if (!(apll->liveRegs & (1 << EAX)))
                    reg = EAX;
                else if (!(apll->liveRegs & (1 << ECX)))
                    reg = ECX;
                else if (!(apll->liveRegs & (1 << EDX)))
                    reg = EDX;
                else
                {
                    pushed = true;
                    reg = (apll->preg & 3) ^ 1;
                    gen_code(op_push, makedreg(reg), 0);
                    pushlevel += 4;
                }
                oa_gen_label(lbl1);
                // I thought about just spinning here but it would have to spin for the rest of its time slice so might as well
                // yield...
                gen_codes(op_mov, ISZ_UCHAR, makedreg(reg), aimmed(1));
                gen_code(op_lock, NULL, NULL);
                gen_codes(op_xchg, ISZ_UCHAR, apll, makedreg(reg));
                gen_codes(op_or, ISZ_UCHAR, makedreg(reg), makedreg(reg));
                gen_code(op_jz, make_label(lbl2), 0);
                callLibrary("___atomic_yield", 0);
                gen_code(op_jmp, make_label(lbl1), 0);
                oa_gen_label(lbl2);
                if (pushed)
                {
                    gen_code(op_pop, makedreg(reg), 0);
                    pushlevel -= 4;
                }
            }
            else
            {
                gen_code(op_mov, apll, aimmed(0));
            }
            break;
        case Optimizer::i_atomic_flag_test_and_set:
            pushed = false;
            getAmodes(q, &opl, q->dc.right, &apll, &aplh);
            getAmodes(q, &opa, q->ans, &apal, &apah);
            if ((apll->mode == am_indisp && apll->preg == EAX) ||
                (apll->mode == am_indispscale && (apll->preg == EAX || apll->sreg == EAX)))
            {
                pushed = true;
                gen_code(op_push, makedreg(EBP), 0);
                pushlevel += 4;
                gen_codes(op_lea, ISZ_UINT, makedreg(EBP), apll);
                apll = makedreg(EBP);
                apll->mode = am_indisp;
                apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
            }
            gen_codes(op_mov, ISZ_UCHAR, apal, aimmed(1));
            gen_code(op_lock, NULL, NULL);
            gen_codes(op_xchg, ISZ_UCHAR, apal, apll);
            if (pushed)
            {
                gen_code(op_pop, makedreg(EBP), NULL);
                pushlevel -= 4;
            }
            break;
        case Optimizer::i_atomic_flag_clear:
            getAmodes(q, &opl, q->dc.right, &apll, &aplh);
            gen_codes(op_mov, ISZ_UCHAR, apll, aimmed(0));
            if (needsync == (Optimizer::mo_seq_cst | 0x80))
            {
                gen_code(op_mfence, NULL, NULL);
            }
            break;
        case Optimizer::i_xchg:
            if ((q->ans->size == ISZ_ULONGLONG || q->ans->size == -ISZ_ULONGLONG) &&
                (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG) &&
                (q->dc.right->size == ISZ_ULONGLONG || q->dc.right->size == -ISZ_ULONGLONG))
            {
                llongatomicmath(op_xchg, op_xchg, q);
            }
            else
            {
                bool pushbp = false;
                getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
                getAmodes(q, &opl, q->dc.left, &apll, &aplh);
                getAmodes(q, &opa, q->ans, &apal, &apah);
                if (apll->mode == am_indispscale || (apll->mode == am_indisp && apll->preg != ESP && apll->preg != EBP))
                {
                    // if (apll->liveRegs & (1 << EBP))
                    {
                        pushbp = true;
                        gen_code(op_push, makedreg(EBP), NULL);
                        pushlevel += 4;
                    }
                    gen_code(op_lea, makedreg(EBP), apll);
                    apll = makedreg(EBP);
                    apll->mode = am_indisp;
                    apll->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
                }

                if (aprl->mode == am_dreg)
                {
                    gen_codes(op_xchg, q->ans->size, apll, aprl);
                }
                else
                {
                    gen_codes(op_mov, q->ans->size, apal, aprl);
                    gen_codes(op_xchg, q->ans->size, apll, apal);
                }
                if (pushbp)
                {
                    gen_code(op_pop, makedreg(EBP), NULL);
                    pushlevel -= 4;
                }
            }
            break;
        case Optimizer::i_cmpswp:
            /*
                address == address2 ? address = value : address2 = address
                rv         right      rv        left    right      rv
                dest       accum      dest     src      accum      dest
                                        zf              nz
                                        true            false
                rm32       eax        rm32      reg     eax        rm32
            








              */
            {
                int sz = q->ans->size;
                if ((sz == ISZ_ULONGLONG || sz == -ISZ_ULONGLONG) &&
                    (q->dc.left->size == ISZ_ULONGLONG || q->dc.left->size == -ISZ_ULONGLONG) &&
                    (q->dc.right->size == ISZ_ULONGLONG || q->dc.right->size == -ISZ_ULONGLONG))
                {
                    bool pushed = false;
                    getAmodes(q, &opl, q->dc.right, &aprl, &aprh);
                    if ((aprl->mode == am_indisp && aprl->preg != ESP && aprl->preg != EBP) || aprl->mode == am_indispscale)
                    {
                        gen_code(op_push, makedreg(EBP), 0);
                        pushlevel += 4;
                        gen_code(op_lea, makedreg(EBP), aprl);
                        pushed = true;
                        aprl = makedreg(EBP);
                        aprl->mode = am_indisp;
                        aprl->offset = Optimizer::simpleIntNode(Optimizer::se_i, 0);
                        aprh = makedreg(EBP);
                        aprh->mode = am_indisp;
                        aprh->offset = Optimizer::simpleIntNode(Optimizer::se_i, 4);
                    }
                    llongatomicmath(op_xchg, op_cmpxchg8b, q);
                    int labno = beGetLabel;
                    gen_code(op_je, make_label(labno), 0);
                    gen_codes(op_mov, ISZ_UINT, aprl, makedreg(EAX));
                    gen_codes(op_mov, ISZ_UINT, aprh, makedreg(EDX));
                    oa_gen_label(labno);
                    if (pushed)
                    {
                        gen_code(op_pop, makedreg(EBP), 0);
                        pushlevel -= 4;
                    }
                }
                else
                {
                    getAmodes(q, &opr, q->dc.right, &aprl, &aprh);
                    getAmodes(q, &opl, q->dc.left, &apll, &aplh);
                    getAmodes(q, &opa, q->ans, &apal, &apah);
                    int lab;
                    int reg = -1, push = false, push1 = false, push2 = false;
                    if (apll->mode != am_dreg || apll->preg == EAX)
                    {
                        int reg1 = -1, reg2 = -1;
                        if (apal->mode == am_indisp)
                        {
                            reg1 = apal->preg;
                        }
                        else if (apal->mode == am_indispscale)
                        {
                            reg1 = apal->preg;
                            reg2 = apal->sreg;
                        }
                        if (!(apll->liveRegs & (1 << ECX)) && reg1 != ECX && reg2 != ECX)
                        {
                            reg = ECX;
                        }
                        else if (!(apll->liveRegs & (1 << EDX)) && reg1 != EDX && reg2 != EDX)
                        {
                            reg = EDX;
                        }
                        else if (!(apll->liveRegs & (1 << EBX)) && reg1 != EBX && reg2 != EBX)
                        {
                            reg = EBX;
                        }
                        else if (!(apll->liveRegs & (1 << ESI)) && reg1 != ESI && reg2 != ESI)
                        {
                            reg = ESI;
                        }
                        else if (!(apll->liveRegs & (1 << EDI)) && reg1 != EDI && reg2 != EDI)
                        {
                            reg = EDI;
                        }
                        if (reg == -1)
                        {
                            reg = EDX;
                            push1 = true;
                            gen_code(op_push, makedreg(EDX), NULL);
                            pushlevel += 4;
                        }
                        gen_codes(op_mov, sz, makedreg(reg), apll);
                    }
                    else
                    {
                        reg = apll->preg;
                    }
                    if ((aprl->mode == am_indisp && aprl->preg == EAX) ||
                        (aprl->mode == am_indispscale && (aprl->preg == EAX || aprl->sreg == EAX)))
                    {
                        push2 = true;
                        gen_code(op_push, makedreg(EAX), NULL);
                        pushlevel += 4;
                    }
                    gen_codes(op_mov, sz, makedreg(EAX), aprl);
                    gen_code(op_lock, NULL, NULL);
                    gen_codes(op_cmpxchg, sz, apal, makedreg(reg));
                    gen_code(op_je, make_label(lab = beGetLabel), NULL);
                    if (push2)
                    {
                        gen_codes(op_mov, sz, makedreg(EDX), makedreg(EAX));
                        oa_gen_label(lab);
                        gen_code(op_pop, makedreg(EAX), NULL);
                        pushlevel -= 4;
                        gen_codes(op_mov, sz, aprl, makedreg(EDX));
                    }
                    else
                    {
                        gen_codes(op_mov, sz, aprl, makedreg(EAX));
                        oa_gen_label(lab);
                    }
                    if (push)
                    {

                        gen_code(op_pop, apll, NULL);
                        pushlevel -= 4;
                    }
                    else if (push1)
                    {
                        gen_code(op_pop, makedreg(EDX), NULL);
                        pushlevel -= 4;
                    }
                }
                gen_code(op_mov, makedreg(EAX), aimmed(0));
                gen_codes(op_setz, ISZ_UCHAR, makedreg(EAX), NULL);
            }
            break;
        default:
            break;
    }
}
void asm_expressiontag(Optimizer::QUAD* q) {}
void asm_seh(Optimizer::QUAD* q) {}
void asm_tag(Optimizer::QUAD* q) {}
}  // namespace occx86