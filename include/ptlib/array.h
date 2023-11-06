/*
 * array.h
 *
 * Linear Array Container classes.
 *
 * Portable Tools Library
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

#ifndef PTLIB_ARRAY_H
#define PTLIB_ARRAY_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/contain.h>

///////////////////////////////////////////////////////////////////////////////
// The abstract array class

/**This class contains a variable length array of arbitrary memory blocks.
   These can be anything from individual bytes to large structures. Note that
   that does \b not include class objects that require construction or
   destruction. Elements in this array will not execute the contructors or
   destructors of objects.

   An abstract array consists of a linear block of memory sufficient to hold
   PContainer::GetSize() elements of <code>elementSize</code> bytes
   each. The memory block itself will automatically be resized when required
   and freed when no more references to it are present.

   The PAbstractArray class would very rarely be descended from directly by
   the user. The <code>PBASEARRAY</code> macro would normally be used to create
   a class and any new classes descended from that. That will instantiate the
   template based on <code>PBaseArray</code> or directly declare and define a class
   (using inline functions) if templates are not being used.

   The <code>PBaseArray</code> class or <code>PBASEARRAY</code> macro will define the correctly
   typed operators for pointer access (operator const T *) and subscript
   access (operator[]).
 */
class PAbstractArray : public PContainer
{
    PCONTAINERINFO(PAbstractArray, PContainer);
  public:
  /**@name Construction */
  //@{
    /**Create a new dynamic array of \p initalSize elements of
       \p elementSizeInBytes bytes each. The array memory is
       initialised to zeros.

       If the initial size is zero then no memory is allocated. Note that the
       internal pointer is set to NULL, not to a pointer to zero bytes of
       memory. This can be an important distinction when the pointer is
       obtained via an operator created in the <code>PBASEARRAY</code> macro.
     */
    PAbstractArray(
      PINDEX elementSizeInBytes,  ///< Size of each element in the array. This must be > 0 or the
                                  ///< constructor will assert.
      PINDEX initialSize = 0      ///< Number of elements to allocate initially.
    );

