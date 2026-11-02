#include <common.h>
#include <libcd.h>
#include "libcd_stream.h"

#ifdef __psyz
void Psyz_CdInterrupts(void);
#endif

u_long StGetNext(u_long** addr, u_long** header) {
    volatile StHEADER* ptr;
    ptr = &StRingAddr[StRingIdx3];
    if (ptr->id == 1) {
        StRingIdx3 = 0;
        if (StEndFrame != 0) {
            ptr->id = 0;
        }
        ptr = &StRingAddr[StRingIdx3];
    }
    if (ptr->id == 2) {
        ptr->id = 4;
        *addr = (u_long*)(&StRingAddr[StRingSize] + (StRingIdx3 * 0x3F));
        *header = (u_long*)ptr;
        return 0;
    } else {
#ifdef __psyz
        // The drive delivers sectors while the game polls an empty ring.
        Psyz_CdInterrupts();
#endif
        return 1;
    }
}
