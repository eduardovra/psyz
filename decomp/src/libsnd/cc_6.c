#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/cc_6", _SsContDataEntry);
#else
#define RPN_PITCH_BEND_RANGE 0

void _SsContDataEntry(short seq_no, short sep_no, unsigned char value) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 channel = score->channel_idx;
    u8 program = score->programs[channel];
    ProgAtr progAtr;
    VagAtr vagAtr = {0};
    SsNrpnAttrProc setAttr;
    int tone;

    SsUtGetProgAtr(score->vab_id, program, &progAtr);
    if (score->unk1C == 1 && score->unk15 == 0) {
        score->unk1D = value;
        score->unk1C = 0;
        score->unk15 = 1;
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        return;
    }
    if (score->unk1E == 2) {
        // Of the RPNs only the pitch bend range changes anything.
        if (score->unk19 == 0) {
            for (tone = 0; tone < progAtr.tones; tone++) {
                SsUtGetVagAtr(score->vab_id, program, tone, &vagAtr);
                if (score->unk18 == RPN_PITCH_BEND_RANGE) {
                    vagAtr.pbmin = value & 0x7F;
                    vagAtr.pbmax = value & 0x7F;
                }
                SsUtSetVagAtr(score->vab_id, program, tone, &vagAtr);
            }
        }
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        score->unk1E = 0;
        return;
    }
    if (score->unk1F == 2) {
        setAttr = (SsNrpnAttrProc)SsFCALL.ccentry[score->fn_idx];
        // The game fills only the entries it uses; PSX would jump to 0.
        if (setAttr && score->unk1B == 16) {
            for (tone = 0; tone < progAtr.tones; tone++) {
                setAttr(
                    score->vab_id, program, tone, vagAtr, score->fn_idx, value);
            }
        } else if (setAttr) {
            setAttr(score->vab_id, program, score->unk1B, vagAtr, score->fn_idx,
                    value);
        }
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        score->unk1F = 0;
        return;
    }
    score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
}
#endif