    /**Create a new dynamic array of \p bufferSizeInElements
       elements of \p elementSizeInBytes bytes each. The contents of
       the memory pointed to by buffer is then used to initialise the newly
       allocated array.

       If the initial size is zero then no memory is allocated. Note that the
       internal pointer is set to NULL, not to a pointer to zero bytes of
       memory. This can be an important distinction when the pointer is
       obtained via an operator created in the <code>PBASEARRAY</code> macro.

       If the \p dynamicAllocation parameter is <code>false</code> then the
       pointer is used directly by the container. It will not be copied to a
       dynamically allocated buffer. If the <code>SetSize()</code> function is used to
       change the size of the buffer, the object will be converted to a
       dynamic form with the contents of the static buffer copied to the
       allocated buffer.
     */
    PAbstractArray(
      PINDEX elementSizeInBytes,   ///< Size of each element in the array. This must be > 0 or the
                                   ///< constructor will assert.
      const void *buffer,          ///< Pointer to an array of elements.
      PINDEX bufferSizeInElements, ///< Number of elements pointed to by buffer.
      PBoolean dynamicAllocation   ///< Buffer is copied and dynamically allocated.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Output the contents of the object to the stream. The exact output is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>operator<<</code> function.

       The default behaviour is to print the class name.
     */
    virtual void PrintOn(
      ostream &strm   // Stream to print the object into.
    ) const;

    /**Input the contents of the object from the stream. The exact input is
       dependent on the exact semantics of the descendent class. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour is to do nothing.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );

    /**Get the relative rank of the two arrays. The following algorithm is
       employed for the comparison:
          \li <code>EqualTo</code>        if the two array memory blocks are identical in
                              length and contents.
          \li <code>LessThan</code>       if the array length is less than the
                              \p obj parameters array length.
          \li <code>GreaterThan</code>    if the array length is greater than the
                              \p obj parameters array length.


       If the array sizes are identical then the memcmp()
       function is used to rank the two arrays.

       @return
       Comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for \p obj logically less than the
       object and <code>GreaterThan</code> for \p obj logically
       greater than the object.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Other <code>PAbstractArray</code> to compare against.
    ) const;
  //@}

  /**@name Overrides from class PContainer */
  //@{
    /**Set the size of the array in elements. A new array may be allocated to
       accomodate the new number of elements. If the array increases in size
       then the new bytes are initialised to zero. If the array is made smaller
       then the data beyond the new size is lost.

       @return
       <code>true</code> if the memory for the array was allocated successfully.
     */
    virtual PBoolean SetSize(
      PINDEX newSize  ///< New size of the array in elements.
    );
  //@}

  /**@name New functions for class */
  //@{
    /**Attach a pointer to a static block to the base array type. The pointer
       is used directly and will not be copied to a dynamically allocated
       buffer. If the <code>SetSize()</code> function is used to change the size of the
       buffer, the object will be converted to a dynamic form with the
       contents of the static buffer copied to the allocated buffer.

       Any dynamically allocated buffer will be freed.
     */
    void Attach(
      const void *buffer, ///< Pointer to an array of elements.
      PINDEX bufferSize   ///< Number of elements pointed to by buffer.
    );

    /**Get a pointer to the internal array and assure that it is of at least
       the specified size. This is useful when the array contents are being
       set by some external or system function eg file read.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Pointer to the array memory.
     */
    void * GetPointer(
      PINDEX minSize = 1  ///< Minimum size the array must be.
    );

    /**Concatenate one array to the end of this array.
       This function will allocate a new array large enough for the existing
       contents and the contents of the parameter. The parameters contents is then
       copied to the end of the existing array.

       Note this does nothing and returns <code>false</code> if the target array is not
       dynamically allocated, or if the two arrays are of base elements of
       different sizes.

       @return
       <code>true</code> if the memory allocation succeeded.
     */
    PBoolean Concatenate(
      const PAbstractArray & array  ///< Array to concatenate.
    );
  //@}

  protected:
    PBoolean InternalSetSize(PINDEX newSize, PBoolean force);

    virtual void PrintElementOn(
      ostream & stream,
      PINDEX index
    ) const;
    virtual void ReadElementFrom(
      istream & stream,
      PINDEX index
    );

    PAbstractArray(
      PContainerReference & reference,
      PINDEX elementSizeInBytes
    );

    /// Size of an element in bytes.
    PINDEX elementSize;

    /// Pointer to the allocated block of memory.
    char * theArray;

    /// Flag indicating the array was allocated on the heap.
    PBoolean allocatedDynamically;

  friend class PArrayObjects;
};


///////////////////////////////////////////////////////////////////////////////
// An array of some base type

/**This template class maps the <code>PAbstractArray</code> to a specific element type. The
   functions in this class primarily do all the appropriate casting of types.

   Note that if templates are not used the <code>PBASEARRAY</code> macro will
   simulate the template instantiation.

   The following classes are instantiated automatically for the basic scalar
   types:
   \li <code>PCharArray</code>
   \li <code>PBYTEArray</code>
   \li <code>PShortArray</code>
   \li <code>PWORDArray</code>
   \li <code>PIntArray</code>
   \li <code>PUnsignedArray</code>
   \li <code>PLongArray</code>
   \li <code>PDWORDArray</code>
 */
template <class T> class PBaseArray : public PAbstractArray
{
    PCLASSINFO(PBaseArray, PAbstractArray);
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of elements of the specified type. The
       array is initialised to all zero bytes. Note that this may not be
       logically equivalent to the zero value for the type, though this would
       be very rare.
     */
    PBaseArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    ) : PAbstractArray(sizeof(T), initialSize) { }

