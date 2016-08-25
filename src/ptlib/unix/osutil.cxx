/*
 * osutil.cxx
 *
 * Operating System classes implementation
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#define _OSUTIL_CXX

#pragma implementation "timer.h"
#pragma implementation "pdirect.h"
#pragma implementation "file.h"
#pragma implementation "textfile.h"
#pragma implementation "conchan.h"
#pragma implementation "ptime.h"
#pragma implementation "timeint.h"
#pragma implementation "filepath.h"
#pragma implementation "lists.h"
#pragma implementation "pstring.h"
#pragma implementation "dict.h"
#pragma implementation "array.h"
#pragma implementation "object.h"
#pragma implementation "contain.h"

#if defined(P_LINUX) || defined(P_GNU_HURD)
#ifndef _REENTRANT
#define _REENTRANT
#endif
#elif defined(P_SOLARIS)
#define _POSIX_PTHREAD_SEMANTICS
#endif

#include <ptlib.h>


#include <fcntl.h>
#ifdef P_VXWORKS
#include <sys/times.h>
#else
#include <time.h>
#include <sys/time.h>
#include <termios.h>
#endif
#include <ctype.h>

#if defined(P_LINUX) || defined(P_GNU_HURD)

#include <mntent.h>
#include <sys/vfs.h>

#if (__GNUC_MINOR__ < 7 && __GNUC__ < 3)
#include <localeinfo.h>
#else
#define P_USE_LANGINFO
#endif

#elif defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS)
#define P_USE_STRFTIME

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/timeb.h>

#if defined(P_NETBSD)
#include <sys/statvfs.h>
#define statfs statvfs
#endif

#elif defined(P_HPUX9) 
#define P_USE_LANGINFO

#elif defined(P_AIX)
#define P_USE_STRFTIME

#include <fstab.h>
#include <sys/stat.h>

#elif defined(P_SOLARIS) 
#define P_USE_LANGINFO
#include <sys/timeb.h>
#include <sys/statvfs.h>
#include <sys/mnttab.h>

#elif defined(P_SUN4)
#include <sys/timeb.h>

#elif defined(__BEOS__)
#define P_USE_STRFTIME

#elif defined(P_IRIX)
#define P_USE_LANGINFO
#include <sys/stat.h>
#include <sys/statfs.h>
#include <stdio.h>
#include <mntent.h>

#elif defined(P_VXWORKS)
#define P_USE_STRFTIME

#elif defined(P_RTEMS)
#define P_USE_STRFTIME
#include <time.h>
#include <stdio.h>
#define random() rand()
#define srandom(a) srand(a)

#elif defined(P_QNX)
#include <sys/dcmd_blk.h>
#include <sys/statvfs.h>
#define P_USE_STRFTIME
#endif

#ifdef P_USE_LANGINFO
#include <langinfo.h>
#endif

#define  LINE_SIZE_STEP  100

#define  DEFAULT_FILE_MODE  (S_IRUSR|S_IWUSR|S_IROTH|S_IRGRP)

#include <ptlib/pprocess.h>

#if !P_USE_INLINES
#include "ptlib/osutil.inl"
#ifdef _WIN32
#include "ptlib/win32/ptlib/ptlib.inl"
#else
#include "ptlib/unix/ptlib/ptlib.inl"
#endif
#endif

#ifdef P_SUN4
extern "C" {
int on_exit(void (*f)(void), caddr_t);
int atexit(void (*f)(void))
{
  return on_exit(f, 0);
}
static char *tzname[2] = { "STD", "DST" };
};
#endif

#define new PNEW


static PMutex waterMarkMutex;
static int lowWaterMark = INT_MAX;
static int highWaterMark = 0;
  
int PX_NewHandle(const char * clsName, int fd)
{
  if (fd < 0)
    return fd;

  PWaitAndSignal m(waterMarkMutex);

  if (fd > highWaterMark) {
    highWaterMark = fd;
    lowWaterMark = fd;

    int maxHandles = PProcess::Current().GetMaxHandles();
    if (fd < (maxHandles-maxHandles/20))
      PTRACE(4, "PWLib\tFile handle high water mark set: " << fd << ' ' << clsName);
    else
      PTRACE(1, "PWLib\tFile handle high water mark within 5% of maximum: " << fd << ' ' << clsName);
  }

  if (fd < lowWaterMark) {
    lowWaterMark = fd;
    PTRACE(4, "PWLib\tFile handle low water mark set: " << fd << ' ' << clsName);
  }

  return fd;
}


static PString CanonicaliseDirectory (const PString & path)

{
  PString canonical_path;

  if (path[0] == '/')
    canonical_path = '/';
  else {
    char *p = getcwd(canonical_path.GetPointer(P_MAX_PATH), P_MAX_PATH);
    PAssertOS (p != NULL);

    // if the path doesn't end in a slash, add one
    if (canonical_path[canonical_path.GetLength()-1] != '/')
      canonical_path += '/';
  }

  const char * ptr = path;
  const char * end;

  for (;;) {
    // ignore slashes
    while (*ptr == '/' && *ptr != '\0')
      ptr++;

    // finished if end of string
    if (*ptr == '\0')
      break;

    // collect non-slash characters
    end = ptr;
    while (*end != '/' && *end != '\0')
      end++;

    // make a string out of the element
    PString element(ptr, end - ptr);
    
    if (element == "..") {
      PINDEX last_char = canonical_path.GetLength()-1;
      if (last_char > 0)
        canonical_path = canonical_path.Left(canonical_path.FindLast('/', last_char-1)+1);
    } else if (element == "." || element == "") {
    } else {
      canonical_path += element;
      canonical_path += '/';
    }
    ptr = end;
  }

  return canonical_path;
}


static PString CanonicaliseFilename(const PString & filename)
{
  if (filename.IsEmpty())
    return filename;

  PINDEX p;
  PString dirname;

  // if there is a slash in the string, extract the dirname
  if ((p = filename.FindLast('/')) != P_MAX_INDEX) {
    dirname = filename(0,p);
    while (filename[p] == '/')
      p++;
  } else
    p = 0;

  return CanonicaliseDirectory(dirname) + filename(p, P_MAX_INDEX);
}


PInt64 PString::AsInt64(unsigned base) const
{
  char * dummy;
#if defined(P_SOLARIS) || defined(__BEOS__) || defined (P_AIX) || defined(P_IRIX) || defined (P_QNX)
  return strtoll(theArray, &dummy, base);
#elif defined(P_VXWORKS) || defined(P_RTEMS)
  return strtol(theArray, &dummy, base);
#else
  return strtoq(theArray, &dummy, base);
#endif
}

PUInt64 PString::AsUnsigned64(unsigned base) const
{
  char * dummy;
#if defined(P_SOLARIS) || defined(__BEOS__) || defined (P_AIX) || defined (P_IRIX) || defined (P_QNX)
  return strtoull(theArray, &dummy, base);
#elif defined(P_VXWORKS) || defined(P_RTEMS)
  return strtoul(theArray, &dummy, base);
#else
  return strtouq(theArray, &dummy, base);
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// timer


PTimeInterval PTimer::Tick()
{
#if defined(_POSIX_MONOTONIC_CLOCK) && !defined(P_MACOSX)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec*1000LL + ts.tv_nsec/1000000LL;
#else
  #warning System does not have clock_gettime with CLOCK_MONOTONIC, using gettimeofday
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec*1000LL + tv.tv_usec/1000LL;
#endif // P_VXWORKS
}


///////////////////////////////////////////////////////////////////////////////
//
// PDirectory
//

void PDirectory::CopyContents(const PDirectory & d)
{
  if (d.entryInfo == NULL)
    entryInfo = NULL;
  else {
    entryInfo  = new PFileInfo;
    *entryInfo = *d.entryInfo;
  }
  directory   = NULL;
  entryBuffer = NULL;
}

void PDirectory::Close()
{
  if (directory != NULL) {
    PAssert(closedir(directory) == 0, POperatingSystemError);
    directory = NULL;
  }

  if (entryBuffer != NULL) {
    free(entryBuffer);
    entryBuffer = NULL;
  }

  if (entryInfo != NULL) {
    delete entryInfo;
    entryInfo = NULL;
  }
}

void PDirectory::Construct ()
{
  directory   = NULL;
  entryBuffer = NULL;
  entryInfo   = NULL;

  PString::AssignContents(CanonicaliseDirectory(*this));
}

PBoolean PDirectory::Open(int ScanMask)

{
  if (directory != NULL)
    Close();

  scanMask = ScanMask;

  if ((directory = opendir(theArray)) == NULL)
    return PFalse;

  entryBuffer = (struct dirent *)malloc(sizeof(struct dirent) + P_MAX_PATH);
  entryInfo   = new PFileInfo;

  if (Next())
    return PTrue;

  Close();
  return PFalse;
}


PBoolean PDirectory::Next()
{
  if (directory == NULL)
    return PFalse;

  do {
    do {
      struct dirent * entryPtr;
      entryBuffer->d_name[0] = '\0';
#if P_HAS_POSIX_READDIR_R == 3
      if (::readdir_r(directory, entryBuffer, &entryPtr) != 0)
        return PFalse;
      if (entryPtr != entryBuffer)
        return PFalse;
#elif P_HAS_POSIX_READDIR_R == 2
      entryPtr = ::readdir_r(directory, entryBuffer);
      if (entryPtr == NULL)
        return PFalse;
#else
      if ((entryPtr = ::readdir(directory)) == NULL)
        return PFalse;
      *entryBuffer = *entryPtr;
      strcpy(entryBuffer->d_name, entryPtr->d_name);
#endif
    } while (strcmp(entryBuffer->d_name, "." ) == 0 || strcmp(entryBuffer->d_name, "..") == 0);

    /* Ignore this file if we can't get info about it */
    if (PFile::GetInfo(*this+entryBuffer->d_name, *entryInfo) == 0)
      continue;
    
    if (scanMask == PFileInfo::AllPermissions)
      return PTrue;
  } while ((entryInfo->type & scanMask) == 0);

  return PTrue;
}


