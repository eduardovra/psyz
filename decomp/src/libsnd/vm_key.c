#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmKeyOn);
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmKeyOff);
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmSeKeyOn);
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", _SsVmSeKeyOff);
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_key", KeyOnCheck);
#else
#define SE_SEQ_SEP_NO 33

int _SsVmKeyOn(
    short seq_sep_no, short vabId, short prog, u16 note, u16 voll, u16 pan) {
    struct SeqStruct* score =
        &_ss_score[seq_sep_no & 0xFF][(seq_sep_no >> 8) & 0xFF];
    struct SpuVoice* voice;
    VagAtr* tone;
    u8 vagIdx[128];
    u8 toneIdx[128];
    u8 count;
    u8 i;
    int keyed = 0;

    if (_SsVmVSetUp(vabId, prog)) {
        return -1;
    }
    _svm_cur.seq_sep_no = seq_sep_no;
    _svm_cur.note = note;
    _svm_cur.fine = 0;
    if (seq_sep_no == SE_SEQ_SEP_NO) {
        _svm_cur.volume = voll;
    } else {
        _svm_cur.volume = voll * score->vol[score->channel_idx] / 127;
    }
    _svm_cur.pan = pan;
    _svm_cur.mvol = _svm_pg[prog].mvol;
    _svm_cur.mpan = _svm_pg[prog].mpan;
    _svm_cur.prog_tones = _svm_pg[prog].tones;
    if ((u8)_svm_cur.fake_program >= _svm_vh->ps) {
        return -1;
    }
    if (voll == 0) {
        return _SsVmKeyOff(seq_sep_no, vabId, prog, note);
    }
    count = _SsVmSelectToneAndVag(toneIdx, vagIdx);
    for (i = 0; i < count; i++) {
        _svm_cur.tone_vag_idx = vagIdx[i];
        _svm_cur.tone = toneIdx[i];
        tone = &_svm_tn[(u8)_svm_cur.fake_program * 16 + toneIdx[i]];
        _svm_cur.tone_prior = tone->prior;
        _svm_cur.tone_vol = tone->vol;
        _svm_cur.tone_pan = tone->pan;
        _svm_cur.tone_center = tone->center;
        _svm_cur.tone_shift = tone->shift;
        _svm_cur.tone_mode = tone->mode;
        _svm_cur.tone_min = tone->min;
        _svm_cur.tone_max = tone->max;
        _svm_cur.voice = (u8)_SsVmAlloc(0);
        if (_svm_cur.voice >= _SsVmMaxVoice) {
            keyed = -1;
            continue;
        }
        voice = &_svm_voice[_svm_cur.voice];
        voice->unk1b = 1;
        voice->unk2 = 0;
        voice->seq_sep_no = seq_sep_no;
        voice->vabId = (u8)_svm_cur.vabId;
        voice->fake_program = (u8)_svm_cur.fake_program;
        voice->prog = prog;
        if (seq_sep_no != SE_SEQ_SEP_NO) {
            voice->voll1 = voll;
        }
        voice->pan = pan;
        voice->tone = (u8)_svm_cur.tone;
        voice->note = note;
        voice->priority = (u8)_svm_cur.tone_prior;
        voice->vag_idx = _svm_cur.tone_vag_idx;
        _SsVmDoAllocate();
        if (_svm_cur.tone_vag_idx == 0xFF) {
            vmNoiseOn(_svm_cur.voice);
        } else {
            _SsVmKeyOnNow(count, note2pitch());
        }
        keyed |= 1 << _svm_cur.voice;
    }
    return keyed;
}

int _SsVmKeyOff(short seq_sep_no, short vabId, short prog, u16 note) {
    int count = 0;
    u8 i;

    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].note != note || _svm_voice[i].prog != prog ||
            _svm_voice[i].seq_sep_no != seq_sep_no ||
            _svm_voice[i].vabId != vabId) {
            continue;
        }
        if (_svm_voice[i].vag_idx == 0xFF) {
            vmNoiseOff(i);
        } else {
            _svm_cur.voice = i;
            _SsVmKeyOffNow(0);
        }
        count++;
    }
    return count;
}

void _SsVmSeKeyOn(
    short vabId, short prog, u16 note, int arg3, u16 voll, u16 volr) {
    u16 vol;
    u16 pan;

    (void)arg3;
    if (voll == volr) {
        pan = 64;
        vol = voll;
    } else if (volr < voll) {
        vol = voll;
        pan = (volr << 6) / voll;
    } else {
        vol = volr;
        pan = 127 - (voll << 6) / volr;
    }
    _SsVmKeyOn(SE_SEQ_SEP_NO, vabId, prog, note, vol, pan);
}

void _SsVmSeKeyOff(short vabId, short prog, u16 note) {
    _SsVmKeyOff(SE_SEQ_SEP_NO, vabId, prog, note);
}

void KeyOnCheck(void) {}
#endif
