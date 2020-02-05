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

/*
 * iout.c
 *
 * output routines for icode code gen->  Used only in optimizer tests.
 *
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "iexpr.h"
#include "be.h"
#include "PreProcessor.h"
#include "CmdSwitch.h"
/*      variable initialization         */
extern int tempCount;
extern BITINT bittab[BITINTBITS];
extern COMPILER_PARAMS cparams;
extern ARCH_ASM* chosenAssembler;
extern QUAD* intermed_head;
extern int cachedTempCount;
extern TEMP_INFO** tempInfo;
extern LIST *localfuncs, *localdata;
extern FILE* icdFile;
extern int nextLabel;
extern LIST* externals;
extern SimpleSymbol* currentFunction;
extern unsigned termCount;
extern QUAD* criticalThunks;
extern int cachedTempCount;
extern LIST* exports;
extern PreProcessor* preProcessor;
extern CmdSwitchString prm_output;

QUAD* currentQuad;

char dataname[40];   /* Name of last label */

static int newlabel;

void putamode(QUAD* q, IMODE* ap);
void nl(void);
void xstringseg(void);

/* Init module */
void outcodeini(void)
{
    newlabel = false;
}
static void iop_nop(QUAD* q)
{
}
static void iop_phi(QUAD* q)
{
}

static void iop_skipcompare(QUAD* q) { }

/*-------------------------------------------------------------------------*/

static void iop_asmgoto(QUAD* q) { }
/*-------------------------------------------------------------------------*/
static void iop_directbranch(QUAD* q)
{
    asm_goto(q);
}

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

static void iop_array(QUAD* q)
{
    asm_add(q);
}

/*-------------------------------------------------------------------------*/

static void iop_arrayindex(QUAD* q)
{
    asm_mul(q);
}
/*-------------------------------------------------------------------------*/

static void iop_arraylsh(QUAD* q)
{
    asm_mul(q);
}
static void iop_struct(QUAD* q)
{
    asm_add(q);
}
/*-------------------------------------------------------------------------*/
static void iop_initblk(QUAD* q)
{
    asm_clrblock(q);
}
static void iop_cpblk(QUAD* q)
{
    asm_assnblock(q);
}
/*-------------------------------------------------------------------------*/

static void iop_asmcond(QUAD* q) { }
/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/
static void iop_dbgblock(QUAD* q)
{
    asm_blockstart(q);
}

/*-------------------------------------------------------------------------*/

static void iop_dbgblockend(QUAD* q)
{
    asm_blockend(q);
}
static void iop_block(QUAD* q) { oprintf(icdFile, "\tBLOCK %d", q->dc.v.label + 1); }

/*-------------------------------------------------------------------------*/

static void iop_blockend(QUAD* q)
{
}

/*-------------------------------------------------------------------------*/

static void iop_livein(QUAD* q)
{
    (void)q;
    diag("op_livein: propogated live-in node");
}

/*-------------------------------------------------------------------------*/

static void iop_icon(QUAD* q)
{
    asm_assn(q);
}

/*-------------------------------------------------------------------------*/

