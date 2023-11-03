/*
 * asner.cxx
 *
 * Abstract Syntax Notation 1 Encoding Rules
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * $Log$
 * Revision 1.2  2016/12/19 15:33:04  willamowius
 * fix typecast for bit indicator
 *
 * Revision 1.1.1.1  2016/08/25 20:01:52  willamowius
 * PTLib fork based on 2.10.9
 *
 * Revision 1.93  2005/11/30 12:47:41  csoutheren
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "asner.h"
#endif

#include <ptclib/asner.h>

#ifdef P_EXPAT
#include <ptclib/pxml.h>
#endif

#define new PNEW


static PINDEX MaximumArraySize     = 128;
static PINDEX MaximumStringSize    = 16*1024;
static PINDEX MaximumSetSize       = 512;


static PINDEX CountBits(unsigned range)
{
  switch (range) {
    case 0 :
      return sizeof(unsigned)*8;
    case 1:
      return 1;
  }

  size_t nBits = 0;
  while (nBits < (sizeof(unsigned)*8) && range > (unsigned)(1 << nBits))
    nBits++;
  return nBits;
}

inline PBoolean CheckByteOffset(PINDEX offset, PINDEX upper = MaximumStringSize)
{
  // a 1mbit PDU has got to be an error
  return (0 <= offset && offset <= upper);
}

static PINDEX FindNameByValue(const PASN_Names *names, unsigned namesCount, PINDEX value)
{
  if (names != NULL) {
    for (unsigned int i = 0;i < namesCount;i++) {
      if (names[i].value == value)
        return i;
    }
  }
  return P_MAX_INDEX;
}

///////////////////////////////////////////////////////////////////////

PASN_Object::PASN_Object(unsigned theTag, TagClass theTagClass, PBoolean extend)
{
  extendable = extend;

  tag = theTag;

  if (theTagClass != DefaultTagClass)
    tagClass = theTagClass;
  else
    tagClass = ContextSpecificTagClass;
}


void PASN_Object::SetTag(unsigned newTag, TagClass tagClass_)
{
  tag = newTag;
  if (tagClass_ != DefaultTagClass)
    tagClass = tagClass_;
}


PINDEX PASN_Object::GetObjectLength() const
{
  PINDEX len = 1;

  if (tag >= 31)
    len += (CountBits(tag)+6)/7;

  PINDEX dataLen = GetDataLength();
  if (dataLen < 128)
    len++;
  else
    len += (CountBits(dataLen)+7)/8 + 1;

  return len + dataLen;
}


void PASN_Object::SetConstraintBounds(ConstraintType, int, unsigned)
{
}


void PASN_Object::SetCharacterSet(ConstraintType, const char *)
{
}


void PASN_Object::SetCharacterSet(ConstraintType, unsigned, unsigned)
{
}


PINDEX PASN_Object::GetMaximumArraySize()
{
  return MaximumArraySize;
}


void PASN_Object::SetMaximumArraySize(PINDEX sz)
{
  MaximumArraySize = sz;
}


PINDEX PASN_Object::GetMaximumStringSize()
{
  return MaximumStringSize;
}


void PASN_Object::SetMaximumStringSize(PINDEX sz)
{
  MaximumStringSize = sz;
}


///////////////////////////////////////////////////////////////////////

PASN_ConstrainedObject::PASN_ConstrainedObject(unsigned tag, TagClass tagClass)
  : PASN_Object(tag, tagClass)
{
  constraint = Unconstrained;
  lowerLimit = 0;
  upperLimit =  UINT_MAX;
}


void PASN_ConstrainedObject::SetConstraintBounds(ConstraintType ctype,
                                                 int lower, unsigned upper)
{
  constraint = ctype;
  if (constraint == Unconstrained) {
    lower = 0;
    upper = UINT_MAX;
  }

  extendable = ctype == ExtendableConstraint;
//  if ((lower >= 0 && upper < 0x7fffffff) ||
//     (lower < 0 && (unsigned)lower <= upper)) {
    lowerLimit = lower;
    upperLimit = upper;
//  }
}


///////////////////////////////////////////////////////////////////////

PASN_Null::PASN_Null(unsigned tag, TagClass tagClass)
  : PASN_Object(tag, tagClass)
{
}


PObject::Comparison PASN_Null::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Null), PInvalidCast);
  return EqualTo;
}


PObject * PASN_Null::Clone() const
{
  PAssert(IsClass(PASN_Null::Class()), PInvalidCast);
  return new PASN_Null(*this);
}


void PASN_Null::PrintOn(std::ostream & strm) const
{
  strm << "<<null>>";
}


PString PASN_Null::GetTypeAsString() const
{
  return "Null";
}


PINDEX PASN_Null::GetDataLength() const
{
  return 0;
}


PBoolean PASN_Null::Decode(PASN_Stream & strm)
{
  return strm.NullDecode(*this);
}


void PASN_Null::Encode(PASN_Stream & strm) const
{
  strm.NullEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_Boolean::PASN_Boolean(PBoolean val)
  : PASN_Object(UniversalBoolean, UniversalTagClass)
{
  value = val;
}


PASN_Boolean::PASN_Boolean(unsigned tag, TagClass tagClass, PBoolean val)
  : PASN_Object(tag, tagClass)
{
  value = val;
}


PObject::Comparison PASN_Boolean::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Boolean), PInvalidCast);
  return value == ((const PASN_Boolean &)obj).value ? EqualTo : GreaterThan;
}


PObject * PASN_Boolean::Clone() const
{
  PAssert(IsClass(PASN_Boolean::Class()), PInvalidCast);
  return new PASN_Boolean(*this);
}


void PASN_Boolean::PrintOn(std::ostream & strm) const
{
  if (value)
    strm << "true";
  else
    strm << "false";
}


PString PASN_Boolean::GetTypeAsString() const
{
  return "Boolean";
}


PINDEX PASN_Boolean::GetDataLength() const
{
  return 1;
}


PBoolean PASN_Boolean::Decode(PASN_Stream & strm)
{
  return strm.BooleanDecode(*this);
}


void PASN_Boolean::Encode(PASN_Stream & strm) const
{
  strm.BooleanEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_Integer::PASN_Integer(unsigned val)
  : PASN_ConstrainedObject(UniversalInteger, UniversalTagClass)
{
  value = val;
}


PASN_Integer::PASN_Integer(unsigned tag, TagClass tagClass, unsigned val)
  : PASN_ConstrainedObject(tag, tagClass)
{
  value = val;
}


PBoolean PASN_Integer::IsUnsigned() const
{
  return constraint != Unconstrained && lowerLimit >= 0;
}


PASN_Integer & PASN_Integer::operator=(unsigned val)
{
  if (constraint == Unconstrained)
    value = val;
  else if (lowerLimit >= 0) { // Is unsigned integer
    if (val < (unsigned)lowerLimit)
      value = lowerLimit;
    else if (val > upperLimit)
      value = upperLimit;
    else
      value = val;
  }
  else {
    int ival = (int)val;
    if (ival < lowerLimit)
      value = lowerLimit;
    else if (upperLimit < INT_MAX && ival > (int)upperLimit)
      value = upperLimit;
    else
      value = val;
  }

  return *this;
}


PObject::Comparison PASN_Integer::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Integer), PInvalidCast);
  const PASN_Integer & other = (const PASN_Integer &)obj;

  if (IsUnsigned()) {
    if (value < other.value)
      return LessThan;
    if (value > other.value)
      return GreaterThan;
  }
  else {
    if ((int)value < (int)other.value)
      return LessThan;
    if ((int)value > (int)other.value)
      return GreaterThan;
  }
  return EqualTo;
}


PObject * PASN_Integer::Clone() const
{
  PAssert(IsClass(PASN_Integer::Class()), PInvalidCast);
  return new PASN_Integer(*this);
}


void PASN_Integer::PrintOn(std::ostream & strm) const
{
  if (constraint == Unconstrained || lowerLimit < 0)
    strm << (int)value;
  else
    strm << value;
}


void PASN_Integer::SetConstraintBounds(ConstraintType type, int lower, unsigned upper)
{
  PASN_ConstrainedObject::SetConstraintBounds(type, lower, upper);
  operator=(value);
}


PString PASN_Integer::GetTypeAsString() const
{
  return "Integer";
}


static PINDEX GetIntegerDataLength(int value)
{
  // create a mask which is the top nine bits of a DWORD, or 0xFF800000
  // on a big endian machine
  int shift = (sizeof(value)-1)*8-1;

  // remove all sequences of nine 0's or 1's at the start of the value
  while (shift > 0 && ((value >> shift)&0x1ff) == (value < 0 ? 0x1ff : 0))
    shift -= 8;

  return (shift+9)/8;
}


PINDEX PASN_Integer::GetDataLength() const
{
  return GetIntegerDataLength(value);
}


PBoolean PASN_Integer::Decode(PASN_Stream & strm)
{
  return strm.IntegerDecode(*this);
}


void PASN_Integer::Encode(PASN_Stream & strm) const
{
  strm.IntegerEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_Enumeration::PASN_Enumeration(unsigned val)
: PASN_Object(UniversalEnumeration, UniversalTagClass, PFalse),names(NULL),namesCount(0)
{
  value = val;
  maxEnumValue = P_MAX_INDEX;
}


PASN_Enumeration::PASN_Enumeration(unsigned tag, TagClass tagClass,
                                   unsigned maxEnum, PBoolean extend,
                                   unsigned val)
  : PASN_Object(tag, tagClass, extend),names(NULL),namesCount(0)
{
  value = val;
  maxEnumValue = maxEnum;
}



PASN_Enumeration::PASN_Enumeration(unsigned tag, TagClass tagClass,
                                   unsigned maxEnum, PBoolean extend,
                                   const PASN_Names * nameSpec,
                                   unsigned namesCnt,
                                   unsigned val)
  : PASN_Object(tag, tagClass, extend),
  names(nameSpec),namesCount(namesCnt)
{
  maxEnumValue = maxEnum;

  PAssert(val <= maxEnum, PInvalidParameter);
  value = val;
}


PObject::Comparison PASN_Enumeration::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Enumeration), PInvalidCast);
  const PASN_Enumeration & other = (const PASN_Enumeration &)obj;

  if (value < other.value)
    return LessThan;

  if (value > other.value)
    return GreaterThan;

  return EqualTo;
}


PObject * PASN_Enumeration::Clone() const
{
  PAssert(IsClass(PASN_Enumeration::Class()), PInvalidCast);
  return new PASN_Enumeration(*this);
}


void PASN_Enumeration::PrintOn(std::ostream & strm) const
{
  PINDEX idx = FindNameByValue(names, namesCount, value);
  if (idx != P_MAX_INDEX)
    strm << names[idx].name;
  else
    strm << '<' << value << '>';
}


PString PASN_Enumeration::GetTypeAsString() const
{
  return "Enumeration";
}


PINDEX PASN_Enumeration::GetDataLength() const
{
  return GetIntegerDataLength(value);
}


PBoolean PASN_Enumeration::Decode(PASN_Stream & strm)
{
  return strm.EnumerationDecode(*this);
}


void PASN_Enumeration::Encode(PASN_Stream & strm) const
{
  strm.EnumerationEncode(*this);
}

PINDEX PASN_Enumeration::GetValueByName(PString name) const
{
  for(unsigned uiIndex = 0; uiIndex < namesCount; uiIndex++){
    if(strcmp(names[uiIndex].name, name) == 0){
      return (maxEnumValue - namesCount + uiIndex + 1);
    }
  }
  return UINT_MAX;
}

///////////////////////////////////////////////////////////////////////

PASN_Real::PASN_Real(double val)
  : PASN_Object(UniversalReal, UniversalTagClass)
{
  value = val;
}


PASN_Real::PASN_Real(unsigned tag, TagClass tagClass, double val)
  : PASN_Object(tag, tagClass)
{
  value = val;
}


PObject::Comparison PASN_Real::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Real), PInvalidCast);
  const PASN_Real & other = (const PASN_Real &)obj;

  if (value < other.value)
    return LessThan;

  if (value > other.value)
    return GreaterThan;

  return EqualTo;
}


PObject * PASN_Real::Clone() const
{
  PAssert(IsClass(PASN_Real::Class()), PInvalidCast);
  return new PASN_Real(*this);
}


void PASN_Real::PrintOn(std::ostream & strm) const
{
  strm << value;
}


PString PASN_Real::GetTypeAsString() const
{
  return "Real";
}


PINDEX PASN_Real::GetDataLength() const
{
  PAssertAlways(PUnimplementedFunction);
  return 0;
}


PBoolean PASN_Real::Decode(PASN_Stream & strm)
{
  return strm.RealDecode(*this);
}


void PASN_Real::Encode(PASN_Stream & strm) const
{
  strm.RealEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_ObjectId::PASN_ObjectId(const char * dotstr)
  : PASN_Object(UniversalObjectId, UniversalTagClass)
{
  if (dotstr != NULL)
    SetValue(dotstr);
}


PASN_ObjectId::PASN_ObjectId(unsigned tag, TagClass tagClass)
  : PASN_Object(tag, tagClass)
{
}


PASN_ObjectId::PASN_ObjectId(const PASN_ObjectId & other)
  : PASN_Object(other),
    value(other.value, other.GetSize())
{
}


PASN_ObjectId & PASN_ObjectId::operator=(const PASN_ObjectId & other)
{
  PASN_Object::operator=(other);
  value = PUnsignedArray(other.value, other.GetSize());
  return *this;
}


PASN_ObjectId & PASN_ObjectId::operator=(const char * dotstr)
{
  if (dotstr != NULL)
    SetValue(dotstr);
  else
    value.SetSize(0);
  return *this;
}


PASN_ObjectId & PASN_ObjectId::operator=(const PString & dotstr)
{
  SetValue(dotstr);
  return *this;
}


PASN_ObjectId & PASN_ObjectId::operator=(const PUnsignedArray & numbers)
{
  SetValue(numbers);
  return *this;
}


void PASN_ObjectId::SetValue(const PString & dotstr)
{
  PStringArray parts = dotstr.Tokenise('.');
  value.SetSize(parts.GetSize());
  for (PINDEX i = 0; i < parts.GetSize(); i++)
    value[i] = parts[i].AsUnsigned();
}


void PASN_ObjectId::SetValue(const unsigned * numbers, PINDEX size)
{
  value = PUnsignedArray(numbers, size);
}


bool PASN_ObjectId::operator==(const char * dotstr) const
{
  PASN_ObjectId id;
  id.SetValue(dotstr);
  return *this == id;
}


PObject::Comparison PASN_ObjectId::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_ObjectId), PInvalidCast);
  const PASN_ObjectId & other = (const PASN_ObjectId &)obj;
  return value.Compare(other.value);
}


PObject * PASN_ObjectId::Clone() const
{
  PAssert(IsClass(PASN_ObjectId::Class()), PInvalidCast);
  return new PASN_ObjectId(*this);
}


void PASN_ObjectId::PrintOn(std::ostream & strm) const
{
  for (PINDEX i = 0; i < value.GetSize(); i++) {
    strm << (unsigned)value[i];
    if (i < value.GetSize()-1)
      strm << '.';
  }
}


PString PASN_ObjectId::AsString() const
{
  PStringStream s;
  PrintOn(s);
  return s;
}


PString PASN_ObjectId::GetTypeAsString() const
{
  return "Object ID";
}


PBoolean PASN_ObjectId::CommonDecode(PASN_Stream & strm, unsigned dataLen)
{
  value.SetSize(0);

  // handle zero length strings correctly
  if (dataLen == 0)
    return PTrue;

  unsigned subId;

  // start at the second identifier in the buffer, because we will later
  // expand the first number into the first two IDs
  PINDEX i = 1;
  while (dataLen > 0) {
    unsigned byte;
    subId = 0;
    do {    /* shift and add in low order 7 bits */
      if (strm.IsAtEnd())
        return PFalse;
      byte = strm.ByteDecode();
      subId = (subId << 7) + (byte & 0x7f);
      dataLen--;
    } while ((byte & 0x80) != 0);
    value.SetAt(i++, subId);
  }

  /*
   * The first two subidentifiers are encoded into the first component
   * with the value (X * 40) + Y, where:
   *  X is the value of the first subidentifier.
   *  Y is the value of the second subidentifier.
   */
  subId = value[1];
  if (subId < 40) {
    value[0] = 0;
    value[1] = subId;
  }
  else if (subId < 80) {
    value[0] = 1;
    value[1] = subId-40;
  }
  else {
    value[0] = 2;
    value[1] = subId-80;
  }

  return PTrue;
}


