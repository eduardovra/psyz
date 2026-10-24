#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_98", _SsContNrpn1);
#else
void _SsContNrpn1(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    SndSsMarkCallbackProc callback;

    if (score->unk1C == 1 && score->unk15 == 0) {
        score->unk1D = value;
        score->unk1C = 0;
        score->unk15 = 1;
    } else if (score->unk1B != 30 && score->unk1B != 20) {
        score->fn_idx = value;
        score->unk1C = 0;
        score->unk1F++;
    }
    if (score->unk1B == 40) {
        callback = _SsMarkCallback[seq_no][sep_no];
        if (callback) {
            callback(seq_no, sep_no, value);
        }
    }
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
