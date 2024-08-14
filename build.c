#define CC_ARGS "-g -ggdb"

#include "build_c/build.h"

/* ========================================================================= */

struct CompileData target_lib_files[] = {
    DIR("build"),

    DIR("build/ir"),
    SP(CT_C, "ir/fastalloc.c"),
    SP(CT_C, "ir/dump.c"),
    SP(CT_C, "ir/verify_cir.c"),
    SP(CT_C, "ir/lifetimes.c"),
    SP(CT_C, "ir/verify_common.c"),
    SP(CT_C, "ir/verify_ssa.c"),
    SP(CT_C, "ir/transform.c"),
    SP(CT_C, "ir/builder.c"),
    SP(CT_C, "ir/cir.h"),
    SP(CT_C, "ir/ir.c"),
    SP(CT_C, "ir/eval.c"),
    SP(CT_C, "ir/analysis.c"),
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

    DIR("build/ir/transform"),
    SP(CT_C, "ir/transform/single_assign_conditional.c"),
    SP(CT_C, "ir/transform/single_assign.c"),
    SP(CT_C, "ir/transform/normalize.c"),
    SP(CT_C, "ir/transform/share_slots.c"),
    SP(CT_C, "ir/transform/ssair_llir_lower.c"),
    SP(CT_C, "ir/transform/cmov_expand.c"),

    DIR("build/common"),
    SP(CT_C, "common/verify.c"),
    SP(CT_CDEF, "common/targets.cdef"),
};

enum CompileResult target_lib() {
    START;
    DO(compile(LI(target_lib_files)));
    DO(linkTask(LI(target_lib_files), "build/lib.a"));
    END;
}

/* ========================================================================= */

struct CompileData target_x86_files[] = {
    DIR("build"),

    DIR("build/cg"),

    DIR("build/cg/x86_stupid"),
    SP(CT_C, "cg/x86_stupid/cg.c"),
};

enum CompileResult target_x86() {
    START;
    DO(compile(LI(target_x86_files)));
    DO(linkTask(LI(target_x86_files), "build/x86.a"));
    END;
}

/* ========================================================================= */

enum CompileResult target_tests() {
    START_TESTING;
    test("", "test.c", 0, CT_C,
            DEP("build/lib.a"),
            DEP("build/x86.a"));
    END_TESTING;
}

/* ========================================================================= */

struct Target targets[] = {
	{ .name = "lib.a", .run = target_lib },
    { .name = "x86.a", .run = target_x86, },
	{ .name = "tests", .run = target_tests },
};

#define TARGETS_LEN (sizeof(targets) / sizeof(targets[0]))

int main(int argc, char **argv) {
    return build_main(argc, argv, targets, TARGETS_LEN);
}

