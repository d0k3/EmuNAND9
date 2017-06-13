#include "common.h"
#include "draw.h"
#include "fs.h"
#include "menu.h"
#include "i2c.h"
#include "emunand9.h"

#define SUBMENU_START 1


MenuInfo menu[] =
{
    {
        #ifndef BUILD_NAME
        "EmuNAND9 Main Menu", 4,
        #else
        BUILD_NAME, 4,
        #endif
        {
            { "Complete EmuNAND Setup",       &CompleteSetupEmuNand,  0 },
            { "Complete RedNAND Setup",       &CompleteSetupEmuNand,  SD_SETUP_MINSIZE | N_WREDNAND },
            { "SD Format Options...",         NULL,                   SUBMENU_START + 0 },
            { "EmuNAND Manager Options...",   NULL,                   SUBMENU_START + 1 }
        }
    },
    {
        "SD Format Options", 4,
        {
            { "Format SD (no EmuNAND)",       &FormatSdCard,          0 },
            { "Format SD (EmuNAND default)",  &FormatSdCard,          SD_SETUP_EMUNAND },
            { "Format SD (EmuNAND minsize)",  &FormatSdCard,          SD_SETUP_EMUNAND | SD_SETUP_MINSIZE },
            { "Format SD (EmuNAND legacy)",   &FormatSdCard,          SD_SETUP_EMUNAND | SD_SETUP_LEGACY }
        }
    },
    {
        "EmuNAND Manager Options", 8,
        {
            { "Clone SysNAND to EmuNAND",     &InjectNand,            N_EMUNAND | N_DIRECTCOPY },
            { "Clone SysNAND to RedNAND",     &InjectNand,            N_EMUNAND | N_WREDNAND | N_DIRECTCOPY },
            { "Restore file to EmuNAND",      &InjectNand,            N_EMUNAND },
            { "Restore file to RedNAND",      &InjectNand,            N_EMUNAND | N_WREDNAND},
            { "Backup SysNAND to file",       &DumpNand,              0 },
            { "Backup EmuNAND to file",       &DumpNand,              N_EMUNAND },
            { "Convert EmuNAND -> RedNAND",   &ConvertEmuNand,        N_EMUNAND | N_WREDNAND },
            { "Convert RedNAND -> EmuNAND",   &ConvertEmuNand,        N_EMUNAND }
        }
    },
    {
        NULL, 0, { { 0 } }, // empty menu to signal end
    }
};


void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


void PowerOff()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while (true);
}

u8 *top_screen, *bottom_screen;

int main(int argc, char** argv)
{
    // Fetch the framebuffer addresses
    if(argc >= 2) {
        // newer entrypoints
        u8 **fb = (u8 **)(void *)argv[1];
        top_screen = fb[0];
        bottom_screen = fb[2];
    } else {
        // outdated entrypoints
        #ifdef EXEC_GATEWAY
            top_screen = (u8*)(*(u32*)((uint32_t)0x080FFFC0 + 4 * (*(u32*)0x080FFFD8 & 1)))
            bottom_screen = (u8*)(*(u32*)((uint32_t)0x080FFFD0 + 4 * (*(u32*)0x080FFFDC & 1)))
        #elif defined(EXEC_A9LH)
            top_screen = (u8*)(*(u32*)0x23FFFE00);
            bottom_screen = (u8*)(*(u32*)0x23FFFE08);
        #else
            #error "Unknown execution method"
        #endif
    }

    ClearScreenFull(true, true);
    InitFS();

    u32 menu_exit = ProcessMenu(menu, SUBMENU_START);
    
    DeinitFS();
    ClearScreenFull(true, true);
    (menu_exit == MENU_EXIT_REBOOT) ? Reboot() : PowerOff();
    return 0;
}