PBoolean PDirectory::IsSubDir() const
{
  if (entryInfo == NULL)
    return PFalse;

  return entryInfo->type == PFileInfo::SubDirectory;
}

PBoolean PDirectory::Restart(int newScanMask)
{
  scanMask = newScanMask;
  if (directory != NULL)
    rewinddir(directory);
  return PTrue;
}

PString PDirectory::GetEntryName() const
{
  if (entryBuffer == NULL)
    return PString();

  return entryBuffer->d_name;
}


PBoolean PDirectory::GetInfo(PFileInfo & info) const
{
  if (entryInfo == NULL)
    return PFalse;

  info = *entryInfo;
  return PTrue;
}


PBoolean PDirectory::Exists(const PString & p)
{
  struct stat sbuf;
  if (stat((const char *)p, &sbuf) != 0)
    return PFalse;

  return S_ISDIR(sbuf.st_mode);
}


PBoolean PDirectory::Create(const PString & p, int perm)
{
  PAssert(!p.IsEmpty(), "attempt to create dir with empty name");
  PINDEX last = p.GetLength()-1;
  PString str = p;
  if (p[last] == '/')
    str = p.Left(last);
#ifdef P_VXWORKS
  return mkdir(str) == 0;
#else    
  return mkdir(str, perm) == 0;
#endif
}

