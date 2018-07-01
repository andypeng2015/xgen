/*@
Copyright (c) 2013-2014, Su Zhenyu steven.known@gmail.com

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
#ifndef _CG_H_
#define _CG_H_

namespace xgen {

class IR2OR;

//Map Symbol Register Id to specifical SR.
typedef xcom::HMap<UINT, SR*> SRegid2SR;

class PRNO2SR : public xcom::HMap<UINT, SR*, xcom::HashFuncBase2<UINT> > {
public:
    PRNO2SR() : xcom::HMap<UINT, SR*, xcom::HashFuncBase2<UINT> >(0) {}
};


class BBVarList : public List<xoc::VAR*> {
    List<xoc::VAR*> m_free_list;
public:
    xoc::VAR * get_free() { return m_free_list.remove_tail(); }

    void free()
    {
        UINT i = 0;
        for (xoc::VAR * v = get_head(); i < get_elem_count();
             i++, v = get_next()) {
            m_free_list.append_tail(v);
        }
        clean();
    }
};


class ArgDesc {
public:
    Vector<SR*> param_vec;
    SR * param_start_addr;
    xoc::Dbx const* param_dbx;
    UINT is_record_addr:1;
    UINT is_record_regvec : 1;
    UINT param_byte_size:30; //stack byte size to be passed.
    UINT param_byte_ofst; //stack byte offset to the base SR.

public:
    ArgDesc() { init(); }

    void init()
    {
        param_start_addr = NULL;
        is_record_addr = false;
        is_record_regvec = false;
        param_dbx = NULL;
        param_byte_size = 0;
        param_byte_ofst = 0;
    }
};


#define ARGDESCMGR_passed_arg_byte_size(s) ((s)->m_passed_arg_byte_size)
#define ARGDESCMGR_total_byte_size(s) ((s)->m_total_byte_size)
class ArgDescMgr {
protected:
    Vector<ArgDesc*> m_arg_desc;
    List<ArgDesc*> m_arg_list;

public:
    //Record the byte size of argument that have been passed.
    UINT m_passed_arg_byte_size;

    //Record the total byte size of argument that to be passed.
    UINT m_total_byte_size;

public:
    ArgDescMgr() 
    {
        m_passed_arg_byte_size = 0;
        m_total_byte_size = 0;
    }
    ~ArgDescMgr()
    {
        for (INT i = 0; i <= m_arg_desc.get_last_idx(); i++) {
            ArgDesc * desc = m_arg_desc.get(i);
            delete desc;
        }
        m_arg_desc.clean();
    }

    //Add arg-desc to the tail of argument-list.
    ArgDesc * addDesc()
    {
        ArgDesc * desc = new ArgDesc();
        m_arg_desc.set(m_arg_desc.get_last_idx() < 0 ?
            0 : m_arg_desc.get_last_idx() + 1, desc);
        m_arg_list.append_tail(desc);
        return desc;
    }

    ArgDesc * pulloutDesc() { return m_arg_list.remove_head(); }
    ArgDesc * getCurrentDesc() { return m_arg_list.get_head(); }

    void incPassedArgByteSize(UINT bytesize)
    { m_passed_arg_byte_size += (INT)ceil_align(bytesize, STACK_ALIGNMENT); }
};


//The module will do the follows works:
// Instruction selection,
// Calling conventions lowering,
// Control flow (dominators, loop nesting) analyzes,
// Data flow (liveness, reaching definitions) analyzes,
// Register allocation,
// Stack frame building,
// Peephole optimizations,
// Branch optimizations,
// Loop unrolling,
// Basic block replication,
// Extended block optimizations,
// Instruction scheduling,
// Matching code idioms such as
// DSP arithmetic by target processor instructions,
// If-conversion based on conditional MOVEs, SELECTs,
// or fully predicated instructions.
//
// Taking advantage of specialized addressing modes and of
// hardware looping capabilities.
// Rewriting loops in order to exploit SIMD instructions
// management of register tuples and complex software pipelining
// in case of clustered architectures tricks to reduce code size
// or enhance code compressibility.
#define CG_or2memaddr_map(r)                ((r)->m_or2memaddr_map)
#define CG_max_real_param_size(r)           ((r)->m_max_real_param_size)
#define CG_bb_level_internal_var_list(r)    ((r)->m_bb_level_internal_var_list)
#define CG_func_level_internal_var_list(r)  ((r)->m_func_level_internal_var_list)
#define CG_builtin_memcpy(r)                ((r)->m_builtin_memcpy)
class CG {
protected:
    xoc::Region * m_ru;
    ORMgr * m_or_mgr;
    xoc::TypeMgr * m_tm;
    UnitSet m_tmp_us; //Used for temporary purpose.
    CGMgr * m_cgmgr;
    //Mapping from STORE/LOAD operation to the target address.
    List<ORBB*> m_or_bb_list; //descripting all basic blocks of the region.
    OR_CFG * m_or_cfg; //CFG of region
    List<DataDepGraph*> m_ddg_list;
    List<BBSimulator*> m_simm_list;
    List<LIS*> m_lis_list;
    Vector<ParallelPartMgr*> * m_ppm_vec; //Record parallel part for CG.
    Vector<xoc::VAR const*> m_params; //record the formal parameter.
    ORBBMgr * m_or_bb_mgr; //manage BB of IR.
    UINT m_or_bb_idx; //take count of ORBB.
    RegSetMgr m_regset_mgr;
    SRegid2SR m_regid2sr_map; //Map Symbol Register Id to specifical SR.
    PRNO2SR m_pr2sr_map;
    UINT m_sect_count;
    UINT m_reg_count;
    SMemPool * m_pool;
    SR * m_param_pointer;
    Reg2SR m_dedicate_sr_tab;
    Section m_code_sect;
    Section m_data_sect;
    Section m_rodata_sect;
    Section m_bss_sect;
    StackSection m_stack_sect;
    Section m_param_sect;
    UINT m_mmd_count;
    Vector<IssuePackageList*> m_ipl_vec; //record IssuePackageList for each ORBB.

    //True if accessing local variable via [FP pointer - Offst].
    bool m_is_use_fp;

    //True if compute the section offset for global/local variable.
    bool m_is_compute_sect_offset;
protected:
    void addSerialDependence(ORBB * bb, DataDepGraph * ddg);

    UINT compute_pad();
    inline SR * computeAndUpdateImmOfst(SR * sr, HOST_UINT frame_size);

    void * xmalloc(INT size)
    {
        ASSERT(size > 0, ("xmalloc: size less zero!"));
        //return MEM_POOL_Alloc(&m_mempool, size);
        ASSERT(m_pool != NULL, ("need graph pool!!"));
        void * p = smpoolMalloc(size, m_pool);
        if (p == NULL) return NULL;
        ::memset(p, 0, size);
        return p;
    }

    void initSections(xoc::VarMgr * vm);

    virtual void finiFuncUnit();

    void localizeBB(SR * sr, ORBB * bb);
    void localizeBBTab(SR * sr, xcom::TTab<ORBB*> * orbbtab);
public:
    UINT m_max_real_param_size;

    //Mapping from STORE/LOAD operation to the target address.
    xcom::TMap<OR*, xoc::VAR const*> m_or2memaddr_map;

    BBVarList m_bb_level_internal_var_list; //record local pr at OR.
    List<xoc::VAR*> m_func_level_internal_var_list; //record global pr at OR.

    //Builtin function should be initialized in initBuiltin().
    xoc::VAR const* m_builtin_memcpy;
public:
    CG(xoc::Region * rg, CGMgr * cgmgr);
    COPY_CONSTRUCTOR(CG);
    virtual ~CG();

    void addBBLevelNewVar(IN xoc::VAR * var);
    void addFuncLevelNewVar(IN xoc::VAR * var);
    void appendReload(ORBB * bb, ORList & ors);
    inline xoc::VAR * addBuiltinVar(CHAR const* buildin_name)
    {
        ASSERT0(m_ru);
        xoc::SYM * s = m_ru->getRegionMgr()->addToSymbolTab(buildin_name);
        return m_ru->getVarMgr()->registerStringVar(
            buildin_name, s, MEMORY_ALIGNMENT);
    }
    ORBB * allocBB();
    RegSet * allocRegSet() { return m_regset_mgr.allocRegSet(); }
    virtual BBSimulator * allocBBSimulator(ORBB * bb, bool is_log = true);
    virtual LIS * allocLIS(
            ORBB * bb,
            DataDepGraph * ddg,
            BBSimulator * sim,
            UINT sch_mode,
            bool change_slot,
            bool change_cluster,
            bool is_log = true);
    virtual DataDepGraph * allocDDG(bool is_log = true);
    void assembleSRVec(SRVec * srvec, SR * sr1, SR * sr2);
    virtual IR2OR * allocIR2OR() = 0;
    virtual OR_CFG * allocORCFG();
    virtual IssuePackage * allocIssuePackage();
    virtual RaMgr * allocRaMgr(List<ORBB*> * bblist, bool is_func);

    //OR Builder
    //Build pseduo instruction that indicate LabelInfo.
    //Note OR_label must be supplied by Target.
    void buildLabel(xoc::LabelInfo const* li, OUT ORList & ors, IN IOC * cont);

    //Build nop instruction.
    virtual OR * buildNop(UNIT unit, CLUST clust) = 0;
    virtual OR * buildOR(OR_TYPE orty, UINT resnum, UINT opndnum, ...);
    virtual OR * buildOR(OR_TYPE orty, IN SRList & result, IN SRList & opnd);
    virtual void buildSpadjust(OUT ORList & ors, IN IOC * cont);
    virtual void buildSpill(
            IN SR * store_val,
            IN xoc::VAR * spill_loc,
            IN ORBB * bb,
            OUT ORList & ors);
    virtual void buildReload(
            IN SR * result_val,
            IN xoc::VAR * reload_loc,
            IN ORBB * bb,
            OUT ORList & ors);

    //'sr_size': The number of integral multiple of byte-size of single SR.
    virtual void buildMul(
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList &,
            IN IOC *)
    {
        DUMMYUSE(is_sign);
        DUMMYUSE(src1);
        DUMMYUSE(src2);
        DUMMYUSE(sr_size);
        ASSERT(0, ("Target Dependent Code"));
    }

    //'sr_size': The number of integral multiple of byte-size of single SR.
    virtual void buildDiv(
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList &,
            IN IOC *)
    {
        DUMMYUSE(is_sign);
        DUMMYUSE(src1);
        DUMMYUSE(src2);
        DUMMYUSE(sr_size);
        ASSERT(0, ("Target Dependent Code"));
    }

    //'sr_size': The number of integral multiple of byte-size of single SR.
    virtual void buildAdd(
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList & ors,
            IN IOC * cont);

    //'sr_size': The number of integral multiple of byte-size of single SR.
    virtual void buildSub(
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList & ors,
            IN IOC * cont);
    virtual void buildAddRegImm(
            SR * src,
            SR * imm,
            UINT sr_size,
            bool is_sign,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildAddRegReg(
            bool is_add,
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildMod(
            CLUST clust,
            SR ** result,
            SR * src1,
            SR * src2,
            UINT sr_size,
            bool is_sign,
            OUT ORList & ors,
            IN IOC * cont);

    //Build copy-operation with given predicate register.
    virtual void buildCopyPred(
            CLUST clust,
            UNIT unit,
            IN SR * to, //should not add 'const' qualifier because RA will change the value.
            IN SR * from,
            IN SR * pd,
            OUT ORList & ors) = 0;

    //Build function call instruction.
    virtual void buildCall(
            xoc::VAR const* callee,
            UINT ret_val_size,
            OUT ORList & ors,
            IOC * cont) = 0;

    //Build indirect function call instruction.
    //Function-Call may violate SP,FP,GP,
    //RFLAG register, return-value register,
    //return address register.
    virtual void buildIcall(
            SR * callee,
            UINT ret_val_size,
            OUT ORList & ors,
            IOC * cont) = 0;

    //Build load-address instruction.
    virtual void buildStore(
            SR * store_val,
            xoc::VAR const* base,
            HOST_INT ofst,
            OUT ORList & ors,
            IN IOC * cont);
    virtual void buildLoad(
            SR * load_val,
            xoc::VAR const* base,
            HOST_INT ofst,
            OUT ORList & ors,
            IN IOC * cont)
    {
        ASSERT0(SR_is_reg(load_val));
        SR * mem_base = genVAR(base);
        buildLoad(load_val, mem_base, genIntImm(ofst, true), ors, cont);
    }
    virtual void buildGeneralLoad(
            IN SR * val,
            HOST_INT ofst,
            OUT ORList & ors,
            IN IOC * cont);
    virtual void buildTypeCvt(
            IR const* tgt,
            IR const* src,
            OUT ORList & ors,
            IN OUT IOC * cont);

    //Implement memory block copy.
    //Note tgt and src should not overlap.
    virtual void buildMemcpyInternal(
            SR * tgt,
            SR * src,
            UINT bytesize,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    //Generate ::memcpy.
    void buildMemcpy(
            SR * tgt,
            SR * src,
            UINT bytesize,
            OUT ORList & ors,
            IN IOC * cont);

    //Generate operations: reg = &var
    virtual void buildLda(
            xoc::VAR const* var,
            HOST_INT lda_ofst,
            Dbx const* dbx,
            OUT ORList & ors,
            IN IOC * cont);

    //Generate opcode of accumulating operation.
    //Form as: A = A op B
    virtual void buildAccumulate(
            OR * red_or,
            SR * red_var,
            SR * restore_val,
            List<OR*> & ors);
    virtual void buildLoad(
            IN SR * load_val,
            IN SR * base,
            IN SR * ofst,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildStore(
            SR * store_val,
            SR * base,
            SR * ofst,
            OUT ORList & ors,
            IN IOC * cont) = 0;

    //Build a general copy operation from register to register.
    //to: source register
    //from: target register
    //unit: function unit.
    //clust: cluster.
    virtual void buildCopy(
            CLUST clust,
            UNIT unit,
            SR * to,
            SR * from,
            OUT ORList & ors) = 0;

    //Generate copy operation from source to register.
    //Source can be immediate or register, and target must be register.
    //Note there is no difference between signed and unsigned number moving.
    virtual void buildMove(
            IN SR * to,
            IN SR * from,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildUncondBr(
            IN SR * tgt_lab,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildCondBr(
            IN SR * tgt_lab,
            OUT ORList & ors,
            IN IOC * cont) = 0;
    virtual void buildCompare(
            OR_TYPE br_cond,
            bool is_truebr,
            IN SR * opnd0,
            IN SR * opnd1,
            OUT ORList & ors,
            IN IOC * cont) = 0;

    //Generate inter-cluster copy operation.
    //Copy from 'src' in 'src_clust' to 'tgt' of in 'tgt_clust'.
    virtual OR * buildBusCopy(
            IN SR * src,
            IN SR * tgt,
            IN SR * pd,
            CLUST src_clust,
            CLUST tgt_clust) = 0;
    virtual void buildShiftLeft(
            IN SR * src,
            ULONG sr_size,
            IN SR * shift_ofst,
            OUT ORList & ors,
            IN OUT IOC * cont) = 0;
    virtual void buildShiftRight(
            IN SR * src,
            ULONG sr_size,
            IN SR * shift_ofst,
            bool is_signed,
            OUT ORList & ors,
            IN OUT IOC * cont) = 0;

    void constructORBBList(IN ORList  & or_list);
    void computeEntryAndExit(
            IN OR_CFG & cfg,
            OUT List<ORBB*> & entry_lst,
            OUT List<ORBB*> & exit_lst);
    INT computeOpndIdx(OR * o, SR const* opnd);
    INT computeResultIdx(OR * o, SR const* res);
    void computeMaxRealParamSpace();
    void computeStackVarImmOffset();
    virtual void computeAndUpdateGlobalVarLayout(
            xoc::VAR const* var,
            OUT SR ** base,
            OUT SR ** base_ofst);
    virtual void computeAndUpdateStackVarLayout(
            xoc::VAR const* var,
            OUT SR ** sp, //stack pointer
            OUT SR ** sp_ofst);
    virtual void computeParamterLayout(
            xoc::VAR const* id,
            OUT SR ** base,
            OUT SR ** ofst);
    virtual UINT computeTotalParameterStackSize(IR * ir);
    virtual void computeVarBaseOffset(
            xoc::VAR const* var,
            ULONGLONG var_ofst,
            OUT SR ** base,
            OUT SR ** ofst);
    virtual CLUST computeAsmCluster(OR * o);

    //Return the index of copied source operand.
    virtual INT computeCopyOpndIdx(OR *)
    { ASSERT(0, ("Target Dependent Code")); return -1; }

    //Compute the byte size of memory which will be loaded/stored.
    virtual INT computeMemByteSize(OR * o)
    {
        CHECK_DUMMYUSE(o);
        ASSERT(OR_is_mem(o), ("Need memory operation"));
        ASSERT(0, ("Target Dependent Code"));
        return -1;
    }

    //Return the alternative equivalent o-type of 'from'
    //that correspond with 'to_unit' and 'to_clust'.
    virtual OR_TYPE computeEquivalentORType(
            OR_TYPE from,
            UNIT to_unit,
            CLUST to_clust)
    {
        DUMMYUSE(to_clust);
        ASSERT(tmGetEqualORType(from), ("miss EquORTypes information"));
        return EQUORTY_unit2ortype(tmGetEqualORType(from), to_unit);
    }

    //Return the alternative equivalent o-type of 'o'
    //that correspond with 'to_unit' and 'to_clust'.
    virtual UNIT computeEquivalentORUnit(IN OR * from, CLUST to_clust)
    {
        DUMMYUSE(from);
        DUMMYUSE(to_clust);
        ASSERT(0, ("Target Dependent Code"));
        return UNIT_UNDEF;
    }

    virtual CLUST computeClusterOfBusOR(OR * o);
    virtual CLUST computeORCluster(OR const* o) const;
    virtual SLOT computeORSlot(OR const* o);
    xoc::VAR const* computeSpillVar(OR * o);

    //Change the function unit and related cluster of 'o'.
    //If is_test is true, this function only check whether the given
    //OR can be changed.
    virtual bool changeORUnit(
            OR * o,
            UNIT to_unit,
            CLUST to_clust,
            Vector<bool> const& regfile_unique,
            bool is_test /*only test purpose*/);

    //Return the combination of all of available function unit of 'o'.
    //This function will modify m_tmp_us internally.
    virtual UnitSet const* computeORUnit(OR const*, OUT UnitSet*);
    virtual UnitSet const* computeORUnit(OR const* o)
    { return computeORUnit(o, NULL); }

    //Change the correlated cluster of 'o'
    //If is_test is true, this function only check whether the given
    //OR can be changed.
    virtual bool changeORCluster(
            OR * o,
            CLUST to_clust,
            Vector<bool> const& regfile_unique,
            bool is_test /*only test purpose*/);

    //Change 'o' to 'ot', modifing all operands and results.
    virtual bool changeORType(
            OR * o,
            OR_TYPE ot,
            CLUST src,
            CLUST tgt,
            Vector<bool> const& regfile_unique);

    virtual SR * dupSR(SR const* sr);
    virtual OR * dupOR(OR const* o);
    virtual void dumpSection();
    void dumpPackage();

    //Expand pseudo SpAdjust operation to real target AddInteger instruction.
    //Note this function is target dependent.
    virtual void expandSpadjust(OR * o, OUT IssuePackageList * ipl);

    //Expand pseudo operation to real target AddInteger instruction.
    //Note this function is target dependent.    
    virtual void expandFakeOR(OR * o, OUT IssuePackageList * ipl);

    //Format label name string in 'buf'.
    CHAR * formatLabelName(xoc::LabelInfo const* lab, OUT xcom::StrBuf & buf)
    {
        CHAR const* prefix = NULL;
        prefix = SYM_name(m_ru->getRegionVar()->get_name());
        buf.strcat("%s_", prefix);
        if (LABEL_INFO_type(lab) == L_ILABEL) {
            buf.strcat(ILABEL_STR_FORMAT, ILABEL_CONT(lab));
        } else if (LABEL_INFO_type(lab) == L_CLABEL) {
            buf.strcat(CLABEL_STR_FORMAT, CLABEL_CONT(lab));
        } else {
            ASSERT(0, ("unknown label type"));
        }
        return buf.buf;
    }
    void freeDdgList();
    void freeSimmList();
    void freeLisList();
    void freeORBBList();
    virtual void freePackage();
    virtual void fixCluster(ORList & spill_ops, CLUST clust);

    Vector<ParallelPartMgr*> * getParallelPartMgrVec() const
    { return m_ppm_vec; }

    SR * genVAR(xoc::VAR const* var);
    SR * genLabel(LabelInfo const* li);
    SR * genSR();
    SR * genSR(REG reg, REGFILE regfile);

    //Generate a SR if bytes_size not more than GENERAL_REGISTER_SIZE,
    //otherwise generate a vector or SR.
    //Return the first register if vector generated.
    SR * genReg(UINT bytes_size = GENERAL_REGISTER_SIZE);

    //Generate SR by specified Symbol Register Id.
    SR * genReg(SymRegId regid);

    //Generate SR that indicates const value.
    SR * genIntImm(HOST_INT val, bool is_signed);
    SR * genFpImm(HOST_FP val);

    //Generate dedicated register by specified physical register.
    SR * genDedicatedReg(REG phy_reg);
    virtual SR * genSP();
    virtual SR * genFP();
    virtual SR * genGP();
    virtual SR * genParamPointer();
    virtual SR * genRflag(); //Generate flag register.
    virtual SR * genPredReg(); //Generate predicate register.
    virtual SR * genTruePred(); //Generate TRUE predicate register.

    //Generate function return-address register.
    virtual SR * genReturnAddr() = 0;
    xoc::Region * getRegion() const { return m_ru; }

    //Generate spill location that same like 'sr'.
    //Or return the spill location if exist.
    //'sr': the referrence SR.
    xoc::VAR * genSpillVar(SR * sr);
    void generateFuncUnitDedicatedCode();
    OR_CFG * getORCfg() const { return m_or_cfg; }
    xoc::TypeMgr * getTypeMgr() const { return m_tm; }
    List<ORBB*> * getORBBList() { return &m_or_bb_list; }

    //Construct a name for VAR that will lived in a ORBB.
    CHAR const* genBBLevelNewVarName(OUT xcom::StrBuf & name);

    //Construct a name for VAR that will lived in Region.
    CHAR const* genFuncLevelNewVarName(OUT xcom::StrBuf & name);
    xoc::VAR * genTempVar(xoc::Type const* type, UINT align, bool func_level);
    OR * getOR(UINT id);
    OR * genOR(OR_TYPE ort) { return m_cgmgr->getORMgr()->gen_or(ort, this); }
    Section * getRodataSection() { return &m_rodata_sect; }
    Section * getCodeSection() { return &m_code_sect; }
    Section * getDataSection() { return &m_data_sect; }
    Section * getBssSection() { return &m_bss_sect; }
    Section * getStackSection() { return &m_stack_sect; }
    Section * getParamSection() { return &m_param_sect; }
    virtual REGFILE getRflagRegfile() const = 0;
    virtual REGFILE getPredicateRegfile() const;
    Vector<xoc::VAR const*> const& get_param_vars() const
    { return m_params; }
    virtual ORAsmInfo * getAsmInfo(OR const*) const
    {
        ASSERT(0, ("Target Dependent Code"));
        return NULL;
    }

    virtual RegFileSet const* getValidRegfileSet(
            OR_TYPE ortype,
            UINT idx,
            bool is_result) const;
    virtual RegSet const* getValidRegSet(
            OR_TYPE ortype,
            UINT idx,
            bool is_result) const;
    SRMgr * getSRMgr() { return m_cgmgr->getSRMgr(); }
    ORMgr * getORMgr() { return m_cgmgr->getORMgr(); }
    SRVecMgr * getSRVecMgr() { return m_cgmgr->getSRVecMgr(); }
    Vector<IssuePackageList*> * getIssuePackageListVec()
    { return &m_ipl_vec; }

    //Map phsical register to dedicated symbol register if exist.
    SR * getDedicatedSRForPhyReg(REG reg);

    virtual void initFuncUnit();
    virtual void initBuiltin();
    void initGlobalVar(VarMgr * vm);
    bool isComputeStackOffset() const { return m_is_compute_sect_offset; }
    bool isAlloca(IR const* ir);
    bool isGRAEnable();
    //Return true if ORBB needs to be assigned cluster.
    bool isAssignClust(ORBB *) { return true; }
    bool isRegflowDep(OR * from, OR * to);
    bool isRegoutDep(OR * from, OR * to);
    bool isOpndSameWithResult(
            SR const* test_tn,
            IN OR * o,
            OUT INT * opndnum,
            OUT INT * resnum);
    //Return true if specified operand or result is Rflag register.
    bool isRflagRegister(OR_TYPE ot, UINT idx, bool is_result) const;
    virtual bool isReductionOR(OR * o);

    //Return true if specified immediate operand is in valid range.
    bool isValidImmOpnd(OR_TYPE ot, UINT idx, HOST_INT imm) const;
    bool isValidImmOpnd(OR_TYPE ot, HOST_INT imm) const;

    //Return true if regfile can be assigned to referred operand.
    virtual bool isValidRegFile(
            OR_TYPE ot,
            INT opndnum,
            REGFILE regfile,
            bool is_result) const;

    //Return true if regfile can be assigned to referred operand.
    virtual bool isValidRegFile(
            OR * o,
            SR const* opnd,
            REGFILE regfile,
            bool is_result) const;
    virtual bool isConsistentRegFileForCopy(REGFILE rf1, REGFILE rf2);

    //If stack-base-pointer register could use 'unit', return true.
    virtual bool isSPUnit(UNIT unit)
    {
        DUMMYUSE(unit);
        ASSERT(0, ("Target Dependent Code"));
        return false;
    }

    virtual bool isSafeToOptimize(OR * prev, OR * next);
    bool isReturnValueSR(SR const* sr) const
    {
        return SR_phy_regid(sr) != REG_UNDEF &&
               tmGetRegSetOfReturnValue()->is_contain(SR_phy_regid(sr));
    }
    bool isArgumentSR(SR const* sr) const
    {
        return SR_phy_regid(sr) != REG_UNDEF &&
               tmGetRegSetOfArgument()->is_contain(SR_phy_regid(sr));
    }
    bool isDedicatedSR(SR const* sr) const
    { return SR_is_dedicated(sr) || isReturnValueSR(sr); }    
    bool isSREqual(SR const* sr1, SR const* sr2) const;

    //Return true if SR is integer register.
    virtual bool isIntRegSR(OR *, SR const*, UINT idx, bool is_result) const
    {
        DUMMYUSE(idx);
        DUMMYUSE(is_result);
        ASSERT(0, ("Target Dependent Code"));
        return false;
    }

    virtual bool isOREqual(OR const* v1, OR const* v2)
    { return v1->is_equal(v2); }

    virtual bool isCompareOR(OR const* o);
    virtual bool isCondExecOR(OR * o);
    virtual bool isBusCluster(CLUST) = 0;

    //SR that can used on each clusters.
    virtual bool isBusSR(SR const*) = 0;

    virtual bool isCopyOR(OR *) = 0;

    //Return true if sr is stack base pointer.
    virtual bool isSP(SR const*) const = 0;

    //Return true if the results of 'o' are independent with other ops, and
    //all results can be recalculated any where.
    //We can sum up some typical simply expressoin to rematerialize as followed:
    //1. load constant
    //2. frame/stack pointer +/- constant
    //   (only use frame/stack register as operand)
    //3. load constant parameter
    //4. load from local data memory for simple
    //5. load address of variable
    virtual bool isRecalcOR(OR *) = 0;

    bool isSameSpillLoc(OR const* or1, OR const* or2);
    bool isSameSpillLoc(xoc::VAR const* or1loc, OR const* or1, OR const* or2);
    virtual bool isReduction(OR * o);
    virtual bool isSameCondExec(OR * prev, OR * next, BBORList & or_list);

    //Return true if both slot1 and slot2 belong to same cluster.
    virtual bool isSameCluster(OR const* or1, OR const* or2) const;

    //Compare if the first cluster is same with the second cluster.
    virtual bool isSameCluster(SLOT, SLOT) const = 0;

    //Return true if slot1 and slot2 use similar func-unit.
    virtual bool isSameLikeCluster(SLOT, SLOT) = 0;

    //Return true if the data BUS operation processed that was
    //using by other general operations.
    virtual bool isSameLikeCluster(OR const*, OR const*) = 0;

    //Return true if or-type has the number of 'res_num' results.
    virtual bool isMultiResultOR(OR_TYPE, UINT res_num) = 0;

    //Return true if or-type has multiple results.
    virtual bool isMultiResultOR(OR_TYPE) = 0;

    //'opnd_num': -1 means return true if or-type has multiple storre-val.
    virtual bool isMultiStore(OR_TYPE, INT opnd_num) = 0;

    //'res_num': -1 means return true if ot has multiple result SR.
    //    non -1 means return true if ot has the number of 'res_num' results.
    virtual bool isMultiLoad(OR_TYPE, INT res_num) = 0;

    virtual bool isValidOpndRegfile(
            OR_TYPE opcode,
            INT opndnum,
            REGFILE regfile) const;
    virtual bool isValidResultRegfile(
            OR_TYPE opcode,
            INT resnum,
            REGFILE regfile) const;
    virtual bool isValidRegInSRVec(OR * o, SR * sr, UINT idx, bool is_result);

    //Return true if the runtime value of base1 is equal to base2.
    //Some target might use mulitple stack pointers.
    virtual bool isStackPointerValueEqu(SR const* base1, SR const* base2)
    {
        DUMMYUSE(base1);
        DUMMYUSE(base2);
        ASSERT(0, ("Target Dependent Code"));
        return false;
    }

    virtual OR_TYPE invertORType(OR_TYPE)
    {
        ASSERT(0, ("Target Dependent Code"));
        return OR_UNDEF;
    }

    SR * mapPR2SR(UINT prno) { return m_pr2sr_map.get(prno); }
    SR * mapSymbolReg2SR(UINT regid) { return m_regid2sr_map.get(regid); }
    bool mustAsmDef(OR const* o, SR const* sr) const;
    bool mustDef(OR const* o, SR const* sr) const;
    bool mustUse(OR const* o, SR const* sr) const;
    bool mayDefInRange(
            SR const* sr,
            IN ORCt * start,
            IN ORCt * end,
            IN ORList & ors);
    bool mayDef(IN OR * o, SR const* sr);
    bool mayUse(IN OR * o, SR const* sr);
    virtual xoc::VAR const* mapOR2Var(OR * o);
    virtual bool mapRegSet2RegFile(
            OUT Vector<INT> & regfilev,
            RegSet const* regs);
    virtual UNIT mapSR2Unit(OR const* o, SR const* sr);
    virtual CLUST mapSlot2Cluster(SLOT slot);

    //Return the regisiter-file set which 'clust' corresponded with.
    virtual List<REGFILE> & mapCluster2RegFileList(
            CLUST clust,
            OUT List<REGFILE> & regfiles)
    {
        DUMMYUSE(clust);
        ASSERT(0, ("Target Dependent Code"));
        return regfiles;
    }

    //Return register-file set which the unit set corresponded with.
    //'units': function unit set
    virtual List<REGFILE> & mapUnitSet2RegFileList(
            IN UnitSet & us,
            OUT List<REGFILE> & regfiles)
    {
        DUMMYUSE(us);
        DUMMYUSE(regfiles);
        ASSERT(0, ("Target Dependent Code"));
        regfiles.clean();
        return regfiles;
    }

    //Mapping from single unit to its corresponed cluster.
    virtual SLOT mapUnit2Slot(UNIT unit, CLUST clst)
    {
        DUMMYUSE(unit);
        DUMMYUSE(clst);
        ASSERT(0, ("Target Dependent Code"));
        return FIRST_SLOT;
    }

    //Mapping from single issue slot(for multi-issue architecture) to
    //its corresponed function unit.
    virtual UNIT mapSlot2Unit(SLOT slot)
    {
        DUMMYUSE(slot);
        ASSERT(0, ("Target Dependent Code"));
        return UNIT_UNDEF;
    }

    //Return the cluster which owns 'regfile'.
    virtual CLUST mapRegFile2Cluster(REGFILE regfile, SR const* sr)
    {
        DUMMYUSE(regfile);
        DUMMYUSE(sr);
        ASSERT(0, ("Target Dependent Code"));
        CLUST clust = CLUST_UNDEF;
        return clust;
    }

    //Return the cluster which owns 'reg'
    virtual CLUST mapReg2Cluster(REG)
    {
        ASSERT(0, ("Target Dependent Code"));
        CLUST clust = CLUST_UNDEF;
        return clust;
    }


    //Return the function unit which can operate on 'regfile'.
    virtual UnitSet & mapRegFile2UnitSet(
            REGFILE regfile,
            SR const* sr,
            OUT UnitSet & us)
    {
        DUMMYUSE(regfile);
        DUMMYUSE(sr);
        DUMMYUSE(us);
        ASSERT(0, ("Target Dependent Code"));
        return us;
    }

    //Return the cluster which owns 'sr'
    virtual CLUST mapSR2Cluster(OR *, SR const*)
    { ASSERT(0, ("Target Dependent Code")); return CLUST_UNDEF; }

    void renameResult(
            OR * o,
            SR * oldsr,
            SR * newsr,
            bool match_phy_reg);
    void renameOpnd(
            OR * o,
            SR * oldsr,
            SR * newsr,
            bool match_phy_reg);
    void renameOpndResultFollowed(
            SR * oldsr,
            SR * newsr,
            ORCt * start,
            BBORList & ors);
    void renameOpndResultFollowed(
            SR * oldsr,
            SR * newsr,
            OR * start,
            BBORList & ors);
    void renameOpndResultInRange(
            SR * oldsr,
            SR * newsr,
            ORCt * start,
            ORCt * end,
            BBORList & orlist);
    void renameOpndResultInRange(
            SR * oldsr,
            SR * newsr,
            OR * start,
            OR * end,
            BBORList & orlist);
    virtual void reviseFormalParameterAndSpadjust();
    virtual void reviseFormalParamAccess(UINT lv_size);

    virtual void storeParamToStack(
            ArgDescMgr * argdesc,
            OUT ORList & ors,
            IN IOC *);
    void setMapPR2SR(UINT prno, SR * sr) { m_pr2sr_map.set(prno, sr); }
    void setMapSymbolReg2SR(UINT regid, SR * sr)
    { DUMMYUSE(regid); m_regid2sr_map.set(SR_sregid(sr), sr); }

    void setCluster(ORList & ors, CLUST clust);
    void setComputeSectOffset(bool doit) { m_is_compute_sect_offset = doit; }

    virtual void package(Vector<BBSimulator*> & simvec);

    virtual void setORListWithSamePredicate(ORList & ops, OR * o);
    virtual void setSpadjustOffset(OR * spadj, INT size);
    virtual void setMapOR2Mem(OR * o, xoc::VAR const* mid_opt);

    void localize();

    bool verify();

    void prependSpill(ORBB * bb, ORList & ors);
    virtual void preLS(
            IN ORBB * bb,
            IN RaMgr * ra_mgr,
            OUT DataDepGraph ** ddg,
            OUT BBSimulator ** sim,
            OUT LIS ** lis);

    //Perform Instruction Scheduling.
    virtual void performIS(
            OUT Vector<BBSimulator*> & simvec,
            RaMgr * ra_mgr);

    //Perform Register Allocation.
    virtual RaMgr * performRA();
    bool perform();
};

} //namespace xgen
#endif
