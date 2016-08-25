/*
 * config.cxx
 *
 * System/application configuration class implementation
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

#define _CONFIG_CXX

#pragma implementation "config.h"

#include <ptlib.h>
#include <ptlib/pprocess.h>

#include "../common/pconfig.cxx"


#define SYS_CONFIG_NAME		"pwlib"

#define	APP_CONFIG_DIR		".pwlib_config/"
#define	SYS_CONFIG_DIR		"/usr/local/pwlib/"

#define	EXTENSION		".ini"
#define	ENVIRONMENT_CONFIG_STR	"/\~~environment~~\/"

#if defined(P_MACOSX) || defined(P_SOLARIS) || defined(P_FREEBSD)
#define environ (NULL)
#endif

#if defined(P_NETBSD) || defined(P_OPENBSD)
extern char **environ;
#endif

//
//  a single key/value pair
//
PDECLARE_CLASS(PXConfigValue, PCaselessString)
  public:
    PXConfigValue(const PString & theKey, const PString & theValue = "") 
      : PCaselessString(theKey), value(theValue) { }
    PString GetValue() const { return value; }
    void    SetValue(const PString & theValue) { value = theValue; }

  protected:
    PString  value;
};

//
//  a list of key/value pairs
//
PLIST(PXConfigSectionList, PXConfigValue);

//
//  a list of key value pairs, with a section name
//
PDECLARE_CLASS(PXConfigSection, PCaselessString)
  public:
    PXConfigSection(const PCaselessString & theName) 
      : PCaselessString(theName) { list.AllowDeleteObjects(); }

    PXConfigSectionList & GetList() { return list; }

  protected:
    PXConfigSectionList list;
};

//
// a list of sections
//

class PXConfig : public PList<PXConfigSection>
{
    PCLASSINFO(PXConfig, PList<PXConfigSection>);
  public:
    PXConfig();
    ~PXConfig();

    void Wait()   { mutex.Wait(); }
    void Signal() { mutex.Signal(); }

    PBoolean ReadFromFile (const PFilePath & filename);
    void ReadFromEnvironment (char **envp);

    PBoolean WriteToFile(const PFilePath & filename);
    PBoolean Flush(const PFilePath & filename);

    void SetDirty()
    {
      PTRACE_IF(4, !dirty, "PTLib\tSetting PXConfig dirty.");
      dirty = PTrue;
    }

    PBoolean      AddInstance();
    PBoolean      RemoveInstance(const PFilePath & filename);

    PINDEX    GetSectionsIndex(const PString & theSection) const;

  protected:
    int       instanceCount;
    PMutex    mutex;
    PBoolean      dirty;
    PBoolean      canSave;
};


static PBoolean IsComment(const PString& str)
{
  return str.GetLength() && strchr(";#", str[0]);
}


//
// a dictionary of configurations, keyed by filename
//
typedef PDictionary<PFilePath, PXConfig> PXConfigDictionaryBase;
class PXConfigDictionary : public PXConfigDictionaryBase
{
    PCLASSINFO(PXConfigDictionary, PXConfigDictionaryBase)
  public:
    PXConfigDictionary();
    ~PXConfigDictionary();
    PXConfig * GetFileConfigInstance(const PFilePath & key, const PFilePath & readKey);
    PXConfig * GetEnvironmentInstance();
    void RemoveInstance(PXConfig * instance);
    void WriteChangedInstances();

  protected:
    PMutex        mutex;
    PXConfig    * environmentInstance;
    PThread     * writeThread;
    PSyncPointAck stopConfigWriteThread;
};


PDECLARE_CLASS(PXConfigWriteThread, PThread)
  public:
    PXConfigWriteThread(PSyncPointAck & stop);
    ~PXConfigWriteThread();
    void Main();
  private:
    PSyncPointAck & stop;
};


PXConfigDictionary * configDict;

#define	new PNEW

//////////////////////////////////////////////////////

void PProcess::CreateConfigFilesDictionary()
{
  configFiles = new PXConfigDictionary;
}


PXConfigWriteThread::PXConfigWriteThread(PSyncPointAck & s)
  : PThread(10000, NoAutoDeleteThread, NormalPriority, "PXConfigWriteThread"),
    stop(s)
{
  Resume();
}

PXConfigWriteThread::~PXConfigWriteThread()
{
}

void PXConfigWriteThread::Main()
{
  PTRACE(4, "PTLib\tConfig file cache write back thread started.");
  while (!stop.Wait(30000))  // if stop.Wait() returns PTrue, we are shutting down
    configDict->WriteChangedInstances();   // check dictionary for items that need writing

  configDict->WriteChangedInstances();

  stop.Acknowledge();
}



PXConfig::PXConfig()
{
  // make sure content gets removed
  AllowDeleteObjects();

  // no instances, initially
  instanceCount = 0;

  // we start off clean
  dirty = PFalse;

  // normally save on exit (except for environment configs)
  canSave = PTrue;

  PTRACE(4, "PTLib\tCreated PXConfig " << this);
}

PXConfig::~PXConfig()
{
  PTRACE(4, "PTLib\tDestroyed PXConfig " << this);
}


PBoolean PXConfig::AddInstance()
{
  mutex.Wait();
  PBoolean stat = instanceCount++ == 0;
  mutex.Signal();

  return stat;
}

PBoolean PXConfig::RemoveInstance(const PFilePath & /*filename*/)
{
  mutex.Wait();

  PAssert(instanceCount != 0, "PConfig instance count dec past zero");

  PBoolean stat = --instanceCount == 0;

  mutex.Signal();

  return stat;
}

