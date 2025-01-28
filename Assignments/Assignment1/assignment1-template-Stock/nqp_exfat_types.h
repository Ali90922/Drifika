#pragma once
#include <stdint.h>

#pragma pack(push, 1)

// Represents the main boot record of an exFAT volume, containing essential metadata about the file system
typedef struct MAIN_BOOT_RECORD
{
    uint8_t jump_boot[3];
    char fs_name[8];
    uint8_t must_be_zero[53];
    uint64_t partition_offset;
    uint64_t volume_length;

    // The Fat Offset basically stores the location of the Fat Table
    uint32_t fat_offset;
    // The fat Length stores the length of the Fat table in terms of numbers of Sectors which it Spans
    uint32_t fat_length;

    uint32_t cluster_heap_offset;
    uint32_t cluster_count;

    // first_cluster_of_root_directory: Points to the first cluster of the root directory in the Cluster Heap
    // The root directory contains directory entries for all files and subdirectories in the root
    uint32_t first_cluster_of_root_directory;

    uint32_t volume_serial_number;
    uint16_t fs_revision;
    uint16_t fs_flags;

    // Defines the size of a sector as 2^n bytes
    uint8_t bytes_per_sector_shift;
    // Defines the size of a cluster as 2^n Sectors
    uint8_t sectors_per_cluster_shift;

    uint8_t number_of_fats;
    uint8_t drive_select;
    uint8_t percent_in_use;
    uint8_t reserved[7];
    uint8_t bootcode[390];
    uint16_t boot_signature;
} main_boot_record;
#pragma pack(pop)

// A header containing exFAT-implementation-specific structures and values is also provided for your convenience
//(nqp_exfat_types.h). The types header file contains pre-defined structures for the main boot sector,
// directory entries, and identifying values for directory entries.

// Represents a file directory entry, containing metadata about a file

#pragma pack(push, 1)
typedef struct FILE_DENTRY
{
    uint8_t secondary_count;
    uint16_t set_checksum;
    uint16_t file_attributes;
    uint16_t reserved;
    uint32_t create_timestamp;
    uint32_t last_modified_timestamp;
    uint32_t last_accessed_timestamp;
    uint8_t create_10ms_increment;
    uint8_t last_modified_10ms_increment;
    uint8_t create_utc_offset;
    uint8_t last_modified_utc_offset;
    uint8_t last_accessed_utc_offset;
    uint8_t reserved2[7];
} file_dentry;
#pragma pack(pop)

// Represents flags in secondary directory entries
#pragma pack(push, 1)
typedef struct GENERAL_SECONDARY_FLAGS
{
    uint8_t allocation_possible : 1;
    uint8_t no_fat_chain : 1;
    uint8_t custom_defined : 6;
} secondary_flags;
#pragma pack(pop)

//  Describes the data stream for a file

#pragma pack(push, 1)
typedef struct STREAM_EXTENSION
{
    secondary_flags flags;
    uint8_t reserved1;
    uint8_t name_length;
    uint16_t name_hash;
    uint16_t reserved2;
    uint64_t valid_data_length;
    uint32_t reserved3;
    uint32_t first_cluster;
    uint64_t data_length;
} stream_extension;
#pragma pack(pop)

// Stores the name for the file
#pragma pack(push, 1)
typedef struct FILE_NAME
{
    secondary_flags flags;
    uint16_t file_name[15];
} file_name;
#pragma pack(pop)

// Tracks which clusters are allocated or

#pragma pack(push, 1)
typedef struct ALLOCATION_BITMAP
{
    uint8_t bitmap_flaJgs;
    uint8_t reserved[18];
    uint32_t first_cluster;
    uint64_t data_length;
} allocation_bitmap;
#pragma pack(pop)

// Represents the label of the file system
#pragma pack(push, 1)
typedef struct VOLUME_LABEL
{
    uint8_t character_count;
    uint16_t volume_label[11];
    uint64_t reserved;
} volume_label;
#pragma pack(pop)

// Represents a directory entry, which can be one of several types

#pragma pack(push, 1)
typedef struct DIRECTORY_ENTRY
{
    uint8_t entry_type;
    union
    {
        allocation_bitmap bitmap;
        volume_label label;
        file_dentry file;
        file_name file_name;
        stream_extension stream_extension;
    };
} directory_entry;
#pragma pack(pop)

// Groups related entries for a file.
// Groups the file Directory Entry, Stream extension and filename --

typedef struct ENTRY_SET
{
    file_dentry file;
    stream_extension stream_extension;
    file_name *filenames;
} entry_set;

#define DENTRY_TYPE_ALLOCATION_BITMAP 0x81
#define DENTRY_TYPE_UP_CASE_TABLE 0x82
#define DENTRY_TYPE_VOLUME_LABEL 0x83
#define DENTRY_TYPE_FILE 0x85
#define DENTRY_TYPE_VOLUME_GUID 0xA0
#define DENTRY_TYPE_TEXFAT_PADDING 0xA1
#define DENTRY_TYPE_WINCE_ACT 0xA2
#define DENTRY_TYPE_STREAM_EXTENSION 0xC0
#define DENTRY_TYPE_FILE_NAME 0xC1
#define DENTRY_TYPE_END 0x00

// Structure Exaplined

// Defines the size of a sector as 2^n bytes
//  uint8_t bytes_per_sector_shift;
// Defines the size of a cluster as 2^n Sectors
// uint8_t sectors_per_cluster_shift;

// Directory Entries as the metadata about a file or directory,
// and the FAT table as the map that tracks where the actual data resides in the Cluster Heap