PBoolean PDirectory::Remove(const PString & p)
{
  PAssert(!p.IsEmpty(), "attempt to remove dir with empty name");
  PString str = p.Left(p.GetLength()-1);
  return rmdir(str) == 0;
}

PString PDirectory::GetVolume() const
{
  PString volume;

#if defined(P_QNX)
  int fd;
  char mounton[257];

  if ((fd = open(operator+("."), O_RDONLY)) != -1) {
  mounton[256] = 0;
  devctl(fd, DCMD_FSYS_MOUNTED_ON, mounton, 256, 0);
  close(fd);
  volume = strdup(mounton);
  } 

#else
  struct stat status;
  if (stat(operator+("."), &status) != -1) {
    dev_t my_dev = status.st_dev;

#if defined(P_LINUX) || defined(P_IRIX) || defined(P_GNU_HURD)

    FILE * fp = setmntent(MOUNTED, "r");
    if (fp != NULL) {
      struct mntent * mnt;
      while ((mnt = getmntent(fp)) != NULL) {
        if (stat(mnt->mnt_dir, &status) != -1 && status.st_dev == my_dev) {
          volume = mnt->mnt_fsname;
          break;
        }
      }
    }
    endmntent(fp);

#elif defined(P_SOLARIS)

    FILE * fp = fopen("/etc/mnttab", "r");
    if (fp != NULL) {
      struct mnttab mnt;
      while (getmntent(fp, &mnt) == 0) {
        if (stat(mnt.mnt_mountp, &status) != -1 && status.st_dev == my_dev) {
          volume = mnt.mnt_special;
          break;
        }
      }
    }
    fclose(fp);

#elif defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS)

    struct statfs * mnt;
    int count = getmntinfo(&mnt, MNT_NOWAIT);
    for (int i = 0; i < count; i++) {
      if (stat(mnt[i].f_mntonname, &status) != -1 && status.st_dev == my_dev) {
        volume = mnt[i].f_mntfromname;
        break;
      }
    }

#elif defined (P_AIX)

    struct fstab * fs;
    setfsent();
    while ((fs = getfsent()) != NULL) {
      if (stat(fs->fs_file, &status) != -1 && status.st_dev == my_dev) {
        volume = fs->fs_spec;
        break;
      }
    }
    endfsent();

#elif defined (P_VXWORKS)

  PAssertAlways("Get Volume - not implemented for VxWorks");
  return PString::Empty();

#else
#warning Platform requires implemetation of GetVolume()

#endif
  }
#endif

  return volume;
}

