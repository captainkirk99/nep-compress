#include <nep_compress.h>

int main(void)
{
    return LZ4_ID != 32004 || BZIP2_ID != 307;
}
