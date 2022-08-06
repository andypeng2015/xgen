/*@
Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/

#include "../xgen/xgeninc.h"
#include "precompile/arm_targ_mach_resource.cpp" //generated by precompilation.

CHAR const* xgen::tmGetRegName(REG reg)
{
    ASSERT0(reg <= REG_LAST);
    return g_register_name[reg];
}


//Return name of function-unit.
CHAR const* xgen::tmGetUnitName(UNIT u)
{
    ASSERT0(u < UNIT_NUM);
    return g_unit_name[u];
}


//Return name of cluster.
CHAR const* xgen::tmGetClusterName(CLUST clust)
{
    ASSERT0(clust < CLUST_NUM);
    return g_clust_name[clust];
}


//Return name of given register-file.
CHAR const* xgen::tmGetRegFileName(REGFILE rf)
{
    ASSERT0(rf < RF_NUM);
    return g_regfile_prop[rf].name;
}


bool xgen::tmIsIntRegFile(REGFILE rf)
{
    ASSERT0(rf < RF_NUM);
    return g_regfile_prop[rf].is_integer;
}


bool xgen::tmIsFloatRegFile(REGFILE rf)
{
    ASSERT0(rf < RF_NUM);
    return g_regfile_prop[rf].is_float;
}


bool xgen::tmIsPredicateRegFile(REGFILE rf)
{
    ASSERT0(rf < RF_NUM);
    return g_regfile_prop[rf].is_predicate;
}


//Return register-file for given register.
REGFILE xgen::tmMapReg2RegFile(REG reg)
{
    return g_reg2regfile[reg];
}


//Return a set of registers for given register-file.
RegSet const* xgen::tmMapRegFile2RegSet(REGFILE rf)
{
    return g_regfile2regset[rf];
}


//Return alloable register set for given register-file.
RegSet const* xgen::tmMapRegFile2RegSetAllocable(REGFILE rf)
{
    ASSERT0(rf < RF_NUM);
    return g_regfile2regset_allocable[rf];
}


//Return alloable register set for given cluster.
RegSet const* xgen::tmMapCluster2RegSetAlloable(CLUST clust)
{
    ASSERT0(clust < CLUST_NUM);
    return g_cluster2regset_allocable[clust];
}


//Return name of given slot.
CHAR const* xgen::tmGetSlotName(SLOT s)
{
    ASSERT0(s < SLOT_NUM);
    return g_slot_name[s];
}


//Return name of OR code.
CHAR const* xgen::tmGetORCodeName(OR_CODE ort)
{
    return OTD_name(tmGetORCodeDesc(ort));
}


//Return description for given or-type.
ORCodeDesc const* xgen::tmGetORCodeDesc(OR_CODE ot)
{
    ORCodeDesc const* od = &g_or_code_desc[ot];
    ASSERTN(od, ("miss ORCodeDesc of '%s'", tmGetORCodeName(ot)));
    return od;
}


//Return a set of registers correspond to given cluster.
RegSet const* xgen::tmMapCluster2RegSet(CLUST cl)
{
    return g_cluster2regset[cl];
}


//Return the number of source operands for given or-type.
UINT xgen::tmGetOpndNum(OR_CODE orcode)
{
    ORCodeDesc const* otd = tmGetORCodeDesc(orcode);
    SRDescGroup<> const* sdg  = OTD_srd_group(otd);
    ASSERTN(sdg, ("miss SRD group info of '%s'", tmGetORCodeName(orcode)));
    return sdg->get_opnd_num();
}


//Return the number of target operands for given or-type.
UINT xgen::tmGetResultNum(OR_CODE orcode)
{
    ORCodeDesc const* otd = tmGetORCodeDesc(orcode);
    SRDescGroup<> const* sdg  = OTD_srd_group(otd);
    ASSERTN(sdg, ("miss SRD group info of '%s'", tmGetORCodeName(orcode)));
    return sdg->get_res_num();
}


//Return description of or-type.
EquORCodes const* xgen::tmGetEqualORCode(OR_CODE ot)
{
    ORCodeDesc const* od = tmGetORCodeDesc(ot);
    ASSERTN(od, ("miss ORCodeDesc info of '%s'", tmGetORCodeName(ot)));
    return OTD_equ_or_codes(od);
}


//Get the number of equivalent or-types that has same functions.
UINT xgen::tmGetNumOfEqualORCode(OR_CODE ot)
{
    ORCodeDesc const* od = tmGetORCodeDesc(ot);
    ASSERTN(od, ("miss ORCodeDesc info of '%s'", tmGetORCodeName(ot)));
    return EQUORC_num_equorcode(OTD_equ_or_codes(od));
}


RegSet const* xgen::tmGetRegSetAllocable()
{
    return (RegSet const*)&g_allocable_regset;
}


RegSet const* xgen::tmGetRegSetOfArgument()
{
    return (RegSet const*)&g_argument_regset;
}


RegSet const* xgen::tmGetRegSetOfReturnValue()
{
    return (RegSet const*)&g_return_value_regset;
}


RegSet const* xgen::tmGetRegSetOfCallerSaved()
{
    return (RegSet const*)&g_caller_saved_regset;
}


RegSet const* xgen::tmGetRegSetOfCalleeSaved()
{
    return (RegSet const*)&g_callee_saved_regset;
}


//Get the Read and Write available cycle.
ORScheInfo const* xgen::tmGetORScheInfo(OR_CODE ot)
{
    ASSERT0(ot < OR_NUM);
    return &OTD_sche_info(&g_or_code_desc[ot]);
}


CGMgr * xgen::allocCGMgr(RegionMgr * rm)
{
    return new ARMCGMgr(rm);
}
