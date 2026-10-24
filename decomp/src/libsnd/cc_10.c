#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_10", _SsContPanpot);
#else
void _SsContPanpot(short seq_no, short sep_no, unsigned char pan) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 channel = score->channel_idx;

    _SsVmSetVol(seq_no | (sep_no << 8), score->vab_id, score->programs[channel],
                score->vol[channel], pan);
    score->panpot[channel] = pan;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
