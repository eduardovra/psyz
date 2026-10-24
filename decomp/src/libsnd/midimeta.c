#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midimeta", _SsGetMetaEvent);
#else
// The only meta event left to this handler is the tempo (0x51); the end of
// track is caught by _SsGetSeqData.
void _SsGetMetaEvent(short seq_no, short sep_no, unsigned char meta) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    unsigned int us_per_beat;
    unsigned int ticks;
    unsigned int frame_ticks;

    (void)meta;
    us_per_beat = *score->seq_ptr++ << 16;
    us_per_beat |= *score->seq_ptr++ << 8;
    us_per_beat |= *score->seq_ptr++;
    score->unk94 = 60000000 / (int)us_per_beat;
    ticks = score->resolution * score->unk94;
    frame_ticks = VBLANK_MINUS * 60;
    if (ticks * 10 < frame_ticks) {
        score->unk54 = score->unk52 = VBLANK_MINUS * 600 / ticks;
    } else {
        score->unk52 = -1;
        score->unk54 = ticks * 10 / frame_ticks;
        if (VBLANK_MINUS * 30 < ticks * 10 % frame_ticks) {
            score->unk54++;
        }
    }
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
