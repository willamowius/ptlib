/*
 * pxmlrpc.cxx
 *
 * XML/RPC support
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

// This depends on the expat XML library by Jim Clark
// See http://www.jclark.com/xml/expat.html for more information

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pxmlrpc.h"
#endif

#if P_XMLRPC

#include <ptclib/pxmlrpc.h>

#include <ptclib/mime.h>
#include <ptclib/http.h>


#define new PNEW


static const char NoIndentElements[] = "methodName name string int boolean double dateTime.iso8601";


/////////////////////////////////////////////////////////////////

PXMLRPCBlock::PXMLRPCBlock()
  : PXML(PXMLParser::NoOptions, NoIndentElements)
{
  faultCode = P_MAX_INDEX;
  SetRootElement("methodResponse");
  params = NULL;
}

PXMLRPCBlock::PXMLRPCBlock(const PString & method)
  : PXML(PXMLParser::NoOptions, NoIndentElements)
{
  faultCode = P_MAX_INDEX;
  SetRootElement("methodCall");
  rootElement->AddChild(new PXMLElement(rootElement, "methodName", method));
  params = NULL;
}

PXMLRPCBlock::PXMLRPCBlock(const PString & method, const PXMLRPCStructBase & data)
  : PXML(PXMLParser::NoOptions, NoIndentElements)
{
  faultCode = P_MAX_INDEX;
  SetRootElement("methodCall");
  rootElement->AddChild(new PXMLElement(rootElement, "methodName", method));
  params = NULL;

  for (PINDEX i = 0; i < data.GetNumVariables(); i++) {
    PXMLRPCVariableBase & variable = data.GetVariable(i);
    if (variable.IsArray())
      AddParam(CreateArray(variable));
    else {
      PXMLRPCStructBase * structVar = variable.GetStruct(0);
      if (structVar != NULL)
        AddParam(*structVar);
      else
        AddParam(CreateValueElement(new PXMLElement(NULL, variable.GetType(), variable.ToString(0))));
    }
  }
}


PBoolean PXMLRPCBlock::Load(const PString & str)
{
  if (!PXML::Load(str))
    return PFalse;

  if (rootElement != NULL)
    params = rootElement->GetElement("params");

  return PTrue;
}


PXMLElement * PXMLRPCBlock::GetParams()
{
  if (params == NULL)
    params = rootElement->AddChild(new PXMLElement(rootElement, "params"));

  return params;
}

PXMLElement * PXMLRPCBlock::CreateValueElement(PXMLElement * element) 
{ 
  PXMLElement * value = new PXMLElement(NULL, "value");
  value->AddChild(element);
  element->SetParent(value);

  return value;
}

PXMLElement * PXMLRPCBlock::CreateScalar(const PString & type, 
                                         const PString & scalar)
{ 
  return CreateValueElement(new PXMLElement(NULL, type, scalar));
}

PXMLElement * PXMLRPCBlock::CreateScalar(const PString & str) 
{ 
  return CreateScalar("string", str);
}

PXMLElement * PXMLRPCBlock::CreateScalar(int value) 
{
  return CreateScalar("int", PString(PString::Unsigned, value)); 
}

PXMLElement * PXMLRPCBlock::CreateScalar(double value)
{ 
  return CreateScalar("double", psprintf("%lf", value)); 
}

PXMLElement * PXMLRPCBlock::CreateDateAndTime(const PTime & time)
{
  return CreateScalar("dateTime.iso8601", PXMLRPC::PTimeToISO8601(time)); 
}

PXMLElement * PXMLRPCBlock::CreateBinary(const PBYTEArray & data)
{
  return CreateScalar("base64", PBase64::Encode(data)); 
}

PXMLElement * PXMLRPCBlock::CreateStruct()
{
  PAssertAlways("Not used");
  return NULL;
}


PXMLElement * PXMLRPCBlock::CreateStruct(const PStringToString & dict)
{
  return CreateStruct(dict, "string");
}

PXMLElement * PXMLRPCBlock::CreateStruct(const PStringToString & dict, const PString & typeStr)
{
  PXMLElement * structElement = new PXMLElement(NULL, "struct");
  PXMLElement * valueElement  = CreateValueElement(structElement);

  PINDEX i;
  for (i = 0; i < dict.GetSize(); i++) {
    PString key = dict.GetKeyAt(i);
    structElement->AddChild(CreateMember(key, CreateScalar(typeStr, dict[key])));
  }

  return valueElement;
}


PXMLElement * PXMLRPCBlock::CreateStruct(const PXMLRPCStructBase & data)
{
  PXMLElement * structElement = new PXMLElement(NULL, "struct");
  PXMLElement * valueElement  = PXMLRPCBlock::CreateValueElement(structElement);

  PINDEX i;
  for (i = 0; i < data.GetNumVariables(); i++) {
    PXMLElement * element;
    PXMLRPCVariableBase & variable = data.GetVariable(i);

    if (variable.IsArray())
      element = CreateArray(variable);
    else {
      PXMLRPCStructBase * nested = variable.GetStruct(0);
      if (nested != NULL)
        element = CreateStruct(*nested);
      else
        element = CreateScalar(variable.GetType(), variable.ToString(0));
    }

    structElement->AddChild(CreateMember(variable.GetName(), element));
  }

  return valueElement;
}


PXMLElement * PXMLRPCBlock::CreateMember(const PString & name, PXMLElement * value)
{
  PXMLElement * member = new PXMLElement(NULL, "member");
  member->AddChild(new PXMLElement(member, "name", name));
  member->AddChild(value);

  return member;
}


PXMLElement * PXMLRPCBlock::CreateArray(const PStringArray & array)
{
  return CreateArray(array, "string");
}


PXMLElement * PXMLRPCBlock::CreateArray(const PStringArray & array, const PString & typeStr)
{
  PXMLElement * arrayElement = new PXMLElement(NULL, "array");

  PXMLElement * dataElement = new PXMLElement(arrayElement, "data");
  arrayElement->AddChild(dataElement);

  PINDEX i;
  for (i = 0; i < array.GetSize(); i++)
    dataElement->AddChild(CreateScalar(typeStr, array[i]));

  return CreateValueElement(arrayElement);
}


PXMLElement * PXMLRPCBlock::CreateArray(const PStringArray & array, const PStringArray & types)
{
  PXMLElement * arrayElement = new PXMLElement(NULL, "array");

  PXMLElement * dataElement = new PXMLElement(arrayElement, "data");
  arrayElement->AddChild(dataElement);

  PINDEX i;
  for (i = 0; i < array.GetSize(); i++)
    dataElement->AddChild(CreateScalar(types[i], array[i]));

  return CreateValueElement(arrayElement);
}


PXMLElement * PXMLRPCBlock::CreateArray(const PArray<PStringToString> & array)
{
  PXMLElement * arrayElement = new PXMLElement(NULL, "array");

  PXMLElement * dataElement = new PXMLElement(arrayElement, "data");
  arrayElement->AddChild(dataElement);

  PINDEX i;
  for (i = 0; i < array.GetSize(); i++)
    dataElement->AddChild(CreateStruct(array[i]));

  return CreateValueElement(arrayElement);
}


PXMLElement * PXMLRPCBlock::CreateArray(const PXMLRPCVariableBase & array)
{
  PXMLElement * arrayElement = new PXMLElement(NULL, "array");

  PXMLElement * dataElement = new PXMLElement(arrayElement, "data");
  arrayElement->AddChild(dataElement);

  PINDEX i;
  for (i = 0; i < array.GetSize(); i++) {
    PXMLElement * element;
    PXMLRPCStructBase * structure = array.GetStruct(i);
    if (structure != NULL)
      element = CreateStruct(*structure);
    else
      element = CreateScalar(array.GetType(), array.ToString(i));
    dataElement->AddChild(element);
  }

  return CreateValueElement(arrayElement);
}


/////////////////////////////////////////////

void PXMLRPCBlock::AddParam(PXMLElement * parm)
{
  GetParams();
  PXMLElement * child = params->AddChild(new PXMLElement(params, "param"));
  child->AddChild(parm);
  parm->SetParent(child);
}

void PXMLRPCBlock::AddParam(const PString & str) 
{ 
  AddParam(CreateScalar(str));
}

void PXMLRPCBlock::AddParam(int value) 
{
  AddParam(CreateScalar(value)); 
}

void PXMLRPCBlock::AddParam(double value)
{ 
  AddParam(CreateScalar(value)); 
}

void PXMLRPCBlock::AddParam(const PTime & time)
{
  AddParam(CreateDateAndTime(time)); 
}

void PXMLRPCBlock::AddBinary(const PBYTEArray & data)
{
  AddParam(CreateBinary(data)); 
}

void PXMLRPCBlock::AddParam(const PXMLRPCStructBase & data)
{
  AddParam(CreateStruct(data));
}

void PXMLRPCBlock::AddStruct(const PStringToString & dict)
{
  AddParam(CreateStruct(dict, "string"));
}

void PXMLRPCBlock::AddStruct(const PStringToString & dict, const PString & typeStr)
{
  AddParam(CreateStruct(dict, typeStr));
}

void PXMLRPCBlock::AddArray(const PStringArray & array)
{
  AddParam(CreateArray(array, "string"));
}

void PXMLRPCBlock::AddArray(const PStringArray & array, const PString & typeStr)
{
  AddParam(CreateArray(array, typeStr));
}

void PXMLRPCBlock::AddArray(const PStringArray & array, const PStringArray & types)
{
  AddParam(CreateArray(array, types));
}

void PXMLRPCBlock::AddArray(const PArray<PStringToString> & array)
{
  AddParam(CreateArray(array));
}


/////////////////////////////////////////////

PBoolean PXMLRPCBlock::ValidateResponse()
{
  // ensure root element exists and has correct name
  if ((rootElement == NULL) || 
      (rootElement->GetName() != "methodResponse")) {
    SetFault(PXMLRPC::ResponseRootNotMethodResponse, "Response root not methodResponse");
    PTRACE(2, "XMLRPC\t" << GetFaultText());
    return PFalse;
  }

  // determine if response returned
  if (params == NULL)
    params = rootElement->GetElement("params");
  if (params == NULL)
    params = rootElement->GetElement("fault");
  if (params == NULL)
    return PTrue;

  // determine if fault
  if (params->GetName() == "fault") {

    // assume fault is a simple struct
    PStringToString faultInfo;
    PXMLElement * value = params->GetElement("value");
    if (value == NULL) {
      PStringStream txt;
      txt << "Fault does not contain value\n" << *this;
      SetFault(PXMLRPC::FaultyFault, txt);
    } else if (!ParseStruct(value->GetElement("struct"), faultInfo) ||
         (faultInfo.GetSize() != 2) ||
         (!faultInfo.Contains("faultCode")) ||
         (!faultInfo.Contains("faultString"))
         ) {
      PStringStream txt;
      txt << "Fault return is faulty:\n" << *this;
      SetFault(PXMLRPC::FaultyFault, txt);
      PTRACE(2, "XMLRPC\t" << GetFaultText());
      return PFalse;
    }

    // get fault code and string
    SetFault(faultInfo["faultCode"].AsInteger(), faultInfo["faultString"]);

    return PFalse;
  }

  // must be params
  else if (params->GetName() != "params") {
    SetFault(PXMLRPC::ResponseUnknownFormat, PString("Response contains unknown element") & params->GetName());
    PTRACE(2, "XMLRPC\t" << GetFaultText());
    return PFalse;
  }

  return PTrue;
}

PBoolean PXMLRPCBlock::ParseScalar(PXMLElement * valueElement, 
                                   PString & type, 
                                   PString & value)
{
  if (valueElement == NULL)
    return PFalse;

  if (!valueElement->IsElement())
    return PFalse;

  if (valueElement->GetName() != "value") {
    SetFault(PXMLRPC::ParamNotValue, "Scalar value does not contain value element");
    PTRACE(2, "RPCXML\t" << GetFaultText());
    return PFalse;
  }

  for (PINDEX i = 0; i < valueElement->GetSize(); i++) {
    PXMLElement * element = (PXMLElement *)valueElement->GetElement(i);
    if (element != NULL && element->IsElement()) {
      type = element->GetName();
      value = element->GetData();
      return PTrue;
    }
  }

  SetFault(PXMLRPC::ScalarWithoutElement, "Scalar without sub-element");
  PTRACE(2, "XMLRPC\t" << GetFaultText());
  return PFalse;
}


static PBoolean ParseStructBase(PXMLRPCBlock & block, PXMLElement * & element)
{
  if (element == NULL)
    return PFalse;

  if (!element->IsElement())
    return PFalse;

  if (element->GetName() == "struct")
    return PTrue;

  if (element->GetName() != "value")
    block.SetFault(PXMLRPC::ParamNotStruct, "Param is not struct");
  else {
    element = element->GetElement("struct");
    if (element != NULL)
      return PTrue;

    block.SetFault(PXMLRPC::ParamNotStruct, "nested structure not present");
  }

  PTRACE(2, "XMLRPC\t" << block.GetFaultText());
  return PFalse;
}


static PXMLElement * ParseStructElement(PXMLRPCBlock & block,
                                        PXMLElement * structElement,
                                        PINDEX idx,
                                        PString & name)
{
  if (structElement == NULL)
    return NULL;

  PXMLElement * member = (PXMLElement *)structElement->GetElement(idx);
  if (member == NULL)
    return NULL;

  if (!member->IsElement())
    return NULL;

  if (member->GetName() != "member") {
    PStringStream txt;
    txt << "Member " << idx << " missing";
    block.SetFault(PXMLRPC::MemberIncomplete, txt);
    PTRACE(2, "XMLRPC\t" << block.GetFaultText());
    return NULL;
  }

  PXMLElement * nameElement  = member->GetElement("name");
  PXMLElement * valueElement = member->GetElement("value");
  if ((nameElement == NULL) || (valueElement == NULL)) {
    PStringStream txt;
    txt << "Member " << idx << " incomplete";
    block.SetFault(PXMLRPC::MemberIncomplete, txt);
    PTRACE(2, "XMLRPC\t" << block.GetFaultText());
    return NULL;
  }

  if (nameElement->GetName() != "name") {
    PStringStream txt;
    txt << "Member " << idx << " unnamed";
    block.SetFault(PXMLRPC::MemberUnnamed, txt);
    PTRACE(2, "XMLRPC\t" << block.GetFaultText());
    return NULL;
  }

  name = nameElement->GetData();
  return valueElement;
}


PBoolean PXMLRPCBlock::ParseStruct(PXMLElement * structElement, 
                               PStringToString & structDict)
{
  if (!ParseStructBase(*this, structElement))
    return PFalse;

  for (PINDEX i = 0; i < structElement->GetSize(); i++) {
    PString name;
    PXMLElement * element = ParseStructElement(*this, structElement, i, name);
    if (element != NULL) {
      PString value;
      PString type;
      if (ParseScalar(element, type, value))
        structDict.SetAt(name, value);
    }
  }

  return PTrue;
}


PBoolean PXMLRPCBlock::ParseStruct(PXMLElement * structElement, PXMLRPCStructBase & data)
{
  if (!ParseStructBase(*this, structElement))
    return PFalse;

  for (PINDEX i = 0; i < structElement->GetSize(); i++) {
    PString name;
    PXMLElement * element = ParseStructElement(*this, structElement, i, name);
    if (element != NULL) {
      PXMLRPCVariableBase * variable = data.GetVariable(name);
      if (variable != NULL) {
        if (variable->IsArray()) {
          if (!ParseArray(element, *variable))
            return PFalse;
        }
        else {
          PXMLRPCStructBase * nested = variable->GetStruct(0);
          if (nested != NULL) {
            if (!ParseStruct(element, *nested))
              return PFalse;
          }
          else {
            PString value;
            PCaselessString type;
            if (!ParseScalar(element, type, value))
              return PFalse;

            if (type != "string" && type != variable->GetType()) {
              PTRACE(2, "RPCXML\tMember " << i << " is not of expected type: " << variable->GetType());
              return PFalse;
            }

            variable->FromString(0, value);
          }
        }
      }
    }
  }

  return PTrue;
}


static PXMLElement * ParseArrayBase(PXMLRPCBlock & block, PXMLElement * element)
{
  if (element == NULL)
    return NULL;

  if (!element->IsElement())
    return NULL;

  if (element->GetName() == "value")
    element = element->GetElement("array");

  if (element == NULL)
    block.SetFault(PXMLRPC::ParamNotArray, "array not present");
  else {
    if (element->GetName() != "array")
      block.SetFault(PXMLRPC::ParamNotArray, "Param is not array");
    else {
      element = element->GetElement("data");
      if (element != NULL)
        return element;
      block.SetFault(PXMLRPC::ParamNotArray, "Array param has no data");
    }
  }

  PTRACE(2, "XMLRPC\t" << block.GetFaultText());
  return NULL;
}


PBoolean PXMLRPCBlock::ParseArray(PXMLElement * arrayElement, PStringArray & array)
{
  PXMLElement * dataElement = ParseArrayBase(*this, arrayElement);
  if (dataElement == NULL)
    return PFalse;

  array.SetSize(dataElement->GetSize());

  PINDEX count = 0;
  for (PINDEX i = 0; i < dataElement->GetSize(); i++) {
    PString value;
    PString type;
    if (ParseScalar((PXMLElement *)dataElement->GetElement(i), type, value))
      array[count++] = value;
  }

  array.SetSize(count);
  return PTrue;
}


PBoolean PXMLRPCBlock::ParseArray(PXMLElement * arrayElement, PArray<PStringToString> & array)
{
  PXMLElement * dataElement = ParseArrayBase(*this, arrayElement);
  if (dataElement == NULL)
    return PFalse;

  array.SetSize(dataElement->GetSize());

  PINDEX count = 0;
  for (PINDEX i = 0; i < dataElement->GetSize(); i++) {
    PStringToString values;
    if (!ParseStruct((PXMLElement *)dataElement->GetElement(i), values))
      return PFalse;

    array[count++] = values;
  }

  array.SetSize(count);
  return PTrue;
}


PBoolean PXMLRPCBlock::ParseArray(PXMLElement * arrayElement, PXMLRPCVariableBase & array)
{
  PXMLElement * dataElement = ParseArrayBase(*this, arrayElement);
  if (dataElement == NULL)
    return PFalse;

  array.SetSize(dataElement->GetSize());

  PINDEX count = 0;
  for (PINDEX i = 0; i < dataElement->GetSize(); i++) {
    PXMLElement * element = (PXMLElement *)dataElement->GetElement(i);

    PXMLRPCStructBase * structure = array.GetStruct(i);
    if (structure != NULL) {
      if (ParseStruct(element, *structure))
        count++;
    }
    else {
      PString value;
      PCaselessString type;
      if (ParseScalar(element, type, value)) {
        if (type != "string" && type != array.GetType())
          PTRACE(2, "RPCXML\tArray entry " << i << " is not of expected type: " << array.GetType());
        else
          array.FromString(count++, value);
      }
    }
  }

  array.SetSize(count);
  return PTrue;
}


PINDEX PXMLRPCBlock::GetParamCount() const
{
  if (params == NULL) 
    return 0;

  PINDEX count = 0;
  for (PINDEX i = 0; i < params->GetSize(); i++) {
    PXMLElement * element = (PXMLElement *)params->GetElement(i);
    if (element != NULL && element->IsElement() && element->GetName() == "param")
      count++;
  }
  return count;
}


PXMLElement * PXMLRPCBlock::GetParam(PINDEX idx) const 
{ 
  if (params == NULL) 
    return NULL;

  PXMLElement * param = NULL;
  PINDEX i;
  PINDEX s = params->GetSize();
  for (i = 0; i < s; i++) {
    PXMLElement * element = (PXMLElement *)params->GetElement(i);
    if (element != NULL && element->IsElement() && element->GetName() == "param") {
      if (idx <= 0) {
        param = element;
        break;
      }
      idx--;
    }
  }

  if (param == NULL)
    return PFalse;

  for (i = 0; i < param->GetSize(); i++) {
    PXMLObject * parm = param->GetElement(i);
    if (parm != NULL && parm->IsElement())
      return (PXMLElement *)parm;
  }

  return NULL;
}


PBoolean PXMLRPCBlock::GetParams(PXMLRPCStructBase & data)
{
  if (params == NULL) 
    return PFalse;

  // Special case to allow for server implementations that always return
  // values as a struct rather than multiple parameters.
  if (GetParamCount() == 1 &&
          (data.GetNumVariables() > 1 || data.GetVariable(0).GetStruct(0) == NULL)) {
    PString type, value;
    if (ParseScalar(GetParam(0), type, value) && type == "struct")
      return GetParam(0, data);
  }

  for (PINDEX i = 0; i < data.GetNumVariables(); i++) {
    PXMLRPCVariableBase & variable = data.GetVariable(i);
    if (variable.IsArray()) {
      if (!ParseArray(GetParam(i), variable))
        return PFalse;
    }
    else {
      PXMLRPCStructBase * structure = variable.GetStruct(0);
      if (structure != NULL) {
        if (!GetParam(i, *structure))
          return PFalse;
      }
      else {
        PString value;
        if (!GetExpectedParam(i, variable.GetType(), value))
          return PFalse;

        variable.FromString(0, value);
      }
    }
  }

  return PTrue;
}


PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PString & type, PString & value)
{
  // get the parameter
  if (!ParseScalar(GetParam(idx), type, value)) {
    PTRACE(2, "XMLRPC\tCannot get scalar parm " << idx);
    return PFalse;
  }

  return PTrue;
}


PBoolean PXMLRPCBlock::GetExpectedParam(PINDEX idx, const PString & expectedType, PString & value)
{
  PString type;

  // get parameter
  if (!GetParam(idx, type, value))
    return PFalse;

  // see if correct type
  if (!expectedType.IsEmpty() && (type != expectedType)) {
    PTRACE(2, "XMLRPC\tExpected parm " << idx << " to be " << expectedType << ", was " << type);
    return PFalse;
  }

  return PTrue;
}


PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PString & result)
{
  return GetExpectedParam(idx, "string", result); 
}

PBoolean PXMLRPCBlock::GetParam(PINDEX idx, int & val)
{
  PString type, result; 
  if (!GetParam(idx, type, result))
    return PFalse;

  if ((type != "i4") && 
      (type != "int") &&
      (type != "boolean")) {
    PTRACE(2, "XMLRPC\tExpected parm " << idx << " to be intger compatible, was " << type);
    return PFalse;
  }

  val = result.AsInteger();
  return PTrue;
}

PBoolean PXMLRPCBlock::GetParam(PINDEX idx, double & val)
{
  PString result; 
  if (!GetExpectedParam(idx, "double", result))
    return PFalse;

  val = result.AsReal();
  return PTrue;
}

// 01234567890123456
// yyyyMMddThh:mm:ss

PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PTime & val, int tz)
{
  PString result; 
  if (!GetExpectedParam(idx, "dateTime.iso8601", result))
    return PFalse;

  return PXMLRPC::ISO8601ToPTime(result, val, tz);
}

PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PStringArray & result)
{
  return ParseArray(GetParam(idx), result);
}

PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PArray<PStringToString> & result)
{
  return ParseArray(GetParam(idx), result);
}


PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PStringToString & result)
{
  return ParseStruct(GetParam(idx), result);
}


PBoolean PXMLRPCBlock::GetParam(PINDEX idx, PXMLRPCStructBase & data)
{
  return ParseStruct(GetParam(idx), data);
}


////////////////////////////////////////////////////////

PXMLRPC::PXMLRPC(const PURL & _url, PXMLParser::Options opts)
  : url(_url)
  , timeout(0, 10) // Seconds
  , m_options(opts)
{
}

PBoolean PXMLRPC::MakeRequest(const PString & method)
{
  PXMLRPCBlock request(method);
  PXMLRPCBlock response;

  return MakeRequest(request, response);
}

PBoolean PXMLRPC::MakeRequest(const PString & method, PXMLRPCBlock & response)
{
  PXMLRPCBlock request(method);

  return MakeRequest(request, response);
}

PBoolean PXMLRPC::MakeRequest(PXMLRPCBlock & request, PXMLRPCBlock & response)
{
  if (PerformRequest(request, response))
    return PTrue;

  faultCode = response.GetFaultCode();
  faultText = response.GetFaultText();

  return PFalse;
}

PBoolean PXMLRPC::MakeRequest(const PString & method, const PXMLRPCStructBase & args, PXMLRPCStructBase & reply)
{
  PXMLRPCBlock request(method, args);
  PXMLRPCBlock response;

  if (!MakeRequest(request, response))
    return PFalse;

  if (response.GetParams(reply))
    return PTrue;

  PTRACE(1, "XMLRPC\tParsing response failed: " << response.GetFaultText());
  return PFalse;
}


PBoolean PXMLRPC::PerformRequest(PXMLRPCBlock & request, PXMLRPCBlock & response)
{
  // create XML version of request
  PString requestXML;
  if (!request.Save(requestXML, m_options)) {
    PStringStream txt;
    txt << "Error creating request XML ("
        << request.GetErrorLine() 
        << ") :" 
        << request.GetErrorString();
    response.SetFault(PXMLRPC::CannotCreateRequestXML, txt);
    PTRACE(2, "XMLRPC\t" << response.GetFaultText());
    return PFalse;
  }

  // make sure the request ends with a newline
  requestXML += "\n";

  // do the request
  PHTTPClient client;
  PMIMEInfo sendMIME, replyMIME;
  sendMIME.SetAt("Server", url.GetHostName());
  sendMIME.SetAt(PHTTP::ContentTypeTag(), "text/xml");

  PTRACE(5, "XMLRPC\tOutgoing XML/RPC:\n" << url << '\n' << sendMIME << requestXML);

  // apply the timeout
  client.SetReadTimeout(timeout);

  PString replyXML;

  // do the request
  PBoolean ok = client.PostData(url, sendMIME, requestXML, replyMIME, replyXML);

  PTRACE(5, "XMLRPC\tIncoming XML/RPC:\n" << replyMIME << replyXML);

  // make sure the request worked
  if (!ok) {
    PStringStream txt;
    txt << "HTTP POST failed: "
        << client.GetLastResponseCode() << ' '
        << client.GetLastResponseInfo() << '\n'
        << replyMIME << '\n'
        << replyXML;
    response.SetFault(PXMLRPC::HTTPPostFailed, txt);
    PTRACE(2, "XMLRPC\t" << response.GetFaultText());
    return PFalse;
  }

  // parse the response
  if (!response.Load(replyXML)) {
    PStringStream txt;
    txt << "Error parsing response XML ("
        << response.GetErrorLine() 
        << ") :" 
        << response.GetErrorString() << '\n';

    PStringArray lines = replyXML.Lines();
    for (int offset = -2; offset <= 2; offset++) {
      int line = response.GetErrorLine() + offset;
      if (line >= 0 && line < lines.GetSize())
        txt << lines[(PINDEX)line] << '\n';
    }

    response.SetFault(PXMLRPC::CannotParseResponseXML, txt);
    PTRACE(2, "XMLRPC\t" << response.GetFaultText());
    return PFalse;
  }

  // validate the response
  if (!response.ValidateResponse()) {
    PTRACE(2, "XMLRPC\tValidation of response failed: " << response.GetFaultText());
    return PFalse;
  }

  return PTrue;
}

PBoolean PXMLRPC::ISO8601ToPTime(const PString & iso8601, PTime & val, int tz)
{
  if ((iso8601.GetLength() != 17) ||
      (iso8601[8]  != 'T') ||
      (iso8601[11] != ':') ||
      (iso8601[14] != ':'))
    return PFalse;

  val = PTime(iso8601.Mid(15,2).AsInteger(),  // seconds
              iso8601.Mid(12,2).AsInteger(),  // minutes
              iso8601.Mid( 9,2).AsInteger(),  // hours
              iso8601.Mid( 6,2).AsInteger(),  // day
              iso8601.Mid( 4,2).AsInteger(),  // month
              iso8601.Mid( 0,4).AsInteger(),  // year
              tz
              );

  return PTrue;
}

PString PXMLRPC::PTimeToISO8601(const PTime & time)
{
  return time.AsString("yyyyMMddThh:mm:ss");

}


/////////////////////////////////////////////////////////////////

PXMLRPCVariableBase::PXMLRPCVariableBase(const char * n, const char * t)
  : name(n),
    type(t != NULL ? t : "string")
{
  PXMLRPCStructBase::GetInitialiser().AddVariable(this);
}


PXMLRPCStructBase * PXMLRPCVariableBase::GetStruct(PINDEX) const
{
  return NULL;
}


PBoolean PXMLRPCVariableBase::IsArray() const
{
  return PFalse;
}


PINDEX PXMLRPCVariableBase::GetSize() const
{
  return 1;
}


PBoolean PXMLRPCVariableBase::SetSize(PINDEX)
{
  return PTrue;
}


PString PXMLRPCVariableBase::ToString(PINDEX) const
{
  PStringStream stream;
  PrintOn(stream);
  return stream;
}


void PXMLRPCVariableBase::FromString(PINDEX, const PString & str)
{
  PStringStream stream(str);
  ReadFrom(stream);
}


PString PXMLRPCVariableBase::ToBase64(PAbstractArray & data) const
{
  return PBase64::Encode(data.GetPointer(), data.GetSize());
}


void PXMLRPCVariableBase::FromBase64(const PString & str, PAbstractArray & data)
{
  PBase64 decoder;
  decoder.StartDecoding();
  decoder.ProcessDecoding(str);
  data = decoder.GetDecodedData();
}


/////////////////////////////////////////////////////////////////

PXMLRPCArrayBase::PXMLRPCArrayBase(PContainer & a, const char * n, const char * t)
  : PXMLRPCVariableBase(n, t),
    array(a)
{
}


void PXMLRPCArrayBase::PrintOn(ostream & strm) const
{
  strm << setfill('\n') << array << setfill(' ');
}


void PXMLRPCArrayBase::Copy(const PXMLRPCVariableBase & other)
{
  array = ((PXMLRPCArrayBase &)other).array;
}


PBoolean PXMLRPCArrayBase::IsArray() const
{
  return PTrue;
}


PINDEX PXMLRPCArrayBase::GetSize() const
{
  return array.GetSize();
}


PBoolean PXMLRPCArrayBase::SetSize(PINDEX sz)
{
  return array.SetSize(sz);
}


/////////////////////////////////////////////////////////////////

PXMLRPCArrayObjectsBase::PXMLRPCArrayObjectsBase(PArrayObjects & a, const char * n, const char * t)
  : PXMLRPCArrayBase(a, n, t),
    array(a)
{
}


PBoolean PXMLRPCArrayObjectsBase::SetSize(PINDEX sz)
{
  if (!array.SetSize(sz))
    return PFalse;

  for (PINDEX i = 0; i < sz; i++) {
    if (array.GetAt(i) == NULL) {
      PObject * object = CreateObject();
      if (object == NULL)
        return PFalse;
      array.SetAt(i, object);
    }
  }

  return PTrue;
}


PString PXMLRPCArrayObjectsBase::ToString(PINDEX i) const
{
  PStringStream stream;
  stream << *array.GetAt(i);
  return stream;
}


void PXMLRPCArrayObjectsBase::FromString(PINDEX i, const PString & str)
{
  PObject * object = array.GetAt(i);
  if (object == NULL) {
    object = CreateObject();
    array.SetAt(i, object);
  }

  PStringStream stream(str);
  stream >> *object;
}


/////////////////////////////////////////////////////////////////

PMutex              PXMLRPCStructBase::initialiserMutex;
PXMLRPCStructBase * PXMLRPCStructBase::initialiserInstance = NULL;


PXMLRPCStructBase::PXMLRPCStructBase()
{
  variablesByOrder.DisallowDeleteObjects();
  variablesByName.DisallowDeleteObjects();

  initialiserMutex.Wait();
  initialiserStack = initialiserInstance;
  initialiserInstance = this;
}


void PXMLRPCStructBase::EndConstructor()
{
  initialiserInstance = initialiserStack;
  initialiserMutex.Signal();
}


PXMLRPCStructBase & PXMLRPCStructBase::operator=(const PXMLRPCStructBase & other)
{
  for (PINDEX i = 0; i < variablesByOrder.GetSize(); i++)
    variablesByOrder[i].Copy(other.variablesByOrder[i]);

  return *this;
}


void PXMLRPCStructBase::PrintOn(ostream & strm) const
{
  for (PINDEX i = 0; i < variablesByOrder.GetSize(); i++) {
    PXMLRPCVariableBase & var = variablesByOrder[i];
    strm << var.GetName() << '=' << var << '\n';
  }
}


void PXMLRPCStructBase::AddVariable(PXMLRPCVariableBase * var)
{
  variablesByOrder.Append(var);
  variablesByName.SetAt(var->GetName(), var);
}


#endif // P_XMLRPC


// End of file ///////////////////////////////////////////////////////////////

