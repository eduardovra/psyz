#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/tempo", _SsSndTempo);
#else
void _SsSndTempo(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    s16 step;
    unsigned int target;

    if (--score->unkA8 < 0) {
        score->flags &= ~0x40;
        score->flags &= ~0x80;
        return;
    }
    step = score->unk4E;
    target = score->unkAC;
    if (step > 0) {
        if (score->unkA8 % step) {
            return;
        }
        if (target < score->unk94) {
            score->unk94--;
        } else if (score->unk94 < target) {
            score->unk94++;
        }
    } else if (target < score->unk94) {
        score->unk94 += step;
        if (score->unk94 < target) {
            score->unk94 = target;
        }
    } else if (score->unk94 < target) {
        score->unk94 -= step;
        if (target < score->unk94) {
            score->unk94 = target;
        }
    }
    score->unk54 =
        (unsigned)(score->resolution * score->unk94 * 10) / (VBLANK_MINUS * 60);
    if (score->unk54 <= 0) {
        score->unk54 = 1;
    }
    if (score->unkA8 == 0 || score->unk94 == target) {
        score->flags &= ~0x40;
        score->flags &= ~0x80;
    }
}
#endif
