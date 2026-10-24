#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_64", _SsContDamper);
#else
void _SsContDamper(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    if (value < 64) {
        _SsVmDamperOff();
    } else {
        _SsVmDamperOn();
    }
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