PBoolean PDirectory::GetVolumeSpace(PInt64 & total, PInt64 & free, DWORD & clusterSize) const
{
#if defined(P_LINUX) || defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS) || defined(P_GNU_HURD)

  struct statfs fs;

  if (statfs(operator+("."), &fs) == -1)
    return PFalse;

  clusterSize = fs.f_bsize;
  total = fs.f_blocks*(PInt64)fs.f_bsize;
  free = fs.f_bavail*(PInt64)fs.f_bsize;
  return PTrue;

#elif defined(P_AIX) || defined(P_VXWORKS)

  struct statfs fs;
  if (statfs((char *) ((const char *)operator+(".") ), &fs) == -1)
    return PFalse;

  clusterSize = fs.f_bsize;
  total = fs.f_blocks*(PInt64)fs.f_bsize;
  free = fs.f_bavail*(PInt64)fs.f_bsize;
  return PTrue;

#elif defined(P_SOLARIS)

  struct statvfs buf;
  if (statvfs(operator+("."), &buf) != 0)
    return PFalse;

  clusterSize = buf.f_frsize;
  total = buf.f_blocks * buf.f_frsize;
  free  = buf.f_bfree  * buf.f_frsize;

  return PTrue;
  
#elif defined(P_IRIX)

  struct statfs fs;

  if (statfs(operator+("."), &fs, sizeof(struct statfs), 0) == -1)
    return PFalse;

  clusterSize = fs.f_bsize;
  total = fs.f_blocks*(PInt64)fs.f_bsize;
  free = fs.f_bfree*(PInt64)fs.f_bsize;
  return PTrue;

#elif defined(P_QNX)

  struct statvfs fs;

  if (statvfs(operator+("."), &fs) == -1)
    return PFalse;

  clusterSize = fs.f_bsize;
  total = fs.f_blocks*(PInt64)fs.f_bsize;
  free = fs.f_bavail*(PInt64)fs.f_bsize;
  return PTrue;

#else

#warning Platform requires implemetation of GetVolumeSpace()
  return PFalse;

#endif
}

PDirectory PDirectory::GetParent() const
{
  if (IsRoot())
    return *this;
  
  return *this + "..";
}

PStringArray PDirectory::GetPath() const
{
  PStringArray path;

  if (IsEmpty())
    return path;

  PStringArray tokens = Tokenise("/");

  path.SetSize(tokens.GetSize()+1);

  PINDEX count = 1; // First path field is volume name, empty under unix
  for (PINDEX i = 0; i < tokens.GetSize(); i++) {
    if (!tokens[i])
     path[count++] = tokens[i];
  }

  path.SetSize(count);

  return path;
}


///////////////////////////////////////////////////////////////////////////////
//
// PFile
//

void PFile::SetFilePath(const PString & newName)
{
  PINDEX p;

  if ((p = newName.FindLast('/')) == P_MAX_INDEX) 
    path = CanonicaliseDirectory("") + newName;
  else
    path = CanonicaliseDirectory(newName(0,p)) + newName(p+1, P_MAX_INDEX);
}


PBoolean PFile::Open(OpenMode mode, int opt)

{
  Close();
  clear();

  if (opt > 0)
    removeOnClose = (opt & Temporary) != 0;

  if (path.IsEmpty()) {
    char templateStr[3+6+1];
    strcpy(templateStr, "PWLXXXXXX");
#ifndef P_VXWORKS
#ifdef P_RTEMS
    _reent _reent_data;
    memset(&_reent_data, 0, sizeof(_reent_data));
    os_handle = _mkstemp_r(&_reent_data, templateStr);
#else
    os_handle = mkstemp(templateStr);
#endif // P_RTEMS
    if (!ConvertOSError(os_handle))
      return PFalse;
    path = templateStr;
  } else {
#else
    static int number = 0;
    sprintf(templateStr+3, "%06d", number++);
    path = templateStr;
  }
  {
#endif // !P_VXWORKS
    int oflags = 0;
    switch (mode) {
      case ReadOnly :
        oflags |= O_RDONLY;
        if (opt == ModeDefault)
          opt = MustExist;
        break;
      case WriteOnly :
        oflags |= O_WRONLY;
        if (opt == ModeDefault)
          opt = Create|Truncate;
        break;
      case ReadWrite :
        oflags |= O_RDWR;
        if (opt == ModeDefault)
          opt = Create;
        break;
  
      default :
        PAssertAlways(PInvalidParameter);
    }
    if ((opt&Create) != 0)
      oflags |= O_CREAT;
    if ((opt&Exclusive) != 0)
      oflags |= O_EXCL;
    if ((opt&Truncate) != 0)
      oflags |= O_TRUNC;


    if (!ConvertOSError(os_handle = PX_NewHandle(GetClass(), ::open(path, oflags, DEFAULT_FILE_MODE))))
      return PFalse;
  }

#ifndef P_VXWORKS
  return ConvertOSError(::fcntl(os_handle, F_SETFD, 1));
#else
  return PTrue;
#endif
}


PBoolean PFile::SetLength(off_t len)
{
  return ConvertOSError(ftruncate(GetHandle(), len));
}


