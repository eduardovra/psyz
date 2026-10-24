#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_7", _SsContMainVol);
#else
void _SsContMainVol(short seq_no, short sep_no, unsigned char vol) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 channel = score->channel_idx;

    _SsVmSetVol(seq_no | (sep_no << 8), score->vab_id, score->programs[channel],
                vol, score->panpot[channel]);
    score->vol[channel] = vol;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
