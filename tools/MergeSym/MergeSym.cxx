/*
 * MergeSym.cxx
 *
 * Symbol merging utility for Windows DLL's.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/pipechan.h>
#include <ptlib/pprocess.h>
#include <set>

void unsetenv(const char *);

class SymbolInfo
{
  public:
    SymbolInfo()
      : m_ordinal(0)
      , m_external(true)
      , m_noName(true)
    { }

    void Set(const PString & cpp, PINDEX ord, bool ext, bool nonam)
    {
      m_unmangled = cpp;
      m_ordinal = ord;
      m_external = ext;
      m_noName = nonam;
    }

    void SetOrdinal(PINDEX ord) { m_ordinal = ord; }
    unsigned GetOrdinal() const { return m_ordinal; }
    bool IsExternal() const { return m_external; }
    bool NoName() const { return m_noName; }

  private:
    PString m_unmangled;
    PINDEX  m_ordinal;
    bool    m_external;
    bool    m_noName;
};

typedef std::map<PCaselessString, SymbolInfo> SortedSymbolList;


std::ostream & operator<<(std::ostream & strm, const SortedSymbolList::iterator & it)
{
  strm << "    " << it->first << " @" << it->second.GetOrdinal();
  if (!it->second.NoName())
    strm << " NONAME";
  return strm << '\n';
}



PDECLARE_CLASS(MergeSym, PProcess)
  public:
    MergeSym();
    void Main();
};

PCREATE_PROCESS(MergeSym);


MergeSym::MergeSym()
  : PProcess("Equivalence", "MergeSym", 1, 8, ReleaseCode, 0)
{
}


void MergeSym::Main()
{
  cout << GetName() << " version " << GetVersion(true)
       << " on " << GetOSClass() << ' ' << GetOSName()
       << " by " << GetManufacturer() << endl;

  PArgList & args = GetArguments();
  args.Parse("vsd:x:I:");

  PFilePath lib_filename, def_filename, out_filename;

  switch (args.GetCount()) {
    case 3 :
      out_filename = args[2];
      def_filename = args[1];
      lib_filename = args[0];
      break;

    case 2 :
      def_filename = out_filename = args[1];
      lib_filename = args[0];
      break;

    case 1 :
      lib_filename = def_filename = args[0];
      def_filename.SetType(".def");
      out_filename = def_filename;
      break;

    default :
      PError << "usage: MergeSym [ -v ] [ -s ] [ -d dumpbin ] [ -x deffile[.def] ] [-I deffilepath ] libfile[.lib] [ deffile[.def] [ outfile[.def] ] ]";
      SetTerminationValue(1);
      return;
  }

  if (lib_filename.GetType().IsEmpty())
    lib_filename.SetType(".lib");

  if (!PFile::Exists(lib_filename)) {
    PError << "MergeSym: library file " << lib_filename << " does not exist.\n";
    SetTerminationValue(1);
    return;
  }

  if (def_filename.GetType().IsEmpty())
    def_filename.SetType(".def");

  if (out_filename.GetType().IsEmpty())
    out_filename.SetType(".def");

  SortedSymbolList def_symbols;
  SortedSymbolList lib_symbols;
  std::vector<bool> ordinals_used(65536);

  if (args.HasOption('x')) {
    PStringArray include_path;
    if (args.HasOption('I')) {
      PString includes = args.GetOptionString('I');
      if (includes.Find(';') == P_MAX_INDEX)
        include_path = includes.Tokenise(',', false);
      else
        include_path = includes.Tokenise(';', false);
    }
    include_path.InsertAt(0, new PString());
    PStringArray file_list = args.GetOptionString('x').Lines();
    for (PINDEX ext_index = 0; ext_index < file_list.GetSize(); ext_index++) {
      PString base_ext_filename = file_list[ext_index];
      PString ext_filename = base_ext_filename;
      if (PFilePath(ext_filename).GetType().IsEmpty())
        ext_filename += ".def";

      PINDEX previous_def_symbols_size = def_symbols.size();
      PINDEX inc_index = 0;
      for (inc_index = 0; inc_index < include_path.GetSize(); inc_index++) {
        PString trial_filename = PDirectory(include_path[inc_index]) + ext_filename;
        if (args.HasOption('v'))
          cout << "\nTrying " << trial_filename << " ..." << flush;
        PTextFile ext;
        if (ext.Open(trial_filename, PFile::ReadOnly)) {
          if (args.HasOption('v'))
            cout << "\nReading external symbols from " << ext.GetFilePath() << " ..." << flush;
          bool prefix = true;
          while (!ext.eof()) {
            PCaselessString line;
            ext >> line;
            if (prefix)
              prefix = line.Find("EXPORTS") == P_MAX_INDEX;
            else {
              PINDEX start = 0;
              while (isspace(line[start]))
                start++;
              PINDEX end = start;
              while (line[end] != '\0' && !isspace(line[end]))
                end++;
              def_symbols[line(start, end-1)]; // Create default
              if (args.HasOption('v') && def_symbols.size()%100 == 0)
                cout << '.' << flush;
            }
          }
          break;
        }
      }
      if (inc_index >= include_path.GetSize())
        PError << "MergeSym: external symbol file \"" << base_ext_filename << "\" not found.\n";
      if (args.HasOption('v'))
        cout << '\n' << (def_symbols.size() - previous_def_symbols_size)
             << " symbols read." << endl;
    }
  }

  PStringList def_file_lines;
  PINDEX next_ordinal = 0;
  PINDEX removed = 0;

  PTextFile def;
  if (def.Open(def_filename, PFile::ReadOnly)) {
    if (args.HasOption('v'))
      cout << "Reading existing ordinals..." << flush;
    bool prefix = true;
    while (!def.eof()) {
      PCaselessString line;
      def >> line;
      if (prefix) {
        def_file_lines.AppendString(line);
        if (line.Find("EXPORTS") != P_MAX_INDEX)
          prefix = false;
      }
      else {
        PINDEX start = 0;
        while (isspace(line[start]))
          start++;
        PINDEX end = start;
        while (line[end] != '\0' && !isspace(line[end]))
          end++;
        PINDEX ordpos = line.Find('@', end);
        if (ordpos != P_MAX_INDEX) {
          PINDEX ordinal = line.Mid(ordpos+1).AsInteger();
          ordinals_used[ordinal] = true;
          if (ordinal > next_ordinal)
            next_ordinal = ordinal;
          PINDEX unmanglepos = line.Find(';', ordpos);
          if (unmanglepos != P_MAX_INDEX)
            unmanglepos++;
          bool noname = line.Find("NONAME", ordpos) < unmanglepos;
          PString unmangled(line.Mid(unmanglepos));
          PCaselessString sym(line(start, end-1));
          if (def_symbols.find(sym) == def_symbols.end())
            def_symbols[sym].Set(unmangled, ordinal, false, noname);
          if (!noname && lib_symbols.find(sym) == lib_symbols.end())
            lib_symbols[sym].Set(unmangled, ordinal, false, noname);
          removed++;
          if (args.HasOption('v') && def_symbols.size()%100 == 0)
            cout << '.' << flush;
        }
      }
    }
    def.Close();
    if (args.HasOption('v'))
      cout << '\n' << removed << " symbols read." << endl;
  }
  else {
    def_file_lines.AppendString("LIBRARY " + def_filename.GetTitle());
    def_file_lines.AppendString("EXPORTS");
  }

  if (args.HasOption('v'))
    cout << "Reading library symbols..." << flush;

  unsetenv("VS_UNICODE_OUTPUT");

  PINDEX linecount = 0;
  PString dumpbin = args.GetOptionString('d', "dumpbin");
  PPipeChannel pipe(dumpbin + " /symbols '" + lib_filename + "'", PPipeChannel::ReadOnly);
  if (!pipe.IsOpen()) {
    PError << "\nMergeSym: could not run \"" << dumpbin << "\".\n";
    SetTerminationValue(2);
    return;
  }

  PTextFile symfile;
 // if (args.HasOption('s')) {
    PFilePath sym_filename = out_filename;
    sym_filename.SetType(".sym");
    if (!symfile.Open(sym_filename, PFile::WriteOnly))
      cerr << "Could not open symbol file " << sym_filename << endl;
 // }

  while (!pipe.eof()) {
    PString line;
    pipe >> line;
    symfile << line;

    PINDEX namepos = line.Find('|');
    if (namepos != P_MAX_INDEX &&
        line.Find(" UNDEF ") == P_MAX_INDEX &&
        line.Find(" External ") != P_MAX_INDEX &&
        line.Find("deleting destructor") == P_MAX_INDEX) {
      while (line[++namepos] == ' ')
        ;
      PINDEX nameend = line.FindOneOf("\r\n\t ", namepos);
      PString name = line(namepos, nameend-1);
      if (name.NumCompare("??_C@_") != EqualTo &&
          name.NumCompare("__real@") != EqualTo &&
          name.NumCompare("?__LINE__Var@") != EqualTo &&
          name.FindOneOf("'\"=") == P_MAX_INDEX &&
          lib_symbols.find(name) == lib_symbols.end()) {
        PINDEX endunmangle, unmangled = line.Find('(', nameend);
        if (unmangled != P_MAX_INDEX)
          endunmangle = line.Find(')', ++unmangled);
        else {
          unmangled = namepos;
          endunmangle = nameend;
        }
        lib_symbols[name].Set(line(unmangled, endunmangle-1), 0, false, true);
      }
    }
    if (args.HasOption('v') && linecount%500 == 0)
      cout << '.' << flush;
    linecount++;
  }

  if (args.HasOption('v'))
    cout << '\n' << lib_symbols.size() << " symbols read.\n"
            "Sorting symbols... " << flush;

  SortedSymbolList::iterator it;
  for (it = def_symbols.begin(); it != def_symbols.end(); ++it) {
    if (lib_symbols.find(it->first) != lib_symbols.end() && !it->second.IsExternal())
      removed--;
  }

  PINDEX added = 0;
  for (it = lib_symbols.begin(); it != lib_symbols.end(); ++it) {
    if (def_symbols.find(it->first) == def_symbols.end()) {
      if (++next_ordinal > 65535)
        next_ordinal = 1;
      while (ordinals_used[next_ordinal]) {
        if (++next_ordinal > 65535) {
          cerr << "Catastrophe! More than 65535 exported symbols in DLL!" << endl;
          SetTerminationValue(1);
          return;
        }
      }
      it->second.SetOrdinal(next_ordinal);
      added++;
    }
  }

  if (added == 0 && removed == 0) {
    cout << "\nNo changes to symbols.\n";
    return;
  }

  SortedSymbolList merged_symbols;
  PINDEX i;

  for (i = 0, it = def_symbols.begin(); it != def_symbols.end(); ++it, ++i) {
    if (lib_symbols.find(it->first) != lib_symbols.end() && !it->second.IsExternal())
      merged_symbols.insert(*it);
    if (args.HasOption('v') && i%100 == 0)
      cout << '.' << flush;
  }
  for (i = 0, it = lib_symbols.begin(); it != lib_symbols.end(); ++it, ++i) {
    if (def_symbols.find(it->first) == def_symbols.end())
      merged_symbols.insert(*it);
    if (args.HasOption('v') && i%100 == 0)
      cout << '.' << flush;
  }

  cout << "\nSymbols merged: " << added << " added, "
       << removed << " removed, "
       << merged_symbols.size() << " total.\n";

  if (def_filename == out_filename)
    return;

  if (args.HasOption('v'))
    cout << "Writing .DEF file..." << flush;

  // If file is read/only, set it to read/write
  PFileInfo info;
  if (PFile::GetInfo(out_filename, info)) {
    if ((info.permissions&PFileInfo::UserWrite) == 0) {
      PFile::SetPermissions(out_filename, info.permissions|PFileInfo::UserWrite);
      cout << "Setting \"" << out_filename << "\" to read/write mode." << flush;
      PFile::Copy(out_filename, out_filename+".original");
    }
  }

  if (!def.Open(out_filename, PFile::WriteOnly)) {
    cerr << "Could not create file " << out_filename << ':' << def.GetErrorText() << endl;
    SetTerminationValue(1);
    return;
  }

  for (PStringList::iterator is = def_file_lines.begin(); is != def_file_lines.end(); ++is)
    def << *is << '\n';
  for (it = merged_symbols.begin(); it != merged_symbols.end(); ++it, ++i) {
    if (!it->second.NoName())
      def << it;
  }
  for (it = merged_symbols.begin(); it != merged_symbols.end(); ++it, ++i) {
    if (it->second.NoName())
      def << it;
  }

  if (args.HasOption('v'))
    cout << merged_symbols.size() << " symbols written." << endl;
} 
// End MergeSym.cxx

 void  unsetenv(const char *name) 
 { 
     char       *envstr; 
  
     if (getenv(name) == NULL) 
         return;                 /* no work */ 
  
     /* 
      * The technique embodied here works if libc follows the Single Unix Spec 
      * and actually uses the storage passed to putenv() to hold the environ 
      * entry.  When we clobber the entry in the second step we are ensuring 
      * that we zap the actual environ member.  However, there are some libc 
      * implementations (notably recent BSDs) that do not obey SUS but copy the 
      * presented string.  This method fails on such platforms.  Hopefully all 
      * such platforms have unsetenv() and thus won't be using this hack. 
      * 
      * Note that repeatedly setting and unsetting a var using this code will 
      * leak memory. 
      */ 
  
     envstr = (char *) malloc(strlen(name) + 2); 
     if (!envstr)                /* not much we can do if no memory */ 
         return; 
  
     /* Override the existing setting by forcibly defining the var */ 
     sprintf(envstr, "%s=", name); 
     _putenv(envstr); 
  
     /* Now we can clobber the variable definition this way: */ 
     strcpy(envstr, "="); 
  
     /* 
      * This last putenv cleans up if we have multiple zero-length names as a 
      * result of unsetting multiple things. 
      */ 
     _putenv(envstr); 
 }

