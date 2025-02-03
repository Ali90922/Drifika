#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

/*
 * The structs in the Linux kernel include types like __le16. These are types 
 * that are stating that they are little-endian (le). x86 systems are little-
 * endian, so we can map __le16 to uint16_t.
 */
typedef uint16_t __le16 ;
typedef uint16_t __u16  ;
typedef uint32_t __le32 ;
typedef uint32_t __u32  ;
typedef uint8_t  __u8   ;

#define EXT2_MAGIC 0xef53
#define EXT2_ROOT_DIR_INODE 2

// Everything below is from fs/ext2/ext2.h in the Linux kernel
// https://elixir.bootlin.com/linux/v6.13/source/fs/ext2/ext2.h

/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)


/*
 * Structure of the super block
 */
struct ext2_super_block {
	__le32	s_inodes_count;		/* Inodes count */
	__le32	s_blocks_count;		/* Blocks count */
	__le32	s_r_blocks_count;	/* Reserved blocks count */
	__le32	s_free_blocks_count;	/* Free blocks count */
	__le32	s_free_inodes_count;	/* Free inodes count */
	__le32	s_first_data_block;	/* First Data Block */
	__le32	s_log_block_size;	/* Block size */
	__le32	s_log_frag_size;	/* Fragment size */
	__le32	s_blocks_per_group;	/* # Blocks per group */
	__le32	s_frags_per_group;	/* # Fragments per group */
	__le32	s_inodes_per_group;	/* # Inodes per group */
	__le32	s_mtime;		/* Mount time */
	__le32	s_wtime;		/* Write time */
	__le16	s_mnt_count;		/* Mount count */
	__le16	s_max_mnt_count;	/* Maximal mount count */
	__le16	s_magic;		/* Magic signature */
	__le16	s_state;		/* File system state */
	__le16	s_errors;		/* Behaviour when detecting errors */
	__le16	s_minor_rev_level; 	/* minor revision level */
	__le32	s_lastcheck;		/* time of last check */
	__le32	s_checkinterval;	/* max. time between checks */
	__le32	s_creator_os;		/* OS */
	__le32	s_rev_level;		/* Revision level */
	__le16	s_def_resuid;		/* Default uid for reserved blocks */
	__le16	s_def_resgid;		/* Default gid for reserved blocks */
	/*
	 * These fields are for EXT2_DYNAMIC_REV superblocks only.
	 *
	 * Note: the difference between the compatible feature set and
	 * the incompatible feature set is that if there is a bit set
	 * in the incompatible feature set that the kernel doesn't
	 * know about, it should refuse to mount the filesystem.
	 * 
	 * e2fsck's requirements are more strict; if it doesn't know
	 * about a feature in either the compatible or incompatible
	 * feature set, it must abort and not try to meddle with
	 * things it doesn't understand...
	 */
	__le32	s_first_ino; 		/* First non-reserved inode */
	__le16   s_inode_size; 		/* size of inode structure */
	__le16	s_block_group_nr; 	/* block group # of this superblock */
	__le32	s_feature_compat; 	/* compatible feature set */
	__le32	s_feature_incompat; 	/* incompatible feature set */
	__le32	s_feature_ro_compat; 	/* readonly-compatible feature set */
	__u8	s_uuid[16];		/* 128-bit uuid for volume */
	char	s_volume_name[16]; 	/* volume name */
	char	s_last_mounted[64]; 	/* directory where last mounted */
	__le32	s_algorithm_usage_bitmap; /* For compression */
	/*
	 * Performance hints.  Directory preallocation should only
	 * happen if the EXT2_COMPAT_PREALLOC flag is on.
	 */
	__u8	s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
	__u8	s_prealloc_dir_blocks;	/* Nr to preallocate for dirs */
	__u16	s_padding1;
	/*
	 * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	 */
	__u8	s_journal_uuid[16];	/* uuid of journal superblock */
	__u32	s_journal_inum;		/* inode number of journal file */
	__u32	s_journal_dev;		/* device number of journal file */
	__u32	s_last_orphan;		/* start of list of inodes to delete */
	__u32	s_hash_seed[4];		/* HTREE hash seed */
	__u8	s_def_hash_version;	/* Default hash version to use */
	__u8	s_reserved_char_pad;
	__u16	s_reserved_word_pad;
	__le32	s_default_mount_opts;
 	__le32	s_first_meta_bg; 	/* First metablock block group */
	__u32	s_reserved[190];	/* Padding to the end of the block */
};

