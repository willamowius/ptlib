/*
 * contain.h
 *
 * Umbrella include for Container Classes.
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

#ifndef PTLIB_CONTAIN_H
#define PTLIB_CONTAIN_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/critsec.h>
#include <ptlib/object.h>



///////////////////////////////////////////////////////////////////////////////
// Abstract container class

// The type below cannot be nested into PContainer as DevStudio 2005 AUTOEXP.DAT doesn't like it
class PContainerReference
{
  public:
    __inline PContainerReference(PINDEX initialSize, bool isConst = false)
      : size(initialSize)
      , count(1)
      , deleteObjects(true)
      , constObject(isConst)
    {
    }

    __inline PContainerReference(const PContainerReference & ref)
      : size(ref.size)
      , count(1)
      , deleteObjects(ref.deleteObjects)
      , constObject(false)
    {  
    }

    PINDEX         size;          // Size of what the container contains
    PAtomicInteger count;         // reference count to the container content - guaranteed to be atomic
    bool           deleteObjects; // Used by PCollection but put here for efficiency
    bool           constObject;   // Indicates object is constant/static, copy on write.

    PDECLARE_POOL_ALLOCATOR();

  private:
    void operator=(const PContainerReference &) { }
};


/** Abstract class to embody the base functionality of a <code>container</code>.

Fundamentally, a container is an object that contains other objects. There
are two main areas of support for tha that are provided by this class. The
first is simply to keep a count of the number of things that the container
contains. The current size is stored and accessed by members of this class.
The setting of size is determined by the semantics of the descendent class
and so is a pure function.

The second area of support is for reference integrity. When an instance of
a container is copied to another instance, the two instance contain the
same thing. There can therefore be multiple references to the same things.
When one reference is destroyed this must {\b not} destroy the contained
object as it may be referenced by another instance of a container class.
To this end a reference count is provided by the PContainer class. This
assures that the container only destroys the objects it contains when there
are no more references to them.

In support of this, descendent classes must provide a <code>DestroyContents()</code>
function. As the normal destructor cannot be used, this function will free
the memory or unlock the resource the container is wrapping.
*/
class PContainer : public PObject
{
  PCLASSINFO(PContainer, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a new unique container.
     */
    PContainer(
      PINDEX initialSize = 0  ///< Initial number of things in the container.
    );

    /**Create a new refernce to container.
       Create a new container referencing the same contents as the container
       specified in the parameter.
     */
    PContainer(
      const PContainer & cont  ///< Container to create a new reference from.
    );

    /**Assign one container reference to another.
       Set the current container to reference the same thing as the container
       specified in the parameter.

       Note that the old contents of the container is dereferenced and if
       it was unique, destroyed using the DestroyContents() function.
     */
    PContainer & operator=(
      const PContainer & cont  ///< Container to create a new reference from.
    );

    /**Destroy the container class.
       This will decrement the reference count on the contents and if unique,
       will destroy it using the <code>DestroyContents()</code> function.
     */
    virtual ~PContainer()
    { Destruct(); }

  //@}

  /**@name Common functions for containers */
  //@{
    /**Get the current size of the container.
       This represents the number of things the container contains. For some
       types of containers this will always return 1.

       @return number of objects in container.
     */
    virtual PINDEX GetSize() const;

    /**Set the new current size of the container.
       The exact behavious of this is determined by the descendent class. For
       instance an array class would reallocate memory to make space for the
       new number of elements.

       Note for some types of containers this does not do anything as they
       inherently only contain one item. The function returns true always and
       the new value is ignored.

       @return
       true if the size was successfully changed. The value false usually
       indicates failure due to insufficient memory.
     */
    virtual PBoolean SetSize(
      PINDEX newSize  ///< New size for the container.
    ) = 0;

    /**Set the minimum size of container.
       This function will set the size of the object to be at least the size
       specified. The <code>SetSize()</code> function is always called, either with the
       new value or the previous size, whichever is the larger.
     */
    PBoolean SetMinSize(
      PINDEX minSize  ///< Possible, new size for the container.
    );

    /**Determine if the container is empty.
       Determine if the container that this object references contains any
       elements.

       @return true if <code>GetSize()</code> returns zero.
     */
    virtual PBoolean IsEmpty() const;

    /**Determine if container is unique reference.
       Determine if this instance is the one and only reference to the
       container contents.

       @return true if the reference count is one.
     */
    PBoolean IsUnique() const;

    /**Make this instance to be the one and only reference to the container
       contents. This implicitly does a clone of the contents of the container
       to make a unique reference. If the instance was already unique then
       the function does nothing.

       @return
       true if the instance was already unique.
     */
    virtual PBoolean MakeUnique();
  //@}

  protected:
    /**Constructor used in support of the Clone() function. This creates a
       new unique reference of a copy of the contents. It does {\b not}
       create another reference.

       The dummy parameter is there to prevent the contructor from being
       invoked automatically by the compiler when a pointer is used by accident
       when a normal instance or reference was expected. The container would
       be silently cloned and the copy used instead of the container expected
       leading to unpredictable results.
     */
    PContainer(
      int dummy,        ///< Dummy to prevent accidental use of the constructor.
      const PContainer * cont  ///< Container class to clone.
    );

    /// Construct using static PContainerReference.
    PContainer(PContainerReference & reference);

    /**Destroy the container contents. This function must be defined by the
       descendent class to do the actual destruction of the contents. It is
       automatically declared when the <code>PCONTAINERINFO()</code> macro is used.

       For all descendent classes not immediately inheriting off the PContainer
       itself, the implementation of DestroyContents() should always call its
       ancestors function. This is especially relevent if many of the standard
       container classes, such as arrays, are descended from as memory leaks
       will occur.
     */
    virtual void DestroyContents() = 0;

    /**Copy the container contents. This copies the contents from one reference
       to another.

       No duplication of contents occurs, for instance if the container is an
       array, the pointer to the array memory is copied, not the array memory
       block itself.

       This function will get called by the base assignment operator.
     */
    virtual void AssignContents(const PContainer & c);

    /**Copy the container contents. This copies the contents from one reference
       to another. It is automatically declared when the <code>PCONTAINERINFO()</code>
       macro is used.
       
       No duplication of contents occurs, for instance if the container is an
       array, the pointer to the array memory is copied, not the array memory
       block itself.

       This function will get called once for every class in the heirarchy, so
       the ancestor function should {\b not} be called.
     */
    void CopyContents(const PContainer & c);

    /**Create a duplicate of the container contents. This copies the contents
       from one container to another, unique container. It is automatically
       declared when the <code>PCONTAINERINFO()</code> macro is used.
       
       This class will duplicate the contents completely, for instance if the
       container is an array, the actual array memory is copied, not just the
       pointer. If the container contains objects that descend from <code>PObject</code>,
       they too should also be cloned and not simply copied.

       This function will get called once for every class in the heirarchy, so
       the ancestor function should {\b not} be called.
       
       <i><b>Note well</b></i>, the logic of the function must be able to
       accept the passed in parameter to clone being the same instance as the
       destination object, ie during execution <code>this == src</code>.
     */
    void CloneContents(const PContainer * src);

    /**Internal function called from container destructors. This will
       conditionally call <code>DestroyContents()</code> to destroy the container contents.
     */
    void Destruct();

    /** Destroy the PContainerReference instance.
        Override if passing a static vallue in via ctor.
      */
    virtual void DestroyReference();

    PContainerReference * reference;
};



/**Macro to declare funtions required in a container.
   This macro is used to declare all the functions that should be implemented
   for a working container class. It will also define some inline code for
   some standard function behaviour.

   This may be used when multiple inheritance requires a special class
   declaration. Normally, the <code>PCONTAINERINFO</code> macro would be used,
   which includes this macro in it.

   The default implementation for contructors, destructor, the assignment
   operator and the MakeUnique() function is as follows:
<pre><code>
        cls(const cls & c)
          : par(c)
        {
          CopyContents(c);
        }

        cls & operator=(const cls & c)
        {
          par::operator=(c);
          return *this;
        }

        cls(int dummy, const cls * c)
          : par(dummy, c)
        {
          CloneContents(c);
        }

        virtual ~cls()
        {
          Destruct();
        }

        PBoolean MakeUnique()
        {
          if (par::MakeUnique())
            return true;
          CloneContents(c);
          return false;
        }
</code></pre>
    Then the <code>DestroyContents()</code>, <code>CloneContents()</code> and <code>CopyContents()</code> functions
    are declared and must be implemented by the programmer. See the
    <code>PContainer</code> class for more information on these functions.
 */
#define PCONTAINERINFO(cls, par) \
    PCLASSINFO(cls, par) \
  public: \
    cls(const cls & c) : par(c) { CopyContents(c); } \
    cls & operator=(const cls & c) \
      { AssignContents(c); return *this; } \
    virtual ~cls() { Destruct(); } \
    virtual PBoolean MakeUnique() \
      { if(par::MakeUnique())return true; CloneContents(this);return false; } \
  protected: \
    cls(int dummy, const cls * c) : par(dummy, c) { CloneContents(c); } \
    virtual void DestroyContents(); \
    void CloneContents(const cls * c); \
    void CopyContents(const cls & c); \
    virtual void AssignContents(const PContainer & c) \
      { par::AssignContents(c); CopyContents((const cls &)c); }


///////////////////////////////////////////////////////////////////////////////
// Abstract collection of objects class

/**A collection is a container that collects together descendents of the
   <code>PObject</code> class. The objects contained in the collection are always
   pointers to objects, not the objects themselves. The life of an object in
   the collection should be carefully considered. Typically, it is allocated
   by the user of the collection when it is added. The collection then
   automatically deletes it when it is removed or the collection is destroyed,
   ie when the container class has no more references to the collection. Other
   models may be accommodated but it is up to the programmer to determine the
   scope and life of the objects.

   The exact form of the collection depends on the descendent of PCollection
   and determines the access modes for the objects in it. Thus a collection
   can be an array which allows fast random access at the expense of slow
   insertion and deletion. Or the collection may be a list which has fast
   insertion and deletion but very slow random access.

   The basic paradigm of all collections is the "virtual array". Regardless of
   the internal implementation of the collection; array, list, sorted list etc,
   the user may access elements via an ordinal index. The implementation then
   optimises the access as best it can. For instance, in a list ordinal zero
   will go directly to the head of the list. Stepping along sequential indexes
   then will return the next element of the list, remembering the new position
   at each step, thus allowing sequential access with little overhead as is
   expected for lists. If a random location is specified, then the list
   implementation must sequentially search for that ordinal from either the
   last location or an end of the list, incurring an overhead.

   All collection classes implement a base set of functions, though they may
   be meaningless or degenerative in some collection types eg <code>Insert()</code>
   for <code>PSortedList</code> will degenerate to be the same as <code>Append()</code>.
 */
class PCollection : public PContainer
{
  PCLASSINFO(PCollection, PContainer);

