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
#include "xgeninc.h"

namespace xgen {

static bool canOpndBeLabel(OR const* o, UINT idx)
{
    ORCodeDesc const* otd = tmGetORCodeDesc(o->getCode());
    SRDescGroup<> const* sdg = OTD_srd_group(otd);
    ASSERT0(sdg);
    SRDesc const* sr_desc = sdg->get_opnd(idx);
    ASSERT0(sr_desc);
    return sr_desc->is_label();
}


static bool canOpndBeLabelList(OR const* o, UINT idx)
{
    ORCodeDesc const* otd = tmGetORCodeDesc(o->getCode());
    SRDescGroup<> const* sdg = OTD_srd_group(otd);
    ASSERT0(sdg);
    SRDesc const* sr_desc = sdg->get_opnd(idx);
    ASSERT0(sr_desc);
    return sr_desc->is_label_list();
}


//
//START ORList
//
void ORList::append_tail(OR * o)
{
    #ifdef _DEBUG_
    for (OR * t = get_head(); t != nullptr; t = get_next()) {
        ASSERTN(t != o, ("already in list."));
    }
    #endif
    List<OR*>::append_tail(o);
}


//Move elements in 'ors' to tail of current list.
void ORList::move_tail(MOD ORList & ors)
{
    #ifdef _DEBUG_
    for (OR * o = get_head(); o != nullptr; o = get_next()) {
        for (OR * oo = ors.get_head(); oo != nullptr; oo = ors.get_next()) {
            ASSERTN(o != oo, ("already in list."));
        }
    }
    #endif
    List<OR*>::move_tail(ors);
}


void ORList::dump(CG const* cg) const
{
    xcom::StrBuf buf(128);
    ORListIter it;
    for (OR const* o = get_head(&it); o != nullptr; o = get_next(&it)) {
        buf.clean();
        o->dump(buf, cg);
        note(cg->getRegion(), "\n%s", buf.buf);
    }
    note(cg->getRegion(), "\n");
}
//END ORList


//
//START OR
//
void OR::clean()
{
    //DO NOT MODIFY 'id'. It is the marker in OR vector in ORMgr.
    code = OR_UNDEF;
    clust = CLUST_UNDEF; //cluster
    container = nullptr;
    order = OR_ORDER_UNDEF;
    ubb = nullptr;
    OR_flag(this) = 0;
    DbxMgr * dbx_mgr = ORBB_cg(OR_bb(this))->getDbxMgr();
    ASSERT0(dbx_mgr);
    dbx.clean(dbx_mgr);
    m_opnd.clean();
    m_result.clean();
}


void OR::copyDbx(Dbx const* dbx, DbxMgr * dbx_mgr)
{
    if (dbx != nullptr) {
        OR_dbx(this).copy(*dbx, dbx_mgr);
    }
}


void OR::copyDbx(IR const* ir, DbxMgr * dbx_mgr)
{
    ASSERT0(ir);
    copyDbx(::getDbx(ir), dbx_mgr);
}


void OR::clone(OR const* o, CG * cg)
{
    //DO NOT MODIFY 'id'.
    m_opnd.copy(o->m_opnd, cg->getORMgr()->get_pool());
    m_result.copy(o->m_result, cg->getORMgr()->get_pool());
    DbxMgr * dbx_mgr = cg->getDbxMgr();
    ASSERT0(dbx_mgr);
    dbx.copy(o->dbx, dbx_mgr);
    code = o->code;
    order = OR_ORDER_UNDEF; //order in BB need to recompute.
    ubb = o->ubb;
    OR_flag(this) = OR_flag(o);
}


//Get imm operand of instruction.
SR * OR::get_imm_sr()
{
    //Normally, operand 1 of 'mov imm to reg OR' should be literal.
    SR * sr = get_opnd(HAS_PREDICATE_REGISTER + 0);
    ASSERTN(sr->is_constant(), ("operand of movi must be literal"));
    return sr;
}


bool OR::is_equal(OR const* o) const
{
    if (OR_code(this) == o->getCode() &&
        OR_bb(this) == o->getBB() &&
        OR_is_call(this) == OR_is_call(o) &&
        OR_is_cond_br(this) == OR_is_call(o) &&
        OR_is_predicated(this) == OR_is_predicated(o) &&
        OR_is_uncond_br(this) == OR_is_uncond_br(o) &&
        OR_is_return(this) == OR_is_return(o) &&
        OR_flag(this) == OR_flag(o)) {
        return true;
    }
    return false;
}


void OR::dump(CG const* cg) const
{
    xcom::StrBuf buf(128);
    dump(buf, cg);
    note(cg->getRegion(), "\n%s", buf.buf);
}


CHAR const* OR::dump(xcom::StrBuf & buf, CG const* cg) const
{
    if (cg->getRegion()->getDbxMgr() != nullptr && g_cg_dump_src_line) {
        DbxMgr::PrtCtx prtctx(LangInfo::LANG_CPP);
        cg->getRegion()->getDbxMgr()->printSrcLine(
            buf, &OR_dbx(this), &prtctx);
    }
    OR * pthis = const_cast<OR*>(this);
    if (cg->isDumpORId()) {
        //Order in BB.
        if (OR_bb(this) == nullptr) {
            buf.strcat("[????]");
        } else {
            buf.strcat("[O:%d]", OR_order(this));
        }

        //Unique id. You can disable it for diminish the output info.
        buf.strcat("[id:%d] ", OR_id(this));
    }

    buf.strcat(OR_code_name(this));
    buf.strcat(" ");

    if (OR_is_store(this)) {
        //[base + ofst] <- pred, store_val

        //memory addr
        buf.strcat("[");
        pthis->get_store_base()->get_name(buf, cg);

        //memory offset
        SR * ofst = pthis->get_store_ofst();
        if (cg->isComputeStackOffset() || ofst->is_int_imm()) {
            ASSERT0(ofst->is_int_imm());
            if (ofst->getInt() != 0) {
                if (ofst->getInt() > 0) {
                    buf.strcat(" + ");
                } else {
                    buf.strcat(" - ");
                }
                ofst->get_name(buf, cg);
            }
        } else {
            ASSERT0(ofst->is_var());
            buf.strcat(" + ");
            buf.strcat("'");
            buf.strcat(SYM_name(VAR_name(SR_var(ofst))));
            buf.strcat("'");
            if (SR_var_ofst(ofst) != 0) {
                buf.strcat(" + ");
                buf.strcat("%d", SR_var_ofst(ofst));
            }
        }

        buf.strcat("] <-- ");

        //predicate register if any.
        pthis->get_pred()->get_name(buf, cg);
        buf.strcat(", ");

        //The value to be stored.
        for (UINT i = 0; i < get_num_store_val(); i++) {
            ASSERTN(pthis->get_store_val(i), ("miss operand"));
            pthis->get_store_val(i)->get_name(buf, cg);
            if (i < get_num_store_val() - 1) {
                buf.strcat(", ");
            }
        }
    } else if (OR_is_load(this)) {
        //reg <- predicate_register, [base + ofst]
        //load value
        for (UINT i = 0; i < get_num_load_val(); i++) {
            ASSERTN(pthis->get_load_val(i) != nullptr, ("miss result"));
            pthis->get_load_val(i)->get_name(buf, cg);
            if (i < get_num_load_val() - 1) {
                buf.strcat(", ");
            }
        }

        buf.strcat(" <-- ");

        //predicate register
        if (HAS_PREDICATE_REGISTER) {
            pthis->get_pred()->get_name(buf, cg);
            buf.strcat(", ");
        }

        //memory address
        buf.strcat("[");
        pthis->get_load_base()->get_name(buf, cg);

        //memory offset
        SR * ofst = pthis->get_load_ofst();
        ASSERTN(ofst, ("miss offset"));
        if (cg->isComputeStackOffset() || ofst->is_int_imm()) {
            ASSERT0(ofst->is_int_imm());
            if (ofst->getInt() != 0) {
                if (ofst->getInt() > 0) {
                    buf.strcat(" + ");
                } else {
                    buf.strcat(" - ");
                }
                ofst->get_name(buf, cg);
            }
        } else {
            ASSERT0(ofst->is_var());
            buf.strcat(" + ");
            buf.strcat("'");
            buf.strcat(SYM_name(VAR_name(SR_var(ofst))));
            buf.strcat("'");
            if (SR_var_ofst(ofst) != 0) {
                buf.strcat(" + ");
                buf.strcat("%d", SR_var_ofst(ofst));
            }
        }

        buf.strcat("]");
    } else {
        for (UINT i = 0; i < result_num(); i++) {
            SR * res = get_result(i);
            res->get_name(buf, cg);
            if (i < result_num() - 1) {
                buf.strcat(", ");
            }
        }

        buf.strcat(" <-- ");

        for (UINT i = 0; i < opnd_num(); i++) {
            SR * opd = get_opnd(i);
            opd->get_name(buf, cg);
            if (i < opnd_num() - 1) {
                buf.strcat(", ");
            }
        }
    }

    //cluster
    if (OR_clust(this) != CLUST_UNDEF) {
        buf.strcat("  %s", tmGetClusterName(OR_clust(this)));
    }

    //assistant info
    if (OR_is_store(this)) {
        xoc::Var const* var = cg->mapOR2Var(pthis);
        if (var != nullptr) {
            buf.strcat("  //store to '");
            var->dump(buf, cg->getVarMgr());
            buf.strcat("'");
        }
    } else if (OR_is_load(this)) {
        xoc::Var const* var = cg->mapOR2Var(pthis);
        if (var != nullptr) {
            buf.strcat("  //load from '");
            var->dump(buf, cg->getVarMgr());
            buf.strcat("'");
        }
    }

    return buf.buf;
}


//Return the idx of opnd 'sr'.
INT OR::get_opnd_idx(SR * sr) const
{
    for (UINT i = 0; i <= m_opnd.getElemNum(); i++) {
        if (m_opnd.get(i) == sr) {
            return i;
        }
    }
    return OR_SR_IDX_UNDEF;
}


//Return the idx of opnd 'sr'.
INT OR::get_result_idx(SR * sr) const
{
    for (UINT i = 0; i <= m_result.getElemNum(); i++) {
        if (m_result.get(i) == sr) {
            return i;
        }
    }
    return OR_SR_IDX_UNDEF;
}


void OR::set_opnd(INT i, SR * sr, CG * cg)
{
    ASSERT0(sr && (UINT)i < opnd_num());
    m_opnd.set(i, sr, cg->getORMgr()->get_pool());
}


void OR::set_result(INT i, SR * sr, CG * cg)
{
    ASSERT0(sr && (UINT)i < result_num());
    m_result.set(i, sr, cg->getORMgr()->get_pool());
}


void OR::setLabel(SR * v, CG * cg)
{
    ASSERT0(v && v->is_label());
    ASSERT0_DUMMYUSE(canOpndBeLabel(this, HAS_PREDICATE_REGISTER + 0));
    set_opnd(HAS_PREDICATE_REGISTER + 0, v, cg);
}


void OR::setLabelList(SR * v, CG * cg)
{
    ASSERT0(v && v->is_label_list());
    ASSERT0_DUMMYUSE(canOpndBeLabelList(this, HAS_PREDICATE_REGISTER + 0));
    set_opnd(HAS_PREDICATE_REGISTER + 0, v, cg);
}
//END OR


//
//START ORMgr
//
void ORMgr::init(SRMgr * srmgr)
{
    if (m_pool != nullptr) { return; }
    ASSERT0(srmgr);
    m_sr_mgr = srmgr;
    m_pool = smpoolCreate(64, MEM_COMM);
}


void ORMgr::destroy()
{
    if (m_pool == nullptr) { return; }
    for (VecIdx i = ORID_UNDEF + 1; i <= get_last_idx(); i++) {
        OR * o = get(i);
        ASSERT0(o);
        delete o; //invoke virtual destructor of OR.
    }
    Vector<OR*>::clean();
    m_free_or_list.clean();
    smpoolDelete(m_pool);
    m_pool = nullptr;
}


OR * ORMgr::allocOR(CG * cg)
{
    ASSERT0(cg);
    return new OR(cg->getDbxMgr());
}


OR * ORMgr::getOR(UINT id)
{
    return get(id);
}


OR * ORMgr::genOR(OR_CODE ort, CG * cg)
{
    OR * o = m_free_or_list.remove_head();
    if (o == nullptr) {
        o = allocOR(cg);
        ASSERT0(ORID_UNDEF == 0);
        //Do not use ORID_UNDEF as index.
        UINT c = get_elem_count();
        if (c == 0) {
            OR_id(o) = ORID_UNDEF + 1;
        } else {
            OR_id(o) = c;
        }
        set(o->id(), o);
    }
    OR_code(o) = ort;
    o->get_opnd_vec()->grow(o->opnd_num(), get_pool());
    o->get_result_vec()->grow(o->result_num(), get_pool());
    if (HAS_PREDICATE_REGISTER) {
        ASSERT0(cg);
        o->set_pred(cg->getTruePred(), cg);
    }
    return o;
}


//Free SR to reuse it at next allocSR().
void ORMgr::freeSR(OR * o)
{
    ASSERT0(m_sr_mgr);
    for (UINT i = 0; i < o->result_num(); i++) {
        SR * sr = o->get_result(i);
        ASSERT0(sr != nullptr);
        if (!SR_is_dedicated(sr)) {
            m_sr_mgr->freeSR(sr);
        }
    }
    for (UINT i = 0; i < o->opnd_num(); i++) {
        SR * sr = o->get_opnd(i);
        ASSERT0(sr != nullptr);
        if (!SR_is_dedicated(sr)) {
            m_sr_mgr->freeSR(sr);
        }
    }
}


//Free OR to reuse it at next allocOR().
void ORMgr::freeOR(OR * o)
{
    ASSERT0(m_sr_mgr);
    freeSR(o);
    o->clean();
    m_free_or_list.append_head(o);
}
//END ORMgr

//
//START RecycORList
//
RecycORList::RecycORList(RecycORListMgr * mgr)
{
    init(mgr);
}


//Lower version (<=4.6) gcc may claim RecycORList is not a direct base of
//RecycORList. Thus we avoid invoking constructor at initializing list in
//other constructor.
//e.g:RecycORList::RecycORList(IR2OR * ir2or) :
//      RecycORList(ir2or->getRecycORListMgr())
RecycORList::RecycORList(IR2OR * ir2or)
{
    init(ir2or->getRecycORListMgr());
}


void RecycORList::init(RecycORListMgr * mgr)
{
    m_mgr = mgr;
    m_entity = mgr->getFree();
    if (m_entity == nullptr) {
        m_entity = new ORList();
    }
}


RecycORList::~RecycORList()
{
    m_mgr->addFree(m_entity);
}
//END RecycORList


//
//START RecycORListMgr
//
RecycORListMgr::~RecycORListMgr()
{
    C<ORList*> * it;
    for (m_free_list.get_head(&it); it != nullptr; m_free_list.get_next(&it)) {
        delete it->val();
    }
}
//END RecycORListMgr

} //namespace xgen
