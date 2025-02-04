fn int max(int a, int b) @export 
{
    int res;
    if (a > b) {
        res = a;
    } else {
        res = b;
    }
    return res;
}