    /**Construct a new dynamic array of elements of the specified type.
     */
    PBaseArray(
      T const * buffer,   ///< Pointer to an array of the elements of type \b T.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    ) : PAbstractArray(sizeof(T), buffer, length, dynamic) { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Clone the object.
     */
    virtual PObject * Clone() const
    {
      return PNEW PBaseArray<T>(*this, GetSize());
    }
  //@}

  /**@name Overrides from class PContainer */
  //@{
    /**Set the specific element in the array. The array will automatically
       expand, if necessary, to fit the new element in.

       @return
       <code>true</code> if new memory for the array was successfully allocated.
     */
    PBoolean SetAt(
      PINDEX index,   ///< Position in the array to set the new value.
      T val           ///< Value to set in the array.
    ) {
      return SetMinSize(index+1) && val==(((T *)theArray)[index] = val);
    }

    /**Get a value from the array. If the \p index is beyond the end
       of the allocated array then a zero value is returned.

       @return
       Value at the array position.
     */
    T GetAt(
      PINDEX index  ///< Position on the array to get value from.
    ) const {
      PASSERTINDEX(index);
      return index < GetSize() ? ((T *)theArray)[index] : (T)0;
    }

    /**Attach a pointer to a static block to the base array type. The pointer
       is used directly and will not be copied to a dynamically allocated
       buffer. If the <code>SetSize()</code> function is used to change the size of the
       buffer, the object will be converted to a dynamic form with the
       contents of the static buffer copied to the allocated buffer.

       Any dynamically allocated buffer will be freed.
     */
    void Attach(
      const T * buffer,   ///< Pointer to an array of elements.
      PINDEX bufferSize   ///< Number of elements pointed to by buffer.
    ) {
      PAbstractArray::Attach(buffer, bufferSize);
    }

    /**Get a pointer to the internal array and assure that it is of at least
       the specified size. This is useful when the array contents are being
       set by some external or system function eg file read.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Pointer to the array memory.
     */
    T * GetPointer(
      PINDEX minSize = 0    ///< Minimum size for returned buffer pointer.
    ) {
      return (T *)PAbstractArray::GetPointer(minSize);
    }
  //@}

  /**@name New functions for class */
  //@{
    /**Get a value from the array. If the \p index is beyond the end
       of the allocated array then a zero value is returned.

       This is functionally identical to the PContainer::GetAt()
       function.

       @return
       Value at the array position.
     */
    T operator[](
      PINDEX index  ///< Position on the array to get value from.
    ) const {
      return GetAt(index);
    }

    /**Get a reference to value from the array. If \p index is
       beyond the end of the allocated array then the array is expanded. If a
       memory allocation failure occurs the function asserts.

       This is functionally similar to the <code>SetAt()</code> function and allows
       the array subscript to be an lvalue.

       @return
       Reference to value at the array position.
     */
    T & operator[](
      PINDEX index  ///< Position on the array to get value from.
    ) {
      PASSERTINDEX(index);
      PAssert(SetMinSize(index+1), POutOfMemory);
      return ((T *)theArray)[index];
    }

    /**Get a pointer to the internal array. The user may not modify the
       contents of this pointer. This is useful when the array contents are
       required by some external or system function eg file write.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Constant pointer to the array memory.
     */
    operator T const *() const {
      return (T const *)theArray;
    }

    /**Concatenate one array to the end of this array.
       This function will allocate a new array large enough for the existing
       contents and the contents of the parameter. The paramters contents is then
       copied to the end of the existing array.

       Note this does nothing and returns <code>false</code> if the target array is not
       dynamically allocated.

       @return
       <code>true</code> if the memory allocation succeeded.
     */
    PBoolean Concatenate(
      const PBaseArray & array  ///< Other array to concatenate
    ) {
      return PAbstractArray::Concatenate(array);
    }
  //@}

  protected:
    virtual void PrintElementOn(
      ostream & stream,
      PINDEX index
    ) const {
      stream << GetAt(index);
    }

    PBaseArray(PContainerReference & reference) : PAbstractArray(reference, sizeof(T)) { }
};

/**Declare a dynamic array base type.
   This macro is used to declare a descendent of <code>PAbstractArray</code> class,
   customised for a particular element type \b T. This macro closes the
   class declaration off so no additional members can be added.

   If the compilation is using templates then this macro produces a typedef
   of the <code>PBaseArray</code> template class.
 */
#define PBASEARRAY(cls, T) typedef PBaseArray<T> cls

/**Begin a declaration of an array of base types.
   This macro is used to declare a descendent of <code>PAbstractArray</code> class,
   customised for a particular element type \b T.

   If the compilation is using templates then this macro produces a descendent
   of the <code>PBaseArray</code> template class. If templates are not being used
   then the macro defines a set of inline functions to do all casting of types.
   The resultant classes have an identical set of functions in either case.

   See the <code>PBaseArray</code> and <code>PAbstractArray</code> classes for more
   information.
 */
#define PDECLARE_BASEARRAY(cls, T) \
  PDECLARE_CLASS(cls, PBaseArray<T>) \
    cls(PINDEX initialSize = 0) \
      : PBaseArray<T>(initialSize) { } \
    cls(PContainerReference & reference) \
      : PBaseArray<T>(reference) { } \
    cls(T const * buffer, PINDEX length, PBoolean dynamic = true) \
      : PBaseArray<T>(buffer, length, dynamic) { } \
    virtual PObject * Clone() const \
      { return PNEW cls(*this, GetSize()); } \


/**This template class maps the <code>PAbstractArray</code> to a specific element type. The
   functions in this class primarily do all the appropriate casting of types.

   Note that if templates are not used the <code>PSCALAR_ARRAY</code> macro will
   simulate the template instantiation.

   The following classes are instantiated automatically for the basic scalar
   types:
   \li <code>PBYTEArray</code>
   \li <code>PShortArray</code>
   \li <code>PWORDArray</code>
   \li <code>PIntArray</code>
   \li <code>PUnsignedArray</code>
   \li <code>PLongArray</code>
   \li <code>PDWORDArray</code>
 */
template <class T> class PScalarArray : public PBaseArray<T>
{
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of elements of the specified type. The
       array is initialised to all zero bytes. Note that this may not be
       logically equivalent to the zero value for the type, though this would
       be very rare.
     */
    PScalarArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    ) : PBaseArray<T>(initialSize) { }

