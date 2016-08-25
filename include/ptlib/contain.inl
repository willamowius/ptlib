/*
 * contain.inl
 *
 * Container Class Inline Function Definitions
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
 * $Id$
 */


///////////////////////////////////////////////////////////////////////////////

PINLINE PContainer & PContainer::operator=(const PContainer & cont)
  { AssignContents(cont); return *this; }

PINLINE void PContainer::CloneContents(const PContainer *)
  { }

PINLINE void PContainer::CopyContents(const PContainer &)
  { }

PINLINE PINDEX PContainer::GetSize() const
  { return PAssertNULL(reference)->size; }

PINLINE PBoolean PContainer::IsEmpty() const
  { return GetSize() == 0; }

PINLINE PBoolean PContainer::IsUnique() const
  { return PAssertNULL(reference)->count <= 1; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PString::PString()
  : PCharArray(1) { }

PINLINE PString::PString(const PString & str)
  : PCharArray(str) { }

PINLINE PString::PString(int, const PString * str)
  : PCharArray(*str) { }

PINLINE PString::PString(const std::string & str)
  : PCharArray(PString(str.c_str())) { }

PINLINE PString::PString(char c)
  : PCharArray(2) { SetAt(0, c); }

PINLINE PString PString::Empty()
  { return PString(); }

PINLINE PString & PString::operator=(const PString & str)
  { AssignContents(str); return *this; }

PINLINE PString & PString::operator=(const char * cstr)
  { AssignContents(PString(cstr)); return *this; }

PINLINE PString & PString::operator=(char ch)
  { AssignContents(PString(ch)); return *this; }

PINLINE PBoolean PString::MakeMinimumSize()
  { return SetSize(GetLength()+1); }

PINLINE PINDEX PString::GetLength() const
  { return strlen(theArray); }

PINLINE bool PString::operator!() const
  { return !IsEmpty(); }

PINLINE PString PString::operator+(const PString & str) const
  { return operator+((const char *)str); }

PINLINE PString operator+(const char * cstr, const PString & str)
  { return PString(cstr) + str; }
  
PINLINE PString operator+(char c, const PString & str)
  { return PString(c) + str; }
  
PINLINE PString & PString::operator+=(const PString & str)
  { return operator+=((const char *)str); }

PINLINE PString PString::operator&(const PString & str) const
  { return operator&((const char *)str); }

PINLINE PString operator&(const char * cstr, const PString & str)
  { return PString(cstr) & str; }
  
PINLINE PString operator&(char c, const PString & str)
  { return PString(c) & str; }
  
PINLINE PString & PString::operator&=(const PString & str)
  { return operator&=((const char *)str); }

PINLINE bool PString::operator==(const PObject & obj) const
  { return PObject::operator==(obj); }

PINLINE bool PString::operator!=(const PObject & obj) const
  { return PObject::operator!=(obj); }

PINLINE bool PString::operator<(const PObject & obj) const
  { return PObject::operator<(obj); }

PINLINE bool PString::operator>(const PObject & obj) const
  { return PObject::operator>(obj); }

PINLINE bool PString::operator<=(const PObject & obj) const
  { return PObject::operator<=(obj); }

PINLINE bool PString::operator>=(const PObject & obj) const
  { return PObject::operator>=(obj); }

PINLINE bool PString::operator*=(const PString & str) const
  { return operator*=((const char *)str); }

PINLINE bool PString::operator==(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == EqualTo; }

PINLINE bool PString::operator!=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != EqualTo; }

PINLINE bool PString::operator<(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == LessThan; }

PINLINE bool PString::operator>(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) == GreaterThan; }

PINLINE bool PString::operator<=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != GreaterThan; }

PINLINE bool PString::operator>=(const char * cstr) const
  { return InternalCompare(0, P_MAX_INDEX, cstr) != LessThan; }

PINLINE PINDEX PString::Find(const PString & str, PINDEX offset) const
  { return Find((const char *)str, offset); }

PINLINE PINDEX PString::FindLast(const PString & str, PINDEX offset) const
  { return FindLast((const char *)str, offset); }

PINLINE PINDEX PString::FindOneOf(const PString & str, PINDEX offset) const
  { return FindOneOf((const char *)str, offset); }

PINLINE PINDEX PString::FindSpan(const PString & str, PINDEX offset) const
  { return FindSpan((const char *)str, offset); }

PINLINE void PString::Splice(const PString & str, PINDEX pos, PINDEX len)
  { Splice((const char *)str, pos, len); }

PINLINE PStringArray
      PString::Tokenise(const PString & separators, PBoolean onePerSeparator) const
  { return Tokenise((const char *)separators, onePerSeparator); }

PINLINE PString::operator const unsigned char *() const
  { return (const unsigned char *)theArray; }

PINLINE PString & PString::vsprintf(const PString & fmt, va_list args)
  { return vsprintf((const char *)fmt, args); }

