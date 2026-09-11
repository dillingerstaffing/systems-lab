#include "oracle.h"
#include "oracle_table.h"

int oracle_perm_ok(uint8_t flags, int access, int mode)
{
    unsigned nibble = ((unsigned)flags >> 1) & 0xFU; /* R,W,X,U bits */
    return (int)oracle_tbl[access][mode][nibble];
}