/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
	__le16	i_mode;		/* File mode */
	__le16	i_uid;		/* Low 16 bits of Owner Uid */
	__le32	i_size;		/* Size in bytes */
	__le32	i_atime;	/* Access time */
	__le32	i_ctime;	/* Creation time */
	__le32	i_mtime;	/* Modification time */
	__le32	i_dtime;	/* Deletion Time */
	__le16	i_gid;		/* Low 16 bits of Group Id */
	__le16	i_links_count;	/* Links count */
	__le32	i_blocks;	/* Blocks count */
	__le32	i_flags;	/* File flags */
	union {
		struct {
			__le32  l_i_reserved1;
		} linux1;
		struct {
			__le32  h_i_translator;
		} hurd1;
		struct {
			__le32  m_i_reserved1;
		} masix1;
	} osd1;				/* OS dependent 1 */
	__le32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__le32	i_generation;	/* File version (for NFS) */
	__le32	i_file_acl;	/* File ACL */
	__le32	i_dir_acl;	/* Directory ACL */
	__le32	i_faddr;	/* Fragment address */
	union {
		struct {
			__u8	l_i_frag;	/* Fragment number */
			__u8	l_i_fsize;	/* Fragment size */
			__u16	i_pad1;
			__le16	l_i_uid_high;	/* these 2 fields    */
			__le16	l_i_gid_high;	/* were reserved2[0] */
			__u32	l_i_reserved2;
		} linux2;
		struct {
			__u8	h_i_frag;	/* Fragment number */
			__u8	h_i_fsize;	/* Fragment size */
			__le16	h_i_mode_high;
			__le16	h_i_uid_high;
			__le16	h_i_gid_high;
			__le32	h_i_author;
		} hurd2;
		struct {
			__u8	m_i_frag;	/* Fragment number */
			__u8	m_i_fsize;	/* Fragment size */
			__u16	m_pad1;
			__u32	m_i_reserved2[2];
		} masix2;
	} osd2;				/* OS dependent 2 */
};


/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};

struct ext2_dir_entry_2 {
	__le32	inode;			/* Inode number */
	__le16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;
	char	*name;			/* File name, up to EXT2_NAME_LEN */
};

// Linux kernel structures stop here.

