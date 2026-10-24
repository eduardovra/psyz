#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_101", _SsContRpn2);
#else
void _SsContRpn2(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    score->unk19 = value;
    score->unk1E++;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
