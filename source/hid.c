#include "hid.h"
#include "draw.h"

u32 InputWait() {
    u32 pad_state_old = HID_STATE;
    while (true) {
        u32 pad_state = HID_STATE;
        if (pad_state ^ pad_state_old)
            return ~pad_state;
    }
}

u32 CheckSequence(const u32* sequence, u32 len) {
    u32 pad_state;
    u32 lvl = 0;
    while (true) {
        ShowProgress(lvl, len);
        if (lvl == len)
            break;
        pad_state = InputWait();
        if (!(pad_state & BUTTON_ANY))
            continue;
        else if (pad_state & sequence[lvl])
            lvl++;
        else if (pad_state & BUTTON_B)
            break;
        else if (lvl == 0 || !(pad_state & sequence[lvl-1]))
            lvl = 0;
    }
    ShowProgress(0, 0);
    if (lvl < len)
        return pad_state;
    return 0;
}
