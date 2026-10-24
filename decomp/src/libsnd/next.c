#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/next", _SsSndNextSep);
#else
void _SsSndNextSep(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];

    score->l_count = 1;
    score->unk21 = 0;
    score->flags &= ~0x100;
    score->flags &= ~8;
    score->flags &= ~2;
    score->flags &= ~4;
    score->flags &= ~0x200;
    score->play_mode = 1;
    score->seq_ptr = score->read_pos;
    score->flags |= 1;
}
#endif
