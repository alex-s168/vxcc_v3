fn int sum(int* arr, usz len) @export 
{
    int sum = 0;
    for (usz i = 0; i < len; i ++)
    {
        sum += arr[i];
    }
    return sum;
}
