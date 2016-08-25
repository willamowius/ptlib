/*
 * pxmlrpc.h
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

#ifndef PTLIB_PXMLRPC_H
#define PTLIB_PXMLRPC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/pxml.h>

#if P_XMLRPC

#include <ptclib/url.h>


class PXMLRPCBlock;
class PXMLRPCVariableBase;
class PXMLRPCStructBase;


/////////////////////////////////////////////////////////////////

class PXMLRPC : public PObject
{
  PCLASSINFO(PXMLRPC, PObject);
  public:
    enum {
      CannotCreateRequestXML          = 100,
      CannotParseResponseXML,
      CannotParseRequestXML,
      HTTPPostFailed,
      CannotReadResponseContentBody,
      ResponseRootNotMethodResponse,
      ResponseEmpty,
      ResponseUnknownFormat,
      ParamNotValue,
      ScalarWithoutElement,
      ParamNotStruct,
      MemberIncomplete,
      MemberUnnamed,
      FaultyFault,
      RequestHasWrongDocumentType,
      RequestHasNoMethodName,
      RequestHasNoParms,
      MethodNameIsEmpty,
      UnknownMethod,
      ParamNotArray,

      UserFault                       = 1000,
    };

    PXMLRPC(
      const PURL & url,
      PXML::Options options = PXML::NoOptions
    );

    void SetTimeout(const PTimeInterval & t) { timeout = t; }

    PBoolean MakeRequest(const PString & method);
    PBoolean MakeRequest(const PString & method,  PXMLRPCBlock & response);
    PBoolean MakeRequest(PXMLRPCBlock & request, PXMLRPCBlock & response);
    PBoolean MakeRequest(const PString & method, const PXMLRPCStructBase & args, PXMLRPCStructBase & reply);

    PString GetFaultText() const { return faultText; }
    PINDEX  GetFaultCode() const { return faultCode; }

    static PBoolean    ISO8601ToPTime(const PString & iso8601, PTime & val, int tz = PTime::GMT);
    static PString PTimeToISO8601(const PTime & val);

  protected:
    PBoolean PerformRequest(PXMLRPCBlock & request, PXMLRPCBlock & response);

    PURL          url;
    PINDEX        faultCode;
    PString       faultText;
    PTimeInterval timeout;
    PXML::Options m_options;
};

/////////////////////////////////////////////////////////////////

class PXMLRPCBlock : public PXML
{
  PCLASSINFO(PXMLRPCBlock, PXML);
  public:
    PXMLRPCBlock();
    PXMLRPCBlock(const PString & method);
    PXMLRPCBlock(const PString & method, const PXMLRPCStructBase & structData);

    PBoolean Load(const PString & str);

    PXMLElement * GetParams();
    PXMLElement * GetParam(PINDEX idx) const;
    PINDEX GetParamCount() const;

    // used when used as a response
    PINDEX  GetFaultCode() const                     { return faultCode; }
    PString GetFaultText() const                     { return faultText; }
    void SetFault(PINDEX code, const PString & text) { faultCode = code; faultText = text; }
    PBoolean ValidateResponse();

    // helper functions for getting parameters
    PBoolean GetParams(PXMLRPCStructBase & data);
    PBoolean GetParam(PINDEX idx, PString & type, PString & result);
    PBoolean GetExpectedParam(PINDEX idx, const PString & expectedType, PString & value);

    PBoolean GetParam(PINDEX idx, PString & result);
    PBoolean GetParam(PINDEX idx, int & result);
    PBoolean GetParam(PINDEX idx, double & result);
    PBoolean GetParam(PINDEX idx, PTime & result, int tz = PTime::GMT);
    PBoolean GetParam(PINDEX idx, PStringToString & result);
    PBoolean GetParam(PINDEX idx, PXMLRPCStructBase & result);
    PBoolean GetParam(PINDEX idx, PStringArray & result);
    PBoolean GetParam(PINDEX idx, PArray<PStringToString> & result);

    // static functions for parsing values
    PBoolean ParseScalar(PXMLElement * element, PString & type, PString & value);
    PBoolean ParseStruct(PXMLElement * element, PStringToString & structDict);
    PBoolean ParseStruct(PXMLElement * element, PXMLRPCStructBase & structData);
    PBoolean ParseArray(PXMLElement * element, PStringArray & array);
    PBoolean ParseArray(PXMLElement * element, PArray<PStringToString> & array);
    PBoolean ParseArray(PXMLElement * element, PXMLRPCVariableBase & array);

    // static functions for creating values
    static PXMLElement * CreateValueElement(PXMLElement * element);
    static PXMLElement * CreateScalar(const PString & type, const PString & scalar);
    static PXMLElement * CreateMember(const PString & name, PXMLElement * value);

    static PXMLElement * CreateScalar(const PString & str);
    static PXMLElement * CreateScalar(int value);
    static PXMLElement * CreateScalar(double value);
    static PXMLElement * CreateDateAndTime(const PTime & time);
    static PXMLElement * CreateBinary(const PBYTEArray & data);

    static PXMLElement * CreateStruct();
    static PXMLElement * CreateStruct(const PStringToString & dict);
    static PXMLElement * CreateStruct(const PStringToString & dict, const PString & typeStr);
    static PXMLElement * CreateStruct(const PXMLRPCStructBase & structData);

    static PXMLElement * CreateArray(const PStringArray & array);
    static PXMLElement * CreateArray(const PStringArray & array, const PString & typeStr);
    static PXMLElement * CreateArray(const PStringArray & array, const PStringArray & types);
    static PXMLElement * CreateArray(const PArray<PStringToString> & array);
    static PXMLElement * CreateArray(const PXMLRPCVariableBase & array);

    // helper functions for adding parameters
    void AddParam(PXMLElement * parm);
    void AddParam(const PString & str);
    void AddParam(int value);
    void AddParam(double value);
    void AddParam(const PTime & time);
    void AddParam(const PXMLRPCStructBase & structData);
    void AddBinary(const PBYTEArray & data);
    void AddStruct(const PStringToString & dict);
    void AddStruct(const PStringToString & dict, const PString & typeStr);
    void AddArray(const PStringArray & array);
    void AddArray(const PStringArray & array, const PString & typeStr);
    void AddArray(const PStringArray & array, const PStringArray & types);
    void AddArray(const PArray<PStringToString> & array);

  protected:
    PXMLElement * params;
    PString faultText;
    PINDEX  faultCode;
};


/////////////////////////////////////////////////////////////////

class PXMLRPCVariableBase : public PObject {
    PCLASSINFO(PXMLRPCVariableBase, PObject);
  protected:
    PXMLRPCVariableBase(const char * name, const char * type = NULL);

  public:
    const char * GetName() const { return name; }
    const char * GetType() const { return type; }

    virtual void Copy(const PXMLRPCVariableBase & other) = 0;
    virtual PString ToString(PINDEX i) const;
    virtual void FromString(PINDEX i, const PString & str);
    virtual PXMLRPCStructBase * GetStruct(PINDEX i) const;
    virtual PBoolean IsArray() const;
    virtual PINDEX GetSize() const;
    virtual PBoolean SetSize(PINDEX);

    PString ToBase64(PAbstractArray & data) const;
    void FromBase64(const PString & str, PAbstractArray & data);

  protected:
    const char * name;
    const char * type;

  private:
    PXMLRPCVariableBase(const PXMLRPCVariableBase &) : PObject() { }
};


class PXMLRPCArrayBase : public PXMLRPCVariableBase {
    PCLASSINFO(PXMLRPCArrayBase, PXMLRPCVariableBase);
  protected:
    PXMLRPCArrayBase(PContainer & array, const char * name, const char * type);
    PXMLRPCArrayBase & operator=(const PXMLRPCArrayBase &);

  public:
    virtual void PrintOn(ostream & strm) const;
    virtual void Copy(const PXMLRPCVariableBase & other);
    virtual PBoolean IsArray() const;
    virtual PINDEX GetSize() const;
    virtual PBoolean SetSize(PINDEX);

  protected:
    PContainer & array;
};


class PXMLRPCArrayObjectsBase : public PXMLRPCArrayBase {
    PCLASSINFO(PXMLRPCArrayObjectsBase, PXMLRPCArrayBase);
  protected:
    PXMLRPCArrayObjectsBase(PArrayObjects & array, const char * name, const char * type);
    PXMLRPCArrayObjectsBase & operator=(const PXMLRPCArrayObjectsBase &);

  public:
    virtual PString ToString(PINDEX i) const;
    virtual void FromString(PINDEX i, const PString & str);
    virtual PBoolean SetSize(PINDEX);

    virtual PObject * CreateObject() const = 0;

  protected:
    PArrayObjects & array;
};


class PXMLRPCStructBase : public PObject {
    PCLASSINFO(PXMLRPCStructBase, PObject);
  protected:
    PXMLRPCStructBase();
    PXMLRPCStructBase & operator=(const PXMLRPCStructBase &);
  private:
    PXMLRPCStructBase(const PXMLRPCStructBase &) : PObject() { }

  public:
    void PrintOn(ostream & strm) const;

    PINDEX GetNumVariables() const { return variablesByOrder.GetSize(); }
    PXMLRPCVariableBase & GetVariable(PINDEX idx) const { return variablesByOrder[idx]; }
    PXMLRPCVariableBase * GetVariable(const char * name) const { return variablesByName.GetAt(name); }

    void AddVariable(PXMLRPCVariableBase * var);
    static PXMLRPCStructBase & GetInitialiser() { return *PAssertNULL(initialiserInstance); }

  protected:
    void EndConstructor();

    PArray<PXMLRPCVariableBase>               variablesByOrder;
    PDictionary<PString, PXMLRPCVariableBase> variablesByName;

    PXMLRPCStructBase        * initialiserStack;
    static PMutex              initialiserMutex;
    static PXMLRPCStructBase * initialiserInstance;
};


#define PXMLRPC_STRUCT_BEGIN(name) \
  class name : public PXMLRPCStructBase { \
    public: name() { EndConstructor(); } \
    public: name(const name & other) { EndConstructor(); operator=(other); } \
    public: name & operator=(const name & other) { PXMLRPCStructBase::operator=(other); return *this; }

#define PXMLRPC_VARIABLE_CLASS(base, type, variable, xmltype, init, extras) \
    private: struct PXMLRPCVar_##variable : public PXMLRPCVariableBase { \
      PXMLRPCVar_##variable() \
        : PXMLRPCVariableBase(#variable, xmltype), \
          instance(((base &)base::GetInitialiser()).variable) \
        { init } \
      virtual void PrintOn (ostream & s) const { s << instance; } \
      virtual void ReadFrom(istream & s)       { s >> instance; } \
      virtual void Copy(const PXMLRPCVariableBase & other) \
                    { instance = ((PXMLRPCVar_##variable &)other).instance; } \
      extras \
      type & instance; \
    } pxmlrpcvar_##variable

#define PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, init, extras) \
    public: type variable; \
    PXMLRPC_VARIABLE_CLASS(base, type, variable, xmltype, init, extras)

#define PXMLRPC_ARRAY_CUSTOM(base, arraytype, basetype, variable, xmltype, par, extras) \
    public: arraytype variable; \
    private: struct PXMLRPCVar_##variable : public par { \
      PXMLRPCVar_##variable() \
        : par(((base &)base::GetInitialiser()).variable, #variable, xmltype), \
          instance((arraytype &)array) \
        { } \
      extras \
      arraytype & instance; \
    } pxmlrpcvar_##variable
#ifdef DOCPLUSPLUS
}
#endif


#define PXMLRPC_STRUCT_END() \
  };


#define PXMLRPC_VARIABLE(base, type, variable, xmltype) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, ;, ;)


#define PXMLRPC_VARIABLE_INIT(base, type, variable, xmltype, init) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, xmltype, instance=init;, ;)


#define PXMLRPC_STRING(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "string")


#define PXMLRPC_STRING_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "string", init)


#define PXMLRPC_INTEGER(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "int")


#define PXMLRPC_INTEGER_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "int", init)


#define PXMLRPC_BOOLEAN(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "boolean")


#define PXMLRPC_BOOLEAN_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "boolean", init)


#define PXMLRPC_DOUBLE(base, type, variable) \
        PXMLRPC_VARIABLE(base, type, variable, "double")


#define PXMLRPC_DOUBLE_INIT(base, type, variable, init) \
        PXMLRPC_VARIABLE_INIT(base, type, variable, "double", init)


#define PXMLRPC_DATETIME(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "dateTime.iso8601", ;, \
                    PString ToString(PINDEX) const { return instance.AsString(PTime::ShortISO8601); } )


#define PXMLRPC_BINARY(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "base64", ;, \
                    PString ToString(PINDEX) const { return ToBase64(instance); } \
                    void FromString(PINDEX, const PString & str) { FromBase64(str, instance); } )


#define PXMLRPC_STRUCT(base, type, variable) \
        PXMLRPC_VARIABLE_CUSTOM(base, type, variable, "struct", ;, \
                    PXMLRPCStructBase * GetStruct(PINDEX) const { return &instance; } )


#define PXMLRPC_ARRAY(base, arraytype, basetype, variable, xmltype) \
        PXMLRPC_ARRAY_CUSTOM(base, arraytype, basetype, variable, xmltype, PXMLRPCArrayObjectsBase, \
                    PObject * CreateObject() const { return new basetype; })


#define PXMLRPC_ARRAY_STRING(base, arraytype, basetype, variable) \
        PXMLRPC_ARRAY(base, arraytype, basetype, variable, "string")

#define PXMLRPC_ARRAY_INTEGER(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PScalarArray<type>, type, variable, "int", PXMLRPCArrayBase, \
                    PString ToString(PINDEX i) const { return PString(instance[i]); } \
                    void FromString(PINDEX i, const PString & str) { instance[i] = (type)str.AsInteger(); })

#define PXMLRPC_ARRAY_DOUBLE(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PScalarArray<type>, type, variable, "double", PXMLRPCArrayBase, \
                    PString ToString(PINDEX i) const { return psprintf("%f", instance[i]); } \
                    void FromString(PINDEX i, const PString & str) { instance[i] = (type)str.AsReal(); })

#define PXMLRPC_ARRAY_STRUCT(base, type, variable) \
        PXMLRPC_ARRAY_CUSTOM(base, PArray<type>, type, variable, "struct", PXMLRPCArrayObjectsBase, \
                             PXMLRPCStructBase * GetStruct(PINDEX i) const { return &instance[i]; } \
                             PObject * CreateObject() const { return new type; })


#define PXMLRPC_FUNC_NOARG_NOREPLY(name) \
  PBoolean name() { return MakeRequest(#name); }


#define PXMLRPC_FUNC_SINGLE_ARG(name, vartype, argtype) \
  class name##_in : public PXMLRPCStructBase { \
    public: name##_in(const argtype & var) : variable(var) { EndConstructor(); } \
        vartype(name##_in, argtype, variable);


#define PXMLRPC_FUNC_MULTI_ARGS(name) \
  PXMLRPC_STRUCT_BEGIN(name##_in)


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_MULTI_REPLY(name) \
  }; PXMLRPC_STRUCT_BEGIN(name##_out)


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_NO_ARGS(name) \
  }; \
  PBoolean name(name##_out & reply) \
    { return MakeRequest(#name, name##_in(), reply); }


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_STRUCT_ARG(name) \
  }; \
  class name##_in_carrier : public PXMLRPCStructBase { \
    public: name##_in_carrier(const name##_in & var) : variable(var) { EndConstructor(); } \
    private: struct var_class : public PXMLRPCVariableBase { \
      var_class(const name##_in & var) \
        : PXMLRPCVariableBase("variable", "struct"), instance(var) { } \
      virtual void PrintOn (ostream & s) const { s << instance; } \
      virtual PXMLRPCStructBase * GetStruct(PINDEX) const { return (PXMLRPCStructBase *)&instance; } \
      virtual void Copy(const PXMLRPCVariableBase &) { } \
      const name##_in & instance; \
    } variable; \
  }; \
  PBoolean name(const name##_in & args, name##_out & reply) \
    { return MakeRequest(#name, name##_in_carrier(args), reply); }


#ifdef DOCPLUSPLUS
{
#endif
#define PXMLRPC_FUNC_NORM_ARGS(name) \
  }; \
  PBoolean name(const name##_in & args, name##_out & reply) \
    { return MakeRequest(#name, args, reply); }



/////////////////////////////////////////////////////////////////

#endif // P_XMLRPC

#endif // PTLIB_PXMLRPC_H


// End Of File ///////////////////////////////////////////////////////////////
