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
        "EmuNAND9 Main Menu", 3,
        #else
        BUILD_NAME, 3,
        #endif
        {
            { "Complete EmuNAND Setup",       &CompleteSetupEmuNand,  0 },
            { "SD Format Options...",         NULL,                   SUBMENU_START + 0 },
            { "EmuNAND Manager Options...",   NULL,                   SUBMENU_START + 1 }
        }
    },
    {
        "SD Format Options", 4,
        {
            { "Format SD (EmuNAND)",          &FormatSdCard,          SD_SETUP_EMUNAND },
            { "Format SD (EmuNAND/auto)",     &FormatSdCard,          SD_SETUP_EMUNAND | SD_USE_STARTER },
            { "Format SD (standard)",         &FormatSdCard,          0 },
            { "Format SD (standard/auto)",    &FormatSdCard,          SD_USE_STARTER }
        }
    },
    {
        "EmuNAND Manager Options", 5,
        {
            { "Clone SysNAND to EmuNAND",     &CloneSysNand,          N_EMUNAND | N_DIRECTCOPY },
            { "Clone NAND.bin to EmuNAND",    &InjectNand,            N_EMUNAND },
            { "Clone EmuNAND.bin to EmuNAND", &InjectNand,            N_EMUNAND | N_EMUNANDBIN },
            { "Dump SysNAND to NAND.bin",     &DumpNand,              0 },
            { "Dump EmuNAND to EmuNAND.bin",  &DumpNand,              N_EMUNAND }
        }
    },
    {
        NULL, 0, {}, // empty menu to signal end
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


int main()
{
    ClearScreenFull(true, true);
    InitFS();

    u32 menu_exit = ProcessMenu(menu, SUBMENU_START);
    
    DeinitFS();
    (menu_exit == MENU_EXIT_REBOOT) ? Reboot() : PowerOff();
    return 0;
}