PBoolean PFile::Rename(const PFilePath & oldname, const PString & newname, PBoolean force)
{
  if (newname.Find('/') != P_MAX_INDEX) {
    errno = EINVAL;
    return PFalse;
  }

  if (rename(oldname, oldname.GetPath() + newname) == 0)
    return PTrue;

  if (!force || errno == ENOENT || !Exists(newname))
    return PFalse;

  if (!Remove(newname, PTrue))
    return PFalse;

  return rename(oldname, oldname.GetPath() + newname) == 0;
}


PBoolean PFile::Move(const PFilePath & oldname, const PFilePath & newname, PBoolean force)
{
  PFilePath from = oldname.GetDirectory() + oldname.GetFileName();
  PFilePath to = newname.GetDirectory() + newname.GetFileName();

  if (rename(from, to) == 0)
    return PTrue;

  if (errno == EXDEV)
    return Copy(from, to, force) && Remove(from);

  if (force && errno == EEXIST)
    if (Remove(to, PTrue))
      if (rename(from, to) == 0)
  return PTrue;

  return PFalse;
}


PBoolean PFile::Exists(const PFilePath & name)
{ 
#ifdef P_VXWORKS
  // access function not defined for VxWorks
  // as workaround, open the file in read-only mode
  // if it succeeds, the file exists
  PFile file(name, ReadOnly, MustExist);
  PBoolean exists = file.IsOpen();
  if(exists == PTrue)
    file.Close();
  return exists;
#else
  return access(name, 0) == 0; 
#endif // P_VXWORKS
}


PBoolean PFile::Access(const PFilePath & name, OpenMode mode)
{
#ifdef P_VXWORKS
  // access function not defined for VxWorks
  // as workaround, open the file in specified mode
  // if it succeeds, the access is allowed
  PFile file(name, mode, ModeDefault);
  PBoolean access = file.IsOpen();
  if(access == PTrue)
    file.Close();
  return access;
#else  
  int accmode;

  switch (mode) {
    case ReadOnly :
      accmode = R_OK;
      break;

    case WriteOnly :
      accmode = W_OK;
      break;

    default :
      accmode = R_OK | W_OK;
  }

  return access(name, accmode) == 0;
#endif // P_VXWORKS
}


PBoolean PFile::GetInfo(const PFilePath & name, PFileInfo & status)
{
  status.type = PFileInfo::UnknownFileType;

  struct stat s;
#ifdef P_VXWORKS
  if (stat(name, &s) != OK)
#else  
  if (lstat(name, &s) != 0)
#endif // P_VXWORKS
    return PFalse;

#ifndef P_VXWORKS
  if (S_ISLNK(s.st_mode)) {
    status.type = PFileInfo::SymbolicLink;
    if (stat(name, &s) != 0) {
      status.created     = 0;
      status.modified    = 0;
      status.accessed    = 0;
      status.size        = 0;
      status.permissions = PFileInfo::AllPermissions;
      return PTrue;
    }
  } 
  else 
#endif // !P_VXWORKS
  if (S_ISREG(s.st_mode))
    status.type = PFileInfo::RegularFile;
  else if (S_ISDIR(s.st_mode))
    status.type = PFileInfo::SubDirectory;
  else if (S_ISFIFO(s.st_mode))
    status.type = PFileInfo::Fifo;
  else if (S_ISCHR(s.st_mode))
    status.type = PFileInfo::CharDevice;
  else if (S_ISBLK(s.st_mode))
    status.type = PFileInfo::BlockDevice;
#if !defined(__BEOS__) && !defined(P_VXWORKS)
  else if (S_ISSOCK(s.st_mode))
    status.type = PFileInfo::SocketDevice;
#endif // !__BEOS__ || !P_VXWORKS

  status.created     = s.st_ctime;
  status.modified    = s.st_mtime;
  status.accessed    = s.st_atime;
  status.size        = s.st_size;
  status.permissions = s.st_mode & PFileInfo::AllPermissions;

  return PTrue;
}


PBoolean PFile::SetPermissions(const PFilePath & name, int permissions)

{
  mode_t mode = 0;

    mode |= S_IROTH;
    mode |= S_IRGRP;

  if (permissions & PFileInfo::WorldExecute)
    mode |= S_IXOTH;
  if (permissions & PFileInfo::WorldWrite)
    mode |= S_IWOTH;
  if (permissions & PFileInfo::WorldRead)
    mode |= S_IROTH;

  if (permissions & PFileInfo::GroupExecute)
    mode |= S_IXGRP;
  if (permissions & PFileInfo::GroupWrite)
    mode |= S_IWGRP;
  if (permissions & PFileInfo::GroupRead)
    mode |= S_IRGRP;

  if (permissions & PFileInfo::UserExecute)
    mode |= S_IXUSR;
  if (permissions & PFileInfo::UserWrite)
    mode |= S_IWUSR;
  if (permissions & PFileInfo::UserRead)
    mode |= S_IRUSR;

#ifdef P_VXWORKS
  PFile file(name, ReadOnly, MustExist);
  if (file.IsOpen())
    return (::ioctl(file.GetHandle(), FIOATTRIBSET, mode) >= 0);

  return PFalse;
#else  
  return chmod ((const char *)name, mode) == 0;
#endif // P_VXWORKS
}

