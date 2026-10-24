#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_91", _SsContExternal);
#else
void _SsContExternal(short seq_no, short sep_no, unsigned char depth) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    SsUtSetReverbDepth(depth, depth);
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
