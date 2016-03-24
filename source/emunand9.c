#include "fs.h"
#include "draw.h"
#include "hid.h"
#include "platform.h"
#include "emunand9.h"
#include "fatfs/sdmmc.h"

#define BUFFER_ADDRESS  ((u8*) 0x21000000)
#define BUFFER_MAX_SIZE ((u32)(8 * 1024 * 1024))
#define FCRAM_END       ((u8*) 0x28000000)

#define NAND_SECTOR_SIZE ((u32)0x200)
#define SECTORS_PER_READ (BUFFER_MAX_SIZE / NAND_SECTOR_SIZE)

#define SD_MINFREE_SECTORS  ((256 * 1024 * 1024) / 0x200)  // have at least 256MB free
#define PARTITION_ALIGN     ((4 * 1024 * 1024) / 0x200)     // align at 4MB
#define MAX_STARTER_SIZE    (16 * 1024 * 1024)              // allow to take over max 16MB boot.3dsx

// minimum sizes for O3DS / N3DS NAND
// see: http://3dbrew.org/wiki/Flash_Filesystem
#define NAND_MIN_SIZE ((GetUnitPlatform() == PLATFORM_3DS) ? 0x3AF00000 : 0x4D800000)

// see below
#define IS_NAND_HEADER(hdr) ((memcmp(buffer + 0x100, nand_magic_n3ds, 0x60) == 0) ||\
                             (memcmp(buffer + 0x100, nand_magic_o3ds, 0x60) == 0))

