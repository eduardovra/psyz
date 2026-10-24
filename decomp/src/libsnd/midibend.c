#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midibend", _SsSetPitchBend);
#else
void _SsSetPitchBend(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 bend = *score->seq_ptr++;

    _SsVmPitchBend(seq_no | (sep_no << 8), score->vab_id,
                   score->programs[score->channel_idx], bend);
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
