fn int assignments(int y) @export 
{
    int x = 0;
    x += y;
    x -= y;
    x %= y;
    x /= y;
    x *= y;
    x <<= y;
    x >>= y;
    x |= y;
    x &= y;
    return (int) x;
}

