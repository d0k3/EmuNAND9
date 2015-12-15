#include "fs.h"
#include "draw.h"
#include "hid.h"
#include "emunand9.h"
#include "fatfs/sdmmc.h"

#define BUFFER_ADDRESS  ((u8*) 0x21000000)
#define BUFFER_MAX_SIZE (8 * 1024 * 1024)

#define NAND_SECTOR_SIZE 0x200
#define SECTORS_PER_READ (BUFFER_MAX_SIZE / NAND_SECTOR_SIZE)

#define SD_MINFREE_SECTORS  ((1024 * 1024 * 1024) / 0x200)  // have at least 1GB free
#define PARTITION_ALIGN     ((4 * 1024 * 1024) / 0x200)     // align at 4MB
#define MAX_STARTER_SIZE    (16 * 1024 * 1024)              // allow to take over max 16MB boot.3dsx

static u32 emunand_header = 0;
static u32 emunand_offset = 0;

u32 unlockSequence[] = { BUTTON_DOWN, BUTTON_UP, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_A };
const char* unlockText = "<Down>, <Up>, <Left>, <Right>, <A>";

u32 CheckEmuNand(void)
{
    u8* buffer = (u8*) 0x20316200;
    u32 nand_size_sectors = getMMCDevice(0)->total_size;
    
    // also set the EmuNAND header / offset here
    emunand_header = 0;
    emunand_offset = 0;
    
    // check the MBR for presence of EmuNAND
    sdmmc_sdcard_readsectors(0, 1, buffer);
    if (nand_size_sectors > getle32(buffer + 0x1BE + 0x8))
        return RES_EMUNAND_NOT_READY;
    
    // check for Gateway type EmuNAND
    sdmmc_sdcard_readsectors(nand_size_sectors, 1, buffer);
    if (memcmp(buffer + 0x100, "NCSD", 4) == 0) {
        emunand_header = nand_size_sectors;
        emunand_offset = 0;
        return RES_EMUNAND_GATEWAY;
    }
    
    // check for RedNAND type EmuNAND
    sdmmc_sdcard_readsectors(1, 1, buffer);
    if (memcmp(buffer + 0x100, "NCSD", 4) == 0) {
        emunand_header = 1;
        emunand_offset = 1;
        return RES_EMUNAND_REDNAND;
    }
        
    // EmuNAND ready but not set up
    return RES_EMUNAND_READY;
}

static inline int ReadNandSectors(u32 sector_no, u32 numsectors, u8 *out, bool use_emunand)
{
    if (use_emunand) {
        if (!emunand_header) CheckEmuNand();
        if (sector_no == 0) {
            int errorcode = sdmmc_sdcard_readsectors(emunand_header, 1, out);
            if (errorcode) return errorcode;
            sector_no = 1;
            numsectors--;
            out += 0x200;
        }
        return sdmmc_sdcard_readsectors(sector_no + emunand_offset, numsectors, out);
    } else return sdmmc_nand_readsectors(sector_no, numsectors, out);
}

static inline int WriteNandSectors(u32 sector_no, u32 numsectors, u8 *in, bool use_emunand)
{
    if (use_emunand) {
        if (emunand_header <= 1) {
            emunand_header = getMMCDevice(0)->total_size;
            emunand_offset = 0;
        }
        if (sector_no == 0) {
            int errorcode = sdmmc_sdcard_writesectors(emunand_header, 1, in);
            if (errorcode) return errorcode;
            sector_no = 1;
            numsectors--;
            in += 0x200;
        }
        return sdmmc_sdcard_writesectors(sector_no + emunand_offset, numsectors, in);
    } else return 0; // return sdmmc_nand_writesectors(sector_no, numsectors, in);
    // SysNAND write stubbed - not needed here, no need to take the risk
}

u32 DumpNand(u32 param)
{
    u8* buffer = BUFFER_ADDRESS;
    u32 nand_size = getMMCDevice(0)->total_size * NAND_SECTOR_SIZE;
    bool use_emunand = (param & N_EMUNAND);
    u32 result = 0;

    if (CheckEmuNand() <= RES_EMUNAND_READY) {
        Debug("EmuNAND not found on SD card");
        return 1;
    }
    
    Debug("Dumping %sNAND (%uMB) to file...", (use_emunand) ? "Emu" : "Sys", nand_size / (1024 * 1024));

    if (!DebugFileCreate((use_emunand) ? "EmuNAND.bin" : "NAND.bin", true))
        return 1;

    u32 n_sectors = nand_size / NAND_SECTOR_SIZE;
    for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
        u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
        ShowProgress(i, n_sectors);
        ReadNandSectors(i, read_sectors, buffer, use_emunand);
        if(!DebugFileWrite(buffer, NAND_SECTOR_SIZE * read_sectors, i * NAND_SECTOR_SIZE)) {
            result = 1;
            break;
        }
    }

    ShowProgress(0, 0);
    FileClose();

    return result;
}

