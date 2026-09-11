#include "satp_mode_validate.h"

int satp_mode_legal_rv64(unsigned mode)
{
    /*
     * The spec's SXLEN=64 MODE encodings (Table 114, section 4.1.11)
     * define exactly one scheme at value 0 (Bare) and three paged
     * schemes at the contiguous values 8, 9, 10 (Sv39, Sv48, Sv57).
     * The range test is the shape of the table itself: value 0 stands
     * alone, then 8-10, with everything between and above reserved or
     * custom-use and everything below 16 exhausted. A value above 15
     * cannot be stored in the 4-bit MODE field, so it is illegal.
     */
    return mode == 0 || (mode >= 8 && mode <= 10);
}
