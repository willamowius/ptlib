/*
 * pldap.cxx
 *
 * Lightweight Directory Access Protocol interface class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2003 Equivalence Pty. Ltd.
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

#ifdef __GNUC__
#pragma implementation "pldap.h"
#endif

#include <ptlib.h>

#include <ptlib/sockets.h>
#include <ptclib/pldap.h>

#define new PNEW


#if P_LDAP

#define LDAP_DEPRECATED 1
#include <ldap.h>


#if defined(_MSC_VER)
  #pragma comment(lib, P_LDAP_LIBRARY)
  #pragma message("LDAP support enabled")
#endif

///////////////////////////////////////////////////////////////////////////////

PLDAPSession::PLDAPSession(const PString & baseDN)
  : ldapContext(NULL),
    errorNumber(LDAP_SUCCESS),
    protocolVersion(LDAP_VERSION3),
    defaultBaseDN(baseDN),
    searchLimit(0),
    timeout(0, 30),
    multipleValueSeparator('\n')
{
}


PLDAPSession::~PLDAPSession()
{
  Close();
}


PBoolean PLDAPSession::Open(const PString & server, WORD port)
{
  Close();

  PString host = server;
  PINDEX colon = server.Find(':');
  if (colon != P_MAX_INDEX) {
    host = server.Left(colon);
    port = PIPSocket::GetPortByService(server.Mid(colon+1), "tcp");
  }

  ldapContext = ldap_init(server, port);
  if (!IsOpen())
    return PFalse;

  SetOption(LDAP_OPT_PROTOCOL_VERSION, protocolVersion);
  return PTrue;
}


PBoolean PLDAPSession::Close()
{
  if (!IsOpen())
    return PFalse;

  ldap_unbind(ldapContext);
  ldapContext = NULL;
  return PTrue;
}


PBoolean PLDAPSession::SetOption(int optcode, int value)
{
  if (!IsOpen())
    return PFalse;

  return ldap_set_option(ldapContext, optcode, &value);
}


PBoolean PLDAPSession::SetOption(int optcode, void * value)
{
  if (!IsOpen())
    return PFalse;

  return ldap_set_option(ldapContext, optcode, value);
}

PBoolean PLDAPSession::StartTLS()
{
#ifdef LDAP_EXOP_START_TLS
  errorNumber = ldap_start_tls_s(ldapContext, NULL, NULL);
  return errorNumber == LDAP_SUCCESS;
#else
  return LDAP_OPERATIONS_ERROR;
#endif
}

PBoolean PLDAPSession::Bind(const PString & who,
                        const PString & passwd,
                        AuthenticationMethod authMethod)
{
  if (!IsOpen())
    return PFalse;

  const char * whoPtr;
  if (who.IsEmpty())
    whoPtr = NULL;
  else
    whoPtr = who;

#ifdef SOLARIS
  static const int AuthMethodCode[NumAuthenticationMethod2] = {
    LDAP_AUTH_SIMPLE, LDAP_AUTH_SASL, LDAP_AUTH_KRBV41_30, LDAP_AUTH_KRBV42_30
#else
  static const int AuthMethodCode[NumAuthenticationMethod] = {
    LDAP_AUTH_SIMPLE, LDAP_AUTH_SASL, LDAP_AUTH_KRBV4
#endif
  };
  errorNumber = ldap_bind_s(ldapContext, whoPtr, passwd, AuthMethodCode[authMethod]);
  return errorNumber == LDAP_SUCCESS;
}


PLDAPSession::ModAttrib::ModAttrib(const PString & n, Operation o)
  : name(n),
    op(o)
{
}


void PLDAPSession::ModAttrib::SetLDAPMod(struct ldapmod & mod, Operation defaultOp)
{
  mod.mod_type = (char *)(const char *)name;

  Operation realOp = op == NumOperations ? defaultOp : op;
  static const int OpCode[NumOperations] = {
    LDAP_MOD_ADD, LDAP_MOD_REPLACE, LDAP_MOD_DELETE
  };
  mod.mod_op = OpCode[realOp];

  if (IsBinary())
    mod.mod_op |= LDAP_MOD_BVALUES;

  SetLDAPModVars(mod);
}


PLDAPSession::StringModAttrib::StringModAttrib(const PString & name,
                                               Operation op)
  : ModAttrib(name, op)
{
}


PLDAPSession::StringModAttrib::StringModAttrib(const PString & name,
                                               const PString & value,
                                               Operation op)
  : ModAttrib(name, op)
{
  AddValue(value);
}


PLDAPSession::StringModAttrib::StringModAttrib(const PString & name,
                                               const PStringList & vals,
                                               Operation op)
  : ModAttrib(name, op),
    values(vals)
{
}


void PLDAPSession::StringModAttrib::SetValue(const PString & value)
{
  values.RemoveAll();
  values.AppendString(value);
}


void PLDAPSession::StringModAttrib::AddValue(const PString & value)
{
  values.AppendString(value);
}


PBoolean PLDAPSession::StringModAttrib::IsBinary() const
{
  return PFalse;
}


void PLDAPSession::StringModAttrib::SetLDAPModVars(struct ldapmod & mod)
{
  pointers.SetSize(values.GetSize()+1);
  PINDEX i;
  for (i = 0; i < values.GetSize(); i++)
    pointers[i] = values[i].GetPointer();
  pointers[i] = NULL;
  mod.mod_values = pointers.GetPointer();
}


PLDAPSession::BinaryModAttrib::BinaryModAttrib(const PString & name,
                                               Operation op)
  : ModAttrib(name, op)
{
}


PLDAPSession::BinaryModAttrib::BinaryModAttrib(const PString & name,
                                               const PBYTEArray & value,
                                               Operation op)
  : ModAttrib(name, op)
{
  AddValue(value);
}


PLDAPSession::BinaryModAttrib::BinaryModAttrib(const PString & name,
                                               const PArray<PBYTEArray> & vals,
                                               Operation op)
  : ModAttrib(name, op),
    values(vals)
{
}


void PLDAPSession::BinaryModAttrib::SetValue(const PBYTEArray & value)
{
  values.RemoveAll();
  values.Append(new PBYTEArray(value));
}


void PLDAPSession::BinaryModAttrib::AddValue(const PBYTEArray & value)
{
  values.Append(new PBYTEArray(value));
}


PBoolean PLDAPSession::BinaryModAttrib::IsBinary() const
{
  return PTrue;
}


void PLDAPSession::BinaryModAttrib::SetLDAPModVars(struct ldapmod & mod)
{
  pointers.SetSize(values.GetSize()+1);
  bervals.SetSize(values.GetSize()*sizeof(berval));
  berval * ber = (berval *)bervals.GetPointer();
  PINDEX i;
  for (i = 0; i < values.GetSize(); i++) {
    ber[i].bv_val = (char *)values[i].GetPointer();
    ber[i].bv_len = values[i].GetSize();
    pointers[i] = &ber[i];
  }
  pointers[i] = NULL;
  mod.mod_bvalues = pointers.GetPointer();
}


static LDAPMod ** CreateLDAPModArray(const PArray<PLDAPSession::ModAttrib> & attributes,
                                     PLDAPSession::ModAttrib::Operation defaultOp,
                                     PBYTEArray & storage)
{
  PINDEX count = attributes.GetSize();
  storage.SetSize(count*sizeof(LDAPMod) + (count+1)*sizeof(LDAPMod *));

  LDAPMod ** attrs = (LDAPMod **)storage.GetPointer();
  LDAPMod *  attr  = (LDAPMod * )&attrs[count+1];
  for (PINDEX i = 0; i < count; i++) {
    attrs[i] = &attr[i];
    attributes[i].SetLDAPMod(attr[i], defaultOp);
  }

  return attrs;
}


static PArray<PLDAPSession::ModAttrib> AttribsFromDict(const PStringToString & attributes)
{
  PArray<PLDAPSession::ModAttrib> attrs(attributes.GetSize());

  for (PINDEX i = 0; i < attributes.GetSize(); i++)
    attrs.SetAt(i, new PLDAPSession::StringModAttrib(attributes.GetKeyAt(i),
                                                     attributes.GetDataAt(i).Lines()));

  return attrs;
}


static PArray<PLDAPSession::ModAttrib> AttribsFromArray(const PStringArray & attributes)
{
  PArray<PLDAPSession::ModAttrib> attrs;

  for (PINDEX i = 0; i < attributes.GetSize(); i++) {
    PString attr = attributes[i];
    PINDEX equal = attr.Find('=');
    if (equal != P_MAX_INDEX)
      attrs.Append(new PLDAPSession::StringModAttrib(attr.Left(equal),
                                                     attr.Mid(equal+1).Lines()));
  }

  return attrs;
}


static PArray<PLDAPSession::ModAttrib> AttribsFromStruct(const PLDAPStructBase & attributes)
{
  PArray<PLDAPSession::ModAttrib> attrs;

  for (PINDEX i = 0; i < attributes.GetNumAttributes(); i++) {
    PLDAPAttributeBase & attr = attributes.GetAttribute(i);
    if (attr.IsBinary())
      attrs.Append(new PLDAPSession::BinaryModAttrib(attr.GetName(), attr.ToBinary()));
    else {
      PString str = attr.ToString();
      if (!str)
        attrs.Append(new PLDAPSession::StringModAttrib(attr.GetName(), str));
    }
  }

  return attrs;
}


PBoolean PLDAPSession::Add(const PString & dn, const PArray<ModAttrib> & attributes)
{
  if (!IsOpen())
    return PFalse;

  PBYTEArray storage;
  int msgid;
  errorNumber = ldap_add_ext(ldapContext,
                             dn,
                             CreateLDAPModArray(attributes, ModAttrib::Add, storage),
                             NULL,
                             NULL,
                             &msgid);
  if (errorNumber != LDAP_SUCCESS)
    return PFalse;

  P_timeval tval = timeout;
  LDAPMessage * result = NULL;
  ldap_result(ldapContext, msgid, LDAP_MSG_ALL, tval, &result);
  if (result)
    errorNumber = ldap_result2error(ldapContext, result, PTrue);

  return errorNumber == LDAP_SUCCESS;
}


PBoolean PLDAPSession::Add(const PString & dn, const PStringToString & attributes)
{
  return Add(dn, AttribsFromDict(attributes));
}


PBoolean PLDAPSession::Add(const PString & dn, const PStringArray & attributes)
{
  return Add(dn, AttribsFromArray(attributes));
}


PBoolean PLDAPSession::Add(const PString & dn, const PLDAPStructBase & attributes)
{
  return Add(dn, AttribsFromStruct(attributes));
}


PBoolean PLDAPSession::Modify(const PString & dn, const PArray<ModAttrib> & attributes)
{
  if (!IsOpen())
    return PFalse;

  PBYTEArray storage;
  int msgid;
  errorNumber = ldap_modify_ext(ldapContext,
                                dn,
                                CreateLDAPModArray(attributes, ModAttrib::Replace, storage),
                                NULL,
                                NULL,
                                &msgid);
  if (errorNumber != LDAP_SUCCESS)
    return PFalse;

  P_timeval tval = timeout;
  LDAPMessage * result = NULL;
  ldap_result(ldapContext, msgid, LDAP_MSG_ALL, tval, &result);
  if (result)
    errorNumber = ldap_result2error(ldapContext, result, PTrue);

  return errorNumber == LDAP_SUCCESS;
}


PBoolean PLDAPSession::Modify(const PString & dn, const PStringToString & attributes)
{
  return Modify(dn, AttribsFromDict(attributes));
}


PBoolean PLDAPSession::Modify(const PString & dn, const PStringArray & attributes)
{
  return Modify(dn, AttribsFromArray(attributes));
}


PBoolean PLDAPSession::Modify(const PString & dn, const PLDAPStructBase & attributes)
{
  return Modify(dn, AttribsFromStruct(attributes));
}


PBoolean PLDAPSession::Delete(const PString & dn)
{
  if (!IsOpen())
    return PFalse;

  int msgid;
  errorNumber = ldap_delete_ext(ldapContext, dn, NULL, NULL, &msgid);
  if (errorNumber != LDAP_SUCCESS)
    return PFalse;

  P_timeval tval = timeout;
  LDAPMessage * result = NULL;
  ldap_result(ldapContext, msgid, LDAP_MSG_ALL, tval, &result);
  if (result)
    errorNumber = ldap_result2error(ldapContext, result, PTrue);

  return errorNumber == LDAP_SUCCESS;
}


PLDAPSession::SearchContext::SearchContext()
{
  result = NULL;
  message = NULL;
  found = PFalse;
  completed = PFalse;
}


PLDAPSession::SearchContext::~SearchContext()
{
  if (message != NULL)
    ldap_msgfree(message);

  if (result != NULL && result != message)
    ldap_msgfree(result);
}


PBoolean PLDAPSession::Search(SearchContext & context,
                          const PString & filter,
                          const PStringArray & attributes,
                          const PString & baseDN,
                          SearchScope scope)
{
  if (!IsOpen())
    return PFalse;

  PCharArray storage;
  char ** attribs = attributes.ToCharArray(&storage);

  PString base = baseDN;
  if (base.IsEmpty())
    base = defaultBaseDN;

  static const int ScopeCode[NumSearchScope] = {
    LDAP_SCOPE_BASE, LDAP_SCOPE_ONELEVEL, LDAP_SCOPE_SUBTREE
  };

  P_timeval tval = timeout;

  errorNumber = ldap_search_ext(ldapContext,
                                base,
                                ScopeCode[scope],
                                filter,
                                attribs,
                                PFalse,
                                NULL,
                                NULL,
                                tval,
                                searchLimit,
                                &context.msgid);

  if (errorNumber != LDAP_SUCCESS)
    return PFalse;

  if (ldap_result(ldapContext, context.msgid, LDAP_MSG_ONE, tval, &context.result) > 0)
    return GetNextSearchResult(context);

  if (context.result)
    errorNumber = ldap_result2error(ldapContext, context.result, PTrue);
  if (errorNumber == 0)
    errorNumber = LDAP_OTHER;
  return PFalse;
}


PBoolean PLDAPSession::GetSearchResult(SearchContext & context, PStringToString & data)
{
  data.RemoveAll();

  if (!IsOpen())
    return PFalse;

  if (context.result == NULL || context.message == NULL || context.completed)
    return PFalse;

  // Extract the resulting data

  data.SetAt("dn", GetSearchResultDN(context));

  BerElement * ber = NULL;
  char * attrib = ldap_first_attribute(ldapContext, context.message, &ber);
  while (attrib != NULL) {

    struct berval ** bvals = ldap_get_values_len(ldapContext, context.message, attrib);
    if (bvals != NULL) {
      PString value = data(attrib);

      for (PINDEX i = 0; bvals[i] != NULL; i++ ) {
        if (!value)
          value += multipleValueSeparator;
        value += PString(bvals[i]->bv_val, bvals[i]->bv_len);
      }
      ber_bvecfree(bvals);

      data.SetAt(attrib, value);
    }

    ldap_memfree(attrib);
    attrib = ldap_next_attribute(ldapContext, context.message, ber);
  }

  if (ber != NULL)
    ber_free (ber, 0);

  return PTrue;
}


PBoolean PLDAPSession::GetSearchResult(SearchContext & context,
                                   const PString & attribute,
                                   PString & data)
{
  data.MakeEmpty();

  if (!IsOpen())
    return PFalse;

  if (context.result == NULL || context.message == NULL || context.completed)
    return PFalse;

  if (attribute == "dn") {
    data = GetSearchResultDN(context);
    return PTrue;
  }

  char ** values = ldap_get_values(ldapContext, context.message, attribute);
  if (values == NULL)
    return PFalse;

  PINDEX count = ldap_count_values(values);
  for (PINDEX i = 0; i < count; i++) {
    if (!data)
      data += multipleValueSeparator;
    data += values[i];
  }

  ldap_value_free(values);
  return PTrue;
}


PBoolean PLDAPSession::GetSearchResult(SearchContext & context,
                                   const PString & attribute,
                                   PStringArray & data)
{
  data.RemoveAll();

  if (!IsOpen())
    return PFalse;

  if (context.result == NULL || context.message == NULL || context.completed)
    return PFalse;

  if (attribute == "dn") {
    data.SetSize(1);
    data[0] = GetSearchResultDN(context);
    return PTrue;
  }

  char ** values = ldap_get_values(ldapContext, context.message, attribute);
  if (values == NULL)
    return PFalse;

  PINDEX count = ldap_count_values(values);
  data.SetSize(count);
  for (PINDEX i = 0; i < count; i++)
    data[i] = values[i];

  ldap_value_free(values);
  return PTrue;
}


PBoolean PLDAPSession::GetSearchResult(SearchContext & context,
                                   const PString & attribute,
                                   PArray<PBYTEArray> & data)
{
  data.RemoveAll();

  if (!IsOpen())
    return PFalse;

  if (attribute == "dn") {
    char * dn = ldap_get_dn(ldapContext, context.message);
    data.Append(new PBYTEArray((const BYTE *)dn, ::strlen(dn)));
    ldap_memfree(dn);
    return PTrue;
  }

  struct berval ** values = ldap_get_values_len(ldapContext, context.message, attribute);
  if (values == NULL)
    return PFalse;

  PINDEX count = ldap_count_values_len(values);
  data.SetSize(count);
  for (PINDEX i = 0; i < count; i++) {
    PBYTEArray* dataPtr = new PBYTEArray(values[i]->bv_len);
    data.SetAt(i, dataPtr);
    
    memcpy(data[i].GetPointer(), (const BYTE *)values[i]->bv_val, values[i]->bv_len); 
  }

  ldap_value_free_len(values);
  return PTrue;
}


PBoolean PLDAPSession::GetSearchResult(SearchContext & context,
                                   PLDAPStructBase & data)
{
  if (!IsOpen())
    return PFalse;

  PBoolean atLeastOne = PFalse;

  for (PINDEX i = 0; i < data.GetNumAttributes(); i++) {
    PLDAPAttributeBase & attr = data.GetAttribute(i);
    if (attr.IsBinary()) {
      PArray<PBYTEArray> bin;
      if (GetSearchResult(context, attr.GetName(), bin)) {
        attr.FromBinary(bin);
        atLeastOne = PTrue;
      }
    }
    else {
      PString str;
      if (GetSearchResult(context, attr.GetName(), str)) {
        attr.FromString(str);
        atLeastOne = PTrue;
      }
    }
  }

  return atLeastOne;
}


PString PLDAPSession::GetSearchResultDN(SearchContext & context)
{
  PString str;

  if (context.message != NULL) {
    char * dn = ldap_get_dn(ldapContext, context.message);
    if (dn != NULL) {
      str = dn;
      ldap_memfree(dn);
    }
  }

  return str;
}


PBoolean PLDAPSession::GetNextSearchResult(SearchContext & context)
{
  if (!IsOpen())
    return PFalse;

  if (context.result == NULL || context.completed)
    return PFalse;

  P_timeval tval = timeout;
  do {
    if (context.message == NULL)
      context.message = ldap_first_message(ldapContext, context.result);
    else
      context.message = ldap_next_message(ldapContext, context.message);

    if (context.message != NULL) {
      switch (ldap_msgtype(context.message)) {
        case LDAP_RES_SEARCH_ENTRY :
          context.found = PTrue;
          errorNumber = LDAP_SUCCESS;
          return PTrue;

        case LDAP_RES_SEARCH_RESULT :
          errorNumber = ldap_result2error(ldapContext, context.message, PFalse);
          if (errorNumber == 0 && !context.found)
            errorNumber = LDAP_NO_RESULTS_RETURNED;
          context.completed = PTrue;
          return PFalse;
        case LDAP_RES_SEARCH_REFERENCE :
          errorNumber = LDAP_SUCCESS;
          return PTrue;
        // Ignore other result message types for now ...
        default:
          PTRACE(3, "Unhandled LDAP message type " << ldap_msgtype(context.message));
      }
    }

    ldap_msgfree(context.result);
  } while (ldap_result(ldapContext, context.msgid, LDAP_MSG_ONE, tval, &context.result) > 0);

  if (context.result)
    errorNumber = ldap_result2error(ldapContext, context.result, PFalse);
  if (errorNumber == 0)
    errorNumber = LDAP_OTHER;
  return PFalse;
}


PList<PStringToString> PLDAPSession::Search(const PString & filter,
                                            const PStringArray & attributes,
                                            const PString & base,
                                            SearchScope scope)
{
  PList<PStringToString> data;

  SearchContext context;
  if (!Search(context, filter, attributes, base, scope))
    return data;

  do {
    PStringToString * entry = new PStringToString;
    if (GetSearchResult(context, *entry))
      data.Append(entry);
    else {
      delete entry;
      break;
    }
  } while (GetNextSearchResult(context));

  return data;
}


PString PLDAPSession::GetErrorText() const
{
  return ldap_err2string(errorNumber);
}


///////////////////////////////////////////////////////////////////////////////

PLDAPAttributeBase::PLDAPAttributeBase(const char * n, void * ptr, PINDEX sz)
  : name(n),
    pointer(ptr),
    size(sz)
{
  PLDAPStructBase::GetInitialiser().AddAttribute(this);
}


PString PLDAPAttributeBase::ToString() const
{
  PStringStream stream;
  PrintOn(stream);
  return stream;
}


void PLDAPAttributeBase::FromString(const PString & str)
{
  PStringStream stream(str);
  ReadFrom(stream);
}


PBYTEArray PLDAPAttributeBase::ToBinary() const
{
  return PBYTEArray((const BYTE *)pointer, size, PFalse);
}


void PLDAPAttributeBase::FromBinary(const PArray<PBYTEArray> & data)
{
  if (data.GetSize() > 0 && data[0].GetSize() == size)
    memcpy(pointer, data[0], size);
}


///////////////////////////////////////////////////////////////////////////////

PMutex            PLDAPStructBase::initialiserMutex;
PLDAPStructBase * PLDAPStructBase::initialiserInstance;

PLDAPStructBase::PLDAPStructBase()
{
  attributes.DisallowDeleteObjects();

  initialiserMutex.Wait();
  initialiserStack = initialiserInstance;
  initialiserInstance = this;
}


PLDAPStructBase & PLDAPStructBase::operator=(const PLDAPStructBase & other)
{
  for (PINDEX i = 0; i < attributes.GetSize(); i++)
    attributes.GetDataAt(i).Copy(other.attributes.GetDataAt(i));

  return *this;
}


PLDAPStructBase & PLDAPStructBase::operator=(const PStringArray & array)
{
  for (PINDEX i = 0; i < array.GetSize(); i++) {
    PString str = array[i];
    PINDEX equal = str.Find('=');
    if (equal != P_MAX_INDEX) {
      PLDAPAttributeBase * attr = GetAttribute(str.Left(equal));
      if (attr != NULL)
        attr->FromString(str.Mid(equal+1));
    }
  }
  return *this;
}


PLDAPStructBase & PLDAPStructBase::operator=(const PStringToString & dict)
{
  for (PINDEX i = 0; i < dict.GetSize(); i++) {
    PLDAPAttributeBase * attr = GetAttribute(dict.GetKeyAt(i));
    if (attr != NULL)
      attr->FromString(dict.GetDataAt(i));
  }
  return *this;
}


void PLDAPStructBase::PrintOn(ostream & strm) const
{
  strm << attributes << '\n';
}


void PLDAPStructBase::AddAttribute(PLDAPAttributeBase * attr)
{
  attributes.SetAt(attr->GetName(), attr);
}


void PLDAPStructBase::EndConstructor()
{
  initialiserInstance = initialiserStack;
  initialiserMutex.Signal();
}

///////////////////////////////////////////////////////////////////

static const char PLDAPSchemaPluginBaseClass[] = "PLDAPSchema";

template <> PLDAPSchema * PDevicePluginFactory<PLDAPSchema>::Worker::Create(const PString & type) const
{
  return PLDAPSchema::CreateSchema(type);
}

PLDAPSchema::PLDAPSchema()
{
}

PLDAPSchema::Attribute::Attribute(const PString & name, AttributeType type)
: m_name(name),m_type(type)
{
}

void PLDAPSchema::LoadSchema()
{
	AttributeList(attributelist); 
}

PLDAPSchema * PLDAPSchema::CreateSchema(const PString & schemaname, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return (PLDAPSchema *)pluginMgr->CreatePluginsDeviceByName(schemaname, PLDAPSchemaPluginBaseClass);
}

PStringList PLDAPSchema::GetSchemaNames(PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsProviding(PLDAPSchemaPluginBaseClass);
}

PStringList PLDAPSchema::GetSchemaFriendlyNames(const PString & schema, PPluginManager * pluginMgr)
{
  if (pluginMgr == NULL)
    pluginMgr = &PPluginManager::GetPluginManager();

  return pluginMgr->GetPluginsDeviceNames(schema, PLDAPSchemaPluginBaseClass);
}

void PLDAPSchema::OnReceivedAttribute(const PString & attribute, const PString & value)
{
   SetAttribute(attribute,value);
}

PBoolean PLDAPSchema::SetAttribute(const PString & attribute, const PString & value)
{

	for (std::list<Attribute>::const_iterator r = attributelist.begin(); r != attributelist.end(); ++r) {
		if ((r->m_name == attribute) &&  (r->m_type != AttributeBinary)) {
	       attributes.insert(make_pair(attribute,value));
		   PTRACE(4, "schema\tMatch " << attribute);
		   return PTrue;
		}
	}

	return PFalse;
}

PBoolean PLDAPSchema::SetAttribute(const PString & attribute, const PBYTEArray & value)
{
	for (std::list<Attribute>::const_iterator r = attributelist.begin(); r != attributelist.end(); ++r) {
		if ((r->m_name == attribute) && (r->m_type == AttributeBinary)) {
	       binattributes.insert(make_pair(attribute,value));
		   PTRACE(4, "schema\tMatch Binary " << attribute);
		   return PTrue;
		}
	}

	return PFalse;
}

PBoolean PLDAPSchema::GetAttribute(const PString & attribute, PString & value)
{
	for (ldapAttributes::const_iterator r = attributes.begin(); r != attributes.end(); ++r) {
		if (r->first == attribute) {
			value = r->second;
			return PTrue;
		}
	}
	return PFalse; 
}

PBoolean PLDAPSchema::GetAttribute(const PString & attribute, PBYTEArray & value)
{
	for (ldapBinAttributes::const_iterator r = binattributes.begin(); r != binattributes.end(); ++r) {
		if (r->first == attribute) {
			value = r->second;
			return PTrue;
		}
	}
	return PFalse; 
}

PStringList PLDAPSchema::GetAttributeList()
{
	PStringList att;
  	for (std::list<Attribute>::iterator r = attributelist.begin(); r != attributelist.end(); ++r) {
        att.AppendString(r->m_name);
	}   
	return att;
}

PBoolean PLDAPSchema::Exists(const PString & attribute)
{
	for (std::list<Attribute>::const_iterator r = attributelist.begin(); r != attributelist.end(); ++r) {
	  if (r->m_name == attribute) {
		  if (r->m_type == AttributeString) {
	          for (ldapAttributes::const_iterator r = attributes.begin(); r != attributes.end(); ++r) {
                  if (r->first == attribute)
			         return PTrue;
		      }
		  } else if (r->m_type == AttributeBinary) {
	          for (ldapBinAttributes::const_iterator r = binattributes.begin(); r != binattributes.end(); ++r) {
                  if (r->first == attribute)
			         return PTrue;
			  }
		  }
	  }
	}
	return PFalse;
}

PLDAPSchema::AttributeType PLDAPSchema::GetAttributeType(const PString & attribute)
{
	for (std::list<Attribute>::const_iterator r = attributelist.begin(); r != attributelist.end(); ++r) {
		if (r->m_name == attribute) 
		   return (AttributeType)r->m_type;
	}
	return AttibuteUnknown;
}

void PLDAPSchema::OnSendSchema(PArray<PLDAPSession::ModAttrib> & attrib, PLDAPSession::ModAttrib::Operation op)
{
	for (ldapAttributes::const_iterator r = attributes.begin(); r != attributes.end(); ++r) 
        attrib.Append(new PLDAPSession::StringModAttrib(r->first,r->second,op));

	for (ldapBinAttributes::const_iterator s = binattributes.begin(); s != binattributes.end(); ++s) {
        attrib.Append(new PLDAPSession::BinaryModAttrib(s->first,s->second,op));
	}
}


#else

  #if defined(_MSC_VER)
    #pragma message("LDAP support DISABLED")
  #endif

#endif // P_LDAP


// End of file ////////////////////////////////////////////////////////////////

