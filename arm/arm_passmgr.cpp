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

Pass * ARMPassMgr::allocDUMgr()
{
    return new ARMDUMgr(m_rg);
}


Pass * ARMPassMgr::allocScalarOpt()
{
    return new ARMScalarOpt(m_rg);
}


Pass * ARMPassMgr::allocRefine()
{
    return new ARMRefine(m_rg);
}


Pass * ARMPassMgr::allocIRSimp()
{
    return (IRSimp*)new ARMIRSimp(m_rg);
}


Pass * ARMPassMgr::allocLinearScanRA()
{
    #if defined REF_TARGMACH_INFO || defined FOR_IP
    return new ARMLinearScanRA(m_rg);
    #else
    ASSERTN(0, ("Target Dependent Code"));
    return nullptr;
    #endif
}


Pass * ARMPassMgr::allocCalcDerivative()
{
    #if defined(FOR_IP)
    return new ARMDerivative(m_rg);
    #else
    ASSERTN(0, ("Target Dependent Code"));
    return nullptr;
    #endif
}


Pass * ARMPassMgr::allocIRMgr()
{
    #if defined REF_TARGMACH_INFO || defined FOR_IP
    return new ARMIRMgr(m_rg);
    #else
    ASSERTN(0, ("Target Dependent Code"));
    return nullptr;
    #endif
}


Pass * ARMPassMgr::allocExtPass(PASS_TYPE passty)
{
    Pass * pass = nullptr;
    switch (passty) {
    case PASS_CALC_DERIVATIVE:
        pass = allocCalcDerivative();
        break;
    default: ASSERTN(0, ("unknown pass type"));
    }
    return pass;
}


Pass * ARMPassMgr::allocPrologueEpilogue()
{
    ASSERTN(0, ("Target Dependent Code"));
    return nullptr;
}
