/*
 * contain.cxx
 *
 * Container Classes
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
#include <ctype.h>


#ifdef __NUCLEUS_PLUS__
extern "C" int vsprintf(char *, const char *, va_list);
#endif

#if P_REGEX
#include <regex.h>
#else
#include "regex/regex.h"
#endif

#define regexpression()  ((regex_t *)expression)

#if !P_USE_INLINES
#include "ptlib/contain.inl"
#endif


PDEFINE_POOL_ALLOCATOR(PContainerReference);


#define new PNEW
#undef  __CLASS__
#define __CLASS__ GetClass()


///////////////////////////////////////////////////////////////////////////////

PContainer::PContainer(PINDEX initialSize)
{
  reference = new PContainerReference(initialSize);
  PAssert(reference != NULL, POutOfMemory);
}


PContainer::PContainer(int, const PContainer * cont)
{
  if (cont == this)
    return;

  PAssert(cont != NULL, PInvalidParameter);
  PAssert2(cont->reference != NULL, cont->GetClass(), "Clone of deleted container");

  reference = new PContainerReference(*cont->reference);
  PAssert(reference != NULL, POutOfMemory);
}


PContainer::PContainer(const PContainer & cont)
{
  if (&cont == this)
    return;

  PAssert2(cont.reference != NULL, cont.GetClass(), "Copy of deleted container");

  ++cont.reference->count;
  reference = cont.reference;  // copy the reference pointer
}


PContainer::PContainer(PContainerReference & ref)
  : reference(&ref)
{
}


void PContainer::AssignContents(const PContainer & cont)
{
  if(cont.reference == NULL){
    PAssertAlways("container reference is null");
    return;
  }
  if(cont.GetClass() == NULL){
    PAssertAlways("container class is null");
    return;
  }

  if (reference == cont.reference)
    return;

  if (--reference->count == 0) {
    DestroyContents();
    DestroyReference();
  }

  PAssert(++cont.reference->count > 1, "Assignment of container that was deleted");
  reference = cont.reference;
}


void PContainer::Destruct()
{
  if (reference != NULL) {
    if (--reference->count <= 0) {
      DestroyContents();
      DestroyReference();
    }
    reference = NULL;
  }
}


void PContainer::DestroyReference()
{
  delete reference;
}


PBoolean PContainer::SetMinSize(PINDEX minSize)
{
  PASSERTINDEX(minSize);
  if (minSize < 0)
    minSize = 0;
  if (minSize < GetSize())
    minSize = GetSize();
  return SetSize(minSize);
}


PBoolean PContainer::MakeUnique()
{
  if (IsUnique())
    return PTrue;

  PContainerReference * oldReference = reference;
  reference = new PContainerReference(*reference);
  --oldReference->count;

  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

static PVariablePoolAllocator<char> PAbstractArray_allocator;

PAbstractArray::PAbstractArray(PINDEX elementSizeInBytes, PINDEX initialSize)
  : PContainer(initialSize)
{
  elementSize = elementSizeInBytes;
  PAssert(elementSize != 0, PInvalidParameter);

  if (GetSize() == 0)
    theArray = NULL;
  else {
    theArray = PAbstractArray_allocator.allocate(GetSize() * elementSize);
    PAssert(theArray != NULL, POutOfMemory);
    memset(theArray, 0, GetSize() * elementSize);
  }

  allocatedDynamically = PTrue;
}


PAbstractArray::PAbstractArray(PINDEX elementSizeInBytes,
                               const void *buffer,
                               PINDEX bufferSizeInElements,
                               PBoolean dynamicAllocation)
  : PContainer(bufferSizeInElements)
{
  elementSize = elementSizeInBytes;
  PAssert(elementSize != 0, PInvalidParameter);

  allocatedDynamically = dynamicAllocation;

  if (GetSize() == 0)
    theArray = NULL;
  else if (dynamicAllocation) {
    PINDEX sizebytes = elementSize*GetSize();
    theArray = PAbstractArray_allocator.allocate(sizebytes);
    PAssert(theArray != NULL, POutOfMemory);
    memcpy(theArray, PAssertNULL(buffer), sizebytes);
  }
  else
    theArray = (char *)buffer;
}


PAbstractArray::PAbstractArray(PContainerReference & reference, PINDEX elementSizeInBytes)
  : PContainer(reference)
  , elementSize(elementSizeInBytes)
  , theArray(NULL)
  , allocatedDynamically(false)
{
}


void PAbstractArray::DestroyContents()
{
  if (theArray != NULL) {
    if (allocatedDynamically)
      PAbstractArray_allocator.deallocate(theArray, elementSize*GetSize());
    theArray = NULL;
  }
}


void PAbstractArray::CopyContents(const PAbstractArray & array)
{
  elementSize = array.elementSize;
  theArray = array.theArray;
  allocatedDynamically = array.allocatedDynamically;

  if (reference->constObject)
    MakeUnique();
}


void PAbstractArray::CloneContents(const PAbstractArray * array)
{
  elementSize = array->elementSize;
  PINDEX sizebytes = elementSize*GetSize();
  char * newArray = PAbstractArray_allocator.allocate(sizebytes);
  if (newArray == NULL)
    reference->size = 0;
  else
    memcpy(newArray, array->theArray, sizebytes);
  theArray = newArray;
  allocatedDynamically = PTrue;
}


void PAbstractArray::PrintOn(ostream & strm) const
{
  char separator = strm.fill();
  int width = (int)strm.width();
  for (PINDEX  i = 0; i < GetSize(); i++) {
    if (i > 0 && separator != '\0')
      strm << separator;
    strm.width(width);
    PrintElementOn(strm, i);
  }
  if (separator == '\n')
    strm << '\n';
}


void PAbstractArray::ReadFrom(istream & strm)
{
  PINDEX i = 0;
  while (strm.good()) {
    ReadElementFrom(strm, i);
    if (!strm.fail())
      i++;
  }
  SetSize(i);
}


PObject::Comparison PAbstractArray::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PAbstractArray), PInvalidCast);
  const PAbstractArray & other = (const PAbstractArray &)obj;

  char * otherArray = other.theArray;
  if (theArray == otherArray)
    return EqualTo;

  if (elementSize < other.elementSize)
    return LessThan;

  if (elementSize > other.elementSize)
    return GreaterThan;

  PINDEX thisSize = GetSize();
  PINDEX otherSize = other.GetSize();

  if (thisSize < otherSize)
    return LessThan;

  if (thisSize > otherSize)
    return GreaterThan;

  if (thisSize == 0)
    return EqualTo;

  int retval = memcmp(theArray, otherArray, elementSize*thisSize);
  if (retval < 0)
    return LessThan;
  if (retval > 0)
    return GreaterThan;
  return EqualTo;
}


PBoolean PAbstractArray::SetSize(PINDEX newSize)
{
  return InternalSetSize(newSize, PFalse);
}


PBoolean PAbstractArray::InternalSetSize(PINDEX newSize, PBoolean force)
{
  if (newSize < 0)
    newSize = 0;

  PINDEX newsizebytes = elementSize*newSize;
  PINDEX oldsizebytes = elementSize*GetSize();

  if (!force && (newsizebytes == oldsizebytes))
    return PTrue;

  char * newArray;

  if (!IsUnique()) {

    if (newsizebytes == 0)
      newArray = NULL;
    else {
      if ((newArray = PAbstractArray_allocator.allocate(newsizebytes)) == NULL)
        return PFalse;
  
      allocatedDynamically = true;

      if (theArray != NULL)
        memcpy(newArray, theArray, PMIN(oldsizebytes, newsizebytes));
    }

    --reference->count;
    reference = new PContainerReference(newSize);

  } else {

    if (theArray != NULL) {
      if (newsizebytes == 0) {
        if (allocatedDynamically)
          PAbstractArray_allocator.deallocate(theArray, oldsizebytes);
        newArray = NULL;
      }
      else {
        if ((newArray = PAbstractArray_allocator.allocate(newsizebytes)) == NULL)
          return false;
        memcpy(newArray, theArray, PMIN(newsizebytes, oldsizebytes));
        if (allocatedDynamically)
          PAbstractArray_allocator.deallocate(theArray, oldsizebytes);
        allocatedDynamically = true;
      }
    }
    else if (newsizebytes != 0) {
      if ((newArray = PAbstractArray_allocator.allocate(newsizebytes)) == NULL)
        return PFalse;
    }
    else
      newArray = NULL;

    reference->size = newSize;
  }

  if (newsizebytes > oldsizebytes)
    memset(newArray+oldsizebytes, 0, newsizebytes-oldsizebytes);

  theArray = newArray;
  return PTrue;
}

void PAbstractArray::Attach(const void *buffer, PINDEX bufferSize)
{
  if (allocatedDynamically && theArray != NULL)
    PAbstractArray_allocator.deallocate(theArray, elementSize*GetSize());

  theArray = (char *)buffer;
  reference->size = bufferSize;
  allocatedDynamically = PFalse;
}


void * PAbstractArray::GetPointer(PINDEX minSize)
{
  PAssert(SetMinSize(minSize), POutOfMemory);
  return theArray;
}


PBoolean PAbstractArray::Concatenate(const PAbstractArray & array)
{
  if (!allocatedDynamically || array.elementSize != elementSize)
    return PFalse;

  PINDEX oldLen = GetSize();
  PINDEX addLen = array.GetSize();

  if (!SetSize(oldLen + addLen))
    return PFalse;

  memcpy(theArray+oldLen*elementSize, array.theArray, addLen*elementSize);
  return PTrue;
}


void PAbstractArray::PrintElementOn(ostream & /*stream*/, PINDEX /*index*/) const
{
}