///////////////////////////////////////////////////////////////////////////////
// PTextFile

PBoolean PTextFile::WriteLine(const PString & str)
{
  return WriteString(str) && WriteChar('\n');
}

PBoolean PTextFile::ReadLine (PString & str)

{
  char * ptr = str.GetPointer(100);
  PINDEX len = 0;
  int c;
  while ((c = ReadChar()) >= 0 && c != '\n') {
    *ptr++ = (char)c;
    if (++len >= str.GetSize())
      ptr = str.GetPointer(len + 100) + len;
  }
  *ptr = '\0';
  PAssert(str.MakeMinimumSize(), POutOfMemory);
  return c >= 0 || len > 0;
}

///////////////////////////////////////////////////////////////////////////////
// PFilePath

PFilePath::PFilePath(const PString & str)
  : PString(CanonicaliseFilename(str))
{
}


PFilePath::PFilePath(const char * cstr)
  : PString(CanonicaliseFilename(cstr))
{
}


PFilePath::PFilePath(const char * prefix, const char * dir)
  : PString()
{
  if (prefix == NULL)
    prefix = "tmp";
  
  PDirectory s(dir);
  if (dir == NULL) 
    s = PDirectory("/tmp");

#ifdef P_VXWORKS
  int number = 0;
  for (;;) {
    *this = s + prefix + psprintf("%06x", number++);
    if (!PFile::Exists(*this))
      break;
  }
#else
  PString p;
  srandom(getpid());
  for (;;) {
    *this = s + prefix + psprintf("%i_%06x", getpid(), random() % 1000000);
    if (!PFile::Exists(*this))
      break;
  }
#endif // P_VXWORKS
}


void PFilePath::AssignContents(const PContainer & cont)
{
  PString::AssignContents(cont);
  PString::AssignContents(CanonicaliseFilename(*this));
}


PString PFilePath::GetPath() const

{
  int i;

  PAssert((i = FindLast('/')) != P_MAX_INDEX, PInvalidArrayIndex);
  return Left(i+1);
}


PString PFilePath::GetTitle() const

{
  PString fn(GetFileName());
  return fn(0, fn.FindLast('.')-1);
}


PString PFilePath::GetType() const

{
  int p = FindLast('.');
  int l = (p == P_MAX_INDEX) ? 0 : (GetLength() - p);

  if (p < 0 || l < 2)
    return PString("");
  else
    return (*this)(p, P_MAX_INDEX);
}


void PFilePath::SetType(const PString & type)
{
  PINDEX dot = Find('.', FindLast('/'));
  if (dot != P_MAX_INDEX)
    Splice(type, dot, GetLength()-dot);
  else
    *this += type;
}


PString PFilePath::GetFileName() const

{
  int i;

  if ((i = FindLast('/')) == P_MAX_INDEX)
    return *this;
  else
    return Right(GetLength()-i-1);
}


PDirectory PFilePath::GetDirectory() const
{
  int i;

  if ((i = FindLast('/')) == P_MAX_INDEX)
    return "./";
  else
    return Left(i);
}


PBoolean PFilePath::IsValid(char c)
{
  return c != '/';
}


PBoolean PFilePath::IsValid(const PString & str)
{
  return str.Find('/') == P_MAX_INDEX;
}


bool PFilePath::IsAbsolutePath(const PString & path)
{
  return path[0] == '/';
}



///////////////////////////////////////////////////////////////////////////////
// PConsoleChannel

PConsoleChannel::PConsoleChannel()
{
}


PConsoleChannel::PConsoleChannel(ConsoleType type)
{
  Open(type);
}


PBoolean PConsoleChannel::Open(ConsoleType type)
{
  switch (type) {
    case StandardInput :
      os_handle = 0;
      return PTrue;

    case StandardOutput :
      os_handle = 1;
      return PTrue;

    case StandardError :
      os_handle = 2;
      return PTrue;
  }

  return PFalse;
}


PString PConsoleChannel::GetName() const
{
#ifdef P_VXWORKS
  PAssertAlways("PConsoleChannel::GetName - Not implemented for VxWorks");
  return PString("Not Implemented");
#else
  return ttyname(os_handle);
#endif // P_VXWORKS
}


PBoolean PConsoleChannel::Close()
{
  os_handle = -1;
  return PTrue;
}


bool PConsoleChannel::SetLocalEcho(bool localEcho)
{
  if (!IsOpen())
    return ConvertOSError(-2, LastReadError);

#ifdef P_VXWORKS
  PAssertAlways("PConsoleChannel::GetName - Not implemented for VxWorks");
  return PString("Not Implemented");
#else
  struct termios ios;
  if (!ConvertOSError(tcgetattr(os_handle, &ios)))
    return false;

  if (localEcho)
    ios.c_lflag |= ECHO;
  else
    ios.c_lflag &= ~ECHO;
  return ConvertOSError(tcsetattr(os_handle, TCSANOW, &ios));
#endif
}