PBoolean PXConfig::Flush(const PFilePath & filename)
{
  mutex.Wait();

  PBoolean stat = instanceCount == 0;

  if (canSave && dirty) {
    WriteToFile(filename);
    dirty = PFalse;
  }

  mutex.Signal();

  return stat;
}

PBoolean PXConfig::WriteToFile(const PFilePath & filename)
{
  // make sure the directory that the file is to be written into exists
  PDirectory dir = filename.GetDirectory();
  if (!dir.Exists() && !dir.Create( 
                                   PFileInfo::UserExecute |
                                   PFileInfo::UserWrite |
                                   PFileInfo::UserRead)) {
    PProcess::PXShowSystemWarning(2000, "Cannot create PWLIB config directory");
    return PFalse;
  }

  PTextFile file;
  if (!file.Open(filename + ".new", PFile::WriteOnly))
    file.Open(filename, PFile::WriteOnly);

  if (!file.IsOpen()) {
    PProcess::PXShowSystemWarning(2001, "Cannot create PWLIB config file: " + file.GetErrorText());
    return PFalse;
  }

  for (PINDEX i = 0; i < GetSize(); i++) {
    PXConfigSectionList & section = (*this)[i].GetList();

    // If the line is a comment, output it as is
    if (IsComment((*this)[i])) {
      file << (*this)[i] << endl;
      continue;
    }

    file << "[" << (*this)[i] << "]" << endl;
    for (PINDEX j = 0; j < section.GetSize(); j++) {
      PXConfigValue & value = section[j];
      PStringArray lines = value.GetValue().Tokenise('\n', PTrue);
      // Preserve name/value pairs with no value, i.e. of the form "name="
      if (lines.IsEmpty())
	    file << value << "=" << endl;
      else {
        for (PINDEX k = 0; k < lines.GetSize(); k++) 
          file << value << "=" << lines[k] << endl;
      }
    }
    file << endl;
  }

  file.flush();
  file.SetLength(file.GetPosition());
  file.Close();

  if (file.GetFilePath() != filename) {
    if (!file.Rename(file.GetFilePath(), filename.GetFileName(), PTrue)) {
      PProcess::PXShowSystemWarning(2001, "Cannot rename config file: " + file.GetErrorText());
      return PFalse;
    }
  }

  PTRACE(4, "PTLib\tSaved config file: " << filename);
  return PTrue;
}


PBoolean PXConfig::ReadFromFile(const PFilePath & filename)
{
  PINDEX len;

  // clear out all information
  RemoveAll();

  PTRACE(4, "PTLib\tReading config file: " << filename);

  // attempt to open file
  PTextFile file;
  if (!file.Open(filename, PFile::ReadOnly))
    return PFalse;

  PXConfigSection * currentSection = NULL;

  // read lines in the file
  while (file.good()) {
    PString line;
    file >> line;
    line = line.Trim();
    if ((len = line.GetLength()) > 0) {
      // Preserve comments
      if (IsComment(line))
        Append(new PXConfigSection(line));
      else {
        if (line[0] == '[') {
          PCaselessString sectionName = (line.Mid(1,len-(line[len-1]==']'?2:1))).Trim();
          PINDEX  index;
          if ((index = GetValuesIndex(sectionName)) != P_MAX_INDEX)
            currentSection = &(*this )[index];
          else {
            currentSection = new PXConfigSection(sectionName);
            Append(currentSection);
          }
        } else if (currentSection != NULL) {
          PINDEX equals = line.Find('=');
          if (equals > 0 && equals != P_MAX_INDEX) {
            PString keyStr = line.Left(equals).Trim();
            PString valStr = line.Right(len - equals - 1).Trim();

            PINDEX index;
            if ((index = currentSection->GetList().GetValuesIndex(keyStr)) != P_MAX_INDEX)  {
              PXConfigValue & value = currentSection->GetList()[index];
              value.SetValue(value.GetValue() + '\n' + valStr);
            } else {
              PXConfigValue * value = new PXConfigValue(keyStr, valStr);
              currentSection->GetList().Append(value);
            }
          }
        }
      }
    }
  }
  
  // close the file and return
  file.Close();
  return PTrue;
}

