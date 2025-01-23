(cu "amd64:cmov,sse"
    (opt
        (max_total_cmov_inline_cost 64)
        (consteval_iterations 5)
        (loop_simplify 1)
        (if_eval 1)
    )

    (type "int" (simple integer
        (size 4)
        (align 2)
    ))

    (ir-block
        ((name "add")
         (export 1))
        (block 
            (args (("int" 0) ("int" 1))) 
            (ops (
                (
                    (outs (
                        ("int" 2)))
                    (name "add")
                    (params (
                        ("operand_a" (var 0))
                        ("operand_b" (var 1))))
                    (args ())
                )
                (
                    (outs (
                        ("int" 3)))
                    (name "imm")
                    (params (
                        ("val" (var 2))))
                    (args ())
                )
            ))
            (rets (3))
        )
    )
)
