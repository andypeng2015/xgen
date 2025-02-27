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
@*/
#include "../opt/cominc.h"
#include "xoccinc.h"
#include "../xgen/xgeninc.h"
#include "errno.h"
#include "../opt/util.h"
#include "../reader/grreader.h"
#include "../opt/comopt.h"
#include "../mach/machinc.h"
#include "../elf/elfinc.h"

namespace xocc {

static bool g_cfg_opt = true;
static bool g_ask_for_help = false;
static CHAR const* g_include_string = nullptr;
static CHAR const* g_exclude_string = nullptr;

static bool dispatchByPrefix(
    CHAR const* cmdstr, INT argc, CHAR const* argv[],
    INT & i, bool ** boption);
static void inferOption();
static void inferDumpOption();
static void inferStringOption();

static bool is_c_source_file(CHAR const* fn)
{
    UINT len = (UINT)::strlen(fn) + 1;
    CHAR * buf = (CHAR*)ALLOCA(len);
    xcom::upper(xcom::getFileSuffix(fn, buf, (UINT)len));
    if (::strcmp(buf, "C") == 0 || ::strcmp(buf, "I") == 0) {
        return true;
    }
    return false;
}


static bool is_gr_source_file(CHAR const* fn)
{
    UINT len = (UINT)::strlen(fn) + 2;
    CHAR * buf = (CHAR*)ALLOCA(len);
    xcom::upper(xcom::getFileSuffix(fn, buf, len));
    if (::strcmp(buf, "GR") == 0 || ::strcmp(buf, "I") == 0) {
        return true;
    }
    return false;
}


static bool is_obj_file(CHAR const* fn)
{
    UINT len = (UINT)strlen(fn) + 1;
    CHAR * buf = (CHAR*)ALLOCA(len);
    upper(xcom::getFileSuffix(fn, buf, len));
    if (strcmp(buf, "O") == 0) {
        return true;
    }
    return false;
}


static bool is_static_lib_file(CHAR const* fn)
{
    UINT len = (UINT)strlen(fn) + 1;
    CHAR * buf = (CHAR*)ALLOCA(len);
    upper(xcom::getFileSuffix(fn, buf, len));
    if (strcmp(buf, "A") == 0) {
        return true;
    }
    return false;
}


static void disable_opt(INT opt_level)
{
    switch (opt_level) {
    case OPT_LEVEL0:
        xoc::g_do_lsra = false;
        //Fall through.
    case OPT_LEVEL1:
    case OPT_LEVEL2:
    case OPT_LEVEL3:
    case SIZE_OPT:
        xoc::g_do_cp = false;
        xoc::g_do_cp_aggressive = false;
        xoc::g_do_dce = false;
        xoc::g_do_dce_aggressive = false;
        xoc::g_do_licm = false;
        xoc::g_do_gcse = false;
        xoc::g_do_rp = false;
        xoc::g_do_lftr = false;
        xoc::g_infer_type = false;
        xoc::g_do_rce = false;
        xoc::g_do_vect = false;
        xoc::g_do_gvn = false;
        g_cfg_opt = false;
        break;
    default: UNREACHABLE();
    }
}


static bool process_opt(INT argc, CHAR const* argv[], INT & i)
{
    DUMMYUSE(argc);
    CHAR const* cmdstr = &argv[i][1];
    switch (cmdstr[1]) {
    case '0':
        xoc::g_opt_level = OPT_LEVEL0;
        xoc::g_pass_opt.disableAllPass();
        //Note the essential options will be set at the end.
        break;
    case '1':
        xoc::g_opt_level = OPT_LEVEL1;
        xoc::g_pass_opt.enablePassInLevel1();
        break;
    case '2':
        xoc::g_opt_level = OPT_LEVEL2;
        xoc::g_pass_opt.enablePassInLevel2();
        g_cfg_opt = true;
        break;
    case '3':
        xoc::g_opt_level = OPT_LEVEL3;
        xoc::g_pass_opt.enablePassInLevel3();
        g_cfg_opt = true;
        break;
    case 's':
    case 'S':
        xoc::g_opt_level = SIZE_OPT;
        xoc::g_pass_opt.enablePassInLevelSize();
        g_cfg_opt = true;
        break;
    default:
        xoc::g_opt_level = OPT_LEVEL1;
        xoc::g_pass_opt.enablePassInLevel1();
    }
    i++;
    return true;
}


static bool process_output_file(INT argc, CHAR const* argv[], INT & i)
{
    if (i + 1 < argc && argv[i + 1] != nullptr) {
        g_output_file_name = argv[i + 1];
    }
    i += 2;
    return true;
}


static CHAR const* process_dump(INT argc, CHAR const* argv[], INT & i)
{
    CHAR const* n = nullptr;
    if (i + 1 < argc && argv[i + 1] != nullptr) {
        n = argv[i + 1];
    }
    i += 2;
    return n;
}


class BoolOption {
protected:
    class Desc {
    public:
        CHAR const* name;
        bool * option;
        CHAR const* intro;
    };
    static Desc const option_desc[];
    static Desc const dump_option_desc[];
    static Desc const elf_option_desc[];
public:
    static CHAR const* dump_option_prefix;
    static CHAR const* disable_option_prefix;
    static CHAR const* only_option_prefix;
    static CHAR const* elf_option_prefix;
public:
    static UINT getNumOfOption();
    static UINT getNumOfDumpOption();
    static UINT getNumOfELFOption();