void PXConfig::ReadFromEnvironment (char **envp)
{
  // clear out all information
  RemoveAll();

  PXConfigSection * currentSection = new PXConfigSection("Options");
  Append(currentSection);

  // can't save environment configs
  canSave = PFalse;

  if (envp == NULL)
    return;

  while (*envp != NULL && **envp != '\0') {
    PString line(*envp);
    PINDEX equals = line.Find('=');
    if (equals > 0) {
      PXConfigValue * value = new PXConfigValue(line.Left(equals), line.Right(line.GetLength() - equals - 1));
      currentSection->GetList().Append(value);
    }
    envp++;
  }
}

PINDEX PXConfig::GetSectionsIndex(const PString & theSection) const
{
  PINDEX len = theSection.GetLength()-1;
  if (theSection[len] != '\\')
    return GetValuesIndex(theSection);
  else
    return GetValuesIndex(theSection.Left(len));
}


static PBoolean LocateFile(const PString & baseName,
                       PFilePath & readFilename,
                       PFilePath & filename)
{
  // check the user's home directory first
  filename = readFilename = PProcess::Current().GetConfigurationFile();
  if (PFile::Exists(filename))
    return PTrue;

  // otherwise check the system directory for a file to read,
  // and then create 
  readFilename = SYS_CONFIG_DIR + baseName + EXTENSION;
  return PFile::Exists(readFilename);
}

///////////////////////////////////////////////////////////////////////////////

PString PProcess::GetConfigurationFile()
{
  if (configurationPaths.IsEmpty()) {
    configurationPaths.AppendString(PXGetHomeDir() + APP_CONFIG_DIR);
    configurationPaths.AppendString(SYS_CONFIG_DIR);
  }

  // See if explicit filename
  if (configurationPaths.GetSize() == 1 && !PDirectory::Exists(configurationPaths[0]))
    return configurationPaths[0];

  PString iniFilename = executableFile.GetTitle() + ".ini";

  for (PINDEX i = 0; i < configurationPaths.GetSize(); i++) {
    PFilePath cfgFile = PDirectory(configurationPaths[i]) + iniFilename;
    if (PFile::Exists(cfgFile))
      return cfgFile;
  }

  return PDirectory(configurationPaths[0]) + iniFilename;
}


////////////////////////////////////////////////////////////
//
// PXConfigDictionary
//

PXConfigDictionary::PXConfigDictionary()
{
  environmentInstance = NULL;
  writeThread = NULL;
  configDict = this;
}


PXConfigDictionary::~PXConfigDictionary()
{
  if (writeThread != NULL) {
    stopConfigWriteThread.Signal();
    writeThread->WaitForTermination();
    delete writeThread;
  }
  delete environmentInstance;
}


PXConfig * PXConfigDictionary::GetEnvironmentInstance()
{
  mutex.Wait();
  if (environmentInstance == NULL) {
    environmentInstance = new PXConfig;
    environmentInstance->ReadFromEnvironment(environ);
  }
  mutex.Signal();
  return environmentInstance;
}


PXConfig * PXConfigDictionary::GetFileConfigInstance(const PFilePath & key, const PFilePath & readKey)
{
  mutex.Wait();

  // start write thread, if not already started
  if (writeThread == NULL)
    writeThread = new PXConfigWriteThread(stopConfigWriteThread);

  PXConfig * config = GetAt(key);
  if (config != NULL) 
    config->AddInstance();
  else {
    config = new PXConfig;
    config->ReadFromFile(readKey);
    config->AddInstance();
    SetAt(key, config);
  }

  mutex.Signal();
  return config;
}

void PXConfigDictionary::RemoveInstance(PXConfig * instance)
{
  mutex.Wait();

  if (instance != environmentInstance) {
    PINDEX index = GetObjectsIndex(instance);
    PAssert(index != P_MAX_INDEX, "Cannot find PXConfig instance to remove");

    // decrement the instance count and remove it if this was the last instance
    PFilePath key = GetKeyAt(index);
    if (instance->RemoveInstance(key)) {
      instance->Flush(key);
      RemoveAt(key);
    }
  }

  mutex.Signal();
}

void PXConfigDictionary::WriteChangedInstances()
{
  mutex.Wait();

  PINDEX i;
  for (i = 0; i < GetSize(); i++) {
    PFilePath key = GetKeyAt(i);
    GetAt(key)->Flush(key);
  }

  mutex.Signal();
}

////////////////////////////////////////////////////////////
//
// PConfig::
//
// Create a new configuration object
//
////////////////////////////////////////////////////////////

