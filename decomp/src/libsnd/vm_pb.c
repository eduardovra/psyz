#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_pb", _SsVmPBVoice);
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_pb", _SsVmPitchBend);
#else
static short _SsVmPBVoice(
    short voice, short seq_sep_no, short vabId, short prog, int bend) {
    struct SpuVoice* v = &_svm_voice[voice];
    VagAtr* tone;
    int delta = (s16)(bend - 64);
    int cents;
    int semitones;
    u16 note;
    u16 fine;

    if (v->seq_sep_no != seq_sep_no || v->vabId != vabId || v->prog != prog) {
        return 0;
    }
    tone = &_svm_tn[(u16)(v->tone + (u8)_svm_cur.fake_program * 16)];
    note = v->note;
    if (delta > 0) {
        cents = delta * tone->pbmax;
        semitones = cents / 63;
        note += semitones;
        fine = (cents - semitones * 63) * 2;
    } else if (delta < 0) {
        cents = delta * tone->pbmin;
        semitones = cents / 64;
        note += semitones - 1;
        fine = (cents - semitones * 64) * 2 + 127;
    } else {
        fine = 0;
    }
    _svm_cur.voice = voice;
    _svm_cur.tone = v->tone;
    _svm_sreg_buf[voice].pitch = note2pitch2(note, fine);
    _svm_sreg_dirty[voice] |= 4;
    return 1;
}

short _SsVmPitchBend(short seq_sep_no, short vabId, short prog, u16 bend) {
    short count = 0;
    short i;

    _SsVmVSetUp(vabId, prog);
    _svm_cur.seq_sep_no = seq_sep_no;
    for (i = 0; i < _SsVmMaxVoice; i++) {
        count += _SsVmPBVoice(i, seq_sep_no, vabId, prog, bend);
    }
    return count;
}
#endif
