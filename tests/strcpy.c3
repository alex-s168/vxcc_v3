fn void strcpy(char* dest, char* src) @export 
{
    for (usz i = 0; src[i] != '\0'; i ++)
        dest[i] = src[i];
}