    /**Construct a new dynamic array of elements of the specified type.
     */
    PScalarArray(
      T const * buffer,   ///< Pointer to an array of the elements of type <b>T</b>.
      PINDEX length,      ///< Number of elements pointed to by <code>buffer</code>.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    ) : PBaseArray<T>(buffer, length, dynamic) { }
  //@}

  protected:
    virtual void ReadElementFrom(
      istream & stream,
      PINDEX index
    ) {
      T t;
      stream >> t;
      if (!stream.fail())
        this->SetAt(index, t);
    }
};


/**Declare a dynamic array base type.
   This macro is used to declare a descendent of <code>PAbstractArray</code> class,
   customised for a particular element type \b T. This macro closes the
   class declaration off so no additional members can be added.

   If the compilation is using templates then this macro produces a typedef
   of the <code>PBaseArray</code> template class.
 */
#define PSCALAR_ARRAY(cls, T) typedef PScalarArray<T> cls


/// Array of characters.
#ifdef DOC_PLUS_PLUS
class PCharArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of char.
       The array is initialised to all zero bytes.
     */
    PCharArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of char.
     */
    PCharArray(
      char const * buffer,   ///< Pointer to an array of chars.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
#else
PDECLARE_BASEARRAY(PCharArray, char);
#endif
  public:
  /**@name Overrides from class PObject */
  //@{
    /// Print the array
    virtual void PrintOn(
      ostream & strm ///< Stream to output to.
    ) const;
    /// Read the array
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );
  //@}
};