static void iop_fcon(QUAD* q)
{
    asm_assn(q);
}
static void iop_imcon(QUAD* q)
{
    asm_assn(q);
}
static void iop_cxcon(QUAD* q)
{
    asm_assn(q);
}
static void iop_tryblock(QUAD* q)
{
    (void)q;
#ifdef XXXXX
    /* hardcoding offset in stack block */
    q->dc.v.label = -xceptoffs + 3 * getSize(bt_pointer);
    if (chosenAssembler->gen->asm_tryblock)
        chosenAssembler->gen->asm_tryblock(q);
    oprintf(icdFile, "\tTRYBLOCK", q->dc.v.label);
    switch (q->dc.left->offset->v.i)
    {
        case 0:
            oprintf(icdFile, "\tTry");
            break;
        case 1:
            oprintf(icdFile, "\tCatch");
            break;
        case 2:
            oprintf(icdFile, "\tEnd");
            break;
        default:
            oprintf(icdFile, "\t???");
            break;
    }
#endif
}
static void iop_substack(QUAD* q)
{
    asm_stackalloc(q);
}
static void iop_functailstart(QUAD* q)
{
    if (currentFunction->tp->btp->type != st_void)
    {
        int r = sizeFromISZ(currentFunction->tp->btp->sizeFromType);
        if (r < 0)
            r = -r;
        asm_functail(q, true, r);
    }
}
static void iop_functailend(QUAD* q)
{
    if (currentFunction->tp->btp->type != st_void)
    {
        int r = sizeFromISZ(currentFunction->tp->btp->sizeFromType);
        if (r < 0)
            r = -r;
        asm_functail(q, false, r);
    }
}
static void iop_gcsestub(QUAD* q) { }
static void iop_atomic_fence(QUAD* q)
{
     asm_atomic(q);
}
static void iop_atomic_flag_fence(QUAD* q)
{
    asm_atomic(q);
}
static void iop_atomic_flag_test_and_set(QUAD* q)
{
    asm_atomic(q);
}
static void iop_atomic_flag_clear(QUAD* q)
{
    asm_atomic(q);
}
static void iop_cmpswp(QUAD* q)
{
    asm_atomic(q);
}
static void iop_xchg(QUAD* q)
{
    asm_atomic(q);
}
/* List of opcodes
 * This list MUST be in the same order as the op_ enums
 */
static void (*oplst[])(QUAD* q) = {
    /* NOPROTO */
    iop_nop,
    iop_phi,
    asm_line,
    asm_passthrough,
    asm_datapassthrough,
    iop_skipcompare,
    asm_label,
    iop_asmgoto,
    asm_goto,
    iop_directbranch,
    asm_gosub,
    asm_fargosub,
    asm_trap,
    asm_int,
    asm_ret,
    asm_fret,
    asm_rett,
    asm_add,
    asm_sub,
    asm_udiv,
    asm_umod,
    asm_sdiv,
    asm_smod,
    asm_muluh,
    asm_mulsh,
    asm_mul,
    asm_lsl,
    asm_lsr,
    asm_asr,
    asm_neg,
    asm_not,
    asm_and,
    asm_or,
    asm_eor,
    asm_setne,
    asm_sete,
    asm_setc,
    asm_seta,
    asm_setnc,
    asm_setbe,
    asm_setl,
    asm_setg,
    asm_setle,
    asm_setge,
    iop_asmcond,
    asm_jne,
    asm_je,
    asm_jc,
    asm_ja,
    asm_jnc,
    asm_jbe,
    asm_jl,
    asm_jg,
    asm_jle,
    asm_jge,
    asm_assn,
    asm_genword,
    asm_coswitch,
    asm_swbranch,
    asm_assnblock,
    asm_clrblock,
    asm_cmpblock,
    asm_parmadj,
    asm_parmblock,
    asm_parm,
    iop_array,
    iop_arrayindex,
    iop_arraylsh,
    iop_struct,
    asm_cppini,
    iop_block,
    iop_blockend,
    iop_dbgblock,
    iop_dbgblockend,
    asm_varstart,
    asm_func,
    iop_livein,
    iop_icon,
    iop_fcon,
    iop_imcon,
    iop_cxcon,
    iop_atomic_flag_test_and_set,
    iop_atomic_flag_clear,
    iop_atomic_fence,
    iop_atomic_flag_fence,
    iop_cmpswp,
    iop_xchg,
    asm_prologue,
    asm_epilogue,
    asm_pushcontext,
    asm_popcontext,
    asm_loadcontext,
    asm_unloadcontext,
    asm_tryblock,
    iop_substack,
    iop_substack,
    asm_loadstack,
    asm_savestack,
    iop_functailstart,
    iop_functailend,
    iop_gcsestub,
    asm_expressiontag,
    asm_tag,
    asm_seh,
    iop_initblk,
    iop_cpblk};

void generate_instructions(QUAD* intermed_head)
{
    QUAD* q = intermed_head;
    while (q)
    {
        (*oplst[q->dc.opcode])(q);
        q = q->fwd;
    }
}
