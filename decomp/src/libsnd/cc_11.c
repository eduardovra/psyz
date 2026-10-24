#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_11", _SsContExpression);
#else
void _SsContExpression(short seq_no, short sep_no, unsigned char expression) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 channel = score->channel_idx;

    _SsVmSetProgVol(score->vab_id, score->programs[channel], expression);
    _SsVmSetVol(seq_no | (sep_no << 8), score->vab_id, score->programs[channel],
                score->vol[channel], score->panpot[channel]);
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
