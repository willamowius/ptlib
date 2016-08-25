/*
 * wincfg.cxx
 *
 * Miscellaneous implementation of classes for Win32
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

#include <ptlib.h>
#include <ptlib/pprocess.h>

#include <winuser.h>
#include <winnls.h>

#define new PNEW


const char LocalMachineStr[] = "HKEY_LOCAL_MACHINE\\";
const char CurrentUserStr[]  = "HKEY_CURRENT_USER\\";
const char ClassesRootStr[]  = "HKEY_CLASSES_ROOT\\";

///////////////////////////////////////////////////////////////////////////////
// Configuration files

#ifndef _WIN32_WCE
class SecurityID
{
  public:
    SecurityID(PSID_IDENTIFIER_AUTHORITY  pIdentifierAuthority,  // pointer to identifier authority
               BYTE nSubAuthorityCount,  // count of subauthorities
               DWORD dwSubAuthority0,  // subauthority 0
               DWORD dwSubAuthority1,  // subauthority 1
               DWORD dwSubAuthority2,  // subauthority 2
               DWORD dwSubAuthority3,  // subauthority 3
               DWORD dwSubAuthority4,  // subauthority 4
               DWORD dwSubAuthority5,  // subauthority 5
               DWORD dwSubAuthority6,  // subauthority 6
               DWORD dwSubAuthority7  // subauthority 7
              )
    {
      if (!AllocateAndInitializeSid(pIdentifierAuthority,  // pointer to identifier authority
                                    nSubAuthorityCount,  // count of subauthorities
                                    dwSubAuthority0,  // subauthority 0
                                    dwSubAuthority1,  // subauthority 1
                                    dwSubAuthority2,  // subauthority 2
                                    dwSubAuthority3,  // subauthority 3
                                    dwSubAuthority4,  // subauthority 4
                                    dwSubAuthority5,  // subauthority 5
                                    dwSubAuthority6,  // subauthority 6
                                    dwSubAuthority7,  // subauthority 7
                                    &sidptr))
        sidptr = NULL;
    }

    SecurityID(LPCTSTR lpSystemName,  // address of string for system name
               LPCTSTR lpAccountName,  // address of string for account name
               LPTSTR ReferencedDomainName,  // address of string for referenced domain 
               LPDWORD cbReferencedDomainName,  // address of size of domain string
               PSID_NAME_USE peUse   // address of SID-type indicator
              )
    {
      DWORD len = 1024;
      sidptr = (PSID)LocalAlloc(LPTR, len);
      if (sidptr != NULL) {
        if (!LookupAccountName(lpSystemName,  // address of string for system name
                               lpAccountName,  // address of string for account name
                               sidptr,  // address of security identifier
                               &len,  // address of size of security identifier
                               ReferencedDomainName,  // address of string for referenced domain 
                               cbReferencedDomainName,  // address of size of domain string
                               peUse   // address of SID-type indicator
                              )) {
          LocalFree(sidptr);
          sidptr = NULL;
        }
      }
    }
    ~SecurityID()
    {
      FreeSid(sidptr);
    }

    operator PSID() const
    {
      return sidptr;
    }

    DWORD GetLength() const
    {
      return GetLengthSid(sidptr);
    }

    PBoolean IsValid() const
    {
      return sidptr != NULL && IsValidSid(sidptr);
    }

  private:
    PSID sidptr;
};


static DWORD SecureCreateKey(HKEY rootKey, const PString & subkey, HKEY & key)
{
  SECURITY_DESCRIPTOR secdesc;
  if (!InitializeSecurityDescriptor(&secdesc, SECURITY_DESCRIPTOR_REVISION))
    return GetLastError();

  static SID_IDENTIFIER_AUTHORITY siaNTAuthority = SECURITY_NT_AUTHORITY;
  SecurityID adminID(&siaNTAuthority, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS, 
                     0, 0, 0, 0, 0, 0);
  if (!adminID.IsValid())
    return GetLastError();

  static SID_IDENTIFIER_AUTHORITY siaSystemAuthority = SECURITY_NT_AUTHORITY;
  SecurityID systemID(&siaSystemAuthority, 1,
                      SECURITY_LOCAL_SYSTEM_RID,
                      0, 0, 0, 0, 0, 0, 0);
  if (!systemID.IsValid())
    return GetLastError();

  static SID_IDENTIFIER_AUTHORITY siaCreatorAuthority = SECURITY_CREATOR_SID_AUTHORITY;
  SecurityID creatorID(&siaCreatorAuthority, 1,
                       SECURITY_CREATOR_OWNER_RID,
                       0, 0, 0, 0, 0, 0, 0);
  if (!creatorID.IsValid())
    return GetLastError();

  SID_NAME_USE snuType;
  char szDomain[100];
  DWORD cchDomainName = sizeof(szDomain);
  SecurityID userID(NULL, PProcess::Current().GetUserName(),
                    szDomain, &cchDomainName, &snuType);
  if (!userID.IsValid())
    return GetLastError();

  DWORD acl_len = sizeof(ACL) + 4*sizeof(ACCESS_ALLOWED_ACE) +
                    adminID.GetLength() + creatorID.GetLength() +
                    systemID.GetLength() + userID.GetLength();
  PBYTEArray dacl_buf(acl_len);
  PACL dacl = (PACL)dacl_buf.GetPointer(acl_len);
  if (!InitializeAcl(dacl, acl_len, ACL_REVISION2))
    return GetLastError();

  if (!AddAccessAllowedAce(dacl, ACL_REVISION2, KEY_ALL_ACCESS, adminID))
    return GetLastError();

  if (!AddAccessAllowedAce(dacl, ACL_REVISION2, KEY_ALL_ACCESS, systemID))
    return GetLastError();

  if (!AddAccessAllowedAce(dacl, ACL_REVISION2, KEY_ALL_ACCESS, creatorID))
    return GetLastError();

  if (!AddAccessAllowedAce(dacl, ACL_REVISION2, KEY_ALL_ACCESS, userID))
    return GetLastError();

  if (!SetSecurityDescriptorDacl(&secdesc, PTrue, dacl, PFalse))
    return GetLastError();

  SECURITY_ATTRIBUTES secattr;
  secattr.nLength = sizeof(secattr);
  secattr.lpSecurityDescriptor = &secdesc;
  secattr.bInheritHandle = PFalse;

  DWORD disposition;

  return RegCreateKeyEx(rootKey, subkey, 0, (char*) "", REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, &secattr, &key, &disposition);
}

#endif // _WIN32_WCE

RegistryKey::RegistryKey(const PString & subkeyname, OpenMode mode)
{
  PAssert(!subkeyname.IsEmpty(), PInvalidParameter);

  PProcess & proc = PProcess::Current();
  DWORD access = mode == ReadOnly ? KEY_READ : KEY_ALL_ACCESS;
  DWORD error;

  PVarString subkey;
  HKEY basekey;
  if (subkeyname.Find(LocalMachineStr) == 0) {
    subkey = subkeyname.Mid(19);
    basekey = HKEY_LOCAL_MACHINE;
  }
  else if (subkeyname.Find(CurrentUserStr) == 0) {
    subkey = subkeyname.Mid(18);
    basekey = HKEY_CURRENT_USER;
  }
  else if (subkeyname.Find(ClassesRootStr) == 0) {
    subkey = subkeyname.Mid(18);
    basekey = HKEY_CLASSES_ROOT;
  }
  else {
    PString adjustedSubkey = subkeyname;
    PINDEX lastCharPos = adjustedSubkey.GetLength()-1;
    while (lastCharPos > 0 && adjustedSubkey[lastCharPos] == '\\')
      adjustedSubkey.Delete(lastCharPos--, 1);
    basekey = NULL;

    subkey = adjustedSubkey;

    if (!proc.GetVersion(PFalse).IsEmpty()) {
      adjustedSubkey.Replace("CurrentVersion", proc.GetVersion(PFalse));
      PVarString keyname = adjustedSubkey;

      error = RegOpenKeyEx(HKEY_CURRENT_USER, keyname, 0, access, &key);
      if (error == ERROR_SUCCESS)
        return;

      PTRACE_IF(1, error == ERROR_ACCESS_DENIED, "PTLib\tAccess denied accessing registry entry HKEY_CURRENT_USER\\" << keyname);

      error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, access, &key);
      if (error == ERROR_SUCCESS)
        return;

      PTRACE_IF(1, error == ERROR_ACCESS_DENIED, "PTLib\tAccess denied accessing registry entry HKEY_LOCAL_MACHINE\\" << keyname);
    }

    error = RegOpenKeyEx(HKEY_CURRENT_USER, subkey, 0, access, &key);
    if (error == ERROR_SUCCESS)
      return;

    PTRACE_IF(1, error == ERROR_ACCESS_DENIED, "PTLib\tAccess denied accessing registry entry HKEY_CURRENT_USER\\" << subkey);
  }

  error = RegOpenKeyEx(basekey != NULL ? basekey : HKEY_LOCAL_MACHINE, subkey, 0, access, &key);
  if (error == ERROR_SUCCESS)
    return;

  PTRACE_IF(1, error == ERROR_ACCESS_DENIED, "PTLib\tAccess denied accessing registry entry "
            << (basekey != NULL ? "" : LocalMachineStr) << subkey);

  key = NULL;
  if (mode != Create)
    return;

  if (basekey == NULL) {
    if (PProcess::Current().IsServiceProcess())
      basekey = HKEY_LOCAL_MACHINE;
    else
      basekey = HKEY_CURRENT_USER;
  }

#ifndef _WIN32_WCE
  error = SecureCreateKey(basekey, subkey, key);
  if (error != ERROR_SUCCESS)
#endif
  {
    DWORD disposition;
    TCHAR empty[1];
    empty[0] = 0;
    error = RegCreateKeyEx(basekey, subkey, 0, empty, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &disposition);
    if (error != ERROR_SUCCESS) {
      PTRACE(1, "PTLib\tCould not create registry entry "
             << (basekey == HKEY_LOCAL_MACHINE ? LocalMachineStr :
                (basekey == HKEY_CURRENT_USER ? CurrentUserStr : "")) << subkey);
      key = NULL;
    }
  }
}


RegistryKey::~RegistryKey()
{
  if (key != NULL)
    RegCloseKey(key);
}


BOOL RegistryKey::EnumKey(PINDEX idx, PString & str)
{
  if (key == NULL)
    return PFalse;

#ifndef _WIN32_WCE
  if( RegEnumKey(key, idx, str.GetPointer(MAX_PATH),MAX_PATH) != ERROR_SUCCESS)

#else // CE has only Unicode based API
  WCHAR wcsKey[MAX_PATH] = {0};

  LONG lResult = RegEnumKey(key, idx, wcsKey, MAX_PATH);
  int size = wcslen(wcsKey);
  if( size )
  {
	wcstombs( (char*) str.GetPointer(size), wcsKey, size );
	str.SetSize(size + 1);
	str.SetAt(size, 0);
  }

  if( lResult != ERROR_SUCCESS )
#endif
    return PFalse;

  str.MakeMinimumSize();
  return PTrue;
}


BOOL RegistryKey::EnumValue(PINDEX idx, PString & str)
{
  if (key == NULL)
    return PFalse;

  DWORD sizeofname = MAX_PATH;
#ifndef _WIN32_WCE
  if (RegEnumValue(key, idx, str.GetPointer(sizeofname),
                         &sizeofname, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)

#else  // CE has only Unicode based API
  WCHAR wcsValue[MAX_PATH] = {0};

  LONG lResult = RegEnumValue(key, idx, wcsValue,
                         &sizeofname, NULL, NULL, NULL, NULL);
  sizeofname = wcslen(wcsValue);
  if( sizeofname )
  {
	wcstombs( (char*) str.GetPointer(sizeofname), wcsValue, sizeofname );
	str.SetSize(sizeofname + 1);
	str.SetAt(sizeofname, 0);
  }

  if( lResult != ERROR_SUCCESS )
#endif
    return PFalse;

  str.MakeMinimumSize();
  return PTrue;
}


BOOL RegistryKey::DeleteKey(const PString & subkey)
{
  if (key == NULL)
    return PTrue;

  return RegDeleteKey(key, PVarString(subkey)) == ERROR_SUCCESS;
}


BOOL RegistryKey::DeleteValue(const PString & value)
{
  if (key == NULL)
    return PTrue;

  return RegDeleteValue(key, PVarString(value)) == ERROR_SUCCESS;
}


BOOL RegistryKey::QueryValue(const PString & value, PString & str)
{
  if (key == NULL)
    return PFalse;

  DWORD type, size;
#ifndef _WIN32_WCE
  if (RegQueryValueEx(key, (char *)(const char *)value,
                                    NULL, &type, NULL, &size) != ERROR_SUCCESS)
#else  
    // WINCE has only Unicode based API
    LONG lResult = ERROR_SUCCESS;
    {   
	  WCHAR wcsValue[MAX_PATH];
	  mbstowcs(wcsValue, value, MAX_PATH);
	  lResult = RegQueryValueEx(key, wcsValue, NULL,
            &type, NULL, &size );
    }
  if( lResult != ERROR_SUCCESS )
#endif
    return PFalse;

  switch (type) {
    case REG_SZ :
    case REG_MULTI_SZ :
    case REG_EXPAND_SZ :
    case REG_BINARY :
#ifndef _WIN32_WCE
      return RegQueryValueEx(key, (char *)(const char *)value, NULL,
                  &type, (LPBYTE)str.GetPointer(size), &size) == ERROR_SUCCESS;
#else  
	{
		// WINCE strings are Unicode
		WCHAR wcsValue[MAX_PATH];
		mbstowcs(wcsValue, value, MAX_PATH);

		// Expand receiving buffer to accomodate wchars
		str.SetSize( str.GetSize() * sizeof (short) );
		if( RegQueryValueEx(key, wcsValue, NULL,
			&type, (LPBYTE) str.GetPointer(size), &size ) != ERROR_SUCCESS )
			return PFalse;
		else
		{
			if( type != REG_BINARY )
			{
				// Convert to char* string
				wcsncpy( wcsValue, (LPCWSTR) str.GetPointer(), MAX_PATH );
				size = wcslen(wcsValue);
				if( size )
				{
					wcstombs( (char*) str.GetPointer(size), wcsValue, size );
					str.SetSize(size + 1);
					str.SetAt(size, 0);
				}
			}
			return PTrue;
		}
	}
#endif // _WIN32_WCE

    case REG_DWORD : {
      DWORD num;
      size = sizeof(num);
#ifndef _WIN32_WCE
      if (RegQueryValueEx(key, (char *)(const char *)value, NULL,
                                &type, (LPBYTE)&num, &size) == ERROR_SUCCESS) {
        str = PString(PString::Signed, num);
#else
	  // WINCE strings are Unicode
	  WCHAR wcsValue[MAX_PATH];
	  mbstowcs(wcsValue, value, MAX_PATH);
      if (RegQueryValueEx(key, wcsValue, NULL,
             &type, (LPBYTE)&num, &size) == ERROR_SUCCESS) {

		str = PString(PString::Signed, num);
#endif
        return PTrue;
      }
    }
    default :
      PAssertAlways("Unsupported registry type.");
  }
  return PFalse;
}


BOOL RegistryKey::QueryValue(const PString & value, DWORD & num, BOOL boolean)
{
  if (key == NULL)
    return PFalse;

  DWORD type, size;
  PVarString valueName = value;

  if (RegQueryValueEx(key, valueName, NULL, &type, NULL, &size) != ERROR_SUCCESS)
    return PFalse;


  switch (type) {
    case REG_BINARY :
      if (size > sizeof(DWORD))
        return PFalse;

      num = 0;
      // Do REG_DWORD case

    case REG_DWORD :
      return RegQueryValueEx(key, valueName, NULL, &type, (LPBYTE)&num, &size) == ERROR_SUCCESS;

    case REG_SZ : {
      PVarString vstr;
      if (RegQueryValueEx(key, valueName, NULL, &type, (LPBYTE)vstr.GetPointer(size), &size) == ERROR_SUCCESS) {
        PString str = vstr;
        num = str.AsInteger();
        if (num == 0 && boolean) {
          int c = toupper(str[0]);
          num = c == 'T' || c == 'Y';
        }
        return PTrue;
      }
      break;
    }
    default :
      PAssertAlways("Unsupported registry type.");
  }

  return PFalse;
}


BOOL RegistryKey::SetValue(const PString & value, const PString & str)
{
  if (key == NULL)
    return PFalse;

  PVarString vstr(str);
  return RegSetValueEx(key, PVarString(value), 0, REG_SZ,
                       (LPBYTE)(const TCHAR *)vstr, str.GetLength()+1) == ERROR_SUCCESS;
}


BOOL RegistryKey::SetValue(const PString & value, DWORD num)
{
  if (key == NULL)
    return PFalse;

  return RegSetValueEx(key, PVarString(value), 0, REG_DWORD, (LPBYTE)&num, sizeof(num)) == ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////

#if P_CONFIG_FILE

static PBoolean IsRegistryPath(const PString & path)
{
  return (path.Find(LocalMachineStr) == 0 && path != LocalMachineStr) ||
         (path.Find(CurrentUserStr) == 0 && path != CurrentUserStr);
}


///////////////////////////////////////////////////////////////////////////////

PString PProcess::GetConfigurationFile()
{
  // No paths set, use defaults
  if (configurationPaths.IsEmpty()) {
    configurationPaths.AppendString(executableFile.GetVolume() + executableFile.GetPath());
    configurationPaths.AppendString(CurrentUserStr);
  }

  // See if explicit filename
  if (configurationPaths.GetSize() == 1 && !PDirectory::Exists(configurationPaths[0]))
    return configurationPaths[0];

  PString iniFilename = executableFile.GetTitle() + ".INI";

  for (PINDEX i = 0; i < configurationPaths.GetSize(); i++) {
    PString path = configurationPaths[i];
    if (IsRegistryPath(path))
      return path;
    PFilePath cfgFile = PDirectory(path) + iniFilename;
    if (PFile::Exists(cfgFile))
      return cfgFile;
  }

  return PString();
}


///////////////////////////////////////////////////////////////////////////////

void PConfig::Construct(Source src, const PString & appname, const PString & manuf)
{
  source = Application;

  switch (src) {
    case System :
      if (IsRegistryPath(appname))
        location = appname;
      else {
        PString dir;
        GetWindowsDirectory(dir.GetPointer(_MAX_PATH), _MAX_PATH);
        Construct(PDirectory(dir)+"WIN.INI");
      }
      break;

    case Application :
      if (IsRegistryPath(defaultSection))
        location = PString();
      else {
        PProcess & proc = PProcess::Current();
        PString cfgPath = proc.GetConfigurationFile();
        if (IsRegistryPath(cfgPath))
          location = cfgPath;
        else if (!cfgPath) {
          source = NumSources; // Make a file based config
          location = cfgPath;
        }
        else {
          location = "SOFTWARE\\";
          if (!manuf)
            location += manuf;
          else if (!proc.GetManufacturer())
            location += proc.GetManufacturer();
          else
            location += "PWLib";
          location += PDIR_SEPARATOR;
          if (appname.IsEmpty())
            location += proc.GetName();
          else
            location += appname;
          location += "\\CurrentVersion\\";
        }
      }
      break;

    default :
      source = src;
  }
}


void PConfig::Construct(const PFilePath & filename)
{
  location = filename;
  source = NumSources;
}


static void RecurseRegistryKeys(const PString & location,
                                PINDEX baseLength,
                                PStringList &sections)
{
  RegistryKey registry(location, RegistryKey::ReadOnly);
  PString name;
  for (PINDEX idx = 0; registry.EnumKey(idx, name); idx++) {
    RecurseRegistryKeys(location + name + '\\', baseLength, sections);
    sections.AppendString(location.Mid(baseLength) + name);
  }
}


static PString PGetPrivateProfileString(const char * lpAppName,
                                        const char * lpKeyName,
                                        const char * lpDefault,
                                        const char * lpFileName)
{
  PString buffer;

  DWORD numNulls = lpAppName != NULL && lpKeyName != NULL ? 1 : 2;
  DWORD size = 100;
  while (size <= 100000 &&
                ::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault,
                                          buffer.GetPointer(size+numNulls), size+numNulls,
                                          lpFileName) == size)
    size *= 10;

  return buffer;
}


PStringArray PConfig::GetSections() const
{
  PStringList sections;

  switch (source) {
    case Application :
      RecurseRegistryKeys(location, location.GetLength(), sections);
      break;

    case NumSources :
      PString buffer = PGetPrivateProfileString(NULL, NULL, "", location);
      char * ptr = buffer.GetPointer();
      while (*ptr != '\0') {
        sections.AppendString(ptr);
        ptr += strlen(ptr)+1;
      }
      break;
  }

  return sections;
}


PStringArray PConfig::GetKeys(const PString & section) const
{
  PStringList keys;

  switch (source) {
    case Environment : {
      char ** ptr = _environ;
      while (*ptr != NULL) {
        PString buf = *ptr++;
        keys.AppendString(buf.Left(buf.Find('=')));
      }
      break;
    }

    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location + section, RegistryKey::ReadOnly);
      PString name;
      for (PINDEX idx = 0; registry.EnumValue(idx, name); idx++)
        keys.AppendString(name);
      break;
    }

    case NumSources :
      PAssert(!section.IsEmpty(), PInvalidParameter);
      PString buffer = PGetPrivateProfileString(section, NULL, "", location);
      char * ptr = buffer.GetPointer();
      while (*ptr != '\0') {
        keys.AppendString(ptr);
        ptr += strlen(ptr)+1;
      }
  }

  return keys;
}


void PConfig::DeleteSection(const PString & section)
{
  switch (source) {
    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location, RegistryKey::ReadWrite);
      registry.DeleteKey(section);
      break;
    }

    case NumSources :
      PAssert(!section.IsEmpty(), PInvalidParameter);
      PAssertOS(WritePrivateProfileString(section, NULL, NULL, location));
  }
}


void PConfig::DeleteKey(const PString & section, const PString & key)
{
  switch (source) {
    case Environment :
      PAssert(!key.IsEmpty() && key.Find('=') == P_MAX_INDEX, PInvalidParameter);
      _putenv(key + "=");
      break;

    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location + section, RegistryKey::ReadWrite);
      registry.DeleteValue(key);
      break;
    }

    case NumSources :
      PAssert(!key.IsEmpty(), PInvalidParameter);
      PAssert(!section.IsEmpty(), PInvalidParameter);
      PAssertOS(WritePrivateProfileString(section, key, NULL, location));
  }
}


PBoolean PConfig::HasKey(const PString & section, const PString & key) const
{
  switch (source) {
    case Environment :
      PAssert(!key.IsEmpty(), PInvalidParameter);
      return getenv(key) != NULL;

    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location + section, RegistryKey::ReadOnly);
      PString dummy;
      return registry.QueryValue(key, dummy);
    }

    case NumSources :
      PAssert(!key.IsEmpty() && !section.IsEmpty(), PInvalidParameter);
      static const char dflt[] = "<<<<<====---PConfig::DefaultValueString---====>>>>>";
      return PGetPrivateProfileString(section, key, dflt, location) != dflt;
  }

  return PFalse;
}


PString PConfig::GetString(const PString & section,
                               const PString & key, const PString & dflt) const
{
  PString str;

  switch (source) {
    case Environment : {
      PAssert(!key.IsEmpty() && key.Find('=') == P_MAX_INDEX, PInvalidParameter);
      char * env = getenv(key);
      if (env != NULL)
        str = env;
      else
        str = dflt;
      break;
    }

    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location + section, RegistryKey::ReadOnly);
      if (!registry.QueryValue(key, str))
        str = dflt;
      break;
    }

    case NumSources :
      PAssert(!key.IsEmpty() && !section.IsEmpty(), PInvalidParameter);
      str = PGetPrivateProfileString(section, key, dflt, location);
      str.MakeMinimumSize();
  }

  return str;
}


void PConfig::SetString(const PString & section,
                                    const PString & key, const PString & value)
{
  switch (source) {
    case Environment :
      PAssert(!key.IsEmpty() && key.Find('=') == P_MAX_INDEX, PInvalidParameter);
      _putenv(key + "=" + value);
      break;

    case Application : {
      PAssert(!section.IsEmpty(), PInvalidParameter);
      RegistryKey registry(location + section, RegistryKey::Create);
      registry.SetValue(key, value);
      break;
    }

    case NumSources :
      PAssert(!key.IsEmpty() && !section.IsEmpty(), PInvalidParameter);
      PAssertOS(WritePrivateProfileString(section, key, value, location));
  }
}


PBoolean PConfig::GetBoolean(const PString & section,
                                          const PString & key, PBoolean dflt) const
{
  if (source != Application) {
    PString str = GetString(section, key, dflt ? "T" : "F").ToUpper();
    int c = toupper(str[0]);
    return c == 'T' || c == 'Y' || str.AsInteger() != 0;
  }

  PAssert(!section.IsEmpty(), PInvalidParameter);
  RegistryKey registry(location + section, RegistryKey::ReadOnly);

  DWORD value;
  if (!registry.QueryValue(key, value, PTrue))
    return dflt;

  return value != 0;
}


void PConfig::SetBoolean(const PString & section, const PString & key, PBoolean value)
{
  if (source != Application)
    SetString(section, key, value ? "True" : "False");
  else {
    PAssert(!section.IsEmpty(), PInvalidParameter);
    RegistryKey registry(location + section, RegistryKey::Create);
    registry.SetValue(key, value ? 1 : 0);
  }
}


long PConfig::GetInteger(const PString & section,
                                          const PString & key, long dflt) const
{
  if (source != Application) {
    PString str(PString::Signed, dflt);
    return GetString(section, key, str).AsInteger();
  }

  PAssert(!section.IsEmpty(), PInvalidParameter);
  RegistryKey registry(location + section, RegistryKey::ReadOnly);

  DWORD value;
  if (!registry.QueryValue(key, value, PFalse))
    return dflt;

  return value;
}


void PConfig::SetInteger(const PString & section, const PString & key, long value)
{
  if (source != Application) {
    PString str(PString::Signed, value);
    SetString(section, key, str);
  }
  else {
    PAssert(!section.IsEmpty(), PInvalidParameter);
    RegistryKey registry(location + section, RegistryKey::Create);
    registry.SetValue(key, value);
  }
}

#endif // P_CONFIG_FILE


// End Of File ///////////////////////////////////////////////////////////////
