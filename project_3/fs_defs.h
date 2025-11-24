#ifndef FS_DEFS_H
#define FS_DEFS_H

#include <stdint.h>

/*
 * Constants
 */
#define BLOCK_SIZE 128
#define MAX_FILENAME 15     // 15 chars + null terminator = 16 bytes
#define FAT_EOF 0xFFFF      // Marker for End of File in FAT
#define FAT_FREE 0x0000     // Marker for Free Block in FAT

// File Types
#define TYPE_FILE 0
#define TYPE_DIR  1

/*
 * Directory Entry Structure
 * We pad this to 32 bytes so it aligns perfectly with 128-byte blocks.
 * 128 bytes / 32 bytes = 4 entries per block.
 */
typedef struct {
    char name[MAX_FILENAME + 1]; // 16 bytes
    uint32_t size;               // 4 bytes (File size in bytes)
    uint16_t head_block;         // 2 bytes (Start index in FAT)
    uint8_t type;                // 1 byte (0=File, 1=Dir)
    uint8_t valid;               // 1 byte (0=Empty Slot, 1=Used)
    uint8_t padding[8];          // 8 bytes padding to reach 32 bytes total
} DirEntry;

#endif /* FS_DEFS_H */