// from an actual N3DS NCSD NAND header, same for all
static u8 nand_magic_n3ds[0x60] = {
    0x4E, 0x43, 0x53, 0x44, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x05, 0x00, 0x00, 0x88, 0x05, 0x00, 0x80, 0x01, 0x00, 0x00,
    0x80, 0x89, 0x05, 0x00, 0x00, 0x20, 0x00, 0x00, 0x80, 0xA9, 0x05, 0x00, 0x00, 0x20, 0x00, 0x00,
    0x80, 0xC9, 0x05, 0x00, 0x80, 0xF6, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// from an actual O3DS NCSD NAND header, same for all
static u8 nand_magic_o3ds[0x60] = {
    0x4E, 0x43, 0x53, 0x44, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x05, 0x00, 0x00, 0x88, 0x05, 0x00, 0x80, 0x01, 0x00, 0x00,
    0x80, 0x89, 0x05, 0x00, 0x00, 0x20, 0x00, 0x00, 0x80, 0xA9, 0x05, 0x00, 0x00, 0x20, 0x00, 0x00,
    0x80, 0xC9, 0x05, 0x00, 0x80, 0xAE, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const u32 unlockSequence[] = { BUTTON_DOWN, BUTTON_UP, BUTTON_LEFT, BUTTON_RIGHT, BUTTON_A };
const char* unlockText = "<Down>, <Up>, <Left>, <Right>, <A>";

static inline u32 GetEmuNandPartitionSectors(void)
{
    u8* mbr = (u8*) 0x20316400;
    sdmmc_sdcard_readsectors(0, 1, mbr);
    return getle32(mbr + 0x1BE + 0x8) - 1;
}

u32 CheckEmuNand(void)
{
    u8* buffer = (u8*) 0x20316200;
    u32 nand_size_sectors = getMMCDevice(0)->total_size;
    u32 hidden_sectors = GetEmuNandPartitionSectors();
    
    if (hidden_sectors >= NAND_MIN_SIZE / NAND_SECTOR_SIZE) {
        // check for RedNAND type EmuNAND
        sdmmc_sdcard_readsectors(1, 1, buffer);
        if (IS_NAND_HEADER(buffer))
            return RES_EMUNAND_REDNAND;
        if (hidden_sectors >= nand_size_sectors) {
            // check for Gateway type EmuNAND
            sdmmc_sdcard_readsectors(nand_size_sectors, 1, buffer);
            if (IS_NAND_HEADER(buffer))
                return RES_EMUNAND_GATEWAY;
        }
        // EmuNAND ready but not set up
        return RES_EMUNAND_READY;
    }
    
    return RES_EMUNAND_NOT_READY;
}

static inline int ReadNandSectors(u32 sector_no, u32 numsectors, u8 *out, bool use_emunand)
{
    if (use_emunand) {
        u32 emunand_state = CheckEmuNand();
        u32 emunand_header = 0;
        u32 emunand_offset = 0;    
        if (emunand_state == RES_EMUNAND_GATEWAY) {
            emunand_header = getMMCDevice(0)->total_size;
            emunand_offset = 0;
        } else if (emunand_state == RES_EMUNAND_REDNAND) {
            emunand_header = 1;
            emunand_offset = 1;
        } else { // EmuNAND not set up
            return 1;
        }
        if (!emunand_header) CheckEmuNand();
        if (sector_no == 0) {
            int errorcode = sdmmc_sdcard_readsectors(emunand_header, 1, out);
            if (errorcode) return errorcode;
            sector_no = 1;
            numsectors--;
            out += 0x200;
        }
        return (numsectors) ? sdmmc_sdcard_readsectors(sector_no + emunand_offset, numsectors, out) : 0;
    } else return sdmmc_nand_readsectors(sector_no, numsectors, out);
}

static inline int WriteNandSectors(u32 sector_no, u32 numsectors, u8 *in, u32 dest)
{
    if (dest != WR_SYSNAND) {
        u32 emunand_header = 0;
        u32 emunand_offset = 0;
        if (dest == WR_EMUNAND_GATEWAY) {
            emunand_header = getMMCDevice(0)->total_size;
            emunand_offset = 0;
        } else {
            emunand_header = 1;
            emunand_offset = 1;
        }
        if (sector_no == 0) {
            int errorcode = sdmmc_sdcard_writesectors(emunand_header, 1, in);
            if (errorcode) return errorcode;
            sector_no = 1;
            numsectors--;
            in += 0x200;
        }
        return (numsectors) ? sdmmc_sdcard_writesectors(sector_no + emunand_offset, numsectors, in) : 0;
    } else return 0; // return sdmmc_nand_writesectors(sector_no, numsectors, in);
    // SysNAND write stubbed - not needed here, no need to take the risk
}

static inline u32 OutputFileNameSelector(char* filename, const char* basename, char* extension, bool emuname) {
    char bases[3][64] = { 0 };
    char* dotpos = NULL;
    
    // build first base name and extension
    strncpy(bases[0], basename, 63);
    dotpos = strrchr(bases[0], '.');
    
    if (dotpos) {
        *dotpos = '\0';
        if (!extension)
            extension = dotpos + 1;
    }
    
    // build other two base names
    snprintf(bases[1], 63, "%s_%s", bases[0], (emuname) ? "emu" : "sys");
    snprintf(bases[2], 63, "%s%s" , (emuname) ? "emu" : "sys", bases[0]);
    
    u32 fn_id = 0;
    u32 fn_num = 0;
    bool exists = false;
    char extstr[16] = { 0 };
    if (extension)
        snprintf(extstr, 15, ".%s", extension);
    Debug("Use arrow keys and <A> to choose a name");
    while (true) {
        char numstr[2] = { 0 };
        // build and output file name (plus "(!)" if existing)
        numstr[0] = (fn_num > 0) ? '0' + fn_num : '\0';
        snprintf(filename, 63, "%s%s%s", bases[fn_id], numstr, extstr);
        if ((exists = FileOpen(filename)))
            FileClose();
        Debug("\r%s%s", filename, (exists) ? " (!)" : "");
        // user input routine
        u32 pad_state = InputWait();
        if (pad_state & BUTTON_DOWN) { // increment filename id
            fn_id = (fn_id + 1) % 3;
        } else if (pad_state & BUTTON_UP) { // decrement filename id
            fn_id = (fn_id > 0) ? fn_id - 1 : 2;
        } else if ((pad_state & BUTTON_RIGHT) && (fn_num < 9)) { // increment number
            fn_num++;
        } else if ((pad_state & BUTTON_LEFT) && (fn_num > 0)) { // decrement number
            fn_num--;
        } else if (pad_state & BUTTON_A) {
            Debug("%s%s", filename, (exists) ? " (!)" : "");
            break;
        } else if (pad_state & BUTTON_B) {
            Debug("(cancelled by user)");
            return 2;
        }
    }
    
    // overwrite confirmation
    if (exists) {
        Debug("Press <A> to overwrite existing file");
        while (true) {
            u32 pad_state = InputWait();
            if (pad_state & BUTTON_A) {
                break;
            } else if (pad_state & BUTTON_B) {
                Debug("(cancelled by user)");
                return 2;
            }
        }
    }
    
    return 0;
}

static inline u32 InputFileNameSelector(char* filename, const char* basename, char* extension, u32 fminsize) {
    char** fnptr = (char**) 0x20400000; // allow using 0x8000 byte
    char* fnlist = (char*) 0x20408000; // allow using 0x80000 byte
    u32 n_names = 0;
    
    // get base name, extension
    char base[64] = { 0 };
    if (basename != NULL) {
        // build base name and extension
        strncpy(base, basename, 63);
        char* dotpos = strrchr(base, '.');
        if (dotpos) {
            *dotpos = '\0';
            if (!extension)
                extension = dotpos + 1;
        }
    }
    
    // pass #1 -> work dir
    // pass #2 -> root dir
    for (u32 i = 0; i < 2; i++) {
        // get the file list - try work directory first
        if (!GetFileList((i) ? "/" : WORK_DIR, fnlist, 0x80000, false))
            continue;
        
        // parse the file names list for usable entries
        for (char* fn = strtok(fnlist, "\n"); fn != NULL; fn = strtok(NULL, "\n")) {
            char* dotpos = strrchr(fn, '.');
            if (strrchr(fn, '/'))
                fn = strrchr(fn, '/') + 1;
            if (strnlen(fn, 128) > 63)
                continue; // file name too long
            if ((basename != NULL) && !strcasestr(fn, base))
                continue; // basename check failed
            if ((extension != NULL) && (dotpos != NULL) && (strncasecmp(dotpos + 1, extension, strnlen(extension, 16))))
                continue; // extension check failed
            else if ((extension == NULL) != (dotpos == NULL))
                continue; // extension check failed
            if (!FileOpen(fn))
                continue; // file can't be opened
            if (fminsize && (FileGetSize() < fminsize)) {
                FileClose();
                continue; // file size check failed
            }
            FileClose();
            // this is a match - keep it
            fnptr[n_names++] = fn;
            if (n_names * sizeof(char**) >= 0x8000)
                return 1;
        }
        if (n_names)
            break;
    }
    if (n_names == 0) {
        Debug("No usable file found");
        return 1;
    }
    
    u32 index = 0;
    Debug("Use arrow keys and <A> to choose a file");
    while (true) {
        snprintf(filename, 63, "%s", fnptr[index]);
        Debug("\r%s", filename);
        u32 pad_state = InputWait();
        if (pad_state & BUTTON_DOWN) { // next filename
            index = (index + 1) % n_names;
        } else if (pad_state & BUTTON_UP) { // previous filename
            index = (index > 0) ? index - 1 : n_names - 1;
        } else if (pad_state & BUTTON_A) {
            Debug("%s", filename);
            break;
        } else if (pad_state & BUTTON_B) {
            Debug("(cancelled by user)");
            return 2;
        }
    }
    
    return 0;
}

u32 DumpNand(u32 param)
{
    char filename[64];
    u8* buffer = BUFFER_ADDRESS;
    u32 nand_size = getMMCDevice(0)->total_size * NAND_SECTOR_SIZE;
    u32 hidden_partition = GetEmuNandPartitionSectors() * NAND_SECTOR_SIZE;
    bool use_emunand = (param & N_EMUNAND);
    u32 result = 0;

    if (use_emunand && (CheckEmuNand() <= RES_EMUNAND_READY)) {
        Debug("EmuNAND not found on SD card");
        return 1;
    }
    
    if (use_emunand && (hidden_partition < nand_size))
        nand_size = NAND_MIN_SIZE;
    
    Debug("Dumping %sNAND (%uMB) to file...", (use_emunand) ? "Emu" : "Sys", nand_size / (1024 * 1024));

    u32 fn_state = OutputFileNameSelector(filename, "NAND", "bin", use_emunand);
    if (fn_state != 0)
        return fn_state;
    if (!DebugFileCreate(filename, true))
        return 1;

    u32 n_sectors = nand_size / NAND_SECTOR_SIZE;
    for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
        u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
        ShowProgress(i, n_sectors);
        if ((ReadNandSectors(i, read_sectors, buffer, use_emunand) != 0) ||
            !FileWrite(buffer, NAND_SECTOR_SIZE * read_sectors, i * NAND_SECTOR_SIZE)) {
            Debug("NAND or SD i/o failure");
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
    u32 hidden_partition = GetEmuNandPartitionSectors() * NAND_SECTOR_SIZE;
    u32 nand_full_size = getMMCDevice(0)->total_size * NAND_SECTOR_SIZE;
    u32 nand_size = nand_full_size;
    u32 write_dest = (param & N_WREDNAND) ? WR_EMUNAND_REDNAND : WR_EMUNAND_GATEWAY; // we won't write to SysNAND, so this option is not included
    u32 result = 0;
    
    if ((write_dest != WR_SYSNAND) && (!(param & N_NOCONFIRM))) switch(CheckEmuNand()) {
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
    
    if ((hidden_partition < nand_size) && (hidden_partition >= NAND_MIN_SIZE)) {
        if (!(param & N_NOCONFIRM) && (write_dest != WR_EMUNAND_REDNAND)) {
            Debug("Not enough room for GW EmuNAND on SD");
            Debug("Use RedNAND instead of GW EmuNAND?");
            Debug("(A to confirm, B to cancel)");
            while(true) {
                u32 pad_state = InputWait();
                if (pad_state & BUTTON_A) break;
                else if (pad_state & BUTTON_B) {
                    FileClose();
                    return 2;
                }
            }
        }
        write_dest = WR_EMUNAND_REDNAND;
        nand_size = NAND_MIN_SIZE;
    } else if (hidden_partition < nand_size) {
        return 1;
    }
    
    if (CheckEmuNand() == RES_EMUNAND_NOT_READY) // better be safe
        return 1;
    
    u32 n_sectors = nand_size / NAND_SECTOR_SIZE;
    if (param & N_DIRECTCOPY) {
        Debug("Cloning SysNAND to %sNAND (%uMB)", (write_dest == WR_EMUNAND_GATEWAY) ? "Emu" : "Red", nand_size / (1024 * 1024));
        for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
            u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
            ShowProgress(i, n_sectors);
            if ((ReadNandSectors(i, read_sectors, buffer, false) != 0) ||
                (WriteNandSectors(i, read_sectors, buffer, write_dest) != 0)) {
                Debug("NAND or SD i/o failure");
                result = 1;
                break;
            }
        }
    } else {
        char filename[64];
        u32 file_size;
        u32 fn_state = InputFileNameSelector(filename, "NAND", "bin", NAND_MIN_SIZE);
        
        if (fn_state != 0)
            return fn_state;
        if (!DebugFileOpen(filename))
            return 1;
        file_size = FileGetSize();
        if(!DebugFileRead(buffer, 0x200, 0)) {
            FileClose();
            return 1;
        }
        if (IS_NAND_HEADER(buffer)) {
            FileClose();
            Debug("Not a proper NAND dump!");
            return 1;
        }
        if (file_size < NAND_MIN_SIZE) {
            Debug("NAND dump misses data!");
            FileClose();
            return 1;
        } else if ((file_size != nand_full_size) && (file_size != NAND_MIN_SIZE)) {
            Debug("NAND dump is %s than NAND memory chip.", (file_size < nand_full_size) ? "smaller" : "bigger");
            Debug("This may happen with dumps from other tools.");
            Debug("Are you sure this dump is valid?");
            Debug("(A to proceed, B to cancel)");
            while(true) {
                u32 pad_state = InputWait();
                if (pad_state & BUTTON_A) break;
                else if (pad_state & BUTTON_B) {
                    FileClose();
                    return 2;
                }
            }
            if (file_size < nand_size) // can only write as much sectors as there are
                n_sectors = file_size / NAND_SECTOR_SIZE;
        }
        
        Debug("Injecting file to %sNAND (%uMB)...", (write_dest == WR_SYSNAND) ? "Sys" : (write_dest == WR_EMUNAND_GATEWAY) ? "Emu" : "Red", nand_size / (1024 * 1024));
        for (u32 i = 0; i < n_sectors; i += SECTORS_PER_READ) {
            u32 read_sectors = min(SECTORS_PER_READ, (n_sectors - i));
            ShowProgress(i, n_sectors);
            if (!FileRead(buffer, NAND_SECTOR_SIZE * read_sectors, i * NAND_SECTOR_SIZE) ||
                (WriteNandSectors(i, read_sectors, buffer, write_dest) != 0)) {
                Debug("SD card i/o failure"); // SysNAND is stubbed!
                result = 1;
                break;
            }
            
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
    
    u32 nand_size_sectors  = (param & SD_SETUP_MINSIZE) ? (NAND_MIN_SIZE / NAND_SECTOR_SIZE) : getMMCDevice(0)->total_size;
    u32 sd_size_sectors    = 0;
    u32 sd_emunand_sectors = 0;
    u32 fat_offset_sectors = 0;
    u32 fat_size_sectors   = 0;
    
    // copy starter.bin to memory for autosetup
    if (starter_size && !CheckFS()) {
        Debug("File system is corrupt or unknown");
        Debug("Continuing without starter.bin...");
        Debug("");
        starter_size = 0;
    } else if (starter_size && FileOpen("starter.bin")) {
        Debug("Copying starter.bin to memory...");
        starter_size = FileGetSize();
        if (starter_size > ((u32)(FCRAM_END - buffer))) {
            Debug("File is %ikB, exceeds RAM size", starter_size / 1024);
            FileClose();
            return 1;
        } else if (starter_size > MAX_STARTER_SIZE) {
            Debug("File is %ikB (recom. max: %ikB)", starter_size / 1024, MAX_STARTER_SIZE / 1024);
            Debug("This could be too big. Still continue?");
            Debug("(A to continue, B to cancel)");
            while (true) {
                u32 pad_state = InputWait();
                if (pad_state & BUTTON_A) break;
                else if (pad_state & BUTTON_B) {
                    FileClose();
                    return 2;
                }
            }
            Debug("");
        }
        if (!DebugFileRead(buffer, starter_size, 0)) {
            FileClose();
            return 1;
        }
        FileClose();
        Debug("starter.bin is stored in memory");
        Debug("");
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
    if (CheckSequence(unlockSequence, sizeof(unlockSequence) / sizeof(u32)) != 0) {
        InitFS();
        return 2;
    }
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
    if (CheckFS()) {
        Debug("Storage: %lluMB free / %lluMB total", RemainingStorageSpace() / (1024*1024), TotalStorageSpace() / (1024*1024));
    } else {
        Debug("Unknown or corrupt file system");
    }
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
    Debug("");
   
    // set FAT partition offset and size
    if (setup_emunand) {
        sd_emunand_sectors = ((param & SD_SETUP_LEGACY)) ? align(nand_size_sectors + 1, 0x200000) - 1 : nand_size_sectors;
        if (sd_emunand_sectors + SD_MINFREE_SECTORS + 1 > sd_size_sectors) {
            Debug("SD is too small for EmuNAND!");
            return 1;
        }
    }
    fat_offset_sectors = align(sd_emunand_sectors + 1, PARTITION_ALIGN);
    fat_size_sectors = sd_size_sectors - fat_offset_sectors;
    
    // make a new MBR
    memset(mbr_info, 0x00, 0x200);
    snprintf(mbr_info->text, 445, "%-*.*s%-16.16s%-8.8s%08X%-8.8s%08X%-8.8s%08X%-8.8s%08X%-8.8s%08X",
        setup_emunand ? 16 : 0, setup_emunand ? 16 : 0,
        setup_emunand ? "GATEWAYNAND" : "", "EMUNAND9SD",
        "SDCSIZE:", (unsigned int) sd_size_sectors,
        "SYSSIZE:", (unsigned int) nand_size_sectors,
        "EMUSIZE:", (unsigned int) sd_emunand_sectors,
        "FATSIZE:", (unsigned int) fat_size_sectors,
        "FATOFFS:", (unsigned int) fat_offset_sectors);
    mbr_info->magic         = 0xAA55;
    part_info[0].status     = 0x80;
    part_info[0].type       = 0x0C;
    memcpy(part_info[0].chs_start, "\x01\x01\x00", 3);
    memcpy(part_info[0].chs_end  , "\xFE\xFF\xFF", 3);
    part_info[0].offset     = fat_offset_sectors;
    part_info[0].size       = fat_size_sectors;
    if (setup_emunand) {
        part_info[1].status = 0x80;
        part_info[1].type   = 0x1C;
        memcpy(part_info[1].chs_start, "\x01\x01\x00", 3);
        memcpy(part_info[1].chs_end  , "\xFE\xFF\xFF", 3);
        part_info[1].offset = 0x1;
        part_info[1].size   = nand_size_sectors;
    }
    
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
    
    // check if it went well
    if (!CheckFS()) {
        Debug("Unknown error / something went wrong here");
        Debug("Format this SD card from your PC first");
        Debug("Then try this again");
        return 1;
    }
    
    // try creating the working directory
    #ifdef WORK_DIR
    DirMake(WORK_DIR);
    DeinitFS();
    InitFS();
    #endif
    
    // setup starter.bin
    if (starter_size) {
        bool is_3dsx = (memcmp(buffer, "3DSX", 4) == 0);
        Debug("Writing starter.bin to SD card...");
        if (!FileCreate("starter.bin", true)) {
            Debug("Failed writing to the SD card!");
            return 1;
        }
        if (!DebugFileWrite(buffer, starter_size, 0)) {
            FileClose();
            return 1;
        }
        FileClose();
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
    u32 res = FormatSdCard(SD_SETUP_EMUNAND | SD_USE_STARTER | param);
    if (res != 0) return res;
    Debug("");
    return InjectNand(N_EMUNAND | N_DIRECTCOPY | N_NOCONFIRM | param);
}