void PAbstractArray::ReadElementFrom(istream & /*stream*/, PINDEX /*index*/)
{
}


///////////////////////////////////////////////////////////////////////////////

void PCharArray::PrintOn(ostream & strm) const
{
  PINDEX width = (int)strm.width();
  if (width > GetSize())
    width -= GetSize();
  else
    width = 0;

  PBoolean left = (strm.flags()&ios::adjustfield) == ios::left;
  if (left)
    strm.write(theArray, GetSize());

  while (width-- > 0)
    strm << (char)strm.fill();

  if (!left)
    strm.write(theArray, GetSize());
}


void PCharArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  while (strm.good()) {
    strm >> theArray[size++];
    if (size >= GetSize())
      SetSize(size+100);
  }

  SetSize(size);
}


void PBYTEArray::PrintOn(ostream & strm) const
{
  PINDEX line_width = (int)strm.width();
  if (line_width == 0)
    line_width = 16;
  strm.width(0);

  PINDEX indent = (int)strm.precision();

  PINDEX val_width = ((strm.flags()&ios::basefield) == ios::hex) ? 2 : 3;

  PINDEX i = 0;
  while (i < GetSize()) {
    if (i > 0)
      strm << '\n';
    PINDEX j;
    for (j = 0; j < indent; j++)
      strm << ' ';
    for (j = 0; j < line_width; j++) {
      if (j == line_width/2)
        strm << ' ';
      if (i+j < GetSize())
        strm << setw(val_width) << (theArray[i+j]&0xff);
      else {
        PINDEX k;
        for (k = 0; k < val_width; k++)
          strm << ' ';
      }
      strm << ' ';
    }
    if ((strm.flags()&ios::floatfield) != ios::fixed) {
      strm << "  ";
      for (j = 0; j < line_width; j++) {
        if (i+j < GetSize()) {
          unsigned val = theArray[i+j]&0xff;
          if (isprint(val))
            strm << (char)val;
          else
            strm << '.';
        }
      }
    }
    i += line_width;
  }
}


void PBYTEArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  while (strm.good()) {
    unsigned v;
    strm >> v;
    theArray[size] = (BYTE)v;
    if (!strm.fail()) {
      size++;
      if (size >= GetSize())
        SetSize(size+100);
    }
  }

  SetSize(size);
}


///////////////////////////////////////////////////////////////////////////////

PBitArray::PBitArray(PINDEX initialSize)
  : PBYTEArray((initialSize+7)>>3)
{
}


PBitArray::PBitArray(const void * buffer,
                     PINDEX length,
                     PBoolean dynamic)
  : PBYTEArray((const BYTE *)buffer, (length+7)>>3, dynamic)
{
}


PObject * PBitArray::Clone() const
{
  return new PBitArray(*this);
}


PINDEX PBitArray::GetSize() const
{
  return PBYTEArray::GetSize()<<3;
}


PBoolean PBitArray::SetSize(PINDEX newSize)
{
  return PBYTEArray::SetSize((newSize+7)>>3);
}


PBoolean PBitArray::SetAt(PINDEX index, PBoolean val)
{
  if (!SetMinSize(index+1))
    return PFalse;

  if (val)
    theArray[index>>3] |= (1 << (index&7));
  else
    theArray[index>>3] &= ~(1 << (index&7));
  return PTrue;
}


PBoolean PBitArray::GetAt(PINDEX index) const
{
  PASSERTINDEX(index);
  if (index >= GetSize())
    return PFalse;

  return (theArray[index>>3]&(1 << (index&7))) != 0;
}


void PBitArray::Attach(const void * buffer, PINDEX bufferSize)
{
  PBYTEArray::Attach((const BYTE *)buffer, (bufferSize+7)>>3);
}


BYTE * PBitArray::GetPointer(PINDEX minSize)
{
  return PBYTEArray::GetPointer((minSize+7)>>3);
}


PBoolean PBitArray::Concatenate(const PBitArray & array)
{
  return PAbstractArray::Concatenate(array);
}


///////////////////////////////////////////////////////////////////////////////

PString::PString(const char * cstr)
  : PCharArray(cstr != NULL ? (int)strlen(cstr)+1 : 1)
{
  if (cstr != NULL)
    memcpy(theArray, cstr, GetSize());
}


PString::PString(const wchar_t * ustr)
{
  if (ustr == NULL)
    SetSize(1);
  else {
    PINDEX len = 0;
    while (ustr[len] != 0)
      len++;
    InternalFromUCS2(ustr, len);
  }
}


PString::PString(const char * cstr, PINDEX len)
  : PCharArray(len+1)
{
  if (len > 0)
    memcpy(theArray, PAssertNULL(cstr), len);
}


PString::PString(const wchar_t * ustr, PINDEX len)
  : PCharArray(len+1)
{
  InternalFromUCS2(ustr, len);
}


PString::PString(const PWCharArray & ustr)
{
  PINDEX size = ustr.GetSize();
  if (size > 0 && ustr[size-1] == 0) // Stip off trailing NULL if present
    size--;
  InternalFromUCS2(ustr, size);
}


static int TranslateHex(char x)
{
  if (x >= 'a')
    return x - 'a' + 10;

  if (x >= 'A')
    return x - 'A' + '\x0a';

  return x - '0';
}


static const unsigned char PStringEscapeCode[]  = {  'a',  'b',  'f',  'n',  'r',  't',  'v' };
static const unsigned char PStringEscapeValue[] = { '\a', '\b', '\f', '\n', '\r', '\t', '\v' };

static void TranslateEscapes(const char * src, char * dst)
{
  bool hadLeadingQuote = *src == '"';
  if (hadLeadingQuote)
    src++;

  while (*src != '\0') {
    int c = *src++ & 0xff;
    if (c == '"' && hadLeadingQuote) {
      *dst = '\0'; // Trailing '"' and remaining string is ignored
      break;
    }

    if (c == '\\') {
      c = *src++ & 0xff;
      for (PINDEX i = 0; i < PARRAYSIZE(PStringEscapeCode); i++) {
        if (c == PStringEscapeCode[i])
          c = PStringEscapeValue[i];
      }

      if (c == 'x' && isxdigit(*src & 0xff)) {
        c = TranslateHex(*src++);
        if (isxdigit(*src & 0xff))
          c = (c << 4) + TranslateHex(*src++);
      }
      else if (c >= '0' && c <= '7') {
        int count = c <= '3' ? 3 : 2;
        src--;
        c = 0;
        do {
          c = (c << 3) + *src++ - '0';
        } while (--count > 0 && *src >= '0' && *src <= '7');
      }
    }

    *dst++ = (char)c;
  }
}


