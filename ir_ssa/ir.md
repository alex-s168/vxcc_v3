# VXCC IR
SSA(-like?) IR

## Block
Has inputs, outputs and operations

## Operation
Has multiple inputs and a variable amount of outputs

## Ifs
```
u32 %0 = 1;
u32 %1, u32 %2 = if cond=%0 then=(){
    ^ 4,5;
} else=(){
    ^ 8,9;
};
```
Is that weird? yes

Is that useful for optimizations? extremely

## Loops
```
for<u32> init=0 cond=(%0){ u32 %1 = lt a=%0 b=10; ^ %1; } stride=1 do=(%0){
    # something with %0
}
```
but there is also foreach and repeat
don't worry, the optimizer converts those automatically
```
repeat<u32> start=0 end=9 stride=1 do=(%0){
    # something with %0
}
```

Loops can have states.