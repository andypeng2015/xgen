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
#include "xoccinc.h"

namespace xocc {

//
//START CLVar
//
CHAR const* CLVar::dumpVARDecl(
    OUT xcom::StrBuf & buf, VarMgr const* vm) const
{
    xocc::DeclAndVarMap const* dvmap = ((CLVarMgr*)vm)->getDVMap();
    if (dvmap == nullptr) { return nullptr; }
    xfe::Decl const* decl = dvmap->mapVar2Decl(const_cast<CLVar*>(this));
    if (decl != nullptr) {
        ASSERT0(DECL_dt(decl) == xfe::DCL_DECLARATION);
        xcom::DefFixedStrBuf tb;
        tb.bind(&buf);
        xfe::format_declaration(tb, decl, true);
        tb.unbind();
        return buf.getBuf();
    }
    return nullptr;
}
//END CLVar


//
//START CLVarMgr
//
Var * CLVarMgr::allocVAR()
{
    return new CLVar();
}
//END CLVarMgr

} //namespace xocc
