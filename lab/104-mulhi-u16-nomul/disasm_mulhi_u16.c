#include "mulhi_u16.h"

__attribute__((noinline)) uint16_t wrap_mulhi_u16(uint16_t a, uint16_t b)
{
    return mulhi_u16(a, b);
}
