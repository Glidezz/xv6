// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC                     魔数
  uint size;         // Size of file system image (blocks)  文件系统所有块
  uint nblocks;      // Number of data blocks               数据块的数量
  uint ninodes;      // Number of inodes.                   inode的数目
  uint nlog;         // Number of log blocks                log的数目
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type  表示文件的类型。常见的类型包括普通文件、目录、字符设备、块设备等。
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system 表示文件系统中连接到该索引节点的硬链接数目。
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses 包含数据块地址的数组。在这个数组中，前 NDIRECT 个元素存储直接数据块的地址，最后一个元素存储间接数据块的地址。数据块地址指向存储实际文件数据的磁盘块。
};

/*
主设备号用于标识设备的整体类型或分类，它指示设备驱动程序应该使用哪个特定的驱动程序来管理该设备。
主设备号通常与设备的类型相关，例如硬盘、光驱、网络接口等。
次设备号用于标识特定类型的设备中的不同实例或不同分区。它用于细分相同类型的设备，
以便操作系统能够区分它们并正确地与其进行通信。例如，对于块设备（如硬盘），次设备号可以用来标识不同的分区。
*/

// Inodes per block. 计算每个数据块中的索引节点数量。
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i 计算包含索引节点 i 的块号
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block 计算每个位图块中的位数数量
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b  计算包含块 b 的空闲位图块号
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
    ushort inum;        // 目录条目对应的索引节点（inode）号
    char name[DIRSIZ];  // 存储目录条目的名称
};

