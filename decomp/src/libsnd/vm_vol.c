#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_vol", _SsVmSetVol);
#else
static unsigned int ApplyPan(unsigned int vol, u8 pan, int right) {
    if (right ? pan < 64 : pan >= 64) {
        return vol * (right ? pan : 127 - pan) / 63;
    }
    return vol;
}

short _SsVmSetVol(short seq_sep_no, short vabId, short prog, u16 vol, u16 pan) {
    struct SeqStruct* score =
        &_ss_score[seq_sep_no & 0xFF][(seq_sep_no & 0xFF00) >> 8];
    VagAtr* tone;
    unsigned int base;
    unsigned int voll;
    unsigned int volr;
    short count = 0;
    short i;
    u8 mpan;

    _SsVmVSetUp(vabId, prog);
    _svm_cur.seq_sep_no = seq_sep_no;
    if (pan == 0) {
        pan = 1;
    }
    if (vol == 0) {
        vol = 1;
    }
    for (i = 0; i < _SsVmMaxVoice; i++) {
        if (_svm_voice[i].seq_sep_no != seq_sep_no ||
            _svm_voice[i].prog != prog || _svm_voice[i].vabId != vabId) {
            continue;
        }
        if (score->vol[score->channel_idx] != vol &&
            score->vol[score->channel_idx] == 0) {
            score->vol[score->channel_idx] = 1;
        }
        base = _svm_voice[i].voll1 * vol / 127;
        base = (int)(base * (_svm_vh->mvol * 0x3FFF)) / (127 * 127);
        tone = &_svm_tn[_svm_voice[i].fake_program * 16 + _svm_voice[i].tone];
        base = base * _svm_pg[prog].mvol * tone->vol / (127 * 127);
        voll = base * score->voll / 127;
        volr = base * score->volr / 127;

        voll = ApplyPan(voll, tone->pan, 0);
        volr = ApplyPan(volr, tone->pan, 1);
        mpan = _svm_pg[_svm_voice[i].prog].mpan;
        voll = ApplyPan(voll, mpan, 0);
        volr = ApplyPan(volr, mpan, 1);
        voll = ApplyPan(voll, (u8)pan, 0);
        volr = ApplyPan(volr, (u8)pan, 1);
        if (_svm_stereo_mono == 1) {
            if (voll < volr) {
                voll = volr;
            } else {
                volr = voll;
            }
        }
        _svm_sreg_buf[i].volume.left = voll * voll / 0x3FFF;
        _svm_sreg_buf[i].volume.right = volr * volr / 0x3FFF;
        _svm_sreg_dirty[i] |= 3;
        count++;
    }
    return count;
}
#endif