    static bool is_option(CHAR const* str, bool ** option)
    {
        ASSERT0(option);
        for (UINT i = 0; i < BoolOption::getNumOfOption(); i++) {
            if (::strcmp(BoolOption::option_desc[i].name, str) == 0) {
                *option = BoolOption::option_desc[i].option;
                return true;
            }
        }
        return false;
    }

    static bool is_dump_option(CHAR const* str, bool ** option)
    {
        ASSERT0(option);
        for (UINT i = 0; i < BoolOption::getNumOfDumpOption(); i++) {
            if (::strcmp(BoolOption::dump_option_desc[i].name, str) == 0) {
                *option = BoolOption::dump_option_desc[i].option;
                return true;
            }
        }
        return false;
    }

    static bool is_elf_option(CHAR const* str, bool ** option)
    {
        ASSERT0(option);
        for (UINT i = 0; i < BoolOption::getNumOfELFOption(); i++) {
            if (::strcmp(BoolOption::elf_option_desc[i].name, str) == 0) {
                *option = BoolOption::elf_option_desc[i].option;
                return true;
            }
        }
        return false;
    }

    static void dump_usage(StrBuf & output)
    {
        UINT max_name_len = 0;
        for (UINT i = 0; i < BoolOption::getNumOfOption(); i++) {
            max_name_len = MAX((UINT)::strlen(BoolOption::option_desc[i].name),
                               max_name_len);
        }
        UINT alignbuflen = max_name_len + 2;

        CHAR * alignbuf = (CHAR*)ALLOCA(alignbuflen);
        for (UINT i = 0; i < BoolOption::getNumOfOption(); i++) {
            UINT j = 0;
            for (; j < ::strlen(BoolOption::option_desc[i].name); j++) {
                alignbuf[j] = BoolOption::option_desc[i].name[j];
            }
            for (; j < alignbuflen - 1; j++) {
                alignbuf[j] = ' '; //pad the rest with blank
            }
            alignbuf[j] = 0;

            output.strcat("\n -%s %s", alignbuf,
                          BoolOption::option_desc[i].intro);
        }
    }

