/*
 * ipacl.cxx
 *
 * IP Access Control Lists
 *
 * Portable Windows Library
 *
 * Copyright (c) 2002 Equivalence Pty. Ltd.
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
#include <ptclib/ipacl.h>

#define new PNEW


PIpAccessControlEntry::PIpAccessControlEntry(PIPSocket::Address addr,
                                             PIPSocket::Address msk,
                                             PBoolean allow)
  : address(addr), mask(msk)
{
  allowed = allow;
  hidden = PFalse;
}


PIpAccessControlEntry::PIpAccessControlEntry(const PString & description)
  : address(0), mask(0xffffffff)
{
  Parse(description);
}


PIpAccessControlEntry & PIpAccessControlEntry::operator=(const PString & description)
{
  Parse(description);
  return *this;
}


PIpAccessControlEntry & PIpAccessControlEntry::operator=(const char * description)
{
  Parse(description);
  return *this;
}


PObject::Comparison PIpAccessControlEntry::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PIpAccessControlEntry), PInvalidCast);
  const PIpAccessControlEntry & other = (const PIpAccessControlEntry &)obj;

  // The larger the mask value, th more specific the range, so earlier in list
  if (mask > other.mask)
    return LessThan;
  if (mask < other.mask)
    return GreaterThan;

  if (!domain && !other.domain)
    return domain.Compare(other.domain);

  if (address > other.address)
    return LessThan;
  if (address < other.address)
    return GreaterThan;

  return EqualTo;
}


void PIpAccessControlEntry::PrintOn(ostream & strm) const
{
  if (!allowed)
    strm << '-';

  if (hidden)
    strm << '@';

  if (domain.IsEmpty())
    strm << address;
  else if (domain[0] != '\xff')
    strm << domain;
  else {
    strm << "ALL";
    return;
  }

  if (mask != 0 && mask != static_cast<DWORD>(0xffffffff))
    strm << '/' << mask;
}


void PIpAccessControlEntry::ReadFrom(istream & strm)
{
  char buffer[200];
  strm >> ws >> buffer;
  Parse(buffer);
}


PBoolean PIpAccessControlEntry::Parse(const PString & description)
{
  domain = PString();
  address = 0;

  if (description.IsEmpty())
    return PFalse;

  // Check for the allow/deny indication in first character of description
  int offset = 1;
  if (description[0] == '-')
    allowed = PFalse;
  else {
    allowed = PTrue;
    if (description[0] != '+')
      offset = 0;
  }

  // Check for indication entry is from the hosts.allow/hosts.deny file
  hidden = PFalse;
  if (description[offset] == '@') {
    offset++;
    hidden = PTrue;
  }

  if (description.Mid(offset) *= "all") {
    domain = "\xff";
    mask = 0;
    return PTrue;
  }

  PINDEX slash = description.Find('/', offset);

  PString preSlash = description(offset, slash-1);
  if (preSlash[0] == '.') {
    // If has a leading dot then assume a domain, ignore anything after slash
    domain = preSlash;
    mask = 0;
    return PTrue;
  }

  if (preSlash.FindSpan("0123456789.") != P_MAX_INDEX) {
    // If is not all numbers and dots can't be an IP number so assume hostname
    domain = preSlash;
  }
  else if (preSlash[preSlash.GetLength()-1] != '.') {
    // Must be explicit IP number if doesn't end in dot
    address = preSlash;
  }
  else {
    // Must be partial IP number, count the dots!
    PINDEX dot = preSlash.Find('.', preSlash.Find('.')+1);
    if (dot == P_MAX_INDEX) {
      // One dot
      preSlash += "0.0.0";
      mask = "255.0.0.0";
    }
    else if ((dot = preSlash.Find('.', dot+1)) == P_MAX_INDEX) {
      // has two dots
      preSlash += "0.0";
      mask = "255.255.0.0";
    }
    else if (preSlash.Find('.', dot+1) == P_MAX_INDEX) {
      // has three dots
      preSlash += "0";
      mask = "255.255.255.0";
    }
    else {
      // Has more than three dots!
      return PFalse;
    }

    address = preSlash;
    return PTrue;
  }

  if (slash == P_MAX_INDEX) {
    // No slash so assume a full mask
    mask = 0xffffffff;
    return PTrue;
  }

  PString postSlash = description.Mid(slash+1);
  if (postSlash.FindSpan("0123456789.") != P_MAX_INDEX) {
    domain = PString();
    address = 0;
    return PFalse;
  }

  if (postSlash.Find('.') != P_MAX_INDEX)
    mask = postSlash;
  else {
    DWORD bits = postSlash.AsUnsigned();
    if (bits > 32)
      mask = PSocket::Host2Net(bits);
    else
      mask = PSocket::Host2Net((DWORD)(0xffffffff << (32 - bits)));
  }

  if (mask == 0)
    domain = "\xff";

  address = (DWORD)address & (DWORD)mask;

  return PTrue;
}


PString PIpAccessControlEntry::AsString() const
{
  PStringStream str;
  str << *this;
  return str;
}


PBoolean PIpAccessControlEntry::IsValid()
{
  return address != 0 || !domain;
}


PBoolean PIpAccessControlEntry::Match(PIPSocket::Address & addr)
{
  switch (domain[0]) {
    case '\0' : // Must have address field set
      break;

    case '.' :  // Are a domain name
      return PIPSocket::GetHostName(addr).Right(domain.GetLength()) *= domain;

    case '\xff' :  // Match all
      return PTrue;

    default : // All else must be a hostname
      if (!PIPSocket::GetHostAddress(domain, address))
        return PFalse;
  }

  return (address & mask) == (addr & mask);
}


///////////////////////////////////////////////////////////////////////////////

PIpAccessControlList::PIpAccessControlList(PBoolean defAllow)
  : defaultAllowance(defAllow)
{
}


static PBoolean ReadConfigFileLine(PTextFile & file, PString & line)
{
  line = PString();

  do {
    if (!file.ReadLine(line))
      return PFalse;
  } while (line.IsEmpty() || line[0] == '#');

  PINDEX lastCharPos;
  while (line[lastCharPos = line.GetLength()-1] == '\\') {
    PString str;
    if (!file.ReadLine(str))
      return PFalse;
    line[lastCharPos] = ' ';
    line += str;
  }

  return PTrue;
}


static void ParseConfigFileExcepts(const PString & str,
                                   PStringList & entries,
                                   PStringList & exceptions)
{
  PStringArray terms = str.Tokenise(' ', PFalse);

  PBoolean hadExcept = PFalse;
  PINDEX d;
  for (d = 0; d < terms.GetSize(); d++) {
    if (terms[d] == "EXCEPT")
      hadExcept = PTrue;
    else if (hadExcept)
      exceptions.AppendString(terms[d]);
    else
      entries.AppendString(terms[d]);
  }
}


static PBoolean SplitConfigFileLine(const PString & line, PString & daemons, PString & clients)
{
  PINDEX colon = line.Find(':');
  if (colon == P_MAX_INDEX)
    return PFalse;

  daemons = line.Left(colon).Trim();

  PINDEX other_colon = line.Find(':', ++colon);
  clients = line(colon, other_colon-1).Trim();

  return PTrue;
}


static bool IsDaemonInConfigFileLine(const PString & daemon, const PString & daemons)
{
  PStringList daemonsIn, daemonsOut;
  ParseConfigFileExcepts(daemons, daemonsIn, daemonsOut);

  for (PStringList::iterator in = daemonsIn.begin(); in != daemonsIn.end(); in++) {
    if (*in == "ALL" || *in == daemon) {
      PStringList::iterator out;
      for (out = daemonsOut.begin(); out != daemonsOut.end(); out++) {
        if (*out == daemon)
          break;
      }
      if (out == daemonsOut.end())
        return true;
    }
  }

  return false;
}


static PBoolean ReadConfigFile(PTextFile & file,
                           const PString & daemon,
                           PStringList & clientsIn,
                           PStringList & clientsOut)
{
  PString line;
  while (ReadConfigFileLine(file, line)) {
    PString daemons, clients;
    if (SplitConfigFileLine(line, daemons, clients) &&
        IsDaemonInConfigFileLine(daemon, daemons)) {
      ParseConfigFileExcepts(clients, clientsIn, clientsOut);
      return PTrue;
    }
  }

  return PFalse;
}


PBoolean PIpAccessControlList::InternalLoadHostsAccess(const PString & daemonName,
                                                   const char * filename,
                                                   PBoolean allowance)
{
  PTextFile file;
  if (!file.Open(PProcess::GetOSConfigDir() + filename, PFile::ReadOnly))
    return true;

  bool ok = true;

  PStringList clientsIn;
  PStringList clientsOut;
  while (ReadConfigFile(file, daemonName, clientsIn, clientsOut)) {
    PStringList::iterator i;
    for (i = clientsOut.begin(); i != clientsOut.end(); i++) {
      if (!Add((allowance ? "-@" : "+@") + *i))
        ok = false;
    }
    for (i = clientsIn.begin(); i != clientsIn.end(); i++) {
      if (!Add((allowance ? "+@" : "-@") + *i))
        ok = false;
    }
  }

  return ok;
}


PBoolean PIpAccessControlList::LoadHostsAccess(const char * daemonName)
{
  PString daemon;
  if (daemonName != NULL)
    daemon = daemonName;
  else
    daemon = PProcess::Current().GetName();

  return InternalLoadHostsAccess(daemon, "hosts.allow", PTrue) &  // Really is a single &
         InternalLoadHostsAccess(daemon, "hosts.deny", PFalse);
}

#ifdef P_CONFIG_LIST

static const char DefaultConfigName[] = "IP Access Control List";

PBoolean PIpAccessControlList::Load(PConfig & cfg)
{
  return Load(cfg, DefaultConfigName);
}


PBoolean PIpAccessControlList::Load(PConfig & cfg, const PString & baseName)
{
  PBoolean ok = PTrue;
  PINDEX count = cfg.GetInteger(baseName & "Array Size");
  for (PINDEX i = 1; i <= count; i++) {
    if (!Add(cfg.GetString(baseName & PString(PString::Unsigned, i))))
      ok = PFalse;
  }

  return ok;
}


void PIpAccessControlList::Save(PConfig & cfg)
{
  Save(cfg, DefaultConfigName);
}


void PIpAccessControlList::Save(PConfig & cfg, const PString & baseName)
{
  PINDEX count = 0;

  for (PINDEX i = 0; i < GetSize(); i++) {
    PIpAccessControlEntry & entry = operator[](i);
    if (!entry.IsHidden()) {
      count++;
      cfg.SetString(baseName & PString(PString::Unsigned, count), entry.AsString());
    }
  }

  cfg.SetInteger(baseName & "Array Size", count);
}

#endif // P_CONFIG_LIST


PBoolean PIpAccessControlList::Add(PIpAccessControlEntry * entry)
{
  if (!entry->IsValid()) {
    delete entry;
    return PFalse;
  }

  PINDEX idx = GetValuesIndex(*entry);
  if (idx == P_MAX_INDEX) {
    Append(entry);
    return PTrue;
  }

  // Return PTrue if the newly added entry is identical to an existing one
  PIpAccessControlEntry & existing = operator[](idx);
  PBoolean ok = existing.IsClass(PIpAccessControlEntry::Class()) &&
            entry->IsClass(PIpAccessControlEntry::Class()) &&
            existing.IsAllowed() == entry->IsAllowed();

  delete entry;
  return ok;
}


PBoolean PIpAccessControlList::Add(const PString & description)
{
  return Add(CreateControlEntry(description));
}


PBoolean PIpAccessControlList::Add(PIPSocket::Address addr, PIPSocket::Address mask, PBoolean allow)
{
  PStringStream description;
  description << (allow ? '+' : '-') << addr << '/' << mask;
  return Add(description);
}


PBoolean PIpAccessControlList::Remove(const PString & description)
{
  PIpAccessControlEntry entry(description);

  if (!entry.IsValid())
    return PFalse;

  return InternalRemoveEntry(entry);
}


PBoolean PIpAccessControlList::Remove(PIPSocket::Address addr, PIPSocket::Address mask)
{
  PIpAccessControlEntry entry(addr, mask, PTrue);
  return InternalRemoveEntry(entry);
}


PBoolean PIpAccessControlList::InternalRemoveEntry(PIpAccessControlEntry & entry)
{
  PINDEX idx = GetValuesIndex(entry);
  if (idx == P_MAX_INDEX)
    return PFalse;

  RemoveAt(idx);
  return PTrue;
}


PIpAccessControlEntry * PIpAccessControlList::CreateControlEntry(const PString & description)
{
  return new PIpAccessControlEntry(description);
}


PIpAccessControlEntry * PIpAccessControlList::Find(PIPSocket::Address address) const
{
  PINDEX size = GetSize();
  if (size == 0)
    return NULL;

  for (PINDEX i = 0; i < GetSize(); i++) {
    PIpAccessControlEntry & entry = operator[](i);
    if (entry.Match(address))
      return &entry;
  }

  return NULL;
}


PBoolean PIpAccessControlList::IsAllowed(PTCPSocket & socket) const
{
  if (IsEmpty())
    return defaultAllowance;

  PIPSocket::Address address;
  if (socket.GetPeerAddress(address))
    return IsAllowed(address);

  return PFalse;
}


PBoolean PIpAccessControlList::IsAllowed(PIPSocket::Address address) const
{
  if (IsEmpty())
    return defaultAllowance;

  PIpAccessControlEntry * entry = Find(address);
  if (entry == NULL)
    return PFalse;

  return entry->IsAllowed();
}


// End of File ///////////////////////////////////////////////////////////////