u32 InjectNand(u32 param)
{
    u8* buffer = BUFFER_ADDRESS;
    u32 nand_size = getMMCDevice(0)->total_size * NAND_SECTOR_SIZE;
    bool use_emunand = (param & N_EMUNAND); // we won't inject to SysNAND
    u32 result = 0;
    u8 magic[4];
    
    if (use_emunand && (!(param & N_NOCONFIRM))) switch(CheckEmuNand()) {
        case RES_EMUNAND_NOT_READY:
            Debug("SD card is not formatted for EmuNAND");
            Debug("Format it first using the Format Menu");
            return 1;
        case RES_EMUNAND_READY:
            break;
        default:
            Debug("There is already an EmuNAND on this SD");
            Debug("If you continue, it will be overwritten");
            Debug("If you wish to proceed, enter:");
            Debug(unlockText);
            Debug("(B to cancel)");
            if (CheckSequence(unlockSequence, sizeof(unlockSequence) / sizeof(u32)) != 0)
                return 2;
            Debug("");
    }
        
    if (!DebugFileOpen((param & N_EMUNANDBIN) ? "EmuNAND.bin" :"NAND.bin"))
        return 1;
    if (nand_size != FileGetSize()) {
        FileClose();
        Debug("NAND dump has the wrong size!");
        return 1;
    };
    if(!DebugFileRead(magic, 4, 0x100)) {
        FileClose();
        return 1;
    }
    if (memcmp(magic, "NCSD", 4) != 0) {
        FileClose();
        Debug("Not a proper NAND dump!");
        return 1;
    }
    
    u32 n_sectors = nand_size / NAND_SECTOR_SIZE;
    if (param & N_DIRECTCOPY) {
        Debug("Cloning SysNAND to EmuNAND (%uMB)", nand_size / (1024 * 1024));
        for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
            u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
            ShowProgress(i, n_sectors);
            ReadNandSectors(i, read_sectors, buffer, false);
            WriteNandSectors(i, read_sectors, buffer, true);
        }
    } else {
        Debug("Injecting file to %sNAND (%uMB)...", (use_emunand) ? "Emu" : "Sys", nand_size / (1024 * 1024));
        for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
            u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
            ShowProgress(i, n_sectors);
            if(!DebugFileRead(buffer, NAND_SECTOR_SIZE * read_sectors, i * NAND_SECTOR_SIZE)) {
                result = 1;
                break;
            }
            WriteNandSectors(i, read_sectors, buffer, use_emunand);
        }
        FileClose();
    }
    ShowProgress(0, 0);

    return result;
}

