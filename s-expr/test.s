("amd64:cmov"
    (opt (
        (max_total_cmov_inline_cost 64)
        (consteval_iterations 5)
        (loop_simplify 1)
        (if_eval 1)
    ))

    (type ("int" simple (
        (float 0)
        (size 4)
        (align 2)
    )))

    (cu-block (
        ((name "add")
         (export 1))
        (block (
            (args (("int" 0) ("int" 1))) 
            (ops (
                (
                    (outs (
                        ("int" 2)))
                    (name "add")
                    (params (
                        ("a" (var 0))
                        ("b" (var 1))))
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
        ))
    ))
)