    static void dump_dump_option(StrBuf & output)
    {
        UINT max_name_len = 0;
        for (UINT i = 0; i < BoolOption::getNumOfDumpOption(); i++) {
            max_name_len =
                MAX((UINT)::strlen(BoolOption::dump_option_desc[i].name),
                    max_name_len);
        }
        UINT alignbuflen = max_name_len + 2;

        CHAR * alignbuf = (CHAR*)ALLOCA(alignbuflen);
        for (UINT i = 0; i < BoolOption::getNumOfDumpOption(); i++) {
            UINT j = 0;
            for (; j < ::strlen(BoolOption::dump_option_desc[i].name); j++) {
                alignbuf[j] = BoolOption::dump_option_desc[i].name[j];
            }
            for (; j < alignbuflen - 1; j++) {
                alignbuf[j] = ' '; //pad the rest with blank
            }
            alignbuf[j] = 0;

            output.strcat("\n -%s%s %s", BoolOption::dump_option_prefix,
                          alignbuf, BoolOption::dump_option_desc[i].intro);
        }
    }
};

CHAR const* BoolOption::dump_option_prefix = "dump-";
CHAR const* BoolOption::disable_option_prefix = "no-";
CHAR const* BoolOption::only_option_prefix = "only-";
CHAR const* BoolOption::elf_option_prefix = "elf-";

BoolOption::Desc const BoolOption::option_desc[] = {
    { "time", &xoc::g_show_time,
      "show compilation time", },
    { "licm", &xoc::g_do_licm,
      "enable loop-invariant-code-motion optimization", },
    { "licm_no_guard", &xoc::g_do_licm_no_guard,
      "enable loop-invariant-code-motion optimization "
      "without inserting loop guard", },
    { "gcse", &xoc::g_do_gcse,
      "enable global-common-subexpression-elimination optimization", },
    { "rp", &xoc::g_do_rp,
      "enable register-promotion optimization", },
    { "cp", &xoc::g_do_cp,
      "enable copy-propagation optimization", },
    { "cp_aggr", &xoc::g_do_cp_aggressive,
      "enable aggressive copy-propagation optimization", },
    { "rce", &xoc::g_do_rce,
      "enable redundant-code-elimination optimization", },
    { "vect", &xoc::g_do_vect,
      "enable auto vectorization", },
    { "dce", &xoc::g_do_dce,
      "enable dead-code-elimination optimization", },
    { "infer_type", &xoc::g_infer_type,
      "enable type inference", },
    { "vrp", &xoc::g_do_vrp,
      "enable value range propagation", },
    { "dce_aggr", &xoc::g_do_dce_aggressive,
      "enable aggressive-dead-code-elimination optimization", },
    { "lftr", &xoc::g_do_lftr,
      "enable linear-function-test-replacement optimization", },
    { "ivr", &xoc::g_do_ivr,
      "enable induction-variable-recognization", },
    { "gvn", &xoc::g_do_gvn,
      "enable global-value-numbering", },
    { "mdssa", &xoc::g_do_mdssa,
      "enable md-ssa analysis", },
    { "prssa", &xoc::g_do_prssa,
      "enable pr-ssa analysis", },
    { "prmode", &xoc::g_is_lower_to_pr_mode,
      "lowering cascading IR mode to pr-transfering mode", },
    { "lowest_height", &xoc::g_is_lower_to_lowest_height,
      "lowering cascading IR to the lowest height", },
    { "prdu", &xoc::g_compute_pr_du_chain,
      "enable classic PR def-use analysis", },
    { "nonprdu", &xoc::g_compute_nonpr_du_chain,
      "enable classic NON-PR def-use analysis", },
    { "redirect_stdout_to_dump_file", &xoc::g_redirect_stdout_to_dump_file,
      "redirect internal compiler output information to given dump file", },
    { "ipa", &xoc::g_do_ipa,
      "enable interprocedual analysis", },
    { "cfgopt", &xocc::g_cfg_opt,
      "enable control-flow-graph optimization", },
    { "schedule_delay_slot", &xgen::g_enable_schedule_delay_slot,
      "enable scheduling branch-delay-slot", },
    { "cg", &xgen::g_do_cg,
      "enable target code generation", },
    { "gra", &xgen::g_do_gra,
      "enable global-register-allocation", },
    { "lis", &xgen::g_do_lis,
      "enable instruction-scheduling", },
    { "cg_for_inner_region", &xgen::g_is_generate_code_for_inner_region,
      "enable code generation for inner region", },
    { "refine", &xoc::g_do_refine,
      "enable refinement optimization", },
    { "refine_duchain", &xoc::g_do_refine_duchain,
      "enable refine-duchain optimization", },
    { "lsra", &xoc::g_do_lsra,
      "enable linear-scan-register-allocation", },
    { "reass", &xoc::g_do_alge_reasscociate,
      "enable algebraic-reasscociation", },
    { "opt_float", &xoc::g_do_opt_float,
      "enable float optimization", },
    #ifdef REF_TARGMACH_INFO
    { "migen", &mach::g_do_migen,
      "enable machine-instruction generation", },
    #endif
};


BoolOption::Desc const BoolOption::dump_option_desc[] = {
    { "cfg", &xoc::g_dump_opt.is_dump_cfg,
      "dump control-flow-graph", },
    { "cfgopt", &xoc::g_dump_opt.is_dump_cfgopt,
      "dump control-flow-graph optimizations", },
    { "dom", &xoc::g_dump_opt.is_dump_dom,
      "dump dominator information", },
    { "aa", &xoc::g_dump_opt.is_dump_aa,
      "dump alias-analysis", },
    { "dce", &xoc::g_dump_opt.is_dump_dce,
      "dump dead-code-elimination", },
    { "infer_type", &xoc::g_dump_opt.is_dump_infertype,
      "dump infer-type", },
    { "vrp", &xoc::g_dump_opt.is_dump_vrp,
      "dump value-range-propagation", },
    { "invert_brtgt", &xoc::g_dump_opt.is_dump_invert_brtgt,
      "dump invert-branch-target", },
    { "lftr", &xoc::g_dump_opt.is_dump_lftr,
      "dump linear-function-test-replacement optimization", },
    { "vect", &xoc::g_dump_opt.is_dump_vectorization,
      "dump loop vectorization", },
    { "ivr", &xoc::g_dump_opt.is_dump_ivr,
      "dump induction-variable-recognization", },
    { "rp", &xoc::g_dump_opt.is_dump_rp,
      "dump register-promotion", },
    { "licm", &xoc::g_dump_opt.is_dump_licm,
      "dump loop-invariant-code-motion", },
    { "gcse", &xoc::g_dump_opt.is_dump_gcse,
      "dump global-common-subexpression-elimination", },
    { "dumgr", &xoc::g_dump_opt.is_dump_dumgr,
      "dump classic def-use information", },
    { "prssa", &xoc::g_dump_opt.is_dump_prssamgr,
      "dump pr-ssa information", },
    { "mdssa", &xoc::g_dump_opt.is_dump_mdssamgr,
      "dump md-ssa information", },
    { "rce", &xoc::g_dump_opt.is_dump_rce,
      "dump redundant-code-elimination", },
    { "cp", &xoc::g_dump_opt.is_dump_cp,
      "dump copy-propagation", },
    { "ir2or", &xgen::g_xgen_dump_opt.is_dump_ir2or,
      "dump IR2OR convertion", },
    { "ra", &xgen::g_xgen_dump_opt.is_dump_ra,
      "dump register allocation", },
    { "lis", &xgen::g_xgen_dump_opt.is_dump_lis,
      "dump instruction-scheduling", },
    { "cg", &xgen::g_xgen_dump_opt.is_dump_cg,
      "dump target code generation", },
    { "cdg", &xoc::g_dump_opt.is_dump_cdg,
      "dump control-dependent-graph", },
    { "simplification", &xoc::g_dump_opt.is_dump_simplification,
      "dump IR simplification", },
    { "refine_duchain", &xoc::g_dump_opt.is_dump_refine_duchain,
      "dump refine-duchain optimization", },
    { "refine", &xoc::g_dump_opt.is_dump_refine,
      "dump refine-duchain optimization", },
    { "gvn", &xoc::g_dump_opt.is_dump_gvn,
      "dump global-value-numbering", },
    { "irparser", &xoc::g_dump_opt.is_dump_irparser,
      "dump IR parser", },
    { "all", &xoc::g_dump_opt.is_dump_all,
      "dump all compiler information", },
    { "nothing", &xoc::g_dump_opt.is_dump_nothing,
      "disable dump", },
    { "gr", &xocc::g_is_dumpgr,
      "output GR language according region information", },
    { "irid", &xoc::g_dump_opt.is_dump_ir_id,
      "dump IR's id", },
    { "lsra", &xoc::g_dump_opt.is_dump_lsra,
      "dump linear-scan-register-allocation", },
    { "reass", &xoc::g_dump_opt.is_dump_alge_reasscociate,
      "dump algebraic-reasscociation", },
    { "option", &xocc::g_is_dump_option,
      "dump all compiling options", },
    { "to_buffer", &xoc::g_dump_opt.is_dump_to_buffer,
      "perfer to dump informations to buffer", },
};

BoolOption::Desc const BoolOption::elf_option_desc[] = {
    { "device", &elf::g_elf_opt.m_is_device_elf,
      "generate device elf", },
    { "fatbin", &elf::g_elf_opt.m_is_fatbin_elf,
      "generate fatbin elf", },
};


UINT BoolOption::getNumOfOption()
{
    return sizeof(BoolOption::option_desc) /
           sizeof(BoolOption::option_desc[0]);
}


UINT BoolOption::getNumOfDumpOption()
{
    return sizeof(BoolOption::dump_option_desc) /
           sizeof(BoolOption::dump_option_desc[0]);
}


UINT BoolOption::getNumOfELFOption()
{
    return sizeof(BoolOption::elf_option_desc) /
           sizeof(BoolOption::elf_option_desc[0]);
}

class IntOption {
public:
    class Desc {
    public:
        CHAR const* name;
        INT * option;
        CHAR const* intro;
    };
    static Desc const option_desc[];
    static UINT getNumOfOption();

