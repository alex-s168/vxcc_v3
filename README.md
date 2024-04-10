# VXCC (v3)
How is this different from VXCC v1 and v2?

Int vxcc-opt, I started with the IR but it was too high level.
In v1, I started with macro expansion codegen but unsuccessfully because the structure was messed up.
In v2, I did the same thing in Kotin and got reasonably far, but not having an SSA IR was very limiting.
So here I started again but with an SSA and with the optimizations firsts.