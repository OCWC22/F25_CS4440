/*
 * fs_defs.h
 * Header file defining the filesystem data structures and constants.
 * 
 * This header file contains:
 * 1. Filesystem constants and limits
 * 2. File allocation table (FAT) markers
 * 3. File type definitions
 * 4. Directory entry structure with precise memory layout
 * 5. Block size and alignment specifications
 * 
 * The filesystem design uses a simple FAT-based approach similar to
 * early DOS filesystems, with fixed-size blocks and sequential
 * allocation through a File Allocation Table.
 * 
 * This header is shared between the filesystem server and clients
 * to ensure consistent data structure interpretation.
 */

#ifndef FS_DEFS_H                      // Prevent multiple inclusion
#define FS_DEFS_H

#include <stdint.h>                     // Standard integer types for precise sizing

/*
 * ====================================================================
 * FILESYSTEM CONSTANTS
 * ====================================================================
 * 
 * These constants define the physical and logical parameters of our
 * simulated filesystem. They must match between server and client.
 */

#define BLOCK_SIZE 128                  // Size of each disk block in bytes
                                        // Chosen to balance efficiency and simplicity
                                        // Small enough for educational purposes
                                        // Large enough to hold meaningful data

#define MAX_FILENAME 15                 // Maximum filename length (excluding null terminator)
                                        // Limits: 15 chars + 1 null byte = 16 bytes total
                                        // Similar to early filesystem limitations
                                        // Prevents excessively long filenames

#define FAT_EOF 0xFFFF                  // End-of-File marker in File Allocation Table
                                        // Indicates last block of a file chain
                                        // Uses maximum 16-bit value for clear detection
                                        // Similar to FAT12/16 EOF markers

#define FAT_FREE 0x0000                 // Free block marker in File Allocation Table
                                        // Indicates available block for allocation
                                        // Zero value makes free block detection efficient
                                        // Unused blocks are marked as free

/*
 * ====================================================================
 * FILE TYPE DEFINITIONS
 * ====================================================================
 * 
 * Simple classification system for filesystem objects.
 * Could be extended to support more types (symlinks, devices, etc.).
 */

#define TYPE_FILE 0                     // Regular file type
                                        // Contains user data with sequential access
                                        // Most common type in typical filesystems

#define TYPE_DIR  1                     // Directory type
                                        // Contains directory entries pointing to other files
                                        // Special structure for hierarchical organization

/*
 * ====================================================================
 * DIRECTORY ENTRY STRUCTURE
 * ====================================================================
 * 
 * This structure defines the layout of directory entries.
 * Each directory entry occupies exactly 32 bytes, allowing
 * 4 entries per 128-byte block (32 * 4 = 128).
 * 
 * Memory Layout (32 bytes total):
 * +----------------+----------------+----------------+----------------+
 * | filename (16)  | size (4)       | head_block (2) | type (1)       |
 * +----------------+----------------+----------------+----------------+
 * | valid (1)      | padding (8)    |                                        |
 * +----------------+----------------+----------------------------------------+
 * 
 * Design Rationale:
 * - Fixed size enables simple block-based storage
 * - Padding ensures proper alignment and future extensibility
 * - Compact but sufficient for educational filesystem
 */
typedef struct {
    char name[MAX_FILENAME + 1];        // Filename with null terminator (16 bytes)
                                        // Stores the name of the file/directory
                                        // Null-terminated C string for compatibility
                                        // Limited to 15 characters plus null byte
    
    uint32_t size;                      // File size in bytes (4 bytes)
                                        // Total size of file content
                                        // Allows efficient size queries without traversal
                                        // 32-bit allows files up to 4GB (sufficient for testing)
    
    uint16_t head_block;                // Index of first block in FAT chain (2 bytes)
                                        // Points to first data block of the file
                                        // 0 indicates empty file, special values reserved
                                        // 16-bit allows up to 65535 blocks in filesystem
    
    uint8_t type;                       // File type identifier (1 byte)
                                        // TYPE_FILE (0) for regular files
                                        // TYPE_DIR (1) for directories
                                        // Could be extended for more file types
    
    uint8_t valid;                      // Entry validity flag (1 byte)
                                        // 0 = Empty/unused directory slot
                                        // 1 = Valid, active directory entry
                                        // Enables efficient directory scanning
    
    uint8_t padding[8];                 // Padding bytes for alignment (8 bytes)
                                        // Ensures structure is exactly 32 bytes
                                        // Reserved for future filesystem features
                                        // Maintains block alignment (4 entries per block)
} DirEntry;

/*
 * ====================================================================
 * DESIGN NOTES AND EXTENSIBILITY
 * ====================================================================
 * 
 * Memory Efficiency:
 * - Each directory block holds exactly 4 entries
 * - No dynamic memory allocation required for directory traversal
 * - Simple pointer arithmetic for entry positioning
 * 
 * Limitations:
 * - Maximum filename length of 15 characters
 * - Maximum file size of 4GB (32-bit size field)
 * - Maximum of 65535 blocks per filesystem (16-bit FAT)
 * - No support for permissions, timestamps, or advanced features
 * 
 * Educational Value:
 * - Demonstrates real filesystem design principles
 * - Shows importance of data structure alignment
 * - Illustrates trade-offs between simplicity and functionality
 * - Provides foundation for understanding modern filesystems
 */

#endif /* FS_DEFS_H */                // End of header guard
