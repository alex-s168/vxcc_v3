#include "build_c/build.h"

/* ========================================================================= */

enum CompileResult target_deps() {
    // currently no deps
	return CR_OK;
}

/* ========================================================================= */

struct CompileData target_gen_files[] = {
    DIR("build"),

    DIR("build/targets"),
    SP(CT_CDEF, "targets/targets.cdef"),
    DIR("build/targets/etca"),
    SP(CT_CDEF, "targets/etca/etca.cdef"),
    DIR("build/targets/x86"),
    SP(CT_CDEF, "targets/x86/x86.cdef"),
    // add target (cdef file)

    DIR("build/ir"),
    SP(CT_CDEF, "ir/ops.cdef"),
};

enum CompileResult target_gen() {
    if (!withPipPackage("generic-lexer")) {
        error("pip package generic-lexer required and could not be installted!");
        return CR_FAIL;
    }

    START;
        DO(compile(LI(target_gen_files)));
    END;
}

/* ========================================================================= */

struct CompileData ir_files[] = {
    DIR("build"),

    DIR("build/targets"),
    SP(CT_C, "targets/targets.c"),

    DIR("build/ir"),
    SP(CT_C, "ir/verify.c"),
	SP(CT_C, "ir/fastalloc.c"),
    SP(CT_C, "ir/dump.c"),
    SP(CT_C, "ir/lifetimes.c"),
    SP(CT_C, "ir/transform.c"),
    SP(CT_C, "ir/builder.c"),
    SP(CT_C, "ir/ir.c"),
    SP(CT_C, "ir/eval.c"),
    SP(CT_C, "ir/analysis.c"),
};

struct CompileData ir_opt_files[] = {
    DIR("build"),
    DIR("build/ir"),
    SP(CT_C, "ir/passes.c"),

    DIR("build/ir/opt"),
    SP(CT_C, "ir/opt/tailcall.c"),
    SP(CT_C, "ir/opt/loop_simplify.c"),
    SP(CT_C, "ir/opt/reduce_loops.c"),
    SP(CT_C, "ir/opt/vars.c"),
    SP(CT_C, "ir/opt/constant_eval.c"),
    SP(CT_C, "ir/opt/inline_vars.c"),
    SP(CT_C, "ir/opt/reduce_if.c"),
    SP(CT_C, "ir/opt/cmov.c"),
    SP(CT_C, "ir/opt/comparisions.c"),
    SP(CT_C, "ir/opt/dce.c"),
    SP(CT_C, "ir/opt/join_compute.c"),
    SP(CT_C, "ir/opt/ll_jumps.c"),
    SP(CT_C, "ir/opt/ll_binary.c"),
    SP(CT_C, "ir/opt/simple_patterns.c"),
    SP(CT_C, "ir/opt/ll_sched.c"),
    SP(CT_C, "ir/opt/if_opts.c"),
    SP(CT_C, "ir/opt/swap_if_cases.c"),
};

struct CompileData ir_transform_files[] = {
    DIR("build"),
    DIR("build/ir"),
    DIR("build/ir/transform"),
    SP(CT_C, "ir/transform/single_assign_conditional.c"),
    SP(CT_C, "ir/transform/single_assign.c"),
    SP(CT_C, "ir/transform/normalize.c"),
    SP(CT_C, "ir/transform/share_slots.c"),
    SP(CT_C, "ir/transform/ssair_llir_lower.c"),
    SP(CT_C, "ir/transform/cmov_expand.c"),
    SP(CT_C, "ir/transform/ll_finalize.c"),
    SP(CT_C, "ir/transform/lower_loops.c"),
};

struct CompileData ir_verify_files[] = {
    DIR("build"),
    DIR("build/ir"),
    SP(CT_C, "ir/verify_cir.c"),
    SP(CT_C, "ir/verify_common.c"),
    SP(CT_C, "ir/verify_ssa.c"),
};

struct CompileData cg_files[] = {
    DIR("build"),
    DIR("build/targets"),
    SP(CT_C, "targets/targets.c"),
    DIR("build/targets/etca"),
    SP(CT_C, "targets/etca/etca.c"),
    DIR("build/targets/x86"),
    SP(CT_C, "targets/x86/x86.c"),
    SP(CT_C, "targets/x86/cg.c"),
    // add target
};

struct CompileData always_files[] = {
    DEP("build/targets/etca/etca.cdef.o"),
    DEP("build/targets/x86/x86.cdef.o"),
    DEP("build/targets/targets.cdef.o"),
    // add target (cdef file)

    DEP("build/ir/ops.cdef.o"),
    NOLD_DEP("ir/ir.h"),
};

enum CompileResult target_lib() {
    START;

    bool all = file_changed("ir/ir.h");

    VaList comp = ASVAR(always_files);

    if (source_changed(LI(ir_files)) || all)
        comp = vaListConcat(comp, ASVAR(ir_files));

    if (file_changed("ir/opt/") || all)
        comp = vaListConcat(comp, ASVAR(ir_opt_files));

    if (file_changed("ir/transform/") || all)
        comp = vaListConcat(comp, ASVAR(ir_transform_files));

    if (source_changed(LI(ir_verify_files)) || all)
        comp = vaListConcat(comp, ASVAR(ir_verify_files));

    if (file_changed("targets/") || all)
        comp = vaListConcat(comp, ASVAR(cg_files));

    DO(compile(VLI(comp)));

    VaList link = ASVAR(always_files);
    link = vaListConcat(link, ASVAR(ir_files));
    link = vaListConcat(link, ASVAR(ir_opt_files));
    link = vaListConcat(link, ASVAR(ir_transform_files));
    link = vaListConcat(link, ASVAR(ir_verify_files));
    link = vaListConcat(link, ASVAR(cg_files));

    DO(linkTask(VLI(link), "build/lib.a"));

    END;
}

/* ========================================================================= */

enum CompileResult target_tests() {
    START_TESTING;
    test("", "test.c", 0, CT_C,
            DEP("build/lib.a"));
    END_TESTING;
}

/* ========================================================================= */

struct Target targets[] = {
    { .name = "deps",  .run = target_deps },
    { .name = "gen",   .run = target_gen },
    { .name = "lib.a", .run = target_lib },
    { .name = "tests", .run = target_tests },
};

automain(targets);