void PASN_ObjectId::CommonEncode(PBYTEArray & encodecObjectId) const
{
  PINDEX length = value.GetSize();
  const unsigned * objId = value;

  if (length < 2) {
    // Thise case is really illegal, but we have to do SOMETHING
    encodecObjectId.SetSize(0);
    return;
  }

  unsigned subId = (objId[0] * 40) + objId[1];
  objId += 2;

  PINDEX outputPosition = 0;

  while (--length > 0) {
    if (subId < 128)
      encodecObjectId[outputPosition++] = (BYTE)subId;
    else {
      unsigned mask = 0x7F; /* handle subid == 0 case */
      int bits = 0;

      /* testmask *MUST* !!!! be of an unsigned type */
      unsigned testmask = 0x7F;
      int      testbits = 0;
      while (testmask != 0) {
        if (subId & testmask) {  /* if any bits set */
          mask = testmask;
          bits = testbits;
        }
        testmask <<= 7;
        testbits += 7;
      }

      /* mask can't be zero here */
      while (mask != 0x7F) {
        /* fix a mask that got truncated above */
        if (mask == 0x1E00000)
          mask = 0xFE00000;

        encodecObjectId[outputPosition++] = (BYTE)(((subId & mask) >> bits) | 0x80);

        mask >>= 7;
        bits -= 7;
      }

      encodecObjectId[outputPosition++] = (BYTE)(subId & mask);
    }

    if (length > 1)
      subId = *objId++;
  }
}