/// Array of short integers.
#ifdef DOC_PLUS_PLUS
class PShortArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of shorts.
       The array is initialised to all zeros.
     */
    PShortArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of shorts.
     */
    PShortArray(
      short const * buffer,   ///< Pointer to an array of shorts.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PShortArray, short);
#endif


/// Array of integers.
#ifdef DOC_PLUS_PLUS
class PIntArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of ints.
       The array is initialised to all zeros.
     */
    PIntArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of ints.
     */
    PIntArray(
      int const * buffer,   ///< Pointer to an array of ints.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PIntArray, int);
#endif


/// Array of long integers.
#ifdef DOC_PLUS_PLUS
class PLongArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of longs.
       The array is initialised to all zeros.
     */
    PLongArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of longs.
     */
    PLongArray(
      long const * buffer,   ///< Pointer to an array of longs.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PLongArray, long);
#endif


/// Array of unsigned characters.
#ifdef DOC_PLUS_PLUS
class PBYTEArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of unsigned chars.
       The array is initialised to all zeros.
     */
    PBYTEArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of unsigned chars.
     */
    PBYTEArray(
      BYTE const * buffer,   ///< Pointer to an array of BYTEs.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PDECLARE_BASEARRAY(PBYTEArray, BYTE);
#endif
  public:
  /**@name Overrides from class PObject */
  //@{
    /// Print the array
    virtual void PrintOn(
      ostream & strm ///< Stream to output to.
    ) const;
    /// Read the array
    virtual void ReadFrom(
      istream &strm   ///< Stream to read the objects contents from.
    );
  //@}
};


/// Array of unsigned short integers.
#ifdef DOC_PLUS_PLUS
class PWORDArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of unsigned shorts.
       The array is initialised to all zeros.
     */
    PWORDArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of unsigned shorts.
     */
    PWORDArray(
      WORD const * buffer,   ///< Pointer to an array of WORDs.
      PINDEX length,      ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PWORDArray, WORD);
#endif


/// Array of unsigned integers.
#ifdef DOC_PLUS_PLUS
class PUnsignedArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of unsigned ints.
       The array is initialised to all zeros.
     */
    PUnsignedArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of unsigned ints.
     */
    PUnsignedArray(
      unsigned const * buffer,  ///< Pointer to an array of unsigned ints.
      PINDEX length,            ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true   ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PUnsignedArray, unsigned);
#endif


/// Array of unsigned long integers.
#ifdef DOC_PLUS_PLUS
class PDWORDArray : public PBaseArray {
  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of unsigned longs.
       The array is initialised to all zeros.
     */
    PDWORDArray(
      PINDEX initialSize = 0  ///< Initial number of elements in the array.
    );

    /**Construct a new dynamic array of DWORDs.
     */
    PDWORDArray(
      DWORD const * buffer,   ///< Pointer to an array of DWORDs.
      PINDEX length,          ///< Number of elements pointed to by \p buffer.
      PBoolean dynamic = true ///< Buffer is copied and dynamically allocated.
    );
  //@}
};
#else
PSCALAR_ARRAY(PDWORDArray, DWORD);
#endif


///////////////////////////////////////////////////////////////////////////////
// Linear array of objects

/** An array of objects.
This class is a collection of objects which are descendents of the
#PObject class. It is implemeted as a dynamic, linear array of
pointers to the objects.

The implementation of an array allows very fast random access to items in
the collection, but has severe penalties for inserting and deleting objects
as all other objects must be moved to accommodate the change.

An array of objects may have "gaps" in it. These are array entries that
contain NULL as the object pointer.

The PArrayObjects class would very rarely be descended from directly by
the user. The <code>PARRAY</code> macro would normally be used to create a class.
That will instantiate the template based on <code>PArray</code> or directly declare
and define the class (using inline functions) if templates are not being used.

The <code>PArray</code> class or <code>PARRAY</code> macro will define the
correctly typed operators for pointer access (operator const T *) and
subscript access (operator[]).
*/
class PArrayObjects : public PCollection
{
    PCONTAINERINFO(PArrayObjects, PCollection);
  public:
  /**@name Construction */
  //@{
    /**Create a new array of objects. The array is initially set to the
       specified size with each entry having NULL as is pointer value.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    PINLINE PArrayObjects(
      PINDEX initialSize = 0  ///< Initial number of objects in the array.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Get the relative rank of the two arrays. The following algorithm is
       employed for the comparison:

       \li <code>EqualTo</code>         if the two array memory blocks are identical in
                            length and each objects values, not pointer, are
                            equal.

       \li <code>LessThan</code>        if the instances object value at an ordinal
                            position is less than the corresponding objects
                            value in the \p obj parameters array.
                            This is also returned if all objects are equal and
                            the instances array length is less than the
                            \p obj parameters array length.

       \li <code>GreaterThan</code>     if the instances object value at an ordinal
                            position is greater than the corresponding objects
                            value in the \p obj parameters array.
                            This is also returned if all objects are equal and
                            the instances array length is greater than the
                            \p obj parameters array length.

       @return
       Comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for \p obj logically less than the
       object and <code>GreaterThan</code> for \p obj logically
       greater than the object.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Other <code>PAbstractArray</code> to compare against.
    ) const;
  //@}

  /**@name Overrides from class PContainer */
  //@{
    /// Get size of array
    virtual PINDEX GetSize() const;

    /**Set the size of the array in objects. A new array may be allocated to
       accomodate the new number of objects. If the array increases in size
       then the new object pointers are initialised to NULL. If the array is
       made smaller then the data beyond the new size is lost.

       @return
       true if the memory for the array was allocated successfully.
     */
    virtual PBoolean SetSize(
      PINDEX newSize  ///< New size of the array in objects.
    );
  //@}

  /**@name Overrides from class PCollection */
  //@{
    /**Append a new object to the collection. This will increase the size of
       the array by one and place the new object at that position.

       @return
       Index of the newly added object.
     */
    virtual PINDEX Append(
      PObject * obj   ///< New object to place into the collection.
    );

    /**Insert a new object immediately before the specified object. If the
       object to insert before is not in the collection then the equivalent of
       the <code>Append()</code> function is performed.

       All objects, including the \p before object are shifted up
       one in the array.

       Note that the object values are compared for the search of the
       <code>before</code> parameter, not the pointers. So the objects in the
       collection must correctly implement the PObject::Compare()
       function.

       @return
       index of the newly inserted object.
     */
    virtual PINDEX Insert(
      const PObject & before,   ///< Object value to insert before.
      PObject * obj             ///< New object to place into the collection.
    );

    /** Insert a new object at the specified ordinal index. If the index is
       greater than the number of objects in the collection then the
       equivalent of the <code>Append()</code> function is performed.

       All objects, including the \p index position object are
       shifted up one in the array.

       @return
       Index of the newly inserted object.
     */
    virtual PINDEX InsertAt(
      PINDEX index,   ///< Index position in collection to place the object.
      PObject * obj   ///< New object to place into the collection.
    );

    /**Remove the object from the collection. If the <code>AllowDeleteObjects</code> option
       is set then the object is also deleted.

       All objects are shifted down to fill the vacated position.

       @return
       <code>true</code> if the object was in the collection.
     */
    virtual PBoolean Remove(
      const PObject * obj   ///< Existing object to remove from the collection.
    );

    /**Remove the object at the specified ordinal index from the collection.
       If the <code>AllowDeleteObjects</code> option is set then the object is also deleted.

       All objects are shifted down to fill the vacated position.

       Note if the \p index is beyond the size of the collection then the
       function will assert.

       @return
       Pointer to the object being removed, or NULL if it was deleted.
     */
    virtual PObject * RemoveAt(
      PINDEX index   ///< Index position in collection to place the object.
    );

    /**Set the object at the specified ordinal position to the new value. This
       will overwrite the existing entry. If the <code>AllowDeleteObjects</code> option is
       set then the old object is also deleted.

       @return
       <code>true</code> if the object was successfully added.
     */
    virtual PBoolean SetAt(
      PINDEX index,   ///< Index position in collection to set.
      PObject * val   ///< New value to place into the collection.
    );

    /**Get the object at the specified ordinal position. If the index was
       greater than the size of the collection then NULL is returned.

       @return
       Pointer to object at the specified index.
     */
    virtual PObject * GetAt(
      PINDEX index  ///< Index position in the collection of the object.
    ) const;

    /**Search the collection for the specific instance of the object. The
       object pointers are compared, not the values. A simple linear search
       from ordinal position zero is performed.

       @return
       Ordinal index position of the object, or <code>P_MAX_INDEX</code>.
     */
    virtual PINDEX GetObjectsIndex(
      const PObject * obj  ///< Object to find.
    ) const;

    /**Search the collection for the specified value of the object. The object
       values are compared, not the pointers.  So the objects in the
       collection must correctly implement the PObject::Compare()
       function. A simple linear search from ordinal position zero is
       performed.

       @return
       Ordinal index position of the object, or <code>P_MAX_INDEX</code>.
     */
    virtual PINDEX GetValuesIndex(
      const PObject & obj   // Object to find equal of.
    ) const;

    /**Remove all of the elements in the collection. This operates by
       continually calling <code>RemoveAt()</code> until there are no objects left.

       The objects are removed from the last, at index
       (GetSize()-1) toward the first at index zero.
     */
    virtual void RemoveAll();
  //@}

  protected:
    // The type below cannot be nested as DevStudio 2005 AUTOEXP.DAT doesn't like it
    PBaseArray<PObject *> * theArray;
};


/**\class PArray
   This template class maps the <code>PArrayObjects</code> to a specific object type.
   The functions in this class primarily do all the appropriate casting of types.

   Note that if templates are not used the <code>PARRAY</code> macro will
   simulate the template instantiation.
*/
template <class T> class PArray : public PArrayObjects
{
    PCLASSINFO(PArray, PArrayObjects);
  public:
  /**@name Construction */
  //@{
    /**Create a new array of objects. The array is initially set to the
       specified size with each entry having NULL as is pointer value.

       Note that by default, objects placed into the list will be deleted when
       removed or when all references to the list are destroyed.
     */
    PArray(
      PINDEX initialSize = 0  ///< Initial number of objects in the array.
    ) : PArrayObjects(initialSize) { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the array. Note that all objects in the
       array are also cloned, so this will make a complete copy of the array.
     */
    virtual PObject * Clone() const
      { return PNEW PArray(0, this); }
  //@}

  /**@name New functions for class */
  //@{
    /**Retrieve a reference  to the object in the array. If there was not an
       object at that ordinal position or the index was beyond the size of the
       array then the function asserts.

       @return
       Reference to the object at \p index position.
     */
    T & operator[](
      PINDEX index  ///< Index position in the collection of the object.
    ) const {
      PObject * obj = GetAt(index);
      PAssert(obj != NULL, PInvalidArrayElement);
      return (T &)*obj;
    }
  //@}

  protected:
    PArray(int dummy, const PArray * c) : PArrayObjects(dummy, c) { }
};


/**Declare an array to a specific type of object.
   This macro is used to declare a descendent of <code>PArrayObjects</code> class,
   customised for a particular object type \b T. This macro closes the
   class declaration off so no additional members can be added.

   If the compilation is using templates then this macro produces a typedef
   of the <code>PArray</code> template class.

   See the <code>PBaseArray</code> class and <code>PDECLARE_ARRAY</code> macro for more
   information.
*/
#define PARRAY(cls, T) typedef PArray<T> cls


/**Begin declaration an array to a specific type of object.
   This macro is used to declare a descendent of <code>PArrayObjects</code> class,
   customised for a particular object type \b T.

   If the compilation is using templates then this macro produces a descendent
   of the <code>PArray</code> template class. If templates are not being used then
   the macro defines a set of inline functions to do all casting of types. The
   resultant classes have an identical set of functions in either case.

   See the <code>PBaseArray</code> and <code>PAbstractArray</code> classes for more
   information.
*/
#define PDECLARE_ARRAY(cls, T) \
  PARRAY(cls##_PTemplate, T); \
  PDECLARE_CLASS(cls, cls##_PTemplate) \
  protected: \
    inline cls(int dummy, const cls * c) \
      : cls##_PTemplate(dummy, c) { } \
  public: \
    inline cls(PINDEX initialSize = 0) \
      : cls##_PTemplate(initialSize) { } \
    virtual PObject * Clone() const \
      { return PNEW cls(0, this); } \


/**This class represents a dynamic bit array.
 */
class PBitArray : public PBYTEArray
{
  PCLASSINFO(PBitArray, PBYTEArray);

  public:
  /**@name Construction */
  //@{
    /**Construct a new dynamic array of bits.
     */
    PBitArray(
      PINDEX initialSize = 0  ///< Initial number of bits in the array.
    );

    /**Construct a new dynamic array of elements of the specified type.
     */
    PBitArray(
      const void * buffer,   ///< Pointer to an array of the elements of type \b T.
      PINDEX length,         ///< Number of bits (not bytes!) pointed to by \p buffer.
      PBoolean dynamic = true    ///< Buffer is copied and dynamically allocated.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Clone the object.
     */
    virtual PObject * Clone() const;
  //@}

  /**@name Overrides from class <code>PContainer</code> */
  //@{
    /**Get the current size of the container.
       This represents the number of things the container contains. For some
       types of containers this will always return 1.

       @return Number of objects in container.
     */
    virtual PINDEX GetSize() const;

    /**Set the size of the array in bits. A new array may be allocated to
       accomodate the new number of bits. If the array increases in size
       then the new bytes are initialised to zero. If the array is made smaller
       then the data beyond the new size is lost.

       @return
       true if the memory for the array was allocated successfully.
     */
    virtual PBoolean SetSize(
      PINDEX newSize  ///< New size of the array in bits, not bytes.
    );

    /**Set the specific bit in the array. The array will automatically
       expand, if necessary, to fit the new element in.

       @return
       true if new memory for the array was successfully allocated.
     */
    PBoolean SetAt(
      PINDEX index,   ///< Position in the array to set the new value.
      PBoolean val           ///< Value to set in the array.
    );

    /**Get a bit from the array. If \p index is beyond the end
       of the allocated array then <code>false</code> is returned.

       @return
       Value at the array position.
     */
    PBoolean GetAt(
      PINDEX index  ///< Position on the array to get value from.
    ) const;

    /**Attach a pointer to a static block to the bit array type. The pointer
       is used directly and will not be copied to a dynamically allocated
       buffer. If the <code>SetSize()</code> function is used to change the size of the
       buffer, the object will be converted to a dynamic form with the
       contents of the static buffer copied to the allocated buffer.

       Any dynamically allocated buffer will be freed.
     */
    void Attach(
      const void * buffer,   ///< Pointer to an array of elements.
      PINDEX bufferSize      ///< Number of bits (not bytes!) pointed to by buffer.
    );

    /**Get a pointer to the internal array and assure that it is of at least
       the specified size. This is useful when the array contents are being
       set by some external or system function eg file read.

       It is unsafe to assume that the pointer is valid for very long after
       return from this function. The array may be resized or otherwise
       changed and the pointer returned invalidated. It should be used for
       simple calls to atomic functions, or very careful examination of the
       program logic must be performed.

       @return
       Pointer to the array memory.
     */
    BYTE * GetPointer(
      PINDEX minSize = 0    ///< Minimum size in bits (not bytes!) for returned buffer pointer.
    );
  //@}

  /**@name New functions for class */
  //@{
    /**Get a value from the array. If the <code>index</code> is beyond the end
       of the allocated array then a zero value is returned.

       This is functionally identical to the <code>PContainer::GetAt()</code>
       function.

       @return
       Value at the array position.
     */
    PBoolean operator[](
      PINDEX index  ///< Position on the array to get value from.
    ) const { return GetAt(index); }

    /**Set a bit to the array.

       This is functionally identical to the PContainer::SetAt(index, true)
       function.
     */
    PBitArray & operator+=(
      PINDEX index  ///< Position on the array to get value from.
    ) { SetAt(index, true); return *this; }

    /**Set a bit to the array.

       This is functionally identical to the PContainer::SetAt(index, true)
       function.
     */
    PBitArray & operator-=(
      PINDEX index  ///< Position on the array to get value from.
    ) { SetAt(index, false); return *this; }

    /**Concatenate one array to the end of this array.
       This function will allocate a new array large enough for the existing
       contents and the contents of the parameter. The paramters contents is then
       copied to the end of the existing array.

       Note this does nothing and returns false if the target array is not
       dynamically allocated.

       @return
       <code>true</code> if the memory allocation succeeded.
     */
    PBoolean Concatenate(
      const PBitArray & array  ///< Other array to concatenate
    );
  //@}
};


#endif // PTLIB_ARRAY_H


// End Of File ///////////////////////////////////////////////////////////////
