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
        "SD Format Options", 2,
        {
            { "Format SD...",                 NULL,                   SUBMENU_START + 2 },
            { "Format SD & setup starter...", NULL,                   SUBMENU_START + 3 }
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
        "Format SD...", 4,
        {
            { "... without EmuNAND",          &FormatSdCard,          0 },
            { "... for EmuNAND (default)",    &FormatSdCard,          SD_SETUP_EMUNAND },
            { "... for EmuNAND (minsize)",    &FormatSdCard,          SD_SETUP_EMUNAND | SD_SETUP_MINSIZE },
            { "... for EmuNAND (legacy)",     &FormatSdCard,          SD_SETUP_EMUNAND | SD_SETUP_LEGACY }
        }
    },
    {
        "Format SD & setup starter...", 4,
        {
            { "... without EmuNAND",          &FormatSdCard,          SD_USE_STARTER },
            { "... for EmuNAND (default)",    &FormatSdCard,          SD_USE_STARTER | SD_SETUP_EMUNAND },
            { "... for EmuNAND (minsize)",    &FormatSdCard,          SD_USE_STARTER | SD_SETUP_EMUNAND | SD_SETUP_MINSIZE },
            { "... for EmuNAND (legacy)",     &FormatSdCard,          SD_USE_STARTER | SD_SETUP_EMUNAND | SD_SETUP_LEGACY }
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


int main()
{
    ClearScreenFull(true, true);
    InitFS();

    u32 menu_exit = ProcessMenu(menu, SUBMENU_START);
    
    DeinitFS();
    (menu_exit == MENU_EXIT_REBOOT) ? Reboot() : PowerOff();
    return 0;
}