  public:
  /**@name Construction */
  //@{
    /**Create a new collection
     */
    PCollection(
      PINDEX initialSize = 0  ///< Initial number of things in the collection.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Print the collection on the stream. This simply executes the
       <code>PObject::PrintOn()</code> function on each element in the
       collection.

       The default behaviour for collections is to print each element
       separated by the stream fill character. Note that if the fill
       character is the default ' ' then no separator is printed at all.

       Also if the fill character is not ' ', the the streams width parameter
       is set before each individual element of the colllection.

       @return the stream printed to.
     */
    virtual void PrintOn(
      ostream &strm   ///< Output stream to print the collection.
    ) const;
  //@}

  /**@name Common functions for collections */
  //@{
    /**Append a new object to the collection.
    
       The exact semantics depends on the specific type of the collection. So
       the function may not place the object at the "end" of the collection at
       all. For example, in a <code>PSortedList</code> the object is placed in the
       correct ordinal position in the list.

       @return index of the newly added object.
     */
    virtual PINDEX Append(
      PObject * obj   ///< New object to place into the collection.
    ) = 0;

    /**Insert a new object immediately before the specified object. If the
       object to insert before is not in the collection then the equivalent of
       the <code>Append()</code> function is performed.
       
       The exact semantics depends on the specific type of the collection. So
       the function may not place the object before the specified object at
       all. For example, in a <code>PSortedList</code> the object is placed in the
       correct ordinal position in the list.

       Note that the object values are compared for the search of the
       <code>before</code> parameter, not the pointers. So the objects in the
       collection must correctly implement the <code>PObject::Compare()</code>
       function.

       @return index of the newly inserted object.
     */
    virtual PINDEX Insert(
      const PObject & before,   ///< Object value to insert before.
      PObject * obj             ///< New object to place into the collection.
    ) = 0;

    /**Insert a new object at the specified ordinal index. If the index is
       greater than the number of objects in the collection then the
       equivalent of the <code>Append()</code> function is performed.

       The exact semantics depends on the specific type of the collection. So
       the function may not place the object at the specified index at all.
       For example, in a <code>PSortedList</code> the object is placed in the correct
       ordinal position in the list.

       @return index of the newly inserted object.
     */
    virtual PINDEX InsertAt(
      PINDEX index,   ///< Index position in collection to place the object.
      PObject * obj   ///< New object to place into the collection.
    ) = 0;

    /**Remove the object from the collection. If the AllowDeleteObjects option
       is set then the object is also deleted.

       Note that the comparison for searching for the object in collection is
       made by pointer, not by value. Thus the parameter must point to the
       same instance of the object that is in the collection.

       @return true if the object was in the collection.
     */
    virtual PBoolean Remove(
      const PObject * obj   ///< Existing object to remove from the collection.
    ) = 0;

    /**Remove the object at the specified ordinal index from the collection.
       If the AllowDeleteObjects option is set then the object is also deleted.

       Note if the index is beyond the size of the collection then the
       function will assert.

       @return pointer to the object being removed, or NULL if it was deleted.
     */
    virtual PObject * RemoveAt(
      PINDEX index   ///< Index position in collection to place the object.
    ) = 0;

    /**Remove all of the elements in the collection. This operates by
       continually calling <code>RemoveAt()</code> until there are no objects left.

       The objects are removed from the last, at index
       <code>(GetSize()-1)</code> toward the first at index zero.
     */
    virtual void RemoveAll();

    /**Set the object at the specified ordinal position to the new value. This
       will overwrite the existing entry. If the AllowDeleteObjects option is
       set then the old object is also deleted.

       The exact semantics depends on the specific type of the collection. For
       some, eg <code>PSortedList</code>, the object inserted will not stay at the
       ordinal position. Also the exact behaviour when the index is greater
       than the size of the collection depends on the collection type, eg in
       an array collection the array is expanded to accommodate the new index,
       whereas in a list it will return false.

       @return true if the object was successfully added.
     */
    virtual PBoolean SetAt(
      PINDEX index,   ///< Index position in collection to set.
      PObject * val   ///< New value to place into the collection.
    ) = 0;

    /**Get the object at the specified ordinal position. If the index was
       greater than the size of the collection then NULL is returned.

       @return pointer to object at the specified index.
     */
    virtual PObject * GetAt(
      PINDEX index  ///< Index position in the collection of the object.
    ) const = 0;

    /**Search the collection for the specific instance of the object. The
       object pointers are compared, not the values. The fastest search
       algorithm is employed depending on the collection type.

       @return ordinal index position of the object, or P_MAX_INDEX.
     */
    virtual PINDEX GetObjectsIndex(
      const PObject * obj  ///< Object to search for.
    ) const = 0;

    /**Search the collection for the specified value of the object. The object
       values are compared, not the pointers.  So the objects in the
       collection must correctly implement the <code>PObject::Compare()</code>
       function. The fastest search algorithm is employed depending on the
       collection type.

       @return ordinal index position of the object, or P_MAX_INDEX.
     */
    virtual PINDEX GetValuesIndex(
      const PObject & obj  ///< Object to search for.
    ) const = 0;

    /**Allow or disallow the deletion of the objects contained in the
       collection. If true then whenever an object is removed, overwritten or
       the colelction is deleted due to all references being destroyed, the
       object is deleted.

       For example:
<pre><code>
              coll.SetAt(2, new PString("one"));
              coll.SetAt(2, new PString("Two"));
</code></pre>
       would automatically delete the string containing "one" on the second
       call to SetAt().
     */
    PINLINE void AllowDeleteObjects(
      PBoolean yes = true   ///< New value for flag for deleting objects
    );

    /**Disallow the deletion of the objects contained in the collection. See
       the <code>AllowDeleteObjects()</code> function for more details.
     */
    void DisallowDeleteObjects();
  //@}

  protected:
    /**Constructor used in support of the Clone() function. This creates a
       new unique reference of a copy of the contents. It does {\b not}
       create another reference.

       The dummy parameter is there to prevent the contructor from being
       invoked automatically by the compiler when a pointer is used by accident
       when a normal instance or reference was expected. The container would
       be silently cloned and the copy used instead of the container expected
       leading to unpredictable results.
     */
    PINLINE PCollection(
      int dummy,        ///< Dummy to prevent accidental use of the constructor.
      const PCollection * coll  ///< Collection class to clone.
    );
};



///////////////////////////////////////////////////////////////////////////////
// The abstract array class

#include <ptlib/array.h>

///////////////////////////////////////////////////////////////////////////////
// The abstract array class

#include <ptlib/lists.h>

///////////////////////////////////////////////////////////////////////////////
// PString class (specialised version of PBASEARRAY(char))

#include <ptlib/dict.h>


///////////////////////////////////////////////////////////////////////////////
// PString class

#include <ptlib/pstring.h>



///////////////////////////////////////////////////////////////////////////////
// Fill in all the inline functions

#if P_USE_INLINES
#include <ptlib/contain.inl>
#endif


#endif // PTLIB_CONTAIN_H


// End Of File ///////////////////////////////////////////////////////////////