    static bool is_option(CHAR const* str, INT ** option)
    {
        ASSERT0(option);
        for (UINT i = 0; i < IntOption::getNumOfOption(); i++) {
            if (::strcmp(IntOption::option_desc[i].name, str) == 0) {
                *option = IntOption::option_desc[i].option;
                return true;
            }
        }
        return false;
    }

    static void dump_usage(StrBuf & output)
    {
        UINT max_name_len = 0;
        for (UINT i = 0; i < IntOption::getNumOfOption(); i++) {
            max_name_len = MAX((UINT)::strlen(IntOption::option_desc[i].name),
                               max_name_len);
        }
        UINT alignbuflen = max_name_len + 2;

        CHAR * alignbuf = (CHAR*)ALLOCA(alignbuflen);
        for (UINT i = 0; i < IntOption::getNumOfOption(); i++) {
            UINT j = 0;
            for (; j < ::strlen(IntOption::option_desc[i].name); j++) {
                alignbuf[j] = IntOption::option_desc[i].name[j];
            }
            for (; j < alignbuflen - 1; j++) {
                alignbuf[j] = ' '; //pad the rest with blank
            }
            alignbuf[j] = 0;

            output.strcat("\n -%s = <integer> %s", alignbuf,
                          IntOption::option_desc[i].intro);
        }
    }
};


IntOption::Desc const IntOption::option_desc[] = {
    { "thres_opt_ir_num", (INT*)&xoc::g_thres_opt_ir_num,
       "the maximum limit of the number of IR to perform optimizations", },
    { "thres_opt_bb_num", (INT*)&xoc::g_thres_opt_bb_num,
       "the maximum limit of the number of IRBB to perform optimizations", }
};


class StringOption {
public:
    class Desc {
    public:
        CHAR const* name;
        CHAR * option;
        CHAR const* intro;
    };
    static Desc const option_desc[];
    static UINT getNumOfOption();

