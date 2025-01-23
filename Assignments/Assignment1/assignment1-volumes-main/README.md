Sample exFAT volumes
====================

Here are some sample volumes that you can use to test your implementation of a
not quite POSIX interface for exFAT.

There are currently two volumes in this package and a `README.md` file (that
you're reading right now!).

The volumes and this `README.md` will be updated after the initial set of
volumes has been published.

Volumes
-------

### `empty.img`

This volume is an empty file system --- it has no files. You should not be able
to find any files on this volume because there are no files.

### `one-file.img`

This volume is a file system with exactly one file on it: this `README.md` in
the root directory. This is the only file in the volume. All directory entries
for the root directory are contained within a single cluster, and the content of
this file is also contained within a single cluster.

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