PString::PString(ConversionType type, const char * str, ...)
{
  switch (type) {
    case Pascal :
      if (*str != '\0') {
        PINDEX len = *str & 0xff;
        PAssert(SetSize(len+1), POutOfMemory);
        memcpy(theArray, str+1, len);
      }
      break;

    case Basic :
      if (str[0] != '\0' && str[1] != '\0') {
        PINDEX len = (str[0] & 0xff) | ((str[1] & 0xff) << 8);
        PAssert(SetSize(len+1), POutOfMemory);
        memcpy(theArray, str+2, len);
      }
      break;

    case Literal :
      PAssert(SetSize(strlen(str)+1), POutOfMemory);
      TranslateEscapes(str, theArray);
      PAssert(MakeMinimumSize(), POutOfMemory);
      break;

    case Printf : {
      va_list args;
      va_start(args, str);
      vsprintf(str, args);
      va_end(args);
      break;
    }

    default :
      PAssertAlways(PInvalidParameter);
  }
}


template <class T> char * p_unsigned2string(T value, T base, char * str)
{
  if (value >= base)
    str = p_unsigned2string<T>(value/base, base, str);
  value %= base;
  if (value < 10)
    *str = (char)(value + '0');
  else
    *str = (char)(value + 'A'-10);
  return str+1;
}


template <class T> char * p_signed2string(T value, T base, char * str)
{
  if (value >= 0)
    return p_unsigned2string<T>(value, base, str);

  *str = '-';
  return p_unsigned2string<T>(-value, base, str+1);
}


