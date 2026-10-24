#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsSeqPlay);

INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsSeqGetEof);

INCLUDE_ASM("asm/nonmatchings/libsnd/midiread", _SsGetSeqData);
#else
int _SsGetSeqData(short seq_no, short sep_no);

void _SsSeqPlay(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    int ticks;

    if (score->delta_value - score->unk54 > 0) {
        if (score->unk52 > 0) {
            score->unk52--;
        } else if (score->unk52 == 0) {
            score->unk52 = score->unk54;
            score->delta_value--;
        } else {
            score->delta_value -= score->unk54;
        }
    } else if (score->delta_value <= score->unk54) {
        ticks = score->delta_value;
        do {
            do {
                _SsGetSeqData(seq_no, sep_no);
            } while (score->delta_value == 0);
            ticks += score->delta_value;
        } while (ticks < score->unk54);
        score->delta_value = ticks - score->unk54;
    }
}

void _SsSeqGetEof(short seq_no, short sep_no, u8 meta) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8* restart;

    (void)meta;
    score->unk21++;
    restart = (score->flags & 0x400) ? score->loop_pos : score->read_pos;
    if (score->l_count == 0 || score->unk21 < score->l_count) {
        score->unk88 = 0;
        score->unk1C = 0;
        score->delta_value = 0;
        score->seq_ptr = restart;
        if (score->l_count != 0) {
            score->next_sep_pos = restart;
        }
        return;
    }
    score->flags &= ~1;
    score->flags &= ~8;
    score->flags &= ~2;
    score->flags |= 0x200;
    score->flags |= 4;
    score->play_mode = 0;
    score->next_sep_pos = restart;
    if (score->unk22 != 0xFF) {
        score->play_mode = 0;
        _SsSndNextSep(score->unk22, score->unk23);
        _SsVmSeqKeyOff(seq_no | (sep_no << 8));
    }
    _SsVmSeqKeyOff(seq_no | (sep_no << 8));
    score->delta_value = score->unk54;
}

// Returns 1 at the end of the track.
int _SsGetSeqData(short seq_no, short sep_no) {
    struct SeqStruct* score = &_ss_score[seq_no][sep_no];
    u8 status;
    u8 note;
    u8 velocity;
    u8 data;

    status = *score->seq_ptr++;
    if ((score->flags & 0x401) == 0x401 &&
        score->seq_ptr == (u8*)(uintptr_t)score->unk10 + 1) {
        _SsSeqGetEof(seq_no, sep_no, ((u8*)(uintptr_t)score->unk10)[1]);
        return -1;
    }
    if (status & 0x80) {
        score->channel_idx = status & 0xF;
        switch (status & 0xF0) {
        case 0x90:
            score->running_status = 0x90;
            note = *score->seq_ptr++;
            velocity = *score->seq_ptr++;
            score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
            ((SsNoteOnProc)SsFCALL.noteon)(seq_no, sep_no, note, velocity);
            break;
        case 0xB0:
            score->running_status = 0xB0;
            ((SsSeqDataProc)SsFCALL.control[CC_NUMBER])(
                seq_no, sep_no, *score->seq_ptr++);
            break;
        case 0xC0:
            score->running_status = 0xC0;
            ((SsSeqDataProc)SsFCALL.programchange)(
                seq_no, sep_no, *score->seq_ptr++);
            break;
        case 0xE0:
            score->running_status = 0xE0;
            score->seq_ptr++;
            ((SsSeqProc)SsFCALL.pitchbend)(seq_no, sep_no);
            break;
        case 0xF0:
            score->running_status = 0xFF;
            score->channel_idx = status & 0xF;
            data = *score->seq_ptr++;
            if (data == 0x2F) {
                _SsSeqGetEof(seq_no, sep_no, data);
                return 1;
            }
            ((SsSeqDataProc)SsFCALL.metaevent)(seq_no, sep_no, data);
            break;
        }
        return 0;
    }
    switch (score->running_status) {
    case 0x90:
        velocity = *score->seq_ptr++;
        score->delta_value = _SsReadDeltaValue(seq_no, sep_no);
        ((SsNoteOnProc)SsFCALL.noteon)(seq_no, sep_no, status, velocity);
        break;
    case 0xB0:
        ((SsSeqDataProc)SsFCALL.control[CC_NUMBER])(seq_no, sep_no, status);
        break;
    case 0xC0:
        ((SsSeqDataProc)SsFCALL.programchange)(seq_no, sep_no, status);
        break;
    case 0xE0:
        ((SsSeqProc)SsFCALL.pitchbend)(seq_no, sep_no);
        break;
    case 0xFF:
        if (status == 0x2F) {
            _SsSeqGetEof(seq_no, sep_no, status);
            return 1;
        }
        ((SsSeqDataProc)SsFCALL.metaevent)(seq_no, sep_no, status);
        break;
    }
    return 0;
}
#endif
