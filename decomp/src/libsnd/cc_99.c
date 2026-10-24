#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_99", _SsContNrpn2);
#else
#define NRPN_LOOP_START 20
#define NRPN_LOOP_END 30
#define NRPN_LOOP_INFINITE 127

void _SsContNrpn2(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    switch (value) {
    case NRPN_LOOP_START:
        score->unk1B = value;
        score->unk1C = 1;
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        score->next_sep_pos = score->seq_ptr;
        break;
    case NRPN_LOOP_END:
        score->unk1B = value;
        if (score->unk1D == 0) {
            score->unk15 = 0;
            score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        } else if (score->unk1D < NRPN_LOOP_INFINITE) {
            score->unk1D--;
            score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
            if (score->unk1D) {
                score->seq_ptr = score->next_sep_pos;
            } else {
                score->unk15 = 0;
            }
        } else {
            _SsReadDeltaValue(seq_no, sep_no);
            score->delta_value = 0;
            score->seq_ptr = score->next_sep_pos;
        }
        break;
    default:
        score->unk1B = value;
        score->unk1F++;
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        break;
    }
}
#endif
