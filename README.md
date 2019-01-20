# pkgcache
Tool to create a custom FreeBSD Package Repository for OFFLINE USE.

## Purpose

Any Operating Systems (OS) nowadays are plagued by numerous issues that must be addressed.  The way this is achieved is by regular updates downloaded over the Internet.  The problem is that the turnaround is so fast that the testing phase is often shortened, bugs slip through, and the system breaks.  This is very frustrating for users that need their system.  Some OSs are worst than others, but unfortunately, FreeBSD is no exception.  Also, downloads can easily be more than 2 GB.  For users having a limited monthly bandwidth, at a cost of $20/GB for example, upgrades become very expansive.  This is especially frustrating when the upgrades are unavoidable and/or are of no use for a specific user.

## Solution

The solution is to create a local repository where you have full control when the upgrade will take place.  Since the official package repository is HUGE and covers several versions, it's more efficient to have a tool that only downloads what the user is really interested in, which is what **pkgcache** does.  This solution was inspired by this article:
https://forums.freebsd.org/threads/guide-building-a-package-repository-with-portmaster.68179/

```
USAGE: pkgcache [-timeout <sec.>] <command> [package-list-filename]
  where COMMAND is:
    add      : Interactively add packages to the package list.
    create   : Create the package list using 'pkg info'.
    download : Download relevant packages via Internet.
    help     : Display this command syntax page.
  Note that the first letter of options and commands is accepted.
```

### Step 1
Establish where to store your local repository.  I recommend some directory served by a local web server.  You may also download from one computer, then transfer the files to the local web server directory.

### Step 2
Set the computer to update offline to the local repository URL.  Although it's not the favored solution, I prefer to change the `url:` field in the `/etc/pkg/FreeBSD.conf` file because I know that the `pkg` command will not pick its packages elsewhere.  For example: `http://localhost/pkg/FreeBSD:11:amd64/latest/`

### Step 3
Generate a list of packages to keep in the local repository.  The beauty of this solution is that installed packages on the computer that downloads, the local web server, and the computer that is being upgraded do not need to be the same package sets.  The package list is just a list of packages that you are really interested in.  The default name is `.pkgcachelist` and is stored in the directory that will store the downloads.  You can kickstart it with the ADD command which will add all the installed packages into the package list.  You can add further packages with the ADD command.  If you have a file named `<filename>` with several package names (one per line), you can add it to the package list using:
```
pkgcache a < <filename>
```
Note that package dependencies will automatically be added to the package list, so that's one less thing to worry about.

### Step 4
Set the URL of the official FreeBSD repository to use in the repository list file.  The repository list file format is very simple.  The first line is the official FreeBSD repository to use.  For example, `http://pkg.freebsd.org/FreeBSD:11:amd64/`  The following lines are the packages name you are interested in (no version number), one per line.

### Step 5
Get the download going with the DOWNLOAD command.  For example,
```
pkgcache d
```

### Step 6
Update your system as you used to using the `pkg` command.  Nothing else changes.

## A note about the official package repository
There are several branches based on the moment in time for you to choose from.  For example, `FreeBSD:11:amd64` has `latest` and `quarterly`.  It also has `release_0`, `release_1` and `release_2` which I assume were created at the time 11.0, 11.1 and 11.2 were released (but don't quote me on this).  Choose the branch most appropriate for your needs.

## How to build and install pkgcache
1. Download the source files and store them in a directory
2. Go to that directory in a terminal window
3. To built the executable file, type `make`
4. To install the executable file, type `make install` as a superuser.  The Makefile will copy the executable file into the
`/usr/bin` directory.  If you want it elsewhere, feel free to copy it by hand instead.

## Version history
1.0 - 2019/01/20 - Initial release

## Compatibility
**pkgcache** has been developed and tested under FreeBSD 11.2.  It should be compatible with other versions because only standard
libraries have been used.

## Donations
Thanks for the support!  
Bitcoin: **1JbiV7rGE5kRKcecTfPv16SXag65o8aQTe**

# Have Fun!

