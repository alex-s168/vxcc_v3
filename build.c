#include "build_c/build.h"

/* ========================================================================= */

enum CompileResult target_deps() {
    ONLY_IF({
        CHANGED("allib/");
        NOT_FILE("allib/build/all.a");
    });

    START;
    ss("allib/", ({
        ss_task("all.a");
    }));
    END;
}

/* ========================================================================= */

struct CompileData target_gen_files[] = {
    DIR("build"),

    DIR("build/common"),
    SP(CT_CDEF, "common/targets.cdef"),
    SP(CT_CDEF, "common/target_etca.cdef"),
    SP(CT_CDEF, "common/target_x86.cdef"),
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
    DIR("build/ir"),
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
    SP(CT_C, "ir/opt.c"),

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

    DIR("build/common"),
    SP(CT_C, "common/verify.c"),
};

struct CompileData cg_files[] = {
    DIR("build"),
    DIR("build/cg"),
    DIR("build/cg/x86_stupid"),
    SP(CT_C, "cg/x86_stupid/cg.c"),
};

struct CompileData parser_files[] = {
    DIR("build"),
    DIR("build/irparser"),
    SP(CT_C, "irparser/parser.c"),
    SP(CT_C, "irparser/match.c"),
};

struct CompileData always_files[] = {
    DEP("build/common/target_etca.cdef.o"),
    DEP("build/common/target_x86.cdef.o"),
    DEP("build/ir/ops.cdef.o"),
    NOLD_DEP("ir/ir.h"),
};

enum CompileResult target_lib() {
    START;

    bool all = !exists("build/lib.a") ||
        file_changed("ir/ir.h");

    VaList comp = ASVAR(always_files);

    if (all || source_changed(LI(ir_files)))
        comp = vaListConcat(comp, ASVAR(ir_files));

    if (all || file_changed("ir/opt/"))
        comp = vaListConcat(comp, ASVAR(ir_opt_files));

    if (all || file_changed("ir/transform/"))
        comp = vaListConcat(comp, ASVAR(ir_transform_files));

    if (all || source_changed(LI(ir_verify_files)))
        comp = vaListConcat(comp, ASVAR(ir_verify_files));

    if (all || file_changed("cg/"))
        comp = vaListConcat(comp, ASVAR(cg_files));

    if (all || file_changed("irparser/"))
        comp = vaListConcat(comp, ASVAR(parser_files));

    DO(compile(VLI(comp)));

    VaList link = ASVAR(always_files);
    link = vaListConcat(link, ASVAR(ir_files));
    link = vaListConcat(link, ASVAR(ir_opt_files));
    link = vaListConcat(link, ASVAR(ir_transform_files));
    link = vaListConcat(link, ASVAR(ir_verify_files));
    link = vaListConcat(link, ASVAR(cg_files));
    link = vaListConcat(link, ASVAR(parser_files));

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