PINDEX PASN_ObjectId::GetDataLength() const
{
  PBYTEArray dummy;
  CommonEncode(dummy);
  return dummy.GetSize();
}


PBoolean PASN_ObjectId::Decode(PASN_Stream & strm)
{
  return strm.ObjectIdDecode(*this);
}


void PASN_ObjectId::Encode(PASN_Stream & strm) const
{
  strm.ObjectIdEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_BitString::PASN_BitString(unsigned nBits, const BYTE * buf)
  : PASN_ConstrainedObject(UniversalBitString, UniversalTagClass),
    totalBits(nBits),
    bitData((totalBits+7)/8)
{
  if (buf != NULL)
    memcpy(bitData.GetPointer(), buf, bitData.GetSize());
}


PASN_BitString::PASN_BitString(unsigned tag, TagClass tagClass, unsigned nBits)
  : PASN_ConstrainedObject(tag, tagClass),
    totalBits(nBits),
    bitData((totalBits+7)/8)
{
}


PASN_BitString::PASN_BitString(const PASN_BitString & other)
  : PASN_ConstrainedObject(other),
    bitData(other.bitData, other.bitData.GetSize())
{
  totalBits = other.totalBits;
}


PASN_BitString & PASN_BitString::operator=(const PASN_BitString & other)
{
  PASN_ConstrainedObject::operator=(other);
  totalBits = other.totalBits;
  bitData = PBYTEArray(other.bitData, other.bitData.GetSize());
  return *this;
}


void PASN_BitString::SetData(unsigned nBits, const PBYTEArray & bytes)
{
  if ((PINDEX)nBits >= MaximumStringSize)
    return;

  bitData = bytes;
  SetSize(nBits);
}


void PASN_BitString::SetData(unsigned nBits, const BYTE * buf, PINDEX size)
{
  if ((PINDEX)nBits >= MaximumStringSize)
    return;

  if (size == 0)
    size = (nBits+7)/8;
  memcpy(bitData.GetPointer(size), buf, size);
  SetSize(nBits);
}


PBoolean PASN_BitString::SetSize(unsigned nBits)
{
  if (!CheckByteOffset(nBits))
    return PFalse;

  if (constraint == Unconstrained)
    totalBits = nBits;
  else if (totalBits < (unsigned)lowerLimit) {
    if (lowerLimit < 0)
      return PFalse;
    totalBits = lowerLimit;
  } else if ((unsigned)totalBits > upperLimit) {
    if (upperLimit > (unsigned)MaximumSetSize)
      return PFalse;
    totalBits = upperLimit;
  } else
    totalBits = nBits;
  return bitData.SetSize((totalBits+7)/8);
}


bool PASN_BitString::operator[](PINDEX bit) const
{
  if ((unsigned)bit < totalBits)
    return (bitData[(unsigned)bit>>3] & (1 << (7 - ((unsigned)bit&7)))) != 0;
  return PFalse;
}


void PASN_BitString::Set(unsigned bit)
{
  if (bit < totalBits)
    bitData[(PINDEX)(bit>>3)] |= 1 << (7 - (bit&7));
}


void PASN_BitString::Clear(unsigned bit)
{
  if (bit < totalBits)
    bitData[(PINDEX)(bit>>3)] &= ~(1 << (7 - (bit&7)));
}


void PASN_BitString::Invert(unsigned bit)
{
  if (bit < totalBits)
    bitData[(PINDEX)(bit>>3)] ^= 1 << (7 - (bit&7));
}


PObject::Comparison PASN_BitString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_BitString), PInvalidCast);
  const PASN_BitString & other = (const PASN_BitString &)obj;
  if (totalBits < other.totalBits)
    return LessThan;
  if (totalBits > other.totalBits)
    return GreaterThan;
  return bitData.Compare(other.bitData);
}


PObject * PASN_BitString::Clone() const
{
  PAssert(IsClass(PASN_BitString::Class()), PInvalidCast);
  return new PASN_BitString(*this);
}


void PASN_BitString::PrintOn(std::ostream & strm) const
{
  int indent = (int)strm.precision() + 2;
  std::ios::fmtflags flags = strm.flags();

  if (totalBits > 128)
    strm << "Hex {\n"
         << std::hex << std::setfill('0') << std::resetiosflags(std::ios::floatfield) << std::setiosflags(std::ios::fixed)
         << std::setw(16) << std::setprecision(indent) << bitData
         << std::dec << std::setfill(' ') << std::resetiosflags(std::ios::floatfield)
         << std::setw(indent-1) << "}";
  else if (totalBits > 32)
    strm << "Hex:"
         << std::hex << std::setfill('0') << std::resetiosflags(std::ios::floatfield) << std::setiosflags(std::ios::fixed)
         << std::setprecision(2) << std::setw(16) << bitData
         << std::dec << std::setfill(' ') << std::resetiosflags(std::ios::floatfield);
  else {
    BYTE mask = 0x80;
    PINDEX offset = 0;
    for (unsigned i = 0; i < totalBits; i++) {
      strm << ((bitData[offset]&mask) != 0 ? '1' : '0');
      mask >>= 1;
      if (mask == 0) {
        mask = 0x80;
        offset++;
      }
    }
  }

  strm.flags(flags);
}


void PASN_BitString::SetConstraintBounds(ConstraintType type, int lower, unsigned upper)
{
  if (lower < 0)
    return;

  PASN_ConstrainedObject::SetConstraintBounds(type, lower, upper);
  SetSize(GetSize());
}


PString PASN_BitString::GetTypeAsString() const
{
  return "Bit String";
}


PINDEX PASN_BitString::GetDataLength() const
{
  return (totalBits+7)/8 + 1;
}


PBoolean PASN_BitString::Decode(PASN_Stream & strm)
{
  return strm.BitStringDecode(*this);
}


