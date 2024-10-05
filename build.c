#include "build_c/build.h"

/* ========================================================================= */

enum CompileResult target_deps() {
    ONLY_IF({
        CHANGED("allib/");
        NOT_FILE("allib/build/kallok.a");
        NOT_FILE("allib/build/kollektions.a");
        NOT_FILE("allib/build/kash.a");
        NOT_FILE("allib/build/germanstr.a");
    });

    START;
    ss("allib/", ({
        ss_task("kallok.a");
        ss_task("kollektions.a");
        ss_task("kash.a");
        ss_task("germanstr.a");
    }));
    END;
}

/* ========================================================================= */

struct CompileData target_gen_files[] = {
    DIR("build"),

    DIR("build/common"),
    SP(CT_CDEF, "common/target_etca.cdef"),
    SP(CT_CDEF, "common/target_x86.cdef"),

    DIR("build/ir"),
    SP(CT_CDEF, "ir/ops.cdef"),
};

enum CompileResult target_gen() {
    START;
        DO(compile(LI(target_gen_files)));
    END;
}

/* ========================================================================= */

struct CompileData target_lib_files[] = {
    DEP("build/common/target_etca.cdef.o"),
    DEP("build/common/target_x86.cdef.o"),
    DEP("build/ir/ops.cdef.o"),

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
    SP(CT_C, "ir/opt/ll_sched.c"),
    SP(CT_C, "ir/opt/if_opts.c"),

    DIR("build/ir/transform"),
    SP(CT_C, "ir/transform/single_assign_conditional.c"),
    SP(CT_C, "ir/transform/single_assign.c"),
    SP(CT_C, "ir/transform/normalize.c"),
    SP(CT_C, "ir/transform/share_slots.c"),
    SP(CT_C, "ir/transform/ssair_llir_lower.c"),
    SP(CT_C, "ir/transform/cmov_expand.c"),
    SP(CT_C, "ir/transform/ll_finalize.c"),
    SP(CT_C, "ir/transform/lower_loops.c"),

    DIR("build/common"),
    SP(CT_C, "common/verify.c"),

    DIR("build/cg"),
    DIR("build/cg/x86_stupid"),
    SP(CT_C, "cg/x86_stupid/cg.c"),

    DIR("build/irparser"),
    SP(CT_C, "irparser/parser.c"),
};

enum CompileResult target_lib() {
    ONLY_IF({
        NOT_FILE("build/lib.a");
        CHANGED("ir/");
        CHANGED("ir/opt/");
        CHANGED("ir/transform/");
        CHANGED("common/");
        CHANGED("cg/x86_stupid/");
        CHANGED("irparser/");
    });

    START;
        DO(compile(LI(target_lib_files)));
        DO(linkTask(LI(target_lib_files), "build/lib.a"));
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

#define TARGETS_LEN (sizeof(targets) / sizeof(targets[0]))

int main(int argc, char **argv) {
    return build_main(argc, argv, targets, TARGETS_LEN);
}