PINLINE PString pvsprintf(const PString & fmt, va_list args)
  { return pvsprintf((const char *)fmt, args); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PCaselessString::PCaselessString()
  : PString() { }

PINLINE PCaselessString::PCaselessString(const char * cstr)
  : PString(cstr) { }

PINLINE PCaselessString::PCaselessString(const PString & str)
  : PString(str) { }

PINLINE PCaselessString::PCaselessString(int dummy,const PCaselessString * str)
  : PString(dummy, str) { }

PINLINE PCaselessString & PCaselessString::operator=(const PString & str)
  { AssignContents(str); return *this; }

PINLINE PCaselessString & PCaselessString::operator=(const char * cstr)
  { AssignContents(PString(cstr)); return *this; }

PINLINE PCaselessString & PCaselessString::operator=(char ch)
  { AssignContents(PString(ch)); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PStringStream::Buffer::Buffer(const Buffer & b)
  : string(b.string) { }

PINLINE PStringStream::Buffer& PStringStream::Buffer::operator=(const Buffer&b)
  { string = b.string; return *this; }

PINLINE PStringStream & PStringStream::operator=(const PStringStream & strm)
  { AssignContents(strm); return *this; }

PINLINE PStringStream & PStringStream::operator=(const PString & str)
  { AssignContents(str); return *this; }

PINLINE PStringStream & PStringStream::operator=(const char * cstr)
  { AssignContents(PString(cstr)); return *this; }

PINLINE PStringStream & PStringStream::operator=(char ch)
  { AssignContents(PString(ch)); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PCollection::PCollection(PINDEX initialSize)
  : PContainer(initialSize) { }

PINLINE PCollection::PCollection(int dummy, const PCollection * c)
  : PContainer(dummy, c) { }

PINLINE void PCollection::AllowDeleteObjects(PBoolean yes)
  { reference->deleteObjects = yes; }

PINLINE void PCollection::DisallowDeleteObjects()
  { AllowDeleteObjects(PFalse); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PArrayObjects::PArrayObjects(PINDEX initialSize)
  : theArray(PNEW PBaseArray<PObject *>(initialSize)) { }


///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringArray::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PStringArray & PStringArray::operator += (const PString & str)
  { Append(str.Clone()); return *this; }

PINLINE PStringArray PStringArray::operator + (const PStringArray & v)
  { PStringArray arr = *this; arr += v; return arr; }

PINLINE PStringArray PStringArray::operator + (const PString & v)
  { PStringArray arr = *this; arr += v; return arr; }

PINLINE PINDEX PStringArray::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractList::PAbstractList()
  : info(new PListInfo) { PAssert(info != NULL, POutOfMemory); }

PINLINE PObject & PAbstractList::GetReferenceAt(PINDEX index) const
  { PObject * obj = GetAt(index);
                       PAssert(obj != NULL, PInvalidArrayIndex); return *obj; }

///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringList::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PINDEX PStringList::InsertString(
                                   const PString & before, const PString & str)
  { return Insert(before, str.Clone()); }

PINLINE PStringList & PStringList::operator += (const PString & str)
  { Append(str.Clone()); return *this; }

PINLINE PStringList PStringList::operator + (const PStringList & v)
  { PStringList arr = *this; arr += v; return arr; }

PINLINE PStringList PStringList::operator + (const PString & v)
  { PStringList arr = *this; arr += v; return arr; }

PINLINE PINDEX PStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }

///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PSortedStringList::AppendString(const PString & str)
  { return Append(str.Clone()); }

PINLINE PINDEX PSortedStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }


///////////////////////////////////////////////////////////////////////////////

PINLINE POrdinalKey::POrdinalKey(PINDEX newKey)
  : theKey(newKey) { }

PINLINE POrdinalKey & POrdinalKey::operator=(PINDEX newKey)
  { theKey = newKey; return *this; }

PINLINE POrdinalKey::operator PINDEX() const
  { return theKey; }

PINLINE PINDEX POrdinalKey::operator++()
  { return ++theKey; }

PINLINE PINDEX POrdinalKey::operator++(int)
  { return theKey++; }

PINLINE PINDEX POrdinalKey::operator--()
  { return --theKey; }

PINLINE PINDEX POrdinalKey::operator--(int)
  { return theKey--; }

PINLINE POrdinalKey & POrdinalKey::operator+=(PINDEX add)
  { theKey += add; return *this; }

PINLINE POrdinalKey & POrdinalKey::operator-=(PINDEX minus)
  { theKey -= minus; return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PBoolean PHashTable::AbstractContains(const PObject & key) const
  { return hashTable->GetElementAt(key) != NULL; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractSet::PAbstractSet()
  { hashTable->deleteKeys = reference->deleteObjects; }
  

PINLINE void PStringSet::Include(const PString & str)
  { PAbstractSet::Append(str.Clone()); }

PINLINE PStringSet & PStringSet::operator+=(const PString & str)
  { PAbstractSet::Append(str.Clone()); return *this; }

PINLINE void PStringSet::Exclude(const PString & str)
  { PAbstractSet::Remove(&str); }

PINLINE PStringSet & PStringSet::operator-=(const PString & str)
  { PAbstractSet::Remove(&str); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractDictionary::PAbstractDictionary()
  { hashTable->deleteKeys = PTrue; }
  
PINLINE PAbstractDictionary::PAbstractDictionary(int dummy,
                                                 const PAbstractDictionary * c)
  : PHashTable(dummy, c) { }


// End Of File ///////////////////////////////////////////////////////////////