void PASN_BitString::Encode(PASN_Stream & strm) const
{
  strm.BitStringEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_OctetString::PASN_OctetString(const char * str, PINDEX size)
  : PASN_ConstrainedObject(UniversalOctetString, UniversalTagClass)
{
  if (str != NULL) {
    if (size == 0)
      size = ::strlen(str);
    SetValue((const BYTE *)str, size);
  }
}


PASN_OctetString::PASN_OctetString(unsigned tag, TagClass tagClass)
  : PASN_ConstrainedObject(tag, tagClass)
{
}


PASN_OctetString::PASN_OctetString(const PASN_OctetString & other)
  : PASN_ConstrainedObject(other),
    value(other.value, other.GetSize())
{
}


PASN_OctetString & PASN_OctetString::operator=(const PASN_OctetString & other)
{
  PASN_ConstrainedObject::operator=(other);
  value = PBYTEArray(other.value, other.GetSize());
  return *this;
}


PASN_OctetString & PASN_OctetString::operator=(const char * str)
{
  if (str == NULL)
    value.SetSize(lowerLimit);
  else
    SetValue((const BYTE *)str, strlen(str));
  return *this;
}


PASN_OctetString & PASN_OctetString::operator=(const PString & str)
{
  SetValue((const BYTE *)(const char *)str, str.GetSize()-1);
  return *this;
}


PASN_OctetString & PASN_OctetString::operator=(const PBYTEArray & arr)
{
  PINDEX len = arr.GetSize();
  if ((unsigned)len > upperLimit || (int)len < lowerLimit)
    SetValue(arr, len);
  else
    value = arr;
  return *this;
}


void PASN_OctetString::SetValue(const BYTE * data, PINDEX len)
{
  if ((unsigned)len > upperLimit)
    len = upperLimit;
  if (SetSize((int)len < lowerLimit ? lowerLimit : len))
    memcpy(value.GetPointer(), data, len);
}


PString PASN_OctetString::AsString() const
{
  if (value.IsEmpty())
    return PString();
  return PString((const char *)(const BYTE *)value, value.GetSize());
}


PObject::Comparison PASN_OctetString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_OctetString), PInvalidCast);
  const PASN_OctetString & other = (const PASN_OctetString &)obj;
  return value.Compare(other.value);
}


PObject * PASN_OctetString::Clone() const
{
  PAssert(IsClass(PASN_OctetString::Class()), PInvalidCast);
  return new PASN_OctetString(*this);
}


void PASN_OctetString::PrintOn(std::ostream & strm) const
{
  int indent = (int)strm.precision() + 2;
  std::ios::fmtflags flags = strm.flags();

  strm << ' ' << value.GetSize() << " octets {\n"
       << std::hex << std::setfill('0') << std::resetiosflags(std::ios::floatfield)
       << std::setprecision(indent) << std::setw(16);

  if (value.GetSize() <= 32 || (flags&std::ios::floatfield) != std::ios::fixed)
    strm << value << '\n';
  else {
    PBYTEArray truncatedArray(value, 32);
    strm << truncatedArray << '\n'
         << std::setfill(' ')
         << std::setw(indent+4) << "...\n";
  }

  strm << std::dec << std::setfill(' ')
       << std::setw(indent-1) << "}";

  strm.flags(flags);
}


void PASN_OctetString::SetConstraintBounds(ConstraintType type, int lower, unsigned upper)
{
  if (lower < 0)
    return;

  PASN_ConstrainedObject::SetConstraintBounds(type, lower, upper);
  SetSize(GetSize());
}


PString PASN_OctetString::GetTypeAsString() const
{
  return "Octet String";
}


PINDEX PASN_OctetString::GetDataLength() const
{
  return value.GetSize();
}


PBoolean PASN_OctetString::SetSize(PINDEX newSize)
{
  if (!CheckByteOffset(newSize, MaximumStringSize))
    return PFalse;

  if (constraint != Unconstrained) {
    if (newSize < (PINDEX)lowerLimit) {
      if (lowerLimit < 0)
        return PFalse;
      newSize = lowerLimit;
    } else if ((unsigned)newSize > upperLimit) {
      if (upperLimit > (unsigned)MaximumStringSize)
        return PFalse;
      newSize = upperLimit;
    }
  }

  return value.SetSize(newSize);
}


PBoolean PASN_OctetString::Decode(PASN_Stream & strm)
{
  return strm.OctetStringDecode(*this);
}


void PASN_OctetString::Encode(PASN_Stream & strm) const
{
  strm.OctetStringEncode(*this);
}

///////////////////////////////////////////////////////////////////////

PASN_ConstrainedString::PASN_ConstrainedString(const char * canonical, PINDEX size,
                                               unsigned tag, TagClass tagClass)
  : PASN_ConstrainedObject(tag, tagClass)
{
  canonicalSet = canonical;
  canonicalSetSize = size;
  canonicalSetBits = CountBits(size);
  SetCharacterSet(canonicalSet, canonicalSetSize, Unconstrained);
}


PASN_ConstrainedString & PASN_ConstrainedString::operator=(const char * str)
{
  if (str == NULL)
    str = "";

  PStringStream newValue;

  PINDEX len = strlen(str);

  // Can't copy any more characters than the upper constraint
  if ((unsigned)len > upperLimit)
    len = upperLimit;

  // Now copy individual characters, if they are in character set constraint
  for (PINDEX i = 0; i < len; i++) {
    PINDEX sz = characterSet.GetSize();
    if (sz == 0 || memchr(characterSet, str[i], sz) != NULL)
      newValue << str[i];
  }

  // Make sure string meets minimum length constraint
  while ((int)len < lowerLimit) {
    newValue << characterSet[0];
    len++;
  }

  value = newValue;
  value.MakeMinimumSize();
  return *this;
}


void PASN_ConstrainedString::SetCharacterSet(ConstraintType ctype, const char * set)
{
  SetCharacterSet(set, strlen(set), ctype);
}


void PASN_ConstrainedString::SetCharacterSet(ConstraintType ctype, unsigned firstChar, unsigned lastChar)
{
  char buffer[256];
  for (unsigned i = firstChar; i < lastChar; i++)
    buffer[i] = (char)i;
  SetCharacterSet(buffer, lastChar - firstChar + 1, ctype);
}


void PASN_ConstrainedString::SetCharacterSet(const char * set, PINDEX setSize, ConstraintType ctype)
{
  if (ctype == Unconstrained) {
    characterSet.SetSize(canonicalSetSize);
    memcpy(characterSet.GetPointer(), canonicalSet, canonicalSetSize);
  }
  else if (setSize >= MaximumSetSize ||
           canonicalSetSize >= MaximumSetSize ||
           characterSet.GetSize() >= MaximumSetSize)
    return;
  else {
    characterSet.SetSize(setSize);
    PINDEX count = 0;
    for (PINDEX i = 0; i < canonicalSetSize; i++) {
      if (memchr(set, canonicalSet[i], setSize) != NULL)
        characterSet[count++] = canonicalSet[i];
    }
    characterSet.SetSize(count);
  }

  charSetUnalignedBits = CountBits(characterSet.GetSize());

  charSetAlignedBits = 1;
  while (charSetUnalignedBits > charSetAlignedBits)
    charSetAlignedBits <<= 1;

  operator=((const char *)value);
}


PObject::Comparison PASN_ConstrainedString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_ConstrainedString), PInvalidCast);
  const PASN_ConstrainedString & other = (const PASN_ConstrainedString &)obj;
  return value.Compare(other.value);
}


void PASN_ConstrainedString::PrintOn(std::ostream & strm) const
{
  strm << value.ToLiteral();
}


void PASN_ConstrainedString::SetConstraintBounds(ConstraintType type,
                                                 int lower, unsigned upper)
{
  if (lower < 0)
    return;

  PASN_ConstrainedObject::SetConstraintBounds(type, lower, upper);
  if (constraint != Unconstrained) {
    if (value.GetSize() < (PINDEX)lowerLimit)
      value.SetSize(lowerLimit);
    else if ((unsigned)value.GetSize() > upperLimit)
      value.SetSize(upperLimit);
  }
}


