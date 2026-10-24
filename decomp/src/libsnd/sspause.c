#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", _SsSndSetPauseMode);
INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", SsSeqPause);
INCLUDE_ASM("asm/nonmatchings/libsnd/sspause", SsSepPause);
#else
static void _SsSndSetPauseMode(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    _SsVmGetSeqVol(seq_no | (sep_no << 8), &score->unk5C, &score->unk5E);
    score->flags &= ~1;
    score->flags &= ~8;
    score->flags |= 2;
}

void SsSeqPause(short seq_access_num) { _SsSndSetPauseMode(seq_access_num, 0); }

void SsSepPause(short sep_access_num, short seq_num) {
    _SsSndSetPauseMode(sep_access_num, seq_num);
}
#endif
