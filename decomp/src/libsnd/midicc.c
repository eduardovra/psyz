#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midicc", _SsSetControlChange);
#else
void _SsSetControlChange(short seq_no, short sep_no, unsigned char control) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 value = *score->seq_ptr++;
    SsSeqDataProc handler;

    switch (control) {
    case 0:
        score->vab_id = value;
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        return;
    case 6:
        handler = (SsSeqDataProc)SsFCALL.control[CC_DATAENTRY];
        break;
    case 7:
        handler = (SsSeqDataProc)SsFCALL.control[CC_MAINVOL];
        break;
    case 10:
        handler = (SsSeqDataProc)SsFCALL.control[CC_PANPOT];
        break;
    case 11:
        handler = (SsSeqDataProc)SsFCALL.control[CC_EXPRESSION];
        break;
    case 64:
        handler = (SsSeqDataProc)SsFCALL.control[CC_DAMPER];
        break;
    case 91:
        handler = (SsSeqDataProc)SsFCALL.control[CC_EXTERNAL];
        break;
    case 98:
        handler = (SsSeqDataProc)SsFCALL.control[CC_NRPN1];
        break;
    case 99:
        handler = (SsSeqDataProc)SsFCALL.control[CC_NRPN2];
        break;
    case 100:
        handler = (SsSeqDataProc)SsFCALL.control[CC_RPN1];
        break;
    case 101:
        handler = (SsSeqDataProc)SsFCALL.control[CC_RPN2];
        break;
    case 121:
        ((SsSeqProc)SsFCALL.control[CC_RESETALL])(seq_no, sep_no);
        return;
    default:
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        return;
    }
    handler(seq_no, sep_no, value);
}
#endif