PINDEX PASN_ConstrainedString::GetDataLength() const
{
  return value.GetSize()-1;
}


PBoolean PASN_ConstrainedString::Decode(PASN_Stream & strm)
{
  return strm.ConstrainedStringDecode(*this);
}


void PASN_ConstrainedString::Encode(PASN_Stream & strm) const
{
  strm.ConstrainedStringEncode(*this);
}


#define DEFINE_STRING_CLASS(name, set) \
  static const char name##StringSet[] = set; \
  PASN_##name##String::PASN_##name##String(const char * str) \
    : PASN_ConstrainedString(name##StringSet, sizeof(name##StringSet)-1, \
                             Universal##name##String, UniversalTagClass) \
    { PASN_ConstrainedString::SetValue(str); } \
  PASN_##name##String::PASN_##name##String(unsigned tag, TagClass tagClass) \
    : PASN_ConstrainedString(name##StringSet, sizeof(name##StringSet)-1, tag, tagClass) \
    { } \
  PASN_##name##String & PASN_##name##String::operator=(const char * str) \
    { PASN_ConstrainedString::SetValue(str); return *this; } \
  PASN_##name##String & PASN_##name##String::operator=(const PString & str) \
    { PASN_ConstrainedString::SetValue(str); return *this; } \
  PObject * PASN_##name##String::Clone() const \
    { PAssert(IsClass(PASN_##name##String::Class()), PInvalidCast); \
      return new PASN_##name##String(*this); } \
  PString PASN_##name##String::GetTypeAsString() const \
    { return #name " String"; }

DEFINE_STRING_CLASS(Numeric,   " 0123456789")
DEFINE_STRING_CLASS(Printable, " '()+,-./0123456789:=?"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "abcdefghijklmnopqrstuvwxyz")
DEFINE_STRING_CLASS(Visible,   " !\"#$%&'()*+,-./0123456789:;<=>?"
                               "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                               "`abcdefghijklmnopqrstuvwxyz{|}~")
DEFINE_STRING_CLASS(IA5,       "\000\001\002\003\004\005\006\007"
                               "\010\011\012\013\014\015\016\017"
                               "\020\021\022\023\024\025\026\027"
                               "\030\031\032\033\034\035\036\037"
                               " !\"#$%&'()*+,-./0123456789:;<=>?"
                               "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                               "`abcdefghijklmnopqrstuvwxyz{|}~\177")
DEFINE_STRING_CLASS(General,   "\000\001\002\003\004\005\006\007"
                               "\010\011\012\013\014\015\016\017"
                               "\020\021\022\023\024\025\026\027"
                               "\030\031\032\033\034\035\036\037"
                               " !\"#$%&'()*+,-./0123456789:;<=>?"
                               "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                               "`abcdefghijklmnopqrstuvwxyz{|}~\177"
                               "\200\201\202\203\204\205\206\207"
                               "\210\211\212\213\214\215\216\217"
                               "\220\221\222\223\224\225\226\227"
                               "\230\231\232\233\234\235\236\237"
                               "\240\241\242\243\244\245\246\247"
                               "\250\251\252\253\254\255\256\257"
                               "\260\261\262\263\264\265\266\267"
                               "\270\271\272\273\274\275\276\277"
                               "\300\301\302\303\304\305\306\307"
                               "\310\311\312\313\314\315\316\317"
                               "\320\321\322\323\324\325\326\327"
                               "\330\331\332\333\334\335\336\337"
                               "\340\341\342\343\344\345\346\347"
                               "\350\351\352\353\354\355\356\357"
                               "\360\361\362\363\364\365\366\367"
                               "\370\371\372\373\374\375\376\377")


///////////////////////////////////////////////////////////////////////

PASN_BMPString::PASN_BMPString(const char * str)
  : PASN_ConstrainedObject(UniversalBMPString, UniversalTagClass)
{
  Construct();
  if (str != NULL)
    SetValue(str);
}


PASN_BMPString::PASN_BMPString(const PWCharArray & wstr)
  : PASN_ConstrainedObject(UniversalBMPString, UniversalTagClass)
{
  Construct();
  SetValue(wstr);
}


PASN_BMPString::PASN_BMPString(unsigned tag, TagClass tagClass)
  : PASN_ConstrainedObject(tag, tagClass)
{
  Construct();
}


void PASN_BMPString::Construct()
{
  firstChar = 0;
  lastChar = 0xffff;
  charSetAlignedBits = 16;
  charSetUnalignedBits = 16;
}


PASN_BMPString::PASN_BMPString(const PASN_BMPString & other)
  : PASN_ConstrainedObject(other),
    value(other.value, other.value.GetSize()),
    characterSet(other.characterSet)
{
  firstChar = other.firstChar;
  lastChar = other.lastChar;
  charSetAlignedBits = other.charSetAlignedBits;
  charSetUnalignedBits = other.charSetUnalignedBits;
}


PASN_BMPString & PASN_BMPString::operator=(const PASN_BMPString & other)
{
  PASN_ConstrainedObject::operator=(other);

  value = PWCharArray(other.value, other.value.GetSize());
  characterSet = other.characterSet;
  firstChar = other.firstChar;
  lastChar = other.lastChar;
  charSetAlignedBits = other.charSetAlignedBits;
  charSetUnalignedBits = other.charSetUnalignedBits;

  return *this;
}


PBoolean PASN_BMPString::IsLegalCharacter(WORD ch)
{
  if (ch < firstChar)
    return PFalse;

  if (ch > lastChar)
    return PFalse;

  if (characterSet.IsEmpty())
    return PTrue;

  const wchar_t * wptr = characterSet;
  PINDEX count = characterSet.GetSize();
  while (count-- > 0) {
    if (*wptr == ch)
      return PTrue;
    wptr++;
  }

  return PFalse;
}

void PASN_BMPString::SetValueRaw(const wchar_t * array, PINDEX paramSize)
{
  // Can't copy any more than the upper constraint
  if ((unsigned)paramSize > upperLimit)
    paramSize = upperLimit;

  // Number of bytes must be at least lhe lower constraint
  PINDEX newSize = (int)paramSize < lowerLimit ? lowerLimit : paramSize;
  value.SetSize(newSize);

  PINDEX count = 0;
  for (PINDEX i = 0; i < paramSize; i++) {
    WORD c = array[i];
    if (IsLegalCharacter(c))
      value[count++] = c;
  }

  // Pad out with the first character till required size
  while (count < newSize)
    value[count++] = firstChar;

}

PASN_BMPString & PASN_BMPString::operator=(const PWCharArray & array)
{
  PINDEX paramSize = array.GetSize();

  // Remove trailing NULL character, if present
  if (paramSize > 0 && array[paramSize-1] == 0)
    paramSize--;

  SetValueRaw(array, paramSize);
 
  return *this;
}


void PASN_BMPString::SetCharacterSet(ConstraintType ctype, const char * charSet)
{
  PWCharArray array(strlen(charSet));

  PINDEX count = 0;
  while (*charSet != '\0')
    array[count++] = (BYTE)*charSet++;

  SetCharacterSet(ctype, array);
}


void PASN_BMPString::SetCharacterSet(ConstraintType ctype, const PWCharArray & charSet)
{
  if (ctype == Unconstrained) {
    firstChar = 0;
    lastChar = 0xffff;
    characterSet.SetSize(0);
  }
  else {
    characterSet = charSet;

    charSetUnalignedBits = CountBits(lastChar - firstChar + 1);
    if (!charSet.IsEmpty()) {
      unsigned count = 0;
      for (PINDEX i = 0; i < charSet.GetSize(); i++) {
        if (characterSet[i] >= firstChar && characterSet[i] <= lastChar)
          count++;
      }
      count = CountBits(count);
      if (charSetUnalignedBits > count)
        charSetUnalignedBits = count;
    }

    charSetAlignedBits = 1;
    while (charSetUnalignedBits > charSetAlignedBits)
      charSetAlignedBits <<= 1;

    SetValue(value);
  }
}


void PASN_BMPString::SetCharacterSet(ConstraintType ctype, unsigned first, unsigned last)
{
  if (ctype != Unconstrained) {
    PAssert(first < 0x10000 && last < 0x10000 && last > first, PInvalidParameter);
    firstChar = (WORD)first;
    lastChar = (WORD)last;
  }
  SetCharacterSet(ctype, characterSet);
}


PObject * PASN_BMPString::Clone() const
{
  PAssert(IsClass(PASN_BMPString::Class()), PInvalidCast);
  return new PASN_BMPString(*this);
}


PObject::Comparison PASN_BMPString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_BMPString), PInvalidCast);
  const PASN_BMPString & other = (const PASN_BMPString &)obj;
  return value.Compare(other.value);
}


void PASN_BMPString::PrintOn(std::ostream & strm) const
{
  int indent = (int)strm.precision() + 2;
  PINDEX sz = value.GetSize();
  strm << ' ' << sz << " characters {\n";
  PINDEX i = 0;
  while (i < sz) {
    strm << std::setw(indent) << " " << std::hex << std::setfill('0');
    PINDEX j;
    for (j = 0; j < 8; j++)
      if (i+j < sz)
        strm << std::setw(4) << value[i+j] << ' ';
      else
        strm << "     ";
    strm << "  ";
    for (j = 0; j < 8; j++) {
      if (i+j < sz) {
        WORD c = value[i+j];
        if (c < 128 && isprint(c))
          strm << (char)c;
        else
          strm << ' ';
      }
    }
    strm << std::dec << std::setfill(' ') << '\n';
    i += 8;
  }
  strm << std::setw(indent-1) << "}";
}


PString PASN_BMPString::GetTypeAsString() const
{
  return "BMP String";
}


PINDEX PASN_BMPString::GetDataLength() const
{
  return value.GetSize()*2;
}


PBoolean PASN_BMPString::Decode(PASN_Stream & strm)
{
  return strm.BMPStringDecode(*this);
}


void PASN_BMPString::Encode(PASN_Stream & strm) const
{
  strm.BMPStringEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_GeneralisedTime & PASN_GeneralisedTime::operator=(const PTime & time)
{
  value = time.AsString("yyyyMMddhhmmss.uz");
  value.Replace("GMT", "Z");
  return *this;
}


PTime PASN_GeneralisedTime::GetValue() const
{
  int year = value(0,3).AsInteger();
  int month = value(4,5).AsInteger();
  int day = value(6,7).AsInteger();
  int hour = value(8,9).AsInteger();
  int minute = value(10,11).AsInteger();
  int seconds = 0;
  int zonePos = 12;

  if (isdigit(value[12])) {
    seconds = value(12,13).AsInteger();
    if (value[14] != '.')
      zonePos = 14;
    else {
      zonePos = 15;
      while (isdigit(value[zonePos]))
        zonePos++;
    }
  }

  int zone = PTime::Local;
  switch (value[zonePos]) {
    case 'Z' :
      zone = PTime::UTC;
      break;
    case '+' :
    case '-' :
      zone = value(zonePos+1,zonePos+2).AsInteger()*60 +
             value(zonePos+3,zonePos+4).AsInteger();
  }

  return PTime(seconds, minute, hour, day, month, year, zone);
}


///////////////////////////////////////////////////////////////////////

PASN_UniversalTime & PASN_UniversalTime::operator=(const PTime & time)
{
  value = time.AsString("yyMMddhhmmssz");
  value.Replace("GMT", "Z");
  value.MakeMinimumSize();
  return *this;
}


PTime PASN_UniversalTime::GetValue() const
{
  int year = value(0,1).AsInteger();
  if (year < 36)
    year += 2000;
  else
    year += 1900;

  int month = value(2,3).AsInteger();
  int day = value(4,5).AsInteger();
  int hour = value(6,7).AsInteger();
  int minute = value(8,9).AsInteger();
  int seconds = 0;
  int zonePos = 10;

  if (isdigit(value[10])) {
    seconds = value(10,11).AsInteger();
    zonePos = 12;
  }

  int zone = PTime::UTC;
  if (value[zonePos] != 'Z')
    zone = value(zonePos+1,zonePos+2).AsInteger()*60 +
           value(zonePos+3,zonePos+4).AsInteger();

  return PTime(seconds, minute, hour, day, month, year, zone);
}


///////////////////////////////////////////////////////////////////////

PASN_Choice::PASN_Choice(unsigned nChoices, PBoolean extend)
  : PASN_Object(0, ApplicationTagClass, extend),names(NULL),namesCount(0)
{
  numChoices = nChoices;
  choice = NULL;
}


PASN_Choice::PASN_Choice(unsigned tag, TagClass tagClass,
                         unsigned upper, PBoolean extend)
  : PASN_Object(tag, tagClass, extend),names(NULL),namesCount(0)
{
  numChoices = upper;
  choice = NULL;
}


PASN_Choice::PASN_Choice(unsigned tag, TagClass tagClass,
                         unsigned upper, PBoolean extend, const PASN_Names * nameSpec,unsigned namesCnt)
  : PASN_Object(tag, tagClass, extend),
    names(nameSpec),namesCount(namesCnt)
{
  numChoices = upper;
  choice = NULL;
}


PASN_Choice::PASN_Choice(const PASN_Choice & other)
  : PASN_Object(other),
  names(other.names),namesCount(other.namesCount)
{
  numChoices = other.numChoices;

  if (other.CheckCreate())
    choice = (PASN_Object *)other.choice->Clone();
  else
    choice = NULL;
}


PASN_Choice & PASN_Choice::operator=(const PASN_Choice & other)
{
  if (&other == this) // Assigning to ourself, just do nothing.
    return *this;

  delete choice;

  PASN_Object::operator=(other);

  numChoices = other.numChoices;
  names = other.names;
  namesCount = other.namesCount;

  if (other.CheckCreate())
    choice = (PASN_Object *)other.choice->Clone();
  else
    choice = NULL;

  return *this;
}


PASN_Choice::~PASN_Choice()
{
  delete choice;
}


void PASN_Choice::SetTag(unsigned newTag, TagClass tagClass)
{
  PASN_Object::SetTag(newTag, tagClass);

  delete choice;

  if (CreateObject())
    choice->SetTag(newTag, tagClass);
}


PString PASN_Choice::GetTagName() const
{
  PINDEX idx = FindNameByValue(names, namesCount, tag);
  if (idx != P_MAX_INDEX)
    return names[idx].name;

  if (CheckCreate() &&
      PIsDescendant(choice, PASN_Choice) &&
      choice->GetTag() == tag &&
      choice->GetTagClass() == tagClass)
    return PString(choice->GetClass()) + "->" + ((PASN_Choice *)choice)->GetTagName();

  return psprintf("<%u>", tag);
}


PBoolean PASN_Choice::CheckCreate() const
{
  if (choice != NULL)
    return PTrue;

  return ((PASN_Choice *)this)->CreateObject();
}


PASN_Object & PASN_Choice::GetObject() const
{
  PAssert(CheckCreate(), "NULL Choice");
  return *choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9

#define CHOICE_CAST_OPERATOR(cls) \
  PASN_Choice::operator cls &() const \
  { \
    PAssert(CheckCreate(), "Cast of NULL choice"); \
    PAssert(choice->IsDescendant(cls::Class()), PInvalidCast); \
    return *(cls *)choice; \
  } \

#else

#define CHOICE_CAST_OPERATOR(cls) \
  PASN_Choice::operator cls &() \
  { \
    PAssert(CheckCreate(), "Cast of NULL choice"); \
    PAssert(PIsDescendant(choice, cls), PInvalidCast); \
    return *(cls *)choice; \
  } \
  PASN_Choice::operator const cls &() const \
  { \
    PAssert(CheckCreate(), "Cast of NULL choice"); \
    PAssert(PIsDescendant(choice, cls), PInvalidCast); \
    return *(const cls *)choice; \
  } \

#endif


CHOICE_CAST_OPERATOR(PASN_Null)
CHOICE_CAST_OPERATOR(PASN_Boolean)
CHOICE_CAST_OPERATOR(PASN_Integer)
CHOICE_CAST_OPERATOR(PASN_Enumeration)
CHOICE_CAST_OPERATOR(PASN_Real)
CHOICE_CAST_OPERATOR(PASN_ObjectId)
CHOICE_CAST_OPERATOR(PASN_BitString)
CHOICE_CAST_OPERATOR(PASN_OctetString)
CHOICE_CAST_OPERATOR(PASN_NumericString)
CHOICE_CAST_OPERATOR(PASN_PrintableString)
CHOICE_CAST_OPERATOR(PASN_VisibleString)
CHOICE_CAST_OPERATOR(PASN_IA5String)
CHOICE_CAST_OPERATOR(PASN_GeneralString)
CHOICE_CAST_OPERATOR(PASN_BMPString)
CHOICE_CAST_OPERATOR(PASN_Sequence)


PObject::Comparison PASN_Choice::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Choice), PInvalidCast);
  const PASN_Choice & other = (const PASN_Choice &)obj;

  CheckCreate();
  other.CheckCreate();

  if (choice == other.choice)
    return EqualTo;

  if (choice == NULL)
    return LessThan;

  if (other.choice == NULL)
    return GreaterThan;

  if (tag < other.tag)
    return LessThan;

  if (tag > other.tag)
    return GreaterThan;

  return choice->Compare(*other.choice);
}


void PASN_Choice::PrintOn(std::ostream & strm) const
{
  strm << GetTagName();

  if (choice != NULL)
    strm << ' ' << *choice;
  else
    strm << " (NULL)";
}


PString PASN_Choice::GetTypeAsString() const
{
  return "Choice";
}


PINDEX PASN_Choice::GetDataLength() const
{
  if (CheckCreate())
    return choice->GetDataLength();

  return 0;
}


PBoolean PASN_Choice::IsPrimitive() const
{
  if (CheckCreate())
    return choice->IsPrimitive();
  return PFalse;
}


PBoolean PASN_Choice::Decode(PASN_Stream & strm)
{
  return strm.ChoiceDecode(*this);
}


void PASN_Choice::Encode(PASN_Stream & strm) const
{
  strm.ChoiceEncode(*this);
}


PINDEX PASN_Choice::GetValueByName(PString name) const
{
  for(unsigned uiIndex = 0; uiIndex < numChoices; uiIndex++){
    if(strcmp(names[uiIndex].name, name) == 0){
      return names[uiIndex].value;
    }
  }
  return UINT_MAX;
}
///////////////////////////////////////////////////////////////////////

PASN_Sequence::PASN_Sequence(unsigned tag, TagClass tagClass,
                             unsigned nOpts, PBoolean extend, unsigned nExtend)
  : PASN_Object(tag, tagClass, extend)
{
  optionMap.SetConstraints(PASN_ConstrainedObject::FixedConstraint, nOpts);
  knownExtensions = nExtend;
  totalExtensions = 0;
  endBasicEncoding = 0;
}


PASN_Sequence::PASN_Sequence(const PASN_Sequence & other)
  : PASN_Object(other),
    fields(other.fields.GetSize()),
    optionMap(other.optionMap),
    extensionMap(other.extensionMap)
{
  for (PINDEX i = 0; i < other.fields.GetSize(); i++)
    fields.SetAt(i, other.fields[i].Clone());

  knownExtensions = other.knownExtensions;
  totalExtensions = other.totalExtensions;
  endBasicEncoding = 0;
}


PASN_Sequence & PASN_Sequence::operator=(const PASN_Sequence & other)
{
  PASN_Object::operator=(other);

  fields.SetSize(other.fields.GetSize());
  for (PINDEX i = 0; i < other.fields.GetSize(); i++)
    fields.SetAt(i, other.fields[i].Clone());

  optionMap = other.optionMap;
  knownExtensions = other.knownExtensions;
  totalExtensions = other.totalExtensions;
  extensionMap = other.extensionMap;

  return *this;
}


PBoolean PASN_Sequence::HasOptionalField(PINDEX opt) const
{
  if (opt < (PINDEX)optionMap.GetSize())
    return optionMap[opt];
  else
    return extensionMap[opt - optionMap.GetSize()];
}


void PASN_Sequence::IncludeOptionalField(PINDEX opt)
{
  if (opt < (PINDEX)optionMap.GetSize())
    optionMap.Set(opt);
  else {
    PAssert(extendable, "Must be extendable type");
    opt -= optionMap.GetSize();
    if (opt >= (PINDEX)extensionMap.GetSize())
      extensionMap.SetSize(opt+1);
    extensionMap.Set(opt);
  }
}


void PASN_Sequence::RemoveOptionalField(PINDEX opt)
{
  if (opt < (PINDEX)optionMap.GetSize())
    optionMap.Clear(opt);
  else {
    PAssert(extendable, "Must be extendable type");
    opt -= optionMap.GetSize();
    extensionMap.Clear(opt);
  }
}


PObject::Comparison PASN_Sequence::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Sequence), PInvalidCast);
  const PASN_Sequence & other = (const PASN_Sequence &)obj;
  return fields.Compare(other.fields);
}


PObject * PASN_Sequence::Clone() const
{
  PAssert(IsClass(PASN_Sequence::Class()), PInvalidCast);
  return new PASN_Sequence(*this);
}


void PASN_Sequence::PrintOn(std::ostream & strm) const
{
  int indent = (int)strm.precision() + 2;
  strm << "{\n";
  for (PINDEX i = 0; i < fields.GetSize(); i++) {
    strm << std::setw(indent+6) << "field[" << i << "] <";
    switch (fields[i].GetTagClass()) {
      case UniversalTagClass :
        strm << "Universal";
        break;
      case ApplicationTagClass :
        strm << "Application";
        break;
      case ContextSpecificTagClass :
        strm << "ContextSpecific";
        break;
      case PrivateTagClass :
        strm << "Private";
      default :
        break;
    }
    strm << '-' << fields[i].GetTag() << '-'
         << fields[i].GetTypeAsString() << "> = "
         << fields[i] << '\n';
  }
  strm << std::setw(indent-1) << "}";
}


PString PASN_Sequence::GetTypeAsString() const
{
  return "Sequence";
}


PINDEX PASN_Sequence::GetDataLength() const
{
  PINDEX len = 0;
  for (PINDEX i = 0; i < fields.GetSize(); i++)
    len += fields[i].GetObjectLength();
  return len;
}


PBoolean PASN_Sequence::IsPrimitive() const
{
  return PFalse;
}


PBoolean PASN_Sequence::Decode(PASN_Stream & strm)
{
  return PreambleDecode(strm) && UnknownExtensionsDecode(strm);
}


void PASN_Sequence::Encode(PASN_Stream & strm) const
{
  PreambleEncode(strm);
  UnknownExtensionsEncode(strm);
}


PBoolean PASN_Sequence::PreambleDecode(PASN_Stream & strm)
{
  return strm.SequencePreambleDecode(*this);
}


void PASN_Sequence::PreambleEncode(PASN_Stream & strm) const
{
  strm.SequencePreambleEncode(*this);
}


PBoolean PASN_Sequence::KnownExtensionDecode(PASN_Stream & strm, PINDEX fld, PASN_Object & field)
{
  return strm.SequenceKnownDecode(*this, fld, field);
}


void PASN_Sequence::KnownExtensionEncode(PASN_Stream & strm, PINDEX fld, const PASN_Object & field) const
{
  strm.SequenceKnownEncode(*this, fld, field);
}


PBoolean PASN_Sequence::UnknownExtensionsDecode(PASN_Stream & strm)
{
  return strm.SequenceUnknownDecode(*this);
}


void PASN_Sequence::UnknownExtensionsEncode(PASN_Stream & strm) const
{
  strm.SequenceUnknownEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_Set::PASN_Set(unsigned tag, TagClass tagClass,
                   unsigned nOpts, PBoolean extend, unsigned nExtend)
  : PASN_Sequence(tag, tagClass, nOpts, extend, nExtend)
{
}


PObject * PASN_Set::Clone() const
{
  PAssert(IsClass(PASN_Set::Class()), PInvalidCast);
  return new PASN_Set(*this);
}


PString PASN_Set::GetTypeAsString() const
{
  return "Set";
}

///////////////////////////////////////////////////////////////////////

PASN_Array::PASN_Array(unsigned tag, TagClass tagClass)
  : PASN_ConstrainedObject(tag, tagClass)
{
}


PASN_Array::PASN_Array(const PASN_Array & other)
  : PASN_ConstrainedObject(other),
    array(other.array.GetSize())
{
  for (PINDEX i = 0; i < other.array.GetSize(); i++)
    array.SetAt(i, other.array[i].Clone());
}


PASN_Array & PASN_Array::operator=(const PASN_Array & other)
{
  PASN_ConstrainedObject::operator=(other);

  array.SetSize(other.array.GetSize());
  for (PINDEX i = 0; i < other.array.GetSize(); i++)
    array.SetAt(i, other.array[i].Clone());

  return *this;
}


PBoolean PASN_Array::SetSize(PINDEX newSize)
{
  if (newSize > MaximumArraySize)
    return PFalse;

  PINDEX originalSize = array.GetSize();
  if (!array.SetSize(newSize))
    return PFalse;

  for (PINDEX i = originalSize; i < newSize; i++) {
    PASN_Object * obj = CreateObject();
    if (obj == NULL)
      return PFalse;

    array.SetAt(i, obj);
  }

  return PTrue;
}


PObject::Comparison PASN_Array::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PASN_Array), PInvalidCast);
  const PASN_Array & other = (const PASN_Array &)obj;
  return array.Compare(other.array);
}


void PASN_Array::PrintOn(std::ostream & strm) const
{
  std::ios::fmtflags oldflags(strm.flags());
  int indent = (int)strm.precision() + 2;
  strm << array.GetSize() << " entries {\n";
  for (PINDEX i = 0; i < array.GetSize(); i++)
    strm << std::setw(indent+1) << "[" << i << "]=" << std::setprecision(indent) << array[i] << '\n';
  strm << std::setw(indent-1) << "}";
  strm.flags(oldflags);
}


void PASN_Array::SetConstraintBounds(ConstraintType type, int lower, unsigned upper)
{
  if (lower < 0)
    return;

  PASN_ConstrainedObject::SetConstraintBounds(type, lower, upper);
  if (constraint != Unconstrained) {
    if (GetSize() < (PINDEX)lowerLimit)
      SetSize(lowerLimit);
    else if (GetSize() > (PINDEX)upperLimit)
      SetSize(upperLimit);
  }
}


PString PASN_Array::GetTypeAsString() const
{
  return "Array";
}


PINDEX PASN_Array::GetDataLength() const
{
  PINDEX len = 0;
  for (PINDEX i = 0; i < array.GetSize(); i++)
    len += array[i].GetObjectLength();
  return len;
}


PBoolean PASN_Array::IsPrimitive() const
{
  return PFalse;
}


PBoolean PASN_Array::Decode(PASN_Stream & strm)
{
  return strm.ArrayDecode(*this);
}


void PASN_Array::Encode(PASN_Stream & strm) const
{
  strm.ArrayEncode(*this);
}


///////////////////////////////////////////////////////////////////////

PASN_Stream::PASN_Stream()
{
  Construct();
}


PASN_Stream::PASN_Stream(const PBYTEArray & bytes)
  : PBYTEArray(bytes)
{
  Construct();
}


PASN_Stream::PASN_Stream(const BYTE * buf, PINDEX size)
  : PBYTEArray(buf, size)
{
  Construct();
}


void PASN_Stream::Construct()
{
  byteOffset = 0;
  bitOffset = 8;
}


void PASN_Stream::PrintOn(std::ostream & strm) const
{
  int indent = (int)strm.precision() + 2;
  strm << " size=" << GetSize()
       << " pos=" << byteOffset << '.' << (8-bitOffset)
       << " {\n";
  PINDEX i = 0;
  while (i < GetSize()) {
    strm << std::setw(indent) << " " << std::hex << std::setfill('0');
    PINDEX j;
    for (j = 0; j < 16; j++)
      if (i+j < GetSize())
        strm << std::setw(2) << (unsigned)(BYTE)theArray[i+j] << ' ';
      else
        strm << "   ";
    strm << "  ";
    for (j = 0; j < 16; j++) {
      if (i+j < GetSize()) {
        BYTE c = theArray[i+j];
        if (c < 128 && isprint(c))
          strm << c;
        else
          strm << ' ';
      }
    }
    strm << std::dec << std::setfill(' ') << '\n';
    i += 16;
  }
  strm << std::setw(indent-1) << "}";
}


void PASN_Stream::SetPosition(PINDEX newPos)
{
  if (!CheckByteOffset(byteOffset))
    return;

  if (newPos > GetSize())
    byteOffset = GetSize();
  else
    byteOffset = newPos;
  bitOffset = 8;
}


void PASN_Stream::ResetDecoder()
{
  byteOffset = 0;
  bitOffset = 8;
}


void PASN_Stream::BeginEncoding()
{
  bitOffset = 8;
  byteOffset = 0;
  PBYTEArray::operator=(PBYTEArray(20));
}


void PASN_Stream::CompleteEncoding()
{
  if (byteOffset != P_MAX_INDEX) {
    if (bitOffset != 8) {
      bitOffset = 8;
      byteOffset++;
    }
    SetSize(byteOffset);
    byteOffset = P_MAX_INDEX;
  }
}


BYTE PASN_Stream::ByteDecode()
{
  if (!CheckByteOffset(byteOffset, GetSize()))
    return 0;

  bitOffset = 8;
  return theArray[byteOffset++];
}


void PASN_Stream::ByteEncode(unsigned value)
{
  if (!CheckByteOffset(byteOffset))
    return;

  if (bitOffset != 8) {
    bitOffset = 8;
    byteOffset++;
  }
  if (byteOffset >= GetSize())
    SetSize(byteOffset+10);
  theArray[byteOffset++] = (BYTE)value;
}


unsigned PASN_Stream::BlockDecode(BYTE * bufptr, unsigned nBytes)
{
  if (nBytes == 0 || bufptr == NULL || !CheckByteOffset(byteOffset+nBytes))
    return 0;

  ByteAlign();

  if (byteOffset+nBytes > (unsigned)GetSize()) {
    nBytes = GetSize() - byteOffset;
    if (nBytes <= 0)
      return 0;
  }

  memcpy(bufptr, &theArray[byteOffset], nBytes);
  byteOffset += nBytes;
  return nBytes;
}


void PASN_Stream::BlockEncode(const BYTE * bufptr, PINDEX nBytes)
{
  if (!CheckByteOffset(byteOffset, GetSize()))
    return;

  if (nBytes == 0)
    return;

  ByteAlign();

  if (byteOffset+nBytes >= GetSize())
    SetSize(byteOffset+nBytes+10);

  memcpy(theArray+byteOffset, bufptr, nBytes);
  byteOffset += nBytes;
}


void PASN_Stream::ByteAlign()
{
  if (!CheckByteOffset(byteOffset, GetSize()))
    return;

  if (bitOffset != 8) {
    bitOffset = 8;
    byteOffset++;
  }
}


///////////////////////////////////////////////////////////////////////

#ifdef  P_INCLUDE_PER
#include "asnper.cxx"
#endif

#ifdef  P_INCLUDE_BER
#include "asnber.cxx"
#endif

#ifdef  P_INCLUDE_XER
#include "asnxer.cxx"
#endif

// End of file ////////////////////////////////////////////////////////////////
