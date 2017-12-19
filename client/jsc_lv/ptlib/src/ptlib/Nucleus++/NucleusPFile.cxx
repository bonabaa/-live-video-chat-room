#include <ptlib.h>

///////////////////////////////////////////////////////////////////////////////
//
// PFile
//

void PFile::SetFilePath(const PString & newName)
{
#ifdef __NUCLEUS_PLUS__
PAssertAlways("No PFile under Nucleus");
#else
  PINDEX p;

  if ((p = newName.FindLast('/')) == P_MAX_INDEX) 
    path = CanonicaliseDirectory("") + newName;
  else
    path = CanonicaliseDirectory(newName(0,p)) + newName(p+1, P_MAX_INDEX);
#endif
}


PBoolean PFile::Open(OpenMode mode, int opt)

{
#ifdef __NUCLEUS_PLUS__
PAssertAlways("No PFile under Nucleus");
return PTrue;
#else
  Close();
  clear();

  if (path.IsEmpty()) {
    char * tmp = tempnam(NULL, "PWL");
    PAssert(tmp != NULL, POperatingSystemError);
    path = PString(tmp);
    runtime_free(tmp);
  }

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

  removeOnClose = opt & Temporary;

  if (!ConvertOSError(os_handle = ::open(path, oflags, DEFAULT_FILE_MODE)))
    return PFalse;

  return ConvertOSError(::fcntl(os_handle, F_SETFD, 1));
#endif
}


PBoolean PFile::SetLength(off_t len)
{
#ifdef __NUCLEUS_PLUS__
PAssertAlways("No PFile under Nucleus");
  return PTrue;
#else
  return ConvertOSError(ftruncate(GetHandle(), len));
#endif
}


PBoolean PFile::Rename(const PFilePath & oldname, const PString & newname, PBoolean force)
{
#ifdef __NUCLEUS_PLUS__
  PAssertAlways("No PFile under Nucleus");
  return PTrue;
#else
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
#endif
}


PBoolean PFile::Access(const PFilePath & name, OpenMode mode)
{
#ifdef __NUCLEUS_PLUS__
  PAssertAlways("No PFile under Nucleus");
  return PTrue;
#else
  int accmode;

  switch (mode) {
    case ReadOnly :
      accmode = 2;
      break;

    case WriteOnly :
      accmode = 4;
      break;

    default :
      accmode = 6;
  }

  return access(name, accmode) == 0;
#endif
}


PBoolean PFile::GetInfo(const PFilePath & name, PFileInfo & status)
{
#ifdef __NUCLEUS_PLUS__
  PAssertAlways("No PFile under Nucleus");
#else
  status.type = PFileInfo::UnknownFileType;

  struct stat s;
  if (lstat(name, &s) != 0)
    return PFalse;

  if (S_ISLNK(s.st_mode)) {
    status.type = PFileInfo::SymbolicLink;
    if (stat(name, &s) != 0) 
      return PFalse;
  } 

  status.created     = s.st_ctime;
  status.modified    = s.st_mtime;
  status.accessed    = s.st_atime;
  status.size        = s.st_size;
  status.permissions = s.st_mode & PFileInfo::AllPermissions;

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
#ifndef __BEOS__
  else if (S_ISSOCK(s.st_mode))
    status.type = PFileInfo::SocketDevice;
#endif // !__BEOS__
#endif

  return PTrue;
}


PBoolean PFile::SetPermissions(const PFilePath & name, int permissions)

{
#ifdef __NUCLEUS_PLUS__
  PAssertAlways("No PFile under Nucleus");
  return PTrue;
#else
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

  return chmod ((const char *)name, mode) == 0;
#endif
}