    static bool is_option(CHAR const* str, CHAR const** option)
    {
        ASSERT0(option);
        for (UINT i = 0; i < StringOption::getNumOfOption(); i++) {
            if (::strcmp(StringOption::option_desc[i].name, str) == 0) {
                *option = StringOption::option_desc[i].option;
                return true;
            }
        }
        return false;
    }

    static void dump_usage(StrBuf & output)
    {
        UINT max_name_len = 0;
        for (UINT i = 0; i < StringOption::getNumOfOption(); i++) {
            max_name_len = MAX((UINT)::strlen(
                StringOption::option_desc[i].name), max_name_len);
        }
        UINT alignbuflen = max_name_len + 2;

        CHAR * alignbuf = (CHAR*)ALLOCA(alignbuflen);
        for (UINT i = 0; i < StringOption::getNumOfOption(); i++) {
            UINT j = 0;
            for (; j < ::strlen(StringOption::option_desc[i].name); j++) {
                alignbuf[j] = StringOption::option_desc[i].name[j];
            }
            for (; j < alignbuflen - 1; j++) {
                alignbuf[j] = ' '; //pad the rest with blank
            }
            alignbuf[j] = 0;

            output.strcat("\n -%s = \"string1,string2,string3\" %s", alignbuf,
                          StringOption::option_desc[i].intro);
        }
    }
};


StringOption::Desc const StringOption::option_desc[] = {
    { "include_region", (CHAR*)&xocc::g_include_string,
       "the list of region that participate in compilation", },
    { "exclude_region", (CHAR*)&xocc::g_exclude_string,
       "the list of region that will be excluded from compilation", },
};


UINT IntOption::getNumOfOption()
{
    return sizeof(IntOption::option_desc) /
           sizeof(IntOption::option_desc[0]);
}


UINT StringOption::getNumOfOption()
{
    return sizeof(StringOption::option_desc) /
           sizeof(StringOption::option_desc[0]);
}


static bool dispatchByPrefixDump(
    CHAR const* cmdstr, INT argc, CHAR const* argv[],
    INT & i, bool ** boption)
{
    if (BoolOption::is_dump_option(cmdstr, boption)) {
        ASSERT0(boption);
        **boption = true;
        i++;
        inferDumpOption();
        return true;
    }
    return false;
}


static bool dispatchByPrefixELF(
    CHAR const* cmdstr, INT argc, CHAR const* argv[],
    INT & i, bool ** boption)
{
    if (BoolOption::is_elf_option(cmdstr, boption)) {
        ASSERT0(boption);
        **boption = true;
        i++;
        return true;
    }
    return false;
}


//Note ONLY will not disable analysis passes, such as AA, DU.
static bool dispatchByPrefixOnly(
    CHAR const* cmdstr, INT argc, CHAR const* argv[],
    INT & i, bool ** boption)
{
    if (BoolOption::is_option(cmdstr, boption)) {
        disable_opt(xoc::g_opt_level);
        ASSERT0(boption);
        **boption = true;
        i++;
        return true;
    }
    return false;
}


static bool recogOption(CHAR const* cmdstr, INT argc, CHAR const* argv[],
                        INT & i, bool ** boption)
{
    if (BoolOption::is_option(cmdstr, boption)) {
        ASSERT0(*boption);
        **boption = true;
        i++;
        return true;
    }

    INT * int_option = nullptr;
    if (IntOption::is_option(cmdstr, &int_option)) {
        ASSERT0(int_option);
        CHAR const* n = nullptr;
        if (i + 1 < argc && argv[i + 1] != nullptr) {
            n = argv[i + 1];
        }
        if (n == nullptr) { return false; }
        if (::strcmp(n, "=") != 0) { return false; }
        if (i + 2 < argc && argv[i + 2] != nullptr) {
            n = argv[i + 2];
        }
        if (n == nullptr) { return false; }
        *int_option = (INT)xcom::xatoll(n, false);
        i += 3;
        return true;
    }

    CHAR const* str_option = nullptr;
    if (StringOption::is_option(cmdstr, &str_option)) {
        ASSERT0(str_option);
        CHAR const* n = nullptr;
        if (i + 1 < argc && argv[i + 1] != nullptr) {
            n = argv[i + 1];
        }
        if (n == nullptr) { return false; }
        if (::strcmp(n, "=") != 0) { return false; }
        if (i + 2 < argc && argv[i + 2] != nullptr) {
            n = argv[i + 2];
        }
        if (n == nullptr) { return false; }
        *(CHAR const**)str_option = n;
        i += 3;
        inferStringOption();
        return true;
    }
    return false;
}


static bool tryDispatchCreateDump(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i)
{
    if (::strcmp(cmdstr, "dump") == 0) {
        CHAR const* n = process_dump(argc, argv, i);
        if (n == nullptr) {
            return false;
        }
        g_dump_file_name = n;
        return true;
    }
    return false;
}


static bool tryDispatchHelp(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i)
{
    if (::strcmp(cmdstr, "help") == 0 || ::strcmp(cmdstr, "h") == 0) {
        g_ask_for_help = true;
        return true;
    }
    return false;
}


//No prefix.
static bool tryDispatchNonPrefixOption(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    ASSERT0(boption);
    return recogOption(cmdstr, argc, argv, i, boption);
}


static bool tryDispatchELFOption(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    CHAR const* prefix = BoolOption::elf_option_prefix;
    if (xcom::xstrcmp(cmdstr, prefix, (INT)strlen(prefix))) {
        return dispatchByPrefixELF(
            cmdstr + strlen(prefix), argc, argv, i, boption);
    }
    return false;
}


static bool tryDispatchOnlyOption(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    CHAR const* prefix = BoolOption::only_option_prefix;
    if (xcom::xstrcmp(cmdstr, prefix, (INT)::strlen(prefix))) {
        ASSERT0(boption);
        return dispatchByPrefixOnly(
            cmdstr + ::strlen(prefix), argc, argv, i, boption);
    }
    return false;
}


static bool tryDispatchDumpOption(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    CHAR const* prefix = BoolOption::dump_option_prefix;
    if (xcom::xstrcmp(cmdstr, prefix, (INT)::strlen(prefix))) {
        return dispatchByPrefixDump(
            cmdstr + ::strlen(prefix), argc, argv, i, boption);
    }
    return false;
}


static bool tryDispatchDisable(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    CHAR const* prefix = BoolOption::disable_option_prefix;
    if (xcom::xstrcmp(cmdstr, prefix, (INT)::strlen(prefix))) {
        ASSERT0(boption);
        bool res = dispatchByPrefix(cmdstr + ::strlen(prefix), argc, argv,
                                    i, boption);
        if (!res) { return res; }
        ASSERT0(*boption);
        **boption = !(**boption);
        return res;
    }
    return false;
}


static bool tryDispatchOptAndOutput(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i)
{
    switch (cmdstr[0]) {
    case 'O':
        if (xcom::xisdigit(cmdstr[1])) {
            return process_opt(argc, argv, i);
        }
        break;
    case 'o':
        if (cmdstr[1] == 0) {
            return process_output_file(argc, argv, i);
        }
        break;
    default:;
    }
    return false;
}


//The function dispatchs the processing of cmdline recursively by
//prefix string.
static bool dispatchByPrefix(
    CHAR const* cmdstr, INT argc, CHAR const* argv[], INT & i, bool ** boption)
{
    if (tryDispatchHelp(cmdstr, argc, argv, i)) {
        return true;
    }
    if (tryDispatchOptAndOutput(cmdstr, argc, argv, i)) {
        return true;
    }
    if (tryDispatchCreateDump(cmdstr, argc, argv, i)) {
        return true;
    }
    if (tryDispatchDisable(cmdstr, argc, argv, i, boption)) {
        return true;
    }
    if (tryDispatchDumpOption(cmdstr, argc, argv, i, boption)) {
        return true;
    }
    if (tryDispatchOnlyOption(cmdstr, argc, argv, i, boption)) {
        return true;
    }
    if (tryDispatchELFOption(cmdstr, argc, argv, i, boption)) {
        return true;
    }
    if (tryDispatchNonPrefixOption(cmdstr, argc, argv, i, boption)) {
        return true;
    }
    return false;
}


static void usage()
{
    StrBuf buf1(128);
    StrBuf buf2(128);
    StrBuf buf3(128);
    StrBuf buf4(128);
    BoolOption::dump_usage(buf1);
    IntOption::dump_usage(buf2);
    StringOption::dump_usage(buf3);
    BoolOption::dump_dump_option(buf4);

    fprintf(stdout,
    "\nXOCC Version %s"
    "\nUsage: xocc [options] file"
    "\noptions: "
    "\n -h,            show command line usage"
    "\n -help,         show command line usage"
    "\n -O0,           compile without any optimization"
    "\n -O1,-O2,-O3    compile with optimizations"
    "\n -dump <file>   create dump file"
    "\n -o <file>      output file name"
    "\n -readgr <file> read GR file"
    "\n -no-<option>   disable option, e.g:-no-dce"
    "\n -dump-<option> dump information about option, e.g:-dump-dce"
    "\n"
    "\noption: %s%s%s"
    "\n"
    "\navailable dump option: %s"
    "\n",
    g_xocc_version,
    buf1.getBuf(),
    buf2.getBuf(),
    buf3.getBuf(),
    buf4.getBuf());
}


static void forceOption()
{
    //Enable infer-type even if it is disabled by cmdline when CG is enabled.
    xoc::g_infer_type = true;
}


static void inferDumpOption()
{
    if (xoc::g_dump_opt.isDumpAll()) {
        xocc::g_is_dump_option = true;
        xoc::g_dump_opt.setDumpAll();
    }
    if (xoc::g_dump_opt.isDumpNothing()) {
        xocc::g_is_dump_option = false;
        xoc::g_dump_opt.setDumpNothing();
    }
}


static void inferStringOption()
{
    if (g_include_string != nullptr) {
        xoc::g_include_region.addString(g_include_string);
    }
    if (g_exclude_string != nullptr) {
        xoc::g_exclude_region.addString(g_exclude_string);
    }
}


static void inferOption()
{
    //Set XGEN option according to XOC option.
    xgen::g_xgen_dump_opt.is_dump_all = xoc::g_dump_opt.isDumpAll();
    xgen::g_xgen_dump_opt.is_dump_nothing = xoc::g_dump_opt.isDumpNothing();
}


//The position of unknown option in 'argv'.
static void report_unknown_option(UINT pos, CHAR const* argv[])
{
    fprintf(stdout, "\n");
    for (UINT i = 0; i <= pos; i++) {
        fprintf(stdout, "%s ", argv[i]);
    }
    fprintf(stdout, "\nunknown option:%s\n", argv[pos]);
}


bool processCmdLine(INT argc, CHAR const* argv[])
{
    if (argc <= 1) { usage(); return false; }

    //For C language, inserting CVT is expected.
    xoc::g_insert_cvt = true;
    xoc::g_do_refine = true;

    //Retain CFG, DU info for IPA used.
    xoc::g_compute_region_imported_defuse_md = true;

    //Post-simplification need IRMgr and PassMgr.
    //g_retain_pass_mgr_for_region = false;
    //g_compute_pr_du_chain = false;
    //g_compute_nonpr_du_chain = false;
    //g_do_call_graph = true;
    //g_do_ipa = true;
    xoc::g_is_support_dynamic_type = true;
    xoc::g_do_opt_float = false;
    for (INT i = 1; i < argc;) {
        if (argv[i][0] == '-') {
            bool * boption = nullptr;
            if (!dispatchByPrefix(&argv[i][1], argc, argv, i, &boption)) {
                report_unknown_option(i, argv);
                return false;
            }
            if (g_ask_for_help) {
                usage();
                return false;
            }
            continue;
        }

        if (is_c_source_file(argv[i])) {
            g_cfile_list.append_tail(argv[i]);
            i++;
            continue;
        }

        if (is_gr_source_file(argv[i])) {
            g_grfile_list.append_tail(argv[i]);
            i++;
            continue;
        }

        if (is_obj_file(argv[i])) {
            g_objfile_list.append_tail(argv[i]);
            i++;
            continue;
        }

        if (is_static_lib_file(argv[i])) {
           g_static_lib_list.append_tail(argv[i]);
           i++;
           continue;
        }

        report_unknown_option(i, argv);
        return false;
    }
    inferOption();
    forceOption();
    return true;
}

} //namespace xocc