PString::PString(short n)
  : PCharArray(sizeof(short)*3+1)
{
  p_signed2string<int>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(unsigned short n)
  : PCharArray(sizeof(unsigned short)*3+1)
{
  p_unsigned2string<unsigned int>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(int n)
  : PCharArray(sizeof(int)*3+1)
{
  p_signed2string<int>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(unsigned int n)
  : PCharArray(sizeof(unsigned int)*3+1)
{
  p_unsigned2string<unsigned int>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(long n)
  : PCharArray(sizeof(long)*3+1)
{
  p_signed2string<long>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(unsigned long n)
  : PCharArray(sizeof(unsigned long)*3+1)
{
  p_unsigned2string<unsigned long>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(PInt64 n)
  : PCharArray(sizeof(PInt64)*3+1)
{
  p_signed2string<PInt64>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(PUInt64 n)
  : PCharArray(sizeof(PUInt64)*3+1)
{
  p_unsigned2string<PUInt64>(n, 10, theArray);
  MakeMinimumSize();
}


PString::PString(ConversionType type, long value, unsigned base)
  : PCharArray(sizeof(long)*3+1)
{
  PAssert(base >= 2 && base <= 36, PInvalidParameter);
  switch (type) {
    case Signed :
      p_signed2string<long>(value, base, theArray);
      break;

    case Unsigned :
      p_unsigned2string<unsigned long>(value, base, theArray);
      break;

    default :
      PAssertAlways(PInvalidParameter);
  }
  MakeMinimumSize();
}


PString::PString(ConversionType type, double value, unsigned places)
{
  switch (type) {
    case Decimal :
      sprintf("%0.*f", (int)places, value);
      break;

    case Exponent :
      sprintf("%0.*e", (int)places, value);
      break;

    default :
      PAssertAlways(PInvalidParameter);
  }
}


PString & PString::operator=(short n)
{
  SetMinSize(sizeof(short)*3+1);
  p_signed2string<int>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(unsigned short n)
{
  SetMinSize(sizeof(unsigned short)*3+1);
  p_unsigned2string<unsigned int>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(int n)
{
  SetMinSize(sizeof(int)*3+1);
  p_signed2string<int>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(unsigned int n)
{
  SetMinSize(sizeof(unsigned int)*3+1);
  p_unsigned2string<unsigned int>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(long n)
{
  SetMinSize(sizeof(long)*3+1);
  p_signed2string<long>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(unsigned long n)
{
  SetMinSize(sizeof(unsigned long)*3+1);
  p_unsigned2string<unsigned long>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(PInt64 n)
{
  SetMinSize(sizeof(PInt64)*3+1);
  p_signed2string<PInt64>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::operator=(PUInt64 n)
{
  SetMinSize(sizeof(PUInt64)*3+1);
  p_unsigned2string<PUInt64>(n, 10, theArray);
  MakeMinimumSize();
  return *this;
}


PString & PString::MakeEmpty()
{
  SetSize(1);
  *theArray = '\0';
  return *this;
}


PObject * PString::Clone() const
{
  return new PString(*this);
}


void PString::PrintOn(ostream &strm) const
{
  strm << theArray;
}


void PString::ReadFrom(istream &strm)
{
  PINDEX bump = 16;
  PINDEX len = 0;
  do {
    if (!SetMinSize(len + (bump *= 2))) {
      strm.setstate(ios::badbit);
      return;
    }

    strm.clear();
    strm.getline(theArray + len, GetSize() - len);
    len += (PINDEX)strm.gcount();
  } while (strm.fail() && !strm.eof());

  if (len > 0 && !strm.eof())
    --len; // Allow for extracted '\n'

  if (len > 0 && theArray[len-1] == '\r')
    theArray[len-1] = '\0';

  PAssert(MakeMinimumSize(), POutOfMemory);
}


PObject::Comparison PString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PString), PInvalidCast);
  return InternalCompare(0, P_MAX_INDEX, ((const PString &)obj).theArray);
}


PINDEX PString::HashFunction() const
{
  // Hash function from "Data Structures and Algorithm Analysis in C++" by
  // Mark Allen Weiss, with limit of only executing over first 8 characters to
  // increase speed when dealing with large strings.

  PINDEX hash = 0;
  for (PINDEX i = 0; i < 8 && theArray[i] != 0; i++)
    hash = (hash << 5) ^ tolower(theArray[i] & 0xff) ^ hash;
  return PABSINDEX(hash)%127;
}


PBoolean PString::IsEmpty() const
{
  return (theArray == NULL) || (*theArray == '\0');
}


PBoolean PString::SetSize(PINDEX newSize)
{
  if (newSize < 1)
    newSize = 1;
  if (!InternalSetSize(newSize, true))
    return false;
  theArray[newSize-1] = '\0';
  return true;
}


PBoolean PString::MakeUnique()
{
  if (IsUnique())
    return PTrue;

  InternalSetSize(GetSize(), PTrue);
  return PFalse;
}


PString PString::operator+(const char * cstr) const
{
  if (cstr == NULL)
    return *this;

  PINDEX olen = GetLength();
  PINDEX alen = strlen(cstr)+1;
  PString str;
  str.SetSize(olen+alen);
  memmove(str.theArray, theArray, olen);
  memcpy(str.theArray+olen, cstr, alen);
  return str;
}


PString PString::operator+(char c) const
{
  PINDEX olen = GetLength();
  PString str;
  str.SetSize(olen+2);
  memmove(str.theArray, theArray, olen);
  str.theArray[olen] = c;
  return str;
}


PString & PString::operator+=(const char * cstr)
{
  if (cstr == NULL)
    return *this;

  PINDEX olen = GetLength();
  PINDEX alen = strlen(cstr)+1;
  SetSize(olen+alen);
  memcpy(theArray+olen, cstr, alen);
  return *this;
}


PString & PString::operator+=(char ch)
{
  PINDEX olen = GetLength();
  SetSize(olen+2);
  theArray[olen] = ch;
  return *this;
}


PString PString::operator&(const char * cstr) const
{
  if (cstr == NULL)
    return *this;

  PINDEX alen = strlen(cstr)+1;
  if (alen == 1)
    return *this;

  PINDEX olen = GetLength();
  PString str;
  PINDEX space = olen > 0 && theArray[olen-1]!=' ' && *cstr!=' ' ? 1 : 0;
  str.SetSize(olen+alen+space);
  memmove(str.theArray, theArray, olen);
  if (space != 0)
    str.theArray[olen] = ' ';
  memcpy(str.theArray+olen+space, cstr, alen);
  return str;
}


PString PString::operator&(char c) const
{
  PINDEX olen = GetLength();
  PString str;
  PINDEX space = olen > 0 && theArray[olen-1] != ' ' && c != ' ' ? 1 : 0;
  str.SetSize(olen+2+space);
  memmove(str.theArray, theArray, olen);
  if (space != 0)
    str.theArray[olen] = ' ';
  str.theArray[olen+space] = c;
  return str;
}


PString & PString::operator&=(const char * cstr)
{
  if (cstr == NULL)
    return *this;

  PINDEX alen = strlen(cstr)+1;
  if (alen == 1)
    return *this;
  PINDEX olen = GetLength();
  PINDEX space = olen > 0 && theArray[olen-1]!=' ' && *cstr!=' ' ? 1 : 0;
  SetSize(olen+alen+space);
  if (space != 0)
    theArray[olen] = ' ';
  memcpy(theArray+olen+space, cstr, alen);
  return *this;
}


PString & PString::operator&=(char ch)
{
  PINDEX olen = GetLength();
  PINDEX space = olen > 0 && theArray[olen-1] != ' ' && ch != ' ' ? 1 : 0;
  SetSize(olen+2+space);
  if (space != 0)
    theArray[olen] = ' ';
  theArray[olen+space] = ch;
  return *this;
}


void PString::Delete(PINDEX start, PINDEX len)
{
  if (start < 0 || len < 0)
    return;

  MakeUnique();

  register PINDEX slen = GetLength();
  if (start > slen)
    return;

  if (len > slen - start)
    SetAt(start, '\0');
  else
    memmove(theArray+start, theArray+start+len, slen-start-len+1);
  MakeMinimumSize();
}


PString PString::operator()(PINDEX start, PINDEX end) const
{
  if (end < 0 || start < 0 || end < start)
    return Empty();

  register PINDEX len = GetLength();
  if (start > len)
    return Empty();

  if (end >= len) {
    if (start == 0)
      return *this;
    end = len-1;
  }
  len = end - start + 1;

  return PString(theArray+start, len);
}


PString PString::Left(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  if (len >= GetLength())
    return *this;

  return PString(theArray, len);
}


PString PString::Right(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  PINDEX srclen = GetLength();
  if (len >= srclen)
    return *this;

  return PString(theArray+srclen-len, len);
}


PString PString::Mid(PINDEX start, PINDEX len) const
{
  if (len <= 0 || start < 0)
    return Empty();

  if (start+len < start) // Beware of wraparound
    return operator()(start, P_MAX_INDEX);
  else
    return operator()(start, start+len-1);
}


bool PString::operator*=(const char * cstr) const
{
  if (cstr == NULL)
    return IsEmpty() != PFalse;

  const char * pstr = theArray;
  while (*pstr != '\0' && *cstr != '\0') {
    if (toupper(*pstr & 0xff) != toupper(*cstr & 0xff))
      return PFalse;
    pstr++;
    cstr++;
  }
  return *pstr == *cstr;
}


PObject::Comparison PString::NumCompare(const PString & str, PINDEX count, PINDEX offset) const
{
  if (offset < 0 || count < 0)
    return LessThan;
  PINDEX len = str.GetLength();
  if (count > len)
    count = len;
  return InternalCompare(offset, count, str);
}


PObject::Comparison PString::NumCompare(const char * cstr, PINDEX count, PINDEX offset) const
{
  if (offset < 0 || count < 0)
    return LessThan;
  PINDEX len = ::strlen(cstr);
  if (count > len)
    count = len;
  return InternalCompare(offset, count, cstr);
}


PObject::Comparison PString::InternalCompare(PINDEX offset, char c) const
{
  if (offset < 0)
    return LessThan;
  const int ch = theArray[offset] & 0xff;
  if (ch < (c & 0xff))
    return LessThan;
  if (ch > (c & 0xff))
    return GreaterThan;
  return EqualTo;
}


PObject::Comparison PString::InternalCompare(
                         PINDEX offset, PINDEX length, const char * cstr) const
{
  if (offset < 0 || length < 0)
    return LessThan;

  if (offset == 0 && theArray == cstr)
    return EqualTo;

  if (offset < 0 || cstr == NULL)
    return IsEmpty() ? EqualTo : LessThan;

  int retval;
  if (length == P_MAX_INDEX)
    retval = strcmp(theArray+offset, cstr);
  else
    retval = strncmp(theArray+offset, cstr, length);

  if (retval < 0)
    return LessThan;

  if (retval > 0)
    return GreaterThan;

  return EqualTo;
}


PINDEX PString::Find(char ch, PINDEX offset) const
{
  if (offset < 0)
    return P_MAX_INDEX;

  register PINDEX len = GetLength();
  while (offset < len) {
    if (InternalCompare(offset, ch) == EqualTo)
      return offset;
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::Find(const char * cstr, PINDEX offset) const
{
  if (cstr == NULL || *cstr == '\0' || offset < 0)
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    return P_MAX_INDEX;

  if (len - clen < 10) {
    while (offset+clen <= len) {
      if (InternalCompare(offset, clen, cstr) == EqualTo)
        return offset;
      offset++;
    }
    return P_MAX_INDEX;
  }

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(theArray[offset+i] & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (offset+clen <= len) {
    if (strSum == cstrSum && InternalCompare(offset, clen, cstr) == EqualTo)
      return offset;
    strSum += toupper(theArray[offset+clen] & 0xff);
    strSum -= toupper(theArray[offset] & 0xff);
    offset++;
  }

  return P_MAX_INDEX;
}


PINDEX PString::FindLast(char ch, PINDEX offset) const
{
  PINDEX len = GetLength();
  if (len == 0 || offset < 0)
    return P_MAX_INDEX;
  if (offset >= len)
    offset = len-1;

  while (InternalCompare(offset, ch) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    offset--;
  }

  return offset;
}


PINDEX PString::FindLast(const char * cstr, PINDEX offset) const
{
  if (cstr == NULL || *cstr == '\0' || offset < 0)
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    offset = len - clen;

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(theArray[offset+i] & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (strSum != cstrSum || InternalCompare(offset, clen, cstr) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    --offset;
    strSum += toupper(theArray[offset] & 0xff);
    strSum -= toupper(theArray[offset+clen] & 0xff);
  }

  return offset;
}


PINDEX PString::FindOneOf(const char * cset, PINDEX offset) const
{
  if (cset == NULL || *cset == '\0' || offset < 0)
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (*p != '\0') {
      if (InternalCompare(offset, *p) == EqualTo)
        return offset;
      p++;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindSpan(const char * cset, PINDEX offset) const
{
  if (cset == NULL || *cset == '\0' || offset < 0)
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (InternalCompare(offset, *p) != EqualTo) {
      if (*++p == '\0')
        return offset;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindRegEx(const PRegularExpression & regex, PINDEX offset) const
{
  if (offset < 0)
    return P_MAX_INDEX;

  PINDEX pos = 0;
  PINDEX len = 0;
  if (FindRegEx(regex, pos, len, offset))
    return pos;

  return P_MAX_INDEX;
}


PBoolean PString::FindRegEx(const PRegularExpression & regex,
                        PINDEX & pos,
                        PINDEX & len,
                        PINDEX offset,
                        PINDEX maxPos) const
{
  if (offset < 0 || maxPos < 0 || offset > GetLength())
    return false;

  if (offset == GetLength()) {
    if (!regex.Execute("", pos, len, 0))
      return false;
  }
  else {
    if (!regex.Execute(&theArray[offset], pos, len, 0))
      return PFalse;
  }

  pos += offset;
  if (pos+len > maxPos)
    return PFalse;

  return PTrue;
}

PBoolean PString::MatchesRegEx(const PRegularExpression & regex) const
{
  PINDEX pos = 0;
  PINDEX len = 0;

  if (!regex.Execute(theArray, pos, len, 0))
    return PFalse;

  return (pos == 0) && (len == GetLength());
}

void PString::Replace(const PString & target,
                      const PString & subs,
                      PBoolean all, PINDEX offset)
{
  if (offset < 0)
    return;
    
  MakeUnique();

  PINDEX tlen = target.GetLength();
  PINDEX slen = subs.GetLength();
  do {
    PINDEX pos = Find(target, offset);
    if (pos == P_MAX_INDEX)
      return;
    Splice(subs, pos, tlen);
    offset = pos + slen;
  } while (all);
}


void PString::Splice(const char * cstr, PINDEX pos, PINDEX len)
{
  if (len < 0 || pos < 0)
    return;

  register PINDEX slen = GetLength();
  if (pos >= slen)
    operator+=(cstr);
  else {
    MakeUnique();
    if (len > slen-pos)
      len = slen-pos;
    PINDEX clen = cstr != NULL ? strlen(cstr) : 0;
    PINDEX newlen = slen-len+clen;
    if (clen > len)
      SetSize(newlen+1);
    if (pos+len < slen)
      memmove(theArray+pos+clen, theArray+pos+len, slen-pos-len+1);
    if (clen > 0)
      memcpy(theArray+pos, cstr, clen);
    theArray[newlen] = '\0';
  }
}


PStringArray
        PString::Tokenise(const char * separators, PBoolean onePerSeparator) const
{
  PStringArray tokens;
  
  if (separators == NULL || IsEmpty())  // No tokens
    return tokens;
    
  PINDEX token = 0;
  PINDEX p1 = 0;
  PINDEX p2 = FindOneOf(separators);

  if (p2 == 0) {
    if (onePerSeparator) { // first character is a token separator
      tokens[token] = Empty();
      token++;                        // make first string in array empty
      p1 = 1;
      p2 = FindOneOf(separators, 1);
    }
    else {
      do {
        p1 = p2 + 1;
      } while ((p2 = FindOneOf(separators, p1)) == p1);
    }
  }

  while (p2 != P_MAX_INDEX) {
    if (p2 > p1)
      tokens[token] = operator()(p1, p2-1);
    else
      tokens[token] = Empty();
    token++;

    // Get next separator. If not one token per separator then continue
    // around loop to skip over all the consecutive separators.
    do {
      p1 = p2 + 1;
    } while ((p2 = FindOneOf(separators, p1)) == p1 && !onePerSeparator);
  }

  tokens[token] = operator()(p1, P_MAX_INDEX);

  return tokens;
}


PStringArray PString::Lines() const
{
  PStringArray lines;
  
  if (IsEmpty())
    return lines;
    
  PINDEX line = 0;
  PINDEX p1 = 0;
  PINDEX p2;
  while ((p2 = FindOneOf("\r\n", p1)) != P_MAX_INDEX) {
    lines[line++] = operator()(p1, p2-1);
    p1 = p2 + 1;
    if (theArray[p2] == '\r' && theArray[p1] == '\n') // CR LF pair
      p1++;
  }
  if (p1 < GetLength())
    lines[line] = operator()(p1, P_MAX_INDEX);
  return lines;
}

PStringArray & PStringArray::operator += (const PStringArray & v)
{
  PINDEX i;
  for (i = 0; i < v.GetSize(); i++)
    AppendString(v[i]);

  return *this;
}

PString PString::LeftTrim() const
{
  const char * lpos = theArray;
  while (isspace(*lpos & 0xff))
    lpos++;
  return PString(lpos);
}


PString PString::RightTrim() const
{
  char * rpos = theArray+GetLength()-1;
  if (!isspace(*rpos & 0xff))
    return *this;

  while (isspace(*rpos & 0xff)) {
    if (rpos == theArray)
      return Empty();
    rpos--;
  }

  // make Apple & Tornado gnu compiler happy
  PString retval(theArray, rpos - theArray + 1);
  return retval;
}


PString PString::Trim() const
{
  const char * lpos = theArray;
  while (isspace(*lpos & 0xff))
    lpos++;
  if (*lpos == '\0')
    return Empty();

  const char * rpos = theArray+GetLength()-1;
	if (!isspace(*rpos & 0xff)) {
		if (lpos == theArray)
			return *this;
		else
			return PString(lpos);
	}

  while (isspace(*rpos & 0xff))
    rpos--;
  return PString(lpos, rpos - lpos + 1);
}


PString PString::ToLower() const
{
  PString newStr(theArray);
  for (char *cpos = newStr.theArray; *cpos != '\0'; cpos++) {
    if (isupper(*cpos & 0xff))
      *cpos = (char)tolower(*cpos & 0xff);
  }
  return newStr;
}


PString PString::ToUpper() const
{
  PString newStr(theArray);
  for (char *cpos = newStr.theArray; *cpos != '\0'; cpos++) {
    if (islower(*cpos & 0xff))
      *cpos = (char)toupper(*cpos & 0xff);
  }
  return newStr;
}


long PString::AsInteger(unsigned base) const
{
  PAssert(base >= 2 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtol(theArray, &dummy, base);
}


DWORD PString::AsUnsigned(unsigned base) const
{
  PAssert(base >= 2 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoul(theArray, &dummy, base);
}


double PString::AsReal() const
{
#ifndef __HAS_NO_FLOAT
  char * dummy;
  return strtod(theArray, &dummy);
#else
  return 0.0;
#endif
}


PWCharArray PString::AsUCS2() const
{
  PWCharArray ucs2(1); // Null terminated empty string

  if (IsEmpty())
    return ucs2;

#ifdef P_HAS_G_CONVERT

  gsize g_len = 0;
  gchar * g_ucs2 = g_convert(theArray, GetSize()-1, "UCS-2", "UTF-8", 0, &g_len, 0);
  if (g_ucs2 != NULL) {
    if (ucs2.SetSize(g_len+1))
      memcpy(ucs2.GetPointer(), g_ucs2, g_len*2);
    g_free(g_ucs2);
    return ucs2;
  }

  PTRACE(1, "PTLib\tg_convert failed with error " << errno);

#elif defined(_WIN32)

  // Note that MB_ERR_INVALID_CHARS is the only dwFlags value supported by Code page 65001 (UTF-8). Windows XP and later.
  PINDEX length = GetLength();
  PINDEX count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, theArray, length, NULL, 0);
  if (count > 0 && ucs2.SetSize(count+1)) { // Allow for trailing NULL
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, theArray, length, ucs2.GetPointer(), ucs2.GetSize());
    return ucs2;
  }

#if PTRACING
  if (GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
    PTRACE(1, "PTLib\tMultiByteToWideChar failed on non legal UTF-8 \"" << theArray << '"');
  else
    PTRACE(1, "PTLib\tMultiByteToWideChar failed with error " << ::GetLastError());
#endif

#endif

  if (ucs2.SetSize(GetSize())) { // Will be at least this big
    PINDEX count = 0;
    PINDEX i = 0;
    PINDEX length = GetSize(); // Include the trailing '\0'
    while (i < length) {
      int c = theArray[i];
      if ((c&0x80) == 0)
        ucs2[count++] = (BYTE)theArray[i++];
      else if ((c&0xe0) == 0xc0) {
        if (i < length-1)
          ucs2[count++] = (WORD)(((theArray[i  ]&0x1f)<<6)|
                                  (theArray[i+1]&0x3f));
        i += 2;
      }
      else if ((c&0xf0) == 0xe0) {
        if (i < length-2)
          ucs2[count++] = (WORD)(((theArray[i  ]&0x0f)<<12)|
                                 ((theArray[i+1]&0x3f)<< 6)|
                                  (theArray[i+2]&0x3f));
        i += 3;
      }
      else {
        if ((c&0xf8) == 0xf0)
          i += 4;
        else if ((c&0xfc) == 0xf8)
          i += 5;
        else
          i += 6;
        if (i <= length)
          ucs2[count++] = 0xffff;
      }
    }

    ucs2.SetSize(count);  // Final size
  }

  return ucs2;
}


void PString::InternalFromUCS2(const wchar_t * ptr, PINDEX len)
{
  if (ptr == NULL || len <= 0) {
    *this = Empty();
    return;
  }

#ifdef P_HAS_G_CONVERT

  gsize g_len = 0;
  gchar * g_utf8 = g_convert(ptr, len, "UTF-8", "UCS-2", 0, &g_len, 0);
  if (g_utf8 == NULL) {
    *this = Empty();
    return;
  }

  if (SetSize(&g_len))
    memcpy(theArray, g_char, g_len);
  g_free(g_utf8);

#elif defined(_WIN32)

  PINDEX count = WideCharToMultiByte(CP_UTF8, 0, ptr, len, NULL, 0, NULL, NULL);
  if (SetSize(count+1))
    WideCharToMultiByte(CP_UTF8, 0, ptr, len, GetPointer(), GetSize(), NULL, NULL);

#else

  PINDEX i;
  PINDEX count = 1;
  for (i = 0; i < len; i++) {
    if (ptr[i] < 0x80)
      count++;
    else if (ptr[i] < 0x800)
      count += 2;
    else
      count += 3;
  }
  if (SetSize(count)) {
    count = 0;
    for (i = 0; i < len; i++) {
      unsigned v = *ptr++;
      if (v < 0x80)
        theArray[count++] = (char)v;
      else if (v < 0x800) {
        theArray[count++] = (char)(0xc0+(v>>6));
        theArray[count++] = (char)(0x80+(v&0x3f));
      }
      else {
        theArray[count++] = (char)(0xd0+(v>>12));
        theArray[count++] = (char)(0x80+((v>>6)&0x3f));
        theArray[count++] = (char)(0x80+(v&0x3f));
      }
    }
  }

#endif
}


PBYTEArray PString::ToPascal() const
{
  PINDEX len = GetLength();
  PAssert(len < 256, "Cannot convert to PASCAL string");
  BYTE buf[256];
  buf[0] = (BYTE)len;
  memcpy(&buf[1], theArray, len);
  return PBYTEArray(buf, len+1);
}


PString PString::ToLiteral() const
{
  PString str('"');
  for (char * p = theArray; *p != '\0'; p++) {
    if (*p == '"')
      str += "\\\"";
    else if (*p == '\\')
      str += "\\\\";
    else if (isprint(*p & 0xff))
      str += *p;
    else {
      PINDEX i;
      for (i = 0; i < PARRAYSIZE(PStringEscapeValue); i++) {
        if (*p == PStringEscapeValue[i]) {
          str += PString('\\') + (char)PStringEscapeCode[i];
          break;
        }
      }
      if (i >= PARRAYSIZE(PStringEscapeValue))
        str.sprintf("\\%03o", *p & 0xff);
    }
  }
  return str + '"';
}


PString & PString::sprintf(const char * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  return vsprintf(fmt, args);
}

#if defined(__GNUC__) || defined(__SUNPRO_CC)
#define _vsnprintf vsnprintf
#endif

PString & PString::vsprintf(const char * fmt, va_list arg)
{
  PINDEX len = theArray != NULL ? GetLength() : 0;
#ifdef P_VXWORKS
  // The library provided with tornado 2.0 does not have the implementation
  // for vsnprintf
  // as workaround, just use a array size of 2000
  PAssert(SetSize(2000), POutOfMemory);
  ::vsprintf(theArray+len, fmt, arg);
#else
  PINDEX providedSpace = 0;
  int requiredSpace;
  do {
    providedSpace += 1000;
    PAssert(SetSize(providedSpace+len), POutOfMemory);
    requiredSpace = _vsnprintf(theArray+len, providedSpace, fmt, arg);
  } while (requiredSpace == -1 || requiredSpace >= providedSpace);
#endif // P_VXWORKS

  PAssert(MakeMinimumSize(), POutOfMemory);
  return *this;
}


PString psprintf(const char * fmt, ...)
{
  PString str;
  va_list args;
  va_start(args, fmt);
  return str.vsprintf(fmt, args);
}


PString pvsprintf(const char * fmt, va_list arg)
{
  PString str;
  return str.vsprintf(fmt, arg);
}


///////////////////////////////////////////////////////////////////////////////

PObject * PCaselessString::Clone() const
{
  return new PCaselessString(*this);
}


PObject::Comparison PCaselessString::InternalCompare(PINDEX offset, char c) const
{
  if (offset < 0)
    return LessThan;

  int c1 = toupper(theArray[offset] & 0xff);
  int c2 = toupper(c & 0xff);
  if (c1 < c2)
    return LessThan;
  if (c1 > c2)
    return GreaterThan;
  return EqualTo;
}


PObject::Comparison PCaselessString::InternalCompare(
                         PINDEX offset, PINDEX length, const char * cstr) const
{
  if (offset < 0 || length < 0)
    return LessThan;

  if (cstr == NULL)
    return IsEmpty() ? EqualTo : LessThan;

  while (length-- > 0 && (theArray[offset] != '\0' || *cstr != '\0')) {
    Comparison c = PCaselessString::InternalCompare(offset++, *cstr++);
    if (c != EqualTo)
      return c;
  }
  return EqualTo;
}



///////////////////////////////////////////////////////////////////////////////

PStringStream::Buffer::Buffer(PStringStream & str, PINDEX size)
  : string(str),
    fixedBufferSize(size != 0)
{
  string.SetMinSize(size > 0 ? size : 256);
  sync();
}


streambuf::int_type PStringStream::Buffer::overflow(int_type c)
{
  if (pptr() >= epptr()) {
    if (fixedBufferSize)
      return EOF;

    int gpos = gptr() - eback();
    int ppos = pptr() - pbase();
    char * newptr = string.GetPointer(string.GetSize() + 32);
    setp(newptr, newptr + string.GetSize() - 1);
    pbump(ppos);
    setg(newptr, newptr + gpos, newptr + ppos);
  }

  if (c != EOF) {
    *pptr() = (char)c;
    pbump(1);
  }

  return 0;
}


streambuf::int_type PStringStream::Buffer::underflow()
{
  return gptr() >= egptr() ? EOF : *gptr();
}


int PStringStream::Buffer::sync()
{
  char * base = string.GetPointer();
  PINDEX len = string.GetLength();
  setg(base, base, base + len);
  setp(base, base + string.GetSize() - 1);
  pbump(len);
  return 0;
}

streambuf::pos_type PStringStream::Buffer::seekoff(off_type off, ios_base::seekdir dir, ios_base::openmode mode)
{
  int len = string.GetLength();
  int gpos = gptr() - eback();
  int ppos = pptr() - pbase();
  char * newgptr;
  char * newpptr;
  switch (dir) {
    case ios::beg :
      if (off < 0)
        newpptr = newgptr = eback();
      else if (off >= len)
        newpptr = newgptr = egptr();
      else
        newpptr = newgptr = eback()+off;
      break;

    case ios::cur :
      if (off < -ppos)
        newpptr = eback();
      else if (off >= len-ppos)
        newpptr = epptr();
      else
        newpptr = pptr()+off;
      if (off < -gpos)
        newgptr = eback();
      else if (off >= len-gpos)
        newgptr = egptr();
      else
        newgptr = gptr()+off;
      break;

    case ios::end :
      if (off < -len)
        newpptr = newgptr = eback();
      else if (off >= 0)
        newpptr = newgptr = egptr();
      else
        newpptr = newgptr = egptr()+off;
      break;

    default:
      PAssertAlways2(string.GetClass(), PInvalidParameter);
      newgptr = gptr();
      newpptr = pptr();
  }

  if ((mode&ios::in) != 0)
    setg(eback(), newgptr, egptr());

  if ((mode&ios::out) != 0)
    setp(newpptr, epptr());

  return gptr() - eback();
}


PStringStream::Buffer::pos_type PStringStream::Buffer::seekpos(pos_type pos, ios_base::openmode mode)
{
  return seekoff(pos, ios_base::beg, mode);
}


#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

PStringStream::PStringStream()
  : iostream(new PStringStream::Buffer(*this, 0))
{
}


PStringStream::PStringStream(PINDEX fixedBufferSize)
  : iostream(new PStringStream::Buffer(*this, fixedBufferSize))
{
}


PStringStream::PStringStream(const PString & str)
  : PString(str),
    iostream(new PStringStream::Buffer(*this, 0))
{
}


PStringStream::PStringStream(const char * cstr)
  : PString(cstr),
    iostream(new PStringStream::Buffer(*this, 0))
{
}

#ifdef _MSC_VER
#pragma warning(default:4355)
#endif


PStringStream::~PStringStream()
{
  delete (PStringStream::Buffer *)rdbuf();
#ifndef _WIN32
  init(NULL);
#endif
}


PString & PStringStream::MakeEmpty()
{
  memset(theArray, 0, GetSize());
  flush();
  return *this;
}


void PStringStream::AssignContents(const PContainer & cont)
{
  PString::AssignContents(cont);
  flush();
}


///////////////////////////////////////////////////////////////////////////////

PStringArray::PStringArray(PINDEX count, char const * const * strarr, PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  if (count == P_MAX_INDEX) {
    count = 0;
    while (strarr[count] != NULL)
      count++;
  }

  SetSize(count);
  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    SetAt(i, newString);
  }
}


PStringArray::PStringArray(const PString & str)
{
  SetSize(1);
  (*theArray)[0] = new PString(str);
}


PStringArray::PStringArray(const PStringList & list)
{
  SetSize(list.GetSize());
  PINDEX count = 0;
  for (PStringList::const_iterator i = list.begin(); i != list.end(); i++)
    (*theArray)[count++] = new PString(*i);
}


PStringArray::PStringArray(const PSortedStringList & list)
{
  SetSize(list.GetSize());
  for (PINDEX i = 0; i < list.GetSize(); i++)
    (*theArray)[i] = new PString(list[i]);
}


void PStringArray::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PString PStringArray::operator[](PINDEX index) const
{
  PASSERTINDEX(index);
  if (index < GetSize() && (*theArray)[index] != NULL)
    return *(PString *)(*theArray)[index];
  return PString::Empty();
}


PString & PStringArray::operator[](PINDEX index)
{
  PASSERTINDEX(index);
  PAssert(SetMinSize(index+1), POutOfMemory);
  if ((*theArray)[index] == NULL)
    (*theArray)[index] = new PString;
  return *(PString *)(*theArray)[index];
}


static void strcpy_with_increment(char * & strPtr, const PString & str)
{
  PINDEX len = str.GetLength()+1;
  memcpy(strPtr, (const char *)str, len);
  strPtr += len;
}

char ** PStringArray::ToCharArray(PCharArray * storage) const
{
  PINDEX i;

  PINDEX mySize = GetSize();
  PINDEX storageSize = (mySize+1)*sizeof(char *);
  for (i = 0; i < mySize; i++)
    storageSize += (*this)[i].GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[mySize+1];

  for (i = 0; i < mySize; i++) {
    storagePtr[i] = strPtr;
    strcpy_with_increment(strPtr, (*this)[i]);
  }

  storagePtr[i] = NULL;

  return storagePtr;
}


///////////////////////////////////////////////////////////////////////////////

PStringList::PStringList(PINDEX count, char const * const * strarr, PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    Append(newString);
  }
}


PStringList::PStringList(const PString & str)
{
  AppendString(str);
}


PStringList::PStringList(const PStringArray & array)
{
  for (PINDEX i = 0; i < array.GetSize(); i++)
    AppendString(array[i]);
}


PStringList::PStringList(const PSortedStringList & list)
{
  for (PINDEX i = 0; i < list.GetSize(); i++)
    AppendString(list[i]);
}

PStringList & PStringList::operator += (const PStringList & v)
{
  for (PStringList::const_iterator i = v.begin(); i != v.end(); i++)
    AppendString(*i);

  return *this;
}


void PStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


///////////////////////////////////////////////////////////////////////////////

PSortedStringList::PSortedStringList(PINDEX count,
                                     char const * const * strarr,
                                     PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    Append(newString);
  }
}


PSortedStringList::PSortedStringList(const PString & str)
{
  AppendString(str);
}


PSortedStringList::PSortedStringList(const PStringArray & array)
{
  for (PINDEX i = 0; i < array.GetSize(); i++)
    AppendString(array[i]);
}


PSortedStringList::PSortedStringList(const PStringList & list)
{
  for (PStringList::const_iterator i = list.begin(); i != list.end(); i++)
    AppendString(*i);
}



void PSortedStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PINDEX PSortedStringList::GetNextStringsIndex(const PString & str) const
{
  PINDEX len = str.GetLength();
  Element * lastElement;
  PINDEX lastIndex = InternalStringSelect(str, len, info->root, lastElement);

  if (lastIndex != 0) {
    Element * prev;
    while ((prev = info->Predecessor(lastElement)) != &info->nil &&
                    ((PString *)prev->data)->NumCompare(str, len) >= EqualTo) {
      lastElement = prev;
      lastIndex--;
    }
  }

  return lastIndex;
}


PINDEX PSortedStringList::InternalStringSelect(const char * str,
                                               PINDEX len,
                                               Element * thisElement,
                                               Element * & lastElement) const
{
  if (thisElement == &info->nil)
    return 0;

  switch (((PString *)thisElement->data)->NumCompare(str, len)) {
    case PObject::LessThan :
    {
      PINDEX index = InternalStringSelect(str, len, thisElement->right, lastElement);
      return thisElement->left->subTreeSize + index + 1;
    }

    case PObject::GreaterThan :
      return InternalStringSelect(str, len, thisElement->left, lastElement);

    default :
      lastElement = thisElement;
      return thisElement->left->subTreeSize;
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringSet::PStringSet(PINDEX count, char const * const * strarr, PBoolean caseless)
  : BaseClass(true)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    if (caseless)
      Include(PCaselessString(strarr[i]));
    else
      Include(PString(strarr[i]));
  }
}


PStringSet::PStringSet(const PString & str)
  : BaseClass(true)
{
  Include(str);
}


PStringSet::PStringSet(const PStringArray & strArray)
  : BaseClass(true)
{
  for (PINDEX i = 0; i < strArray.GetSize(); ++i)
    Include(strArray[i]);
}


PStringSet::PStringSet(const PStringList & strList)
  : BaseClass(true)
{
  for (PStringList::const_iterator it = strList.begin(); it != strList.end(); ++it)
    Include(*it);
}


void PStringSet::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    Include(str);
  }
}


///////////////////////////////////////////////////////////////////////////////

POrdinalToString::POrdinalToString(PINDEX count, const Initialiser * init)
{
  while (count-- > 0) {
    SetAt(init->key, init->value);
    init++;
  }
}


void POrdinalToString::ReadFrom(istream & strm)
{
  while (strm.good()) {
    POrdinalKey key;
    char equal;
    PString str;
    strm >> key >> ws >> equal >> str;
    if (equal != '=')
      SetAt(key, PString::Empty());
    else
      SetAt(key, str.Mid(equal+1));
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringToOrdinal::PStringToOrdinal(PINDEX count,
                                   const Initialiser * init,
                                   PBoolean caseless)
{
  while (count-- > 0) {
    if (caseless)
      SetAt(PCaselessString(init->key), init->value);
    else
      SetAt(init->key, init->value);
    init++;
  }
}


void PStringToOrdinal::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    PINDEX equal = str.FindLast('=');
    if (equal == P_MAX_INDEX)
      SetAt(str, 0);
    else
      SetAt(str.Left(equal), str.Mid(equal+1).AsInteger());
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringToString::PStringToString(PINDEX count,
                                 const Initialiser * init,
                                 PBoolean caselessKeys,
                                 PBoolean caselessValues)
{
  while (count-- > 0) {
    if (caselessValues)
      if (caselessKeys)
        SetAt(PCaselessString(init->key), PCaselessString(init->value));
      else
        SetAt(init->key, PCaselessString(init->value));
    else
      if (caselessKeys)
        SetAt(PCaselessString(init->key), init->value);
      else
        SetAt(init->key, init->value);
    init++;
  }
}


void PStringToString::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    PINDEX equal = str.Find('=');
    if (equal == P_MAX_INDEX)
      SetAt(str, PString::Empty());
    else
      SetAt(str.Left(equal), str.Mid(equal+1));
  }
}


char ** PStringToString::ToCharArray(bool withEqualSign, PCharArray * storage) const
{
  PINDEX i;

  PINDEX mySize = GetSize();
  PINDEX numPointers = mySize*(withEqualSign ? 1 : 2) + 1;
  PINDEX storageSize = numPointers*sizeof(char *);
  for (i = 0; i < mySize; i++)
    storageSize += GetKeyAt(i).GetLength()+1 + GetDataAt(i).GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[numPointers];
  PINDEX strIndex = 0;

  for (i = 0; i < mySize; i++) {
    storagePtr[strIndex++] = strPtr;
    if (withEqualSign)
      strcpy_with_increment(strPtr, GetKeyAt(i) + '=' + GetDataAt(i));
    else {
      strcpy_with_increment(strPtr, GetKeyAt(i));
      storagePtr[strIndex++] = strPtr;
      strcpy_with_increment(strPtr, GetDataAt(i));
    }
  }

  storagePtr[strIndex] = NULL;

  return storagePtr;
}


///////////////////////////////////////////////////////////////////////////////


PString PStringOptions::GetString(const PCaselessString & key, const char * dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? *str : PString(dflt);
}


bool PStringOptions::GetBoolean(const PCaselessString & key, bool dflt) const
{
  PString * str = PStringToString::GetAt(key);
  if (str == NULL)
    return dflt;

  if (str->IsEmpty() || str->AsUnsigned() != 0)
    return true;

  PCaselessString test(*str);
  return test.NumCompare("t") == EqualTo || test.NumCompare("y") == EqualTo;
}


long PStringOptions::GetInteger(const PCaselessString & key, long dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? str->AsInteger() : dflt;
}


void PStringOptions::SetInteger(const PCaselessString & key, long value)
{
  PStringToString::SetAt(key, PString(PString::Signed, value));
}


double PStringOptions::GetReal(const PCaselessString & key, double dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? str->AsReal() : dflt;
}


void PStringOptions::SetReal(const PCaselessString & key, double value, int decimals)
{
  PStringToString::SetAt(key, PString(decimals < 0 ? PString::Exponent : PString::Decimal, value, decimals));
}


///////////////////////////////////////////////////////////////////////////////

PRegularExpression::PRegularExpression()
{
  lastError   = NotCompiled;
  expression  = NULL;
  flagsSaved  = IgnoreCase;
}


PRegularExpression::PRegularExpression(const PString & pattern, int flags)
{
  expression = NULL;
  bool b = Compile(pattern, flags);
  PAssert(b, PString("regular expression compile failed : " + GetErrorText()));
}


PRegularExpression::PRegularExpression(const char * pattern, int flags)
{
  expression = NULL;
  bool b = Compile(pattern, flags);
  PAssert(b, PString("regular expression compile failed : " + GetErrorText()));
}

PRegularExpression::PRegularExpression(const PRegularExpression & from)
{
  expression   = NULL;
  bool b = Compile(from.patternSaved, from.flagsSaved);
  PAssert(b, PString("regular expression compile failed : " + GetErrorText()));
}

PRegularExpression & PRegularExpression::operator =(const PRegularExpression & from)
{
  if (expression != from.expression) {
    if (expression != NULL && expression != from.expression) {
      regfree(regexpression());
      delete regexpression();
    }
    expression = NULL;
  }
  Compile(from.patternSaved, from.flagsSaved);
  return *this;
}

PRegularExpression::~PRegularExpression()
{
  if (expression != NULL) {
    regfree(regexpression());
    delete regexpression();
  }
}


void PRegularExpression::PrintOn(ostream &strm) const
{
  strm << patternSaved;
}


PRegularExpression::ErrorCodes PRegularExpression::GetErrorCode() const
{
  return lastError;
}


PString PRegularExpression::GetErrorText() const
{
  PString str;
  regerror(lastError, regexpression(), str.GetPointer(256), 256);
  return str;
}


PBoolean PRegularExpression::Compile(const PString & pattern, int flags)
{
  return Compile((const char *)pattern, flags);
}


PBoolean PRegularExpression::Compile(const char * pattern, int flags)
{
  patternSaved = pattern;
  flagsSaved   = flags;

  if (expression != NULL) {
    regfree(regexpression());
    delete regexpression();
    expression = NULL;
  }
  if (pattern == NULL || *pattern == '\0')
    lastError = BadPattern;
  else {
    expression = new regex_t;
    lastError = (ErrorCodes)regcomp(regexpression(), pattern, flags);
  }
  return lastError == NoError;
}


PBoolean PRegularExpression::Execute(const PString & str, PINDEX & start, int flags) const
{
  PINDEX dummy;
  return Execute((const char *)str, start, dummy, flags);
}


PBoolean PRegularExpression::Execute(const PString & str, PINDEX & start, PINDEX & len, int flags) const
{
  return Execute((const char *)str, start, len, flags);
}


PBoolean PRegularExpression::Execute(const char * cstr, PINDEX & start, int flags) const
{
  PINDEX dummy;
  return Execute(cstr, start, dummy, flags);
}


PBoolean PRegularExpression::Execute(const char * cstr, PINDEX & start, PINDEX & len, int flags) const
{
  if (expression == NULL) {
    lastError = NotCompiled;
    return PFalse;
  }

  if (lastError != NoError && lastError != NoMatch)
    return PFalse;

  regmatch_t match;

  lastError = (ErrorCodes)regexec(regexpression(), cstr, 1, &match, flags);
  if (lastError != NoError)
    return PFalse;

  start = match.rm_so;
  len = match.rm_eo - start;
  return PTrue;
}


PBoolean PRegularExpression::Execute(const PString & str, PIntArray & starts, int flags) const
{
  PIntArray dummy;
  return Execute((const char *)str, starts, dummy, flags);
}


PBoolean PRegularExpression::Execute(const PString & str,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 int flags) const
{
  return Execute((const char *)str, starts, ends, flags);
}


PBoolean PRegularExpression::Execute(const char * cstr, PIntArray & starts, int flags) const
{
  PIntArray dummy;
  return Execute(cstr, starts, dummy, flags);
}


PBoolean PRegularExpression::Execute(const char * cstr,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 int flags) const
{
  if (expression == NULL) {
    lastError = NotCompiled;
    return PFalse;
  }

  PINDEX count = starts.GetSize();
  if (count == 0) {
    starts.SetSize(1);
    count = 1;
  }
  ends.SetSize(count);

  regmatch_t * matches = new regmatch_t[count];

  lastError = (ErrorCodes)::regexec(regexpression(), cstr, count, matches, flags);
  if (lastError == NoError) {
    for (PINDEX i = 0; i < count; i++) {
      starts[i] = matches[i].rm_so;
      ends[i] = matches[i].rm_eo;
    }
  }

  delete [] matches;

  return lastError == NoError;
}


PBoolean PRegularExpression::Execute(const char * cstr, PStringArray & substring, int flags) const
{
  if (expression == NULL) {
    lastError = NotCompiled;
    return PFalse;
  }

  PINDEX count = substring.GetSize();
  if (count == 0) {
    substring.SetSize(1);
    count = 1;
  }

  regmatch_t * matches = new regmatch_t[count];

  lastError = (ErrorCodes)::regexec(regexpression(), cstr, count, matches, flags);
  if (lastError == NoError) {
    for (PINDEX i = 0; i < count; i++)
      substring[i] = PString(cstr+matches[i].rm_so, matches[i].rm_eo-matches[i].rm_so);
  }

  delete [] matches;

  return lastError == NoError;
}


PString PRegularExpression::EscapeString(const PString & str)
{
  PString translated = str;

  PINDEX lastPos = 0;
  PINDEX nextPos;
  while ((nextPos = translated.FindOneOf("\\^$+?*.[]()|{}", lastPos)) != P_MAX_INDEX) {
    translated.Splice("\\", nextPos);
    lastPos = nextPos+2;
  }

  return translated;
}


// End Of File ///////////////////////////////////////////////////////////////
