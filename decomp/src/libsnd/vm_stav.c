#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_stav", _SsVmSelectToneAndVag);
#else
// Collects the program's tones whose key range holds the current note.
u8 _SsVmSelectToneAndVag(u8* toneIdx, u8* vagIdx) {
    VagAtr* tone;
    u8 count = 0;
    u8 i;

    for (i = 0; i < (u8)_svm_cur.prog_tones; i++) {
        tone = &_svm_tn[(u8)_svm_cur.fake_program * 16 + i];
        if ((u8)_svm_cur.note < tone->min || tone->max < (u8)_svm_cur.note) {
            continue;
        }
        vagIdx[count] = tone->vag;
        toneIdx[count] = i;
        count++;
    }
    return count;
}
#endif
