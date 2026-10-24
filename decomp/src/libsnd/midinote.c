#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midinote", _SsNoteOn);
#else
void _SsNoteOn(
    short seq_no, short sep_no, unsigned char note, unsigned char velocity) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 channel = score->channel_idx;
    u8 pan = score->panpot[channel];

    if (score->voll == 0) {
        return;
    }
    if (velocity) {
        if ((score->channel_mute >> channel) & 1) {
            return;
        }
        _SsVmKeyOn(seq_no | (sep_no << 8), score->vab_id,
                   score->programs[channel], note, velocity, pan);
    } else {
        _SsVmKeyOff(seq_no | (sep_no << 8), score->vab_id,
                    score->programs[channel], note);
    }
}
#endif
