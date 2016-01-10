#pragma once

#include "common.h"
#ifdef USE_THEME
#include "draw.h"

#define GFX_PROGRESS  "progress.bin"
#define GFX_DONE      "done.bin"
#define GFX_FAILED    "failed.bin"
#define GFX_UNMOUNT   "unmount.bin"
#define GFX_DANGER_E  "danger_e.bin"
#define GFX_DANGER_S  "danger_s.bin"
#define GFX_DEBUG_BG  "debug_bg.bin"
#define GFX_LOGO      "logo.bin"

#define COLOR_RED           RGB(0xFF, 0x00, 0x00)
#define COLOR_GREEN         RGB(0x00, 0xFF, 0x00)
#define COLOR_BLUE          RGB(0xFF, 0x00, 0xFF)
#define COLOR_GREY          RGB(0x77, 0x77, 0x77)
#define COLOR_PURPLE        RGB(0x66, 0x00, 0xFF)

#define LOGO_TOP        true
#define LOGO_TEXT_X     10
#define LOGO_TEXT_Y     SCREEN_HEIGHT - 10
#define LOGO_COLOR_BG   COLOR_TRANSPARENT
#define LOGO_COLOR_FONT COLOR_WHITE

#define STD_COLOR_BG   LOGO_COLOR_BG
#define STD_COLOR_FONT LOGO_COLOR_FONT

#define DBG_COLOR_BG   COLOR_BLACK
#define DBG_COLOR_FONT COLOR_WHITE

#define DBG_START_Y 10
#define DBG_END_Y   (SCREEN_HEIGHT - 10)
#define DBG_START_X 10
#define DBG_END_X   (SCREEN_WIDTH_TOP - 10)
#define DBG_STEP_Y  10

void LoadThemeGfx(const char* filename, bool use_top);
void LoadThemeGfxMenu(u32 index);
void LoadThemeGfxLogo(void);
#endif
