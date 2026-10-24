#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_100", _SsContRpn1);
#else
void _SsContRpn1(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    score->unk18 = value;
    score->unk1E++;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
