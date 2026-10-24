#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_0", _SsContBankChange);
#else
void _SsContBankChange(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    score->vab_id = *score->seq_ptr++;
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