u32 FormatSdCard(u32 param)
{
    MbrInfo* mbr_info = (MbrInfo*) 0x20316000;
    MbrPartitionInfo* part_info = mbr_info->partitions;
    u8* buffer = (u8*) 0x21000000;
    
    bool setup_emunand = (param & SD_SETUP_EMUNAND);
    u32 starter_size = (param & SD_USE_STARTER) ? 1 : 0;
    
    u32 nand_size_sectors  = getMMCDevice(0)->total_size;
    u32 sd_size_sectors    = 0;
    u32 sd_emunand_sectors = 0;
    u32 fat_offset_sectors = 0;
    u32 fat_size_sectors   = 0;
    
    // copy starter.bin to memory for autosetup
    if (starter_size && FileOpen("starter.bin")) {
        Debug("Copying starter.bin to memory...");
        starter_size = FileGetSize();
        if (starter_size > MAX_STARTER_SIZE) {
            Debug("File is %ikB (max %ikB)", starter_size / 1024, MAX_STARTER_SIZE / 1024);
            FileClose();
            return 1;
        }
        if (!DebugFileRead(buffer, starter_size, 0)) {
            FileClose();
            return 1;
        }
        FileClose();
        Debug("starter.bin is stored in memory");
    } else if (starter_size) {
        Debug("File starter.bin was not found!");
        Debug("You may continue without autosetup");
        Debug("(A to continue, B to cancel)");
        while(true) {
            u32 pad_state = InputWait();
            if (pad_state & BUTTON_A) break;
            else if (pad_state & BUTTON_B) return 2;
        }
        Debug("");
        starter_size = 0;
    }
    
    // give the user one last chance to swap the SD card
    DeinitFS();
    Debug("You may now switch the SD card");
    Debug("The inserted SD card will be formatted");
    Debug("All data on it will be lost!");
    Debug("If you wish to proceed, enter:");
    Debug(unlockText);
    Debug("(B to cancel)");
    if (CheckSequence(unlockSequence, sizeof(unlockSequence) / sizeof(u32)) != 0)
        return 2;
    Debug("");
    InitFS();
    
    // check SD size
    sd_size_sectors = getMMCDevice(1)->total_size;
    if (!sd_size_sectors) {
        Debug("SD card not properly detected!");
        return 1;
    }
    
    // this is the point of no return
    Debug("Total SD card size: %lluMB", ((u64) sd_size_sectors * 0x200) / (1024 * 1024));
    Debug("Storage: %lluMB free / %lluMB total", RemainingStorageSpace() / (1024*1024), TotalStorageSpace() / (1024*1024));
    switch(CheckEmuNand()) {
        case RES_EMUNAND_READY:
            Debug("Already formatted for EmuNAND");
            break;
        case RES_EMUNAND_GATEWAY:
            Debug("Already contains a GW EmuNAND");
            break;
        case RES_EMUNAND_REDNAND:
            Debug("Already contains a RedNAND");
            break;
        default:
            Debug("Not yet formatted for EmuNAND");
            break;
    }
    Debug("Press A to format, B to cancel");
    while(true) {
        u32 pad_state = InputWait();
        if (pad_state & BUTTON_A) break;
        else if (pad_state & BUTTON_B) return 2;
    }
    InitFS();
    Debug("");
   
    // set FAT partition offset and size
    if (setup_emunand) {
        if (nand_size_sectors + SD_MINFREE_SECTORS + 1 > sd_size_sectors) {
            Debug("SD is too small for EmuNAND!");
            return 1;
        }
        sd_emunand_sectors = nand_size_sectors;
    }
    fat_offset_sectors = align(sd_emunand_sectors + 1, PARTITION_ALIGN);
    fat_size_sectors = sd_size_sectors - fat_offset_sectors;
    
    // make a new MBR
    memset(mbr_info, 0x00, 0x200);
    sprintf(mbr_info->text, "%-*.*s%-16.16s%-8.8s%08X%-8.8s%08X%-8.8s%08X%-8.8s%08X",
        setup_emunand ? 16 : 0, setup_emunand ? 16 : 0,
        setup_emunand ? "GATEWAYNAND" : "", "EMUNAND9SD",
        "SDCSIZE:", (unsigned int) sd_size_sectors,
        "NNDSIZE:", (unsigned int) nand_size_sectors,
        "FATSIZE:", (unsigned int) fat_size_sectors,
        "FATOFFS:", (unsigned int) fat_offset_sectors);
    mbr_info->magic         = 0xAA55;
    part_info->status       = 0x80;
    part_info->type         = 0x0C;
    memcpy(part_info->chs_start, "\x01\x01\x00", 3);
    memcpy(part_info->chs_end  , "\xFE\xFF\xFF", 3);
    part_info->offset       = fat_offset_sectors;
    part_info->size         = fat_size_sectors;
    
    // here the actual formatting takes place
    DeinitFS();
    Debug("Writing new master boot record...");
    sdmmc_sdcard_writesectors(0, 1, (u8*) mbr_info);
    InitFS();
    Debug("Formatting FAT partition...");
    if (!PartitionFormat("EMUNAND9SD"))
        return 1;
    DeinitFS();
    InitFS();
    
    if (starter_size) {
        bool is_3dsx = (memcmp(buffer, "3DSX", 4) == 0);
        Debug("Writing %s to SD card...", (is_3dsx) ? "boot.3dsx" : "launcher.dat");
        if (!FileCreate((is_3dsx) ? "/boot.3dsx" : "/launcher.dat", true)) {
            Debug("Failed writing to the SD card!");
            return 1;
        }
        if (!DebugFileWrite(buffer, starter_size, 0)) {
            FileClose();
            return 1;
        }
        FileClose();
        Debug("Setup %s to finish autosetup", (is_3dsx) ? "*hax" : "entrypoint");
    }
    
    return 0;
}

u32 CompleteSetupEmuNand(u32 param)
{
    u32 res = FormatSdCard(SD_SETUP_EMUNAND | SD_USE_STARTER);
    if (res != 0) return res;
    return InjectNand(N_EMUNAND | N_DIRECTCOPY | N_NOCONFIRM);
}