void PConfig::Construct(Source src,
                        const PString & appname,
                        const PString & /*manuf*/)
{
  // handle cnvironment configs differently
  if (src == PConfig::Environment)  {
    config = configDict->GetEnvironmentInstance();
    return;
  }
  
  PString name;
  PFilePath filename, readFilename;
  
  // look up file name to read, and write
  if (src == PConfig::System)
    LocateFile(SYS_CONFIG_NAME, readFilename, filename);
  else
    filename = readFilename = PProcess::Current().GetConfigurationFile();

  // get, or create, the configuration
  config = configDict->GetFileConfigInstance(filename, readFilename);
}

PConfig::PConfig(int, const PString & name)
  : defaultSection("Options")
{
  PFilePath readFilename, filename;
  LocateFile(name, readFilename, filename);
  config = configDict->GetFileConfigInstance(filename, readFilename);
}


void PConfig::Construct(const PFilePath & theFilename)
{
  config = configDict->GetFileConfigInstance(theFilename, theFilename);
}


PConfig::~PConfig()
{
  configDict->RemoveInstance(config);
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Return a list of all the section names in the file.
//
////////////////////////////////////////////////////////////

PStringArray PConfig::GetSections() const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX sz = config->GetSize();
  PStringArray sections(sz);

  for (PINDEX i = 0; i < sz; i++)
    sections[i] = (*config)[i];

  config->Signal();

  return sections;
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Return a list of all the keys in the section. If the section name is
// not specified then use the default section.
//
////////////////////////////////////////////////////////////

PStringArray PConfig::GetKeys(const PString & theSection) const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  PStringArray keys;

  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) {
    PXConfigSectionList & section = (*config)[index].GetList();
    keys.SetSize(section.GetSize());
    for (PINDEX i = 0; i < section.GetSize(); i++)
      keys[i] = section[i];
  }

  config->Signal();
  return keys;
}



////////////////////////////////////////////////////////////
//
// PConfig::
//
// Delete all variables in the specified section. If the section name is
// not specified then use the default section.
//
////////////////////////////////////////////////////////////

void PConfig::DeleteSection(const PString & theSection)
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) {
    config->RemoveAt(index);
    config->SetDirty();
  }

  config->Signal();
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Delete the particular variable in the specified section.
//
////////////////////////////////////////////////////////////

void PConfig::DeleteKey(const PString & theSection, const PString & theKey)
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) {
    PXConfigSectionList & section = (*config)[index].GetList();
    PINDEX index_2;
    if ((index_2 = section.GetValuesIndex(theKey)) != P_MAX_INDEX) {
      section.RemoveAt(index_2);
      config->SetDirty();
    }
  }

  config->Signal();
}



////////////////////////////////////////////////////////////
//
// PConfig::
//
// Test if there is a value for the key.
//
////////////////////////////////////////////////////////////

PBoolean PConfig::HasKey(const PString & theSection, const PString & theKey) const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PBoolean present = PFalse;
  PINDEX index;
  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) {
    PXConfigSectionList & section = (*config)[index].GetList();
    present = section.GetValuesIndex(theKey) != P_MAX_INDEX;
  }

  config->Signal();
  return present;
}



////////////////////////////////////////////////////////////
//
// PConfig::
//
// Get a string variable determined by the key in the section.
//
////////////////////////////////////////////////////////////

PString PConfig::GetString(const PString & theSection,
                                    const PString & theKey, const PString & dflt) const
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PString value = dflt;
  PINDEX index;
  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) {

    PXConfigSectionList & section = (*config)[index].GetList();
    if ((index = section.GetValuesIndex(theKey)) != P_MAX_INDEX) 
      value = section[index].GetValue();
  }

  config->Signal();
  return value;
}


////////////////////////////////////////////////////////////
//
// PConfig::
//
// Set a string variable determined by the key in the section.
//
////////////////////////////////////////////////////////////

void PConfig::SetString(const PString & theSection,
                        const PString & theKey,
                        const PString & theValue)
{
  PAssert(config != NULL, "config instance not set");
  config->Wait();

  PINDEX index;
  PXConfigSection * section;
  PXConfigValue   * value;

  if ((index = config->GetSectionsIndex(theSection)) != P_MAX_INDEX) 
    section = &(*config)[index];
  else {
    section = new PXConfigSection(theSection);
    config->Append(section);
    config->SetDirty();
  } 

  if ((index = section->GetList().GetValuesIndex(theKey)) != P_MAX_INDEX) 
    value = &(section->GetList()[index]);
  else {
    value = new PXConfigValue(theKey);
    section->GetList().Append(value);
    config->SetDirty();
  }

  if (theValue != value->GetValue()) {
    value->SetValue(theValue);
    config->SetDirty();
  }

  config->Signal();
}


///////////////////////////////////////////////////////////////////////////////