//////////////////////////////////////////////////////
//
//  PTime
//

void PTime::SetCurrentTime()
{
#ifdef P_VXWORKS
  struct timespec ts;
  clock_gettime(0,&ts);
  theTime = ts.tv_sec;
  microseconds = ts.tv_sec*10000 + ts.tv_nsec/100000L;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  theTime = tv.tv_sec;
  microseconds = tv.tv_usec;
#endif // P_VXWORKS
}


PBoolean PTime::GetTimeAMPM()
{
#if defined(P_USE_LANGINFO)
  return strstr(nl_langinfo(T_FMT), "%p") != NULL;
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_hour = 20;
  t.tm_min = 12;
  t.tm_sec = 11;
  strftime(buf, sizeof(buf), "%X", &t);
  return strstr(buf, "20") != NULL;
#else
#warning No AMPM implementation
  return PFalse;
#endif
}


PString PTime::GetTimeAM()
{
#if defined(P_USE_LANGINFO)
  return PString(nl_langinfo(AM_STR));
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_hour = 10;
  t.tm_min = 12;
  t.tm_sec = 11;
  strftime(buf, sizeof(buf), "%p", &t);
  return buf;
#else
#warning Using default AM string
  return "AM";
#endif
}


PString PTime::GetTimePM()
{
#if defined(P_USE_LANGINFO)
  return PString(nl_langinfo(PM_STR));
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_hour = 20;
  t.tm_min = 12;
  t.tm_sec = 11;
  strftime(buf, sizeof(buf), "%p", &t);
  return buf;
#else
#warning Using default PM string
  return "PM";
#endif
}


PString PTime::GetTimeSeparator()
{
#if defined(P_LINUX) || defined(P_HPUX9) || defined(P_SOLARIS) || defined(P_IRIX) || defined(P_GNU_HURD)
#  if defined(P_USE_LANGINFO)
     char * p = nl_langinfo(T_FMT);
#  elif defined(P_LINUX) || defined(P_GNU_HURD)
     char * p = _time_info->time; 
#  endif
  char buffer[2];
  while (*p == '%' || isalpha(*p))
    p++;
  buffer[0] = *p;
  buffer[1] = '\0';
  return PString(buffer);
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_hour = 10;
  t.tm_min = 11;
  t.tm_sec = 12;
  strftime(buf, sizeof(buf), "%X", &t);
  char * sp = strstr(buf, "11") + 2;
  char * ep = sp;
  while (*ep != '\0' && !isdigit(*ep))
    ep++;
  return PString(sp, ep-sp);
#else
#warning Using default time separator
  return ":";
#endif
}

PTime::DateOrder PTime::GetDateOrder()
{
#if defined(P_USE_LANGINFO) || defined(P_LINUX) || defined(P_GNU_HURD)
#  if defined(P_USE_LANGINFO)
     char * p = nl_langinfo(D_FMT);
#  else
     char * p = _time_info->date; 
#  endif

  while (*p == '%')
    p++;
  switch (tolower(*p)) {
    case 'd':
      return DayMonthYear;
    case 'y':
      return YearMonthDay;
    case 'm':
    default:
      break;
  }
  return MonthDayYear;

#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_mday = 22;
  t.tm_mon = 10;
  t.tm_year = 99;
  strftime(buf, sizeof(buf), "%x", &t);
  char * day_pos = strstr(buf, "22");
  char * mon_pos = strstr(buf, "11");
  char * yr_pos = strstr(buf, "99");
  if (yr_pos < day_pos)
    return YearMonthDay;
  if (day_pos < mon_pos)
    return DayMonthYear;
  return MonthDayYear;
#else
#warning Using default date order
  return DayMonthYear;
#endif
}

PString PTime::GetDateSeparator()
{
#if defined(P_USE_LANGINFO) || defined(P_LINUX) || defined(P_GNU_HURD)
#  if defined(P_USE_LANGINFO)
     char * p = nl_langinfo(D_FMT);
#  else
     char * p = _time_info->date; 
#  endif
  char buffer[2];
  while (*p == '%' || isalpha(*p))
    p++;
  buffer[0] = *p;
  buffer[1] = '\0';
  return PString(buffer);
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_mday = 22;
  t.tm_mon = 10;
  t.tm_year = 99;
  strftime(buf, sizeof(buf), "%x", &t);
  char * sp = strstr(buf, "22") + 2;
  char * ep = sp;
  while (*ep != '\0' && !isdigit(*ep))
    ep++;
  return PString(sp, ep-sp);
#else
#warning Using default date separator
  return "/";
#endif
}

