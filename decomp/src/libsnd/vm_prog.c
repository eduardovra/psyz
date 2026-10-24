#include "libsnd_private.h"

#ifndef __psyz
INCLUDE_ASM("asm/nonmatchings/libsnd/vm_prog", _SsVmSetProgVol);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_prog", _SsVmGetProgVol);
#else
short _SsVmSetProgVol(short vabId, short prog, u8 vol) {
    if (_SsVmVSetUp(vabId, prog)) {
        return -1;
    }
    _svm_pg[prog].mvol = vol;
    return _svm_pg[prog].mvol;
}

short _SsVmGetProgVol(short vabId, short prog) {
    if (_SsVmVSetUp(vabId, prog)) {
        return -1;
    }
    return _svm_pg[prog].mvol;
}
#endif

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_prog", _SsVmSetProgPan);

INCLUDE_ASM("asm/nonmatchings/libsnd/vm_prog", _SsVmGetProgPan);
