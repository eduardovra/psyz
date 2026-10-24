#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_121", _SsContResetAll);
#else
void _SsContResetAll(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    SsUtReverbOff();
    _SsVmDamperOff();
    score->programs[score->channel_idx] = score->channel_idx;
    score->unk18 = 0;
    score->unk19 = 0;
    score->vol[score->channel_idx] = 127;
    score->panpot[score->channel_idx] = 64;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