PString PTime::GetDayName(PTime::Weekdays day, NameType type)
{
#if defined(P_USE_LANGINFO)
  return PString(
     (type == Abbreviated) ? nl_langinfo((nl_item)(ABDAY_1+(int)day)) :
                   nl_langinfo((nl_item)(DAY_1+(int)day))
                );

#elif defined(P_LINUX) || defined(P_GNU_HURD)
  return (type == Abbreviated) ? PString(_time_info->abbrev_wkday[(int)day]) :
                       PString(_time_info->full_wkday[(int)day]);

#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_wday = day;
  strftime(buf, sizeof(buf), type == Abbreviated ? "%a" : "%A", &t);
  return buf;
#else
#warning Using default day names
  static char *defaultNames[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
    "Saturday"
  };

  static char *defaultAbbrev[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  return (type == Abbreviated) ? PString(defaultNames[(int)day]) :
                       PString(defaultAbbrev[(int)day]);
#endif
}

PString PTime::GetMonthName(PTime::Months month, NameType type) 
{
#if defined(P_USE_LANGINFO)
  return PString(
     (type == Abbreviated) ? nl_langinfo((nl_item)(ABMON_1+(int)month-1)) :
                   nl_langinfo((nl_item)(MON_1+(int)month-1))
                );
#elif defined(P_LINUX) || defined(P_GNU_HURD)
  return (type == Abbreviated) ? PString(_time_info->abbrev_month[(int)month-1]) :
                       PString(_time_info->full_month[(int)month-1]);
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_mon = month-1;
  strftime(buf, sizeof(buf), type == Abbreviated ? "%b" : "%B", &t);
  return buf;
#else
#warning Using default monthnames
  static char *defaultNames[] = {
  "January", "February", "March", "April", "May", "June", "July", "August",
  "September", "October", "November", "December" };

  static char *defaultAbbrev[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
  "Sep", "Oct", "Nov", "Dec" };

  return (type == Abbreviated) ? PString(defaultNames[(int)month-1]) :
                       PString(defaultAbbrev[(int)month-1]);
#endif
}


PBoolean PTime::IsDaylightSavings()
{
  time_t theTime = ::time(NULL);
  struct tm ts;
  return os_localtime(&theTime, &ts)->tm_isdst != 0;
}

int PTime::GetTimeZone(PTime::TimeZoneType type) 
{
#if defined(P_LINUX) || defined(P_SOLARIS) || defined (P_AIX) || defined(P_IRIX) || defined(P_GNU_HURD)
  long tz = -::timezone/60;
  if (type == StandardTime)
    return tz;
  else
    return tz + ::daylight*60;
#elif defined(P_FREEBSD) || defined(P_OPENBSD) || defined(P_NETBSD) || defined(P_MACOSX) || defined(P_MACOS) || defined(__BEOS__) || defined(P_QNX) || defined(P_GNU_HURD)
  time_t t;
  time(&t);
  struct tm ts;
  struct tm * tm = os_localtime(&t, &ts);
  int tz = tm->tm_gmtoff/60;
  if (type == StandardTime && tm->tm_isdst)
    return tz-60;
  if (type != StandardTime && !tm->tm_isdst)
    return tz + 60;
  return tz;
#elif defined(P_SUN4) 
  struct timeb tb;
  ftime(&tb);
  if (type == StandardTime || tb.dstflag == 0)
    return -tb.timezone;
  else
    return -tb.timezone + 60;
#else
#warning No timezone information
  return 0;
#endif
}

PString PTime::GetTimeZoneString(PTime::TimeZoneType type) 
{
#if defined(P_LINUX) || defined(P_SUN4) || defined(P_SOLARIS) || defined (P_AIX) || defined(P_IRIX) || defined(P_QNX)
  const char * str = (type == StandardTime) ? ::tzname[0] : ::tzname[1]; 
  if (str != NULL)
    return str;
  return PString(); 
#elif defined(P_USE_STRFTIME)
  char buf[30];
  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_isdst = type != StandardTime;
  strftime(buf, sizeof(buf), "%Z", &t);
  return buf;
#else
#warning No timezone name information
  return PString(); 
#endif
}

// note that PX_tm is local storage inside the PTime instance

#ifdef P_PTHREADS
struct tm * PTime::os_localtime(const time_t * clock, struct tm * ts)
{
  return ::localtime_r(clock, ts);
}
#else
struct tm * PTime::os_localtime(const time_t * clock, struct tm *)
{
  return ::localtime(clock);
}
#endif

#ifdef P_PTHREADS
struct tm * PTime::os_gmtime(const time_t * clock, struct tm * ts)
{
  return ::gmtime_r(clock, ts);
}
#else
struct tm * PTime::os_gmtime(const time_t * clock, struct tm *)
{
  return ::gmtime(clock);
}
#endif

// End Of File ///////////////////////////////////////////////////////////////
