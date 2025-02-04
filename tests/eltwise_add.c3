fn void eltwise_add(int* o, int* a, int* b, usz len) @export 
{
    for (usz i = 0; i < len; i ++)
    {
        o[i] = a[i] + b[i];
    }
}
