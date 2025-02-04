fn void colwise_reduce_sum(int* dest, int* a, usz w, usz h) @export 
{
    for (usz col = 0; col < w; col ++)
    {
        int sum = 0;
        for (usz row = 0; row < h; row ++)
        {
            sum += a[col * w + row];
        }
        dest[col] = sum;
    }
}
