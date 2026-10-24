#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midiprog", _SsSetProgramChange);
#else
void _SsSetProgramChange(short seq_no, short sep_no, unsigned char program) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    score->programs[score->channel_idx] = program;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