int main( int argc, char **argv )
{
    int exit_code = EXIT_SUCCESS;
    int fd = -1;

    struct ext2_super_block sb = { 0 };
    struct ext2_group_desc group = { 0 };
    struct ext2_inode root_directory = { 0 };
    struct ext2_dir_entry_2 dir_entry = { 0 };

    uint32_t block_size = 0;
    uint8_t bits = 0;
    uint32_t set_bits = 0;
    uint16_t dir_entry_len = 0;
    uint32_t bytes_read = 0;

    if ( argc != 2 )
    {
        fprintf( stderr, "Usage: ./%s <ext2-volume>\n", argv[0] );
        exit_code = EXIT_FAILURE;
    }
    else
    {
        fd = open( argv[1], O_RDONLY );

        // go to the beginning of the superblock
        lseek( fd, 1024, SEEK_SET );
        read( fd, &sb, sizeof( struct ext2_super_block ) );

        if ( sb.s_magic == EXT2_MAGIC )
        {
            printf( "This appears to be an ext2-formatted file system; the "
                    "magic number is correct (0x%x).\n", sb.s_magic );

            // just like "BytesPerSectorShift" in exFAT, the "log_block_size"
            // is the exponent of an expression (the log here means log() as
            // in logarithm). We can calculate the block size using the
            // expression 1024 << log_block_size, see:
            // https://www.nongnu.org/ext2-doc/ext2.html#s-log-block-size
            block_size = 1024 << sb.s_log_block_size;

            // now let's go to the first block group (this is the first block
            // after the superblock; the superblock is 1k). This code assumes
            // that the block size is >= 2kb (though that isn't a universally
            // safe assumption to make!).
            lseek( fd, block_size, SEEK_SET );
            read( fd, &group, sizeof( struct ext2_group_desc ) );

            // we want to find the block bitmap and check that it's consistent
            // with the free blocks count in the superblock (the same approach
            // can be applied to the inode bitmap and free inodes count).
            lseek( fd, block_size * group.bg_block_bitmap, SEEK_SET ); 

            // we need to read in block_count *bits* (each block is represented
            // by a single bit in the block bitmap; block 0 is bit 0, block 1 is
            // bit 1, block 2 is bit 2, etc).
            for ( uint8_t i = 0; i < (sb.s_blocks_count / 8); i++ )
            {
                read( fd, &bits, sizeof( uint8_t ) );
                set_bits += __builtin_popcount( bits );
            }

            if ( sb.s_blocks_count - set_bits == sb.s_free_blocks_count )
            {
                printf( "Block allocation structure and superblock are "
                        "consistent: expected %d free blocks, counted %d "
                        "set bits with %d unset bits.\n", sb.s_free_blocks_count,
                        set_bits, sb.s_blocks_count - set_bits );
            }
            else
            {
                printf( "Block allocation structure and superblock are NOT "
                        "consistent: expected %d free blocks, counted %d "
                        "set bits with %d unset bits.\n", sb.s_free_blocks_count,
                        set_bits, sb.s_blocks_count - set_bits );
                // ===================== QUESTION =====================
                // what would the correct action be if we were going to attempt
                // to repair this file system? Do we trust the superblock or the
                // allocation structure? How might we be able to figure out
                // which is correct? Are there other structures we could check
                // to help decide which one is correct?
                // ====================================================
            }

            // now let's see if we can find any disconnected inodes --- inodes
            // that are in the table, but not in the root directory specifically
            // ===================== QUESTION =====================
            // (how might we check this for an arbitrary inode?)
            // ====================================================
            lseek( fd, block_size * group.bg_inode_table, SEEK_SET );
            
            // let's read the root directory inode first so we can find its
            // data blocks with directory entries. The root directory is at
            // a well-known location of 2. The inode table is a 1-based array
            // so we need to subtract 1 from the inode number.
            lseek( fd, ( EXT2_ROOT_DIR_INODE - 1) * sizeof( struct ext2_inode ), SEEK_CUR );
            read( fd, &root_directory, sizeof( struct ext2_inode ) );

            // now we can go to the directory and start looking at directory
            // entries in the directory; we know how many blocks we should look
            // at by using the i_blocks property of the inode:
            // i_blocks / ((1024<<s_log_block_size)/512)
            // see: https://www.nongnu.org/ext2-doc/ext2.html#i-blocks
            for ( uint32_t block = 0; block < (root_directory.i_blocks / (block_size / 512)); block++)
            {
                bytes_read = 0;

                lseek( fd, root_directory.i_block[block] * block_size, SEEK_SET );

                while ( bytes_read < block_size )
                {
                    dir_entry_len = sizeof( struct ext2_dir_entry_2 ) - sizeof( char * );
                    bytes_read += read( fd, &dir_entry, dir_entry_len ); 
                    // then we need to read the name
                    dir_entry.name = malloc( sizeof( char ) * (dir_entry.rec_len - dir_entry_len ) );
                    bytes_read += read( fd, dir_entry.name, ( dir_entry.rec_len - dir_entry_len ) );

                    // we should put the inode for each directory entry
                    // into a **set**.
                }
            }

            // once we've constructed our set, then we should consult the inode
            // bitmap to figure out which inodes in the inode table are actual
            // inodes.
            
        }
        else
        {
            printf( "This does not appear to be an ext2-formatted file system; "
                    "the magic number is not correct (expected 0x%x, "
                    "got 0x%x).\n", EXT2_MAGIC, sb.s_magic );
            // ===================== QUESTION =====================
            // what would the correct action be if we were going to attempt to
            // repair this file system?
            // ====================================================
        }

    }

    return exit_code;
}


