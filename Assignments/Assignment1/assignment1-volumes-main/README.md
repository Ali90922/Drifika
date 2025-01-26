Sample exFAT volumes
====================

Here are some sample volumes that you can use to test your implementation of a
not quite POSIX interface for exFAT.

There are currently ~~two~~ seven volumes in this package and a `README.md` file
(that you're reading right now!).

The volumes and this `README.md` will be updated after the initial set of
volumes has been published.

Volumes
-------

### `empty.img`

This volume is an empty file system --- it has no files. You should not be able
to find any files on this volume because there are no files. The root directory 
will have directory entries (more metadata about the file system), but will not
contain any entry sets for files.

The cluster size on this volume is 4096 bytes.

### `one-file.img`

This volume is a file system with exactly one file on it: this `README.md` in
the root directory. This is the only file in the volume. All directory entries
for the root directory are contained within a single cluster, and the content of
this file is also contained within a single cluster.

The cluster size on this volume is 4096 bytes.

[Note]{.mark}: The `README.md` is not the same as what you're reading now, it
does not have the information about any of the following volumes. When the file
was written to the volume it was smaller than the cluster size.

### `one-big-file.img`

This volume is a file system with exactly one "big" file on it: an image from a
book called [Geometria et Perspectiva by Lorenz St&ouml;er]. The image is
1958023 bytes in size, and the cluster size on this volume is 4096 bytes (so the
image should take 479 total clusters for the image data). The file name is
`picture.jpg`.

The file on this volume may not have a cluster chain (see [section 6.3.4
GeneralPrimaryFlags Field], this specifically describes a property called
`NoFatChain`).

[section 6.3.4 GeneralPrimaryFlags Field]: https://learn.microsoft.com/en-us/windows/win32/fileio/exfat-specification#634-generalprimaryflags-field
[Geometria et Perspectiva by Lorenz St&ouml;er]: https://artvee.com/dl/geometria-et-perspectiva-pl-01/

### `many-small-files.img`

This volume is a file system with many small files (files with sizes smaller
than one cluster). The cluster size on this volume is 512 bytes (it's the same
size as the sector size).

This volume has some important properties:

1. The root directory consists of more than one cluster.
2. The clusters for the root directory are not contiguous (i.e., the cluster at
   `FirstClusterOfRootDirectory + 1` is the data for one of the files on the
   volume, not the continuation of directory entries for the root directory).

All the files in this volume are plain text files. Their file names are in the
format `%d.txt` (a number, then the extension `.txt`) and the content of each
file is just the number and a newline character (e.g., the file named `11.txt`
has the contents `11\n`, it is 3 bytes long). All of the files are in the root
directory.

I (me, Franklin) made these files using a small [fish] shell script:

```bash
for x in (seq 1 500)
    printf "%d\n" $x > $x.txt
end
```

[fish]: https://fishshell.com/

### `fragmented.img`

This is a volume that contains many small files and one "big" file (a file
bigger than the cluster size). The volume has been prepared so that the "big"
file is [fragmented] --- the clusters for the "big" file are *not* contiguous
(unlike the `one-big-file.img` volume).

Similar to `many-small-files.img`, the cluster size for this volume is 512 
bytes (it's the same as the sector size).

The initial set of small files was created using the same shell script as in
`many-small-files.img`:

```bash
# this time it's 5000 files, though I could only successfully make 3436 files.
# question for the reader: why only 3436? the volume's size is 4MB, and even if
# every file has 4 bytes in it, we're only talking about 20KB of data.
for x in (seq 1 5000) 
    printf "%d\n" $x > $x.txt
end
```

Then all of the even-numbered files were deleted:

```bash
# I guess I could have written a script, but this also works just fine:
rm *0.txt *2.txt *4.txt *6.txt *8.txt
```

Finally, the "big" file ([another image from the book Geometria et Perspectiva
by Lorenz St&ouml;er]) was moved into the root directory with the file name
`picture.jpg`.

[fragmented]: https://en.wikipedia.org/wiki/Fragmentation_(computing)
[another image from the book Geometria et Perspectiva by Lorenz St&ouml;er]: https://artvee.com/dl/geometria-et-perspectiva-pl-06/

### `nested-dirs.img`

This is a volume that has a deeply nested directory structure with exactly one
file on it. The cluster size on this volume is 4096 bytes (entry sets in 
directories should not span multiple clusters).

The directories on this volume were made with the following command:

```bash
# this should make 1110 directories
#   10 * 10 * 10 leaves + 10 * 10 middle + 10 at the root
# but it can only make 508 before there's "no space left on device". Question
# for the reader: why? There are no files in the volume, how could there be no
# space left?
mkdir -p {0..9}/{0..9}/{0..9}
```

One single directory is removed (so we can get at least one cluster back, the
device is full!):

```bash
rmdir 0/5/8
```

A single file is placed at `/0/5/9/hello.txt`. This file is 4 bytes in size and
contains `hi!\n`.

### `normal.img`

This is a volume that has a "normal" workload of files on it: the [template
repository for assignment 1] was cloned into this volume.

This volume has a cluster size of 4096 bytes (entry sets in directories should
not generally span multiple clusters).

[template repository for assignment 1]: https://code.cs.umanitoba.ca/comp3430-winter2025/assignment1-template

Creating more volumes
---------------------

More volumes will be published that have different properties (e.g., the root
directory spanning multiple, non-contiguous clusters; volumes that have
different cluster sizes), but you are able to make your own volumes in a similar
way to the way that we made an ext2-formatted file system.

You must use a Linux machine where you have administrative access (you
unfortunately cannot use Aviary to make these volumes). If you're using a
Windows machine, you can install the [Windows Subsystem for Linux]. If you're
using macOS you can use something like [Lima].

[Windows Subsystem for Linux]: https://learn.microsoft.com/en-us/windows/wsl/install
[Lima]: https://github.com/lima-vm/lima

```bash
truncate --size=4m volume.img           # make an empty 4MB file.
mkfs.exfat volume.img -L "volume label" # format the file as exFAT.

# you can change the cluster size of the volume using the -c option, see 
# man mkfs.exfat for more information about the options for this program.

# mount the file system (you must have administrative access to do this)
mkdir exfat-volume
sudo mount volume.img exfat-volume

# add files to the file system
# we're using `sudo` for simplicity, we really should have set the mount options
# to give our user account permission to read and write the file system.
sudo cp README.md exfat-volume

# unmount the file system so that changes are written to the file.
sudo umount exfat-volume
```
