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

void * RegFileGroup::xmalloc(INT size)
{
    ASSERTN(m_is_init, ("must be initialized before clone."));
    ASSERTN(size > 0, ("xmalloc: size less zero!"));
    ASSERTN(m_pool,("need graph pool!!"));
    void * p = smpoolMalloc(size, m_pool);
    ASSERT0(p);
    ::memset((void*)p,0,size);
    return p;
}


void RegFileGroup::init()
{
    if (m_is_init) return;
    m_bb = nullptr;
    m_group2orlist_map.init();
    m_oridx2group_map.init();
    m_pool = smpoolCreate(64, MEM_COMM);
    m_is_init = true;
}


//Keep m_bb unchanged.
void RegFileGroup::destroy()
{
    if (!m_is_init) { return; }
    for (VecIdx i = 0; i <= m_group2orlist_map.get_last_idx(); i++) {
        List<OR*> * orlist = m_group2orlist_map.get(i);
        if (orlist == nullptr) {
            continue;
        }
        orlist->destroy();
    }
    m_group2orlist_map.destroy();
    m_oridx2group_map.destroy();
    smpoolDelete(m_pool);
    m_is_init = false;
}


INT RegFileGroup::getNumOfGroup() const
{
    ASSERTN(m_is_init, ("List not yet initialized."));
    return m_group2orlist_map.get_elem_count();
}


INT RegFileGroup::getORGroup(OR const* o) const
{
    ASSERTN(o, ("o is nullptr."));
    ASSERTN(m_is_init, ("List not yet initialized."));
    return m_oridx2group_map.get(o->id());
}


List<OR*> * RegFileGroup::getORListInGroup(INT i)
{
    ASSERTN(i >= 0, ("layer idx must be positive."));
    ASSERTN(m_is_init, ("List not yet initialized."));
    return m_group2orlist_map.get(i);
}


void RegFileGroup::clone(RegFileGroup & group)
{
    ASSERTN(m_is_init, ("List not yet initialized."));
    m_bb = group.m_bb;

    //clone group info
    for (INT i = 0; i < group.getNumOfGroup(); i++) {
        List<OR*> * orlist = group.getORListInGroup(i);
        if (orlist == nullptr) {
            m_group2orlist_map.set(i, (ORList*)nullptr);
            continue;
        }
        List<OR*> * neworlist = (List<OR*>*)
            xmalloc(sizeof(List<OR*>));
        neworlist->init();
        neworlist->copy(*orlist);
        m_group2orlist_map.set(i, neworlist);
    }

    //clone OR group mapping
    for (VecIdx i = 0; i <= group.m_oridx2group_map.get_last_idx(); i++) {
        m_oridx2group_map.set(i, group.m_oridx2group_map.get(i));
    }
}


void RegFileGroup::addOR(OR * o, INT layer)
{
    ASSERTN(m_is_init, ("List not yet initialized."));
    ASSERTN(o && layer >= 0 && layer < MAX_LAYER, ("Illegal info"));
    List<OR*> * orlist = m_group2orlist_map.get(layer);
    if (orlist == nullptr) {
        orlist = (List<OR*>*)xmalloc(sizeof(List<OR*>));
        orlist->init();
        m_group2orlist_map.set(layer, orlist);
    }
    if (orlist->find(o)) {
        //o has been grouped already
        return;
    }
    orlist->append_tail(o);
    m_oridx2group_map.set(o->id(), layer);
}


void RegFileGroup::dump()
{
    FILE * h = m_cg->getRegion()->getLogMgr()->getFileHandler();
    fprintf(h, "\nRegion:%s, ORBB:%d, RegFileGroup: ",
            m_cg->getRegion()->getRegionName(), m_bb->id());

    StrBuf buf(64);
    for (VecIdx i = 0; i <= m_group2orlist_map.get_last_idx(); i++) {
        fprintf(h, "\n\tGroup(%d):\n", i);
        List<OR*> * orlist = m_group2orlist_map.get(i);
        if (orlist != nullptr) {
            for (OR * o = orlist->get_head(); o; o = orlist->get_next()) {
                fprintf(h, "\t\t");
                ASSERT0(m_cg);
                buf.clean();
                fprintf(h, "%s", o->dump(buf, m_cg));
            }
        }
    }
    fprintf(h, "\n");
    fflush(h);
}


//Compute regfile group by traversing OR in BB.
void RegFileGroup::computeGroup()
{
    ASSERT0(m_bb);
    if (ORBB_ornum(m_bb) == 0) { return; }

    //Partitioning OR into different layers according to REGFILE they belong.
    //e.g:1. t1[A] =
    //       ...
    //    2.        = t1[A]
    //       ...
    //    3. t2[A] =
    //  Now t1,t2 assiged same regfile, if we assigned same
    //  register for them, that will be incur
    //  anti-dep between 2. and 3, and out-dep between 1. and 3.
    //  In order to reduce these redundant
    //  and harmful impact on sceduling dependence edges,
    //  we assign same layer for symbol registers with same regfile,
    //  and anticipate these symbol registers could be assigned
    //  with different physcial registers.
    //  Partitioning different results to different layer.
    ORCt * ct;
    for (OR * o = ORBB_orlist(m_bb)->get_head(&ct);
         o != nullptr; o = ORBB_orlist(m_bb)->get_next(&ct)) {
        //Determine layer by the last allocable regfile.
        //'o' has not layer if there are not any results.
        for (UINT i = 0; i < o->result_num(); i++) {
            SR * sr = o->get_result(i);
            ASSERT0(sr);
            if (sr->is_reg() &&
                sr->getRegFile() != RF_UNDEF &&
                !m_cg->isRflagRegister(o->getCode(), i, true) &&
                !m_cg->isDedicatedSR(sr)) {
                addOR(o, sr->getRegFile());
            }
        }
    }
}


void RegFileGroup::recomputeGroup(ORBB * bb)
{
    destroy();
    init();
    setBB(bb);
    computeGroup();
}


void RegFileGroup::setBB(ORBB * bb)
{
    if (bb != nullptr) {
        m_cg = ORBB_cg(bb);
    }
    m_bb = bb;
}

} //namespace xgen
