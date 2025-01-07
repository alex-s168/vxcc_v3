## Step 1: create a `vx_CU`
A compilation unit is a collection of code blocks and data blocks that create an object file together.

First you need to create a CU by using `void vx_CU_init(vx_CU* dest, const char * targetStr);`.
For that, you need a target string.
example target strings:
- `amd64`
- `amd64:aes,avx2`

If you need your CU to be heap allocated (which you probably need to do), use `fastalloc()`.
After that, you can configure some optimization settings in `vx_CU->opt`.

Code:
```c 
vx_CU* cu = fastalloc(sizeof(vx_CU));
vx_CU_init(cu, "amd64");
// some configurations (don't copy these!)
cu->opt.max_total_cmov_inline_cost = 4;
```

## Step 2: create types
Code:
```c 
vx_IrType* i32 = vx_IrType_heap();
i32->kind = VX_IR_TYPE_KIND_BASE;
i32->debugName = "i32";

i32->base.sizeless = false;
i32->base.size = 4;
i32->base.align = 2;
i32->base.isfloat = false;
```

## Step 3: add data blocks
data blocks can be constant strings, static arrays, ...

Code:
```c 
vx_Data* data = fastalloc(sizeof(vx_Data));
data->name = "my_array";
data->numel = 20;
data->elty = some_type;
data->comptime_elt_size = vx_IrType_size(some_type);
data->data = some_data;
data->writable = true;

vx_CUBlock* cb = vx_CU_addBlock(cu);
cb->type = VX_CU_BLOCK_DATA;
cb->v.data = data;
cb->do_export = false;
```

## Step 4: add code blocks 
A IrBlock has input values (with types), output values, and some `IrOp`s.

```c 
vx_IrBlock* block = vx_IrBlock_initHeap(NULL, NULL); // the root block (of a function) does not have parents

// add some inputs:
vx_IrVar var = vx_IrBlock_newVar(block, NULL); // NULL because there is no declaration (because it's in the args) 
vx_IrBlock_addIn(block, var, some_type);

// add some operations:
vx_IrOp* op1 = vx_IrBlock_addOpBuilding(block);
vx_IrOp_init(op1, VX_IR_OP_IMM, block);
vx_IrOp_addParam_s(op1, VX_IR_NAME_VAL, VX_IR_VAL_IMM_INT(1));
vx_IrVar out = vx_IrBlock_newVar(block, op1);
vx_IrOp_addOut(out, some_type);

// and some outputs:
vx_IrBlock_addOut(block, out);

// only do these for root blocks:
block->name = "main";
vx_CU_addIrBlock(cu, block, true);
```

## Step 5: add operations
A operation has parameters, arguments and outputs.
Parameters are named arguments.
Arguments and parameters are just `vx_IrValue`s, which can be created with `VX_IR_VAL_VAR()`, `VX_IR_VAL_IMM_INT()`, and others.

You can find a list of all operations and their parameters in `ir/ops.cdef`.

Exmaple: `VX_IR_OP_ADD` takes in a `VX_IR_NAME_OPERAND_A` and a `VX_IR_NAME_OPERAND_B` argument.

Note that for some arithmetic operations, there are signed and unsigned variants.

In the frontend, the IR does not have to be SSA, because there is some code that converts multiple assignments to the same variable 
(even in a loop or if body), to SSA. You should never (on purpose) store and load from local variables, unless there is no alternative.

To get a pointer to local variables, use `VX_IR_OP_PLACE`. But avoid using that if not neccesarry.

To return values, you can also use `VX_IR_OP_RETURN`, but doing that is not recommended.

You should never use these operations:
- `EA`: effective address computation: might not be supported by all backends. automatically generated if possible.
- `CMOV`: conditional select value:    ^^^
- `GOTO`, `COND`: using these disables a lot of optimizations.
- `TAILCALL`: just always use `CALL` instead

Structs are currently not handled by VXCC, which means that like in LLVM, you have to take care of struct calling conventions yourself (if you need them).
You don't need those for most (if not all) C standard library functions, because they only apply if you pass or return a struct by value.
If you don't need them for linking with existing C code, you can just "invent" your own calling convention for structs, like passing all members as arguments.

## Step 6: compile the CU 
See `vx_CU_compile()`. Note however, that object file output currently requires `nasm` in the `PATH`.
