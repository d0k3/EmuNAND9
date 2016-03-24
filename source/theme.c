#include "theme.h"
#ifdef USE_THEME
#include "emunand9.h"
#include "fs.h"

bool ImportFrameBuffer(const char* path, u32 use_top) {
    u32 bufsize = BYTES_PER_PIXEL * SCREEN_HEIGHT * ((use_top) ? SCREEN_WIDTH_TOP : SCREEN_WIDTH_BOT);
    u8* buffer0 = (use_top) ? TOP_SCREEN0 : BOT_SCREEN0;
    u8* buffer1 = (use_top) ? TOP_SCREEN1 : BOT_SCREEN1;
    bool result;
    
    if (!FileOpen(path)) return false;
    result = FileRead(buffer0, bufsize, 0);
    memcpy(buffer1, buffer0, bufsize);
    FileClose();
    
    return result;
}

void LoadThemeGfx(const char* filename, bool use_top) {
    char path[256];
    #ifdef APP_TITLE
    snprintf(path, 256, "//3ds/%s/UI/%s", APP_TITLE, filename);
    if (ImportFrameBuffer(path, use_top)) return;
    #endif
    snprintf(path, 256, "//%s/%s", USE_THEME, filename);
    if (!ImportFrameBuffer(path, use_top))
        DrawStringF(10, 230, true, "Not found: %s", filename);
}

void LoadThemeGfxMenu(u32 index) {
    char filename[16];
    snprintf(filename, 16, "menu%04lu.bin", index);
    LoadThemeGfx(filename, !LOGO_TOP); // this goes where the logo goes not
}

void LoadThemeGfxLogo(void) {
    u32 emunand_state = CheckEmuNand();
    LoadThemeGfx(GFX_LOGO, LOGO_TOP);
    #if defined LOGO_TEXT_X && defined LOGO_TEXT_Y
    if (CheckFS()) {
        DrawStringF(LOGO_TEXT_X, LOGO_TEXT_Y -  0, LOGO_TOP, "SD storage: %lluMB / %lluMB", RemainingStorageSpace() / (1024*1024), TotalStorageSpace() / (1024*1024));
    } else {
        DrawStringF(LOGO_TEXT_X, LOGO_TEXT_Y -  0, LOGO_TOP, "SD storage: unknown filesystem");
    }
    DrawStringF(LOGO_TEXT_X, LOGO_TEXT_Y - 10, LOGO_TOP, "EmuNAND: %s",
        (emunand_state == RES_EMUNAND_NOT_READY) ? "SD not ready" :
        (emunand_state == RES_EMUNAND_GATEWAY) ? "GW EmuNAND" : 
        (emunand_state == RES_EMUNAND_REDNAND) ? "RedNAND" : "SD is ready" );
    #ifdef WORK_DIR
    if (DirOpen(WORK_DIR)) {
        DrawStringF(LOGO_TEXT_X, LOGO_TEXT_Y - 20, LOGO_TOP, "Work directory: %s", WORK_DIR);
        DirClose();
    }
    #endif
    #endif
}
#endif
