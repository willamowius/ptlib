/*
 * collect.cxx
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


PDEFINE_POOL_ALLOCATOR(PListElement)
PDEFINE_POOL_ALLOCATOR(PListInfo)
PDEFINE_POOL_ALLOCATOR(PSortedListElement)
PDEFINE_POOL_ALLOCATOR(PSortedListInfo)
PDEFINE_POOL_ALLOCATOR(PHashTableElement)


#define new PNEW
#undef  __CLASS__
#define __CLASS__ GetClass()


///////////////////////////////////////////////////////////////////////////////

void PCollection::PrintOn(ostream &strm) const
{
  char separator = strm.fill();
  int width = (int)strm.width();
  for (PINDEX  i = 0; i < GetSize(); i++) {
    if (i > 0 && separator != ' ')
      strm << separator;
    PObject * obj = GetAt(i);
    if (obj != NULL) {
      if (separator != ' ')
        strm.width(width);
      strm << *obj;
    }
  }
  if (separator == '\n')
    strm << '\n';
}


void PCollection::RemoveAll()
{
  while (GetSize() > 0)
    RemoveAt(0);
}


///////////////////////////////////////////////////////////////////////////////

void PArrayObjects::CopyContents(const PArrayObjects & array)
{
  theArray = array.theArray;
}


void PArrayObjects::DestroyContents()
{
  if (reference->deleteObjects && theArray != NULL) {
    for (PINDEX i = 0; i < theArray->GetSize(); i++) {
      if ((*theArray)[i] != NULL)
        delete (*theArray)[i];
    }
  }
  delete theArray;
  theArray = NULL;
}


void PArrayObjects::RemoveAll()
{
  SetSize(0);
}


void PArrayObjects::CloneContents(const PArrayObjects * array)
{
  PBaseArray<PObject *> & oldArray = *array->theArray;
  theArray = new PBaseArray<PObject *>(oldArray.GetSize());
  for (PINDEX i = 0; i < GetSize(); i++) {
    PObject * ptr = oldArray[i];
    if (ptr != NULL)
      SetAt(i, ptr->Clone());
  }
}


PObject::Comparison PArrayObjects::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PArrayObjects), PInvalidCast);
  const PArrayObjects & other = (const PArrayObjects &)obj;
  PINDEX i;
  for (i = 0; i < GetSize(); i++) {
    if (i >= other.GetSize() || *(*theArray)[i] < *(*other.theArray)[i])
      return LessThan;
    if (*(*theArray)[i] > *(*other.theArray)[i])
      return GreaterThan;
  }
  return i < other.GetSize() ? GreaterThan : EqualTo;
}


PINDEX PArrayObjects::GetSize() const
{
  return theArray->GetSize();
}


PBoolean PArrayObjects::SetSize(PINDEX newSize)
{
  PINDEX sz = theArray->GetSize();
  if (reference->deleteObjects && sz > 0) {
    for (PINDEX i = sz; i > newSize; i--) {
      PObject * obj = theArray->GetAt(i-1);
      if (obj != NULL)
        delete obj;
    }
  }
  return theArray->SetSize(newSize);
}


PINDEX PArrayObjects::Append(PObject * obj)
{
  PINDEX where = GetSize();
  SetAt(where, obj);
  return where;
}


PINDEX PArrayObjects::Insert(const PObject & before, PObject * obj)
{
  PINDEX where = GetObjectsIndex(&before);
  InsertAt(where, obj);
  return where;
}


PBoolean PArrayObjects::Remove(const PObject * obj)
{
  PINDEX i = GetObjectsIndex(obj);
  if (i == P_MAX_INDEX)
    return PFalse;
  RemoveAt(i);
  return PTrue;
}


PObject * PArrayObjects::GetAt(PINDEX index) const
{
  return (*theArray)[index];
}


PBoolean PArrayObjects::SetAt(PINDEX index, PObject * obj)
{
  if (!theArray->SetMinSize(index+1))
    return PFalse;
  PObject * oldObj = theArray->GetAt(index);
  if (oldObj != NULL && reference->deleteObjects)
    delete oldObj;
  (*theArray)[index] = obj;
  return PTrue;
}


PINDEX PArrayObjects::InsertAt(PINDEX index, PObject * obj)
{
  PINDEX i = GetSize();
  SetSize(i+1);
  for (; i > index; i--)
    (*theArray)[i] = (*theArray)[i-1];
  (*theArray)[index] = obj;
  return index;
}


PObject * PArrayObjects::RemoveAt(PINDEX index)
{
  PObject * obj = (*theArray)[index];

  PINDEX size = GetSize()-1;
  PINDEX i;
  for (i = index; i < size; i++)
    (*theArray)[i] = (*theArray)[i+1];
  (*theArray)[i] = NULL;

  SetSize(size);

  if (obj != NULL && reference->deleteObjects) {
    delete obj;
    obj = NULL;
  }

  return obj;
}


PINDEX PArrayObjects::GetObjectsIndex(const PObject * obj) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    if ((*theArray)[i] == obj)
      return i;
  }
  return P_MAX_INDEX;
}


PINDEX PArrayObjects::GetValuesIndex(const PObject & obj) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    PObject * elmt = (*theArray)[i];
    if (elmt != NULL && *elmt == obj)
      return i;
  }
  return P_MAX_INDEX;
}


///////////////////////////////////////////////////////////////////////////////

void PAbstractList::DestroyContents()
{
  RemoveAll();
  delete info;
  info = NULL;
}


void PAbstractList::CopyContents(const PAbstractList & list)
{
  info = list.info;
}


void PAbstractList::CloneContents(const PAbstractList * list)
{
  Element * element = list->info->head;

  info = new PListInfo;
  PAssert(info != NULL, POutOfMemory);

  while (element != NULL) {
    Element * newElement = new Element(element->data->Clone());

    if (info->head == NULL)
      info->head = info->tail = newElement;
    else {
      newElement->prev = info->tail;
      info->tail->next = newElement;
      info->tail = newElement;
    }

    element = element->next;
  }
}


PObject::Comparison PAbstractList::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PAbstractList), PInvalidCast);
  Element * elmt1 = info->head;
  Element * elmt2 = ((const PAbstractList &)obj).info->head;
  while (elmt1 != NULL || elmt2 != NULL) {
    if (elmt1 == NULL)
      return LessThan;
    if (elmt2 == NULL)
      return GreaterThan;
    if (*elmt1->data < *elmt2->data)
      return LessThan;
    if (*elmt1->data > *elmt2->data)
      return GreaterThan;
    elmt1 = elmt1->next;
    elmt2 = elmt2->next;
  }
  return EqualTo;
}


PBoolean PAbstractList::SetSize(PINDEX)
{
  return PTrue;
}


PINDEX PAbstractList::Append(PObject * obj)
{
  if (PAssertNULL(obj) == NULL)
    return P_MAX_INDEX;

  Element * element = new Element(obj);
  if (info->tail != NULL)
    info->tail->next = element;
  element->prev = info->tail;
  element->next = NULL;
  if (info->head == NULL)
    info->head = element;
  info->tail = element;

  PINDEX lastIndex = GetSize();
  reference->size++;
  return lastIndex;
}


PINDEX PAbstractList::Insert(const PObject & before, PObject * obj)
{
  if (PAssertNULL(obj) == NULL)
    return P_MAX_INDEX;
  
  PINDEX where = GetObjectsIndex(&before);
  InsertAt(where, obj);
  return where;
}


PINDEX PAbstractList::InsertAt(PINDEX index, PObject * obj)
{
  if (PAssertNULL(obj) == NULL)
    return P_MAX_INDEX;
  
  if (index >= GetSize())
    return Append(obj);

  Element * lastElement;
  PAssert(SetCurrent(index, lastElement), PInvalidArrayIndex);

  Element * newElement = new Element(obj);
  if (lastElement->prev != NULL)
    lastElement->prev->next = newElement;
  else
    info->head = newElement;
  newElement->prev = lastElement->prev;
  newElement->next = lastElement;
  lastElement->prev = newElement;

  reference->size++;
  return index;
}


PBoolean PAbstractList::Remove(const PObject * obj)
{
  if (info == NULL){
    PAssertAlways("info is null");
    return false;
  }

  Element * elmt = info->head;
  while (elmt != NULL) {
    if (elmt->data == obj) {
      RemoveElement(elmt);
      return true;
    }
    elmt = elmt->next;
  }
  
  return false;
}


PObject * PAbstractList::RemoveAt(PINDEX index)
{
  if (info == NULL){
    PAssertAlways("info is null");
    return NULL;
  }

  Element * elmt;
  if (!SetCurrent(index, elmt)) {
    PAssertAlways(PInvalidArrayIndex);
    return NULL;
  }

  return RemoveElement(elmt);
}


PObject * PAbstractList::RemoveElement(PListElement * elmt)
{
  if (elmt == NULL){
    PAssertAlways("elmt is null");
    return NULL;
  }
  
  if (elmt->prev != NULL)
    elmt->prev->next = elmt->next;
  else {
    info->head = elmt->next;
    if (info->head != NULL)
      info->head->prev = NULL;
  }

  if (elmt->next != NULL)
    elmt->next->prev = elmt->prev;
  else {
    info->tail = elmt->prev;
    if (info->tail != NULL)
      info->tail->next = NULL;
  }

  if((reference == NULL) || (reference->size == 0)){
    PAssertAlways("reference is null or reference->size == 0");
    return NULL;
  }
  reference->size--;

  PObject * obj = elmt->data;
  if (obj != NULL && reference->deleteObjects) {
    delete obj;
    obj = NULL;
  }
  delete elmt;
  return obj;
}


PObject * PAbstractList::GetAt(PINDEX index) const
{
  Element * lastElement;
  return SetCurrent(index, lastElement) ? lastElement->data : (PObject *)NULL;
}


PBoolean PAbstractList::SetAt(PINDEX index, PObject * val)
{
  Element * lastElement;
  if (!SetCurrent(index, lastElement))
    return PFalse;
  lastElement->data = val;
  return PTrue;
}

PBoolean PAbstractList::ReplaceAt(PINDEX index, PObject * val)
{
  Element * lastElement;
  if (!SetCurrent(index, lastElement))
    return PFalse;
  
  if (lastElement->data != NULL && reference->deleteObjects) {
    delete lastElement->data;
  }

  lastElement->data = val;
  return PTrue;
}

PINDEX PAbstractList::GetObjectsIndex(const PObject * obj) const
{
  PINDEX index = 0;
  Element * element = info->head;

  while (element != NULL) {
    if (element->data == obj) 
      return index;
    element = element->next;
    index++;
  }

  return P_MAX_INDEX;
}


PINDEX PAbstractList::GetValuesIndex(const PObject & obj) const
{
  PINDEX index = 0;
  Element * element = info->head;
  while (element != NULL) {
    if (*element->data == obj)
      return index;
    element = element->next;
    index++;
  }

  return P_MAX_INDEX;
}


PBoolean PAbstractList::SetCurrent(PINDEX index, Element * & lastElement) const
{
  if (index >= GetSize())
    return PFalse;

  PINDEX lastIndex;
  if (index < GetSize()/2) {
    lastIndex = 0;
    lastElement = info->head;
  }
  else {
    lastIndex = GetSize()-1;
    lastElement = info->tail;
  }

  while (lastIndex < index) {
    lastElement = lastElement->next;
    ++lastIndex;
  }

  while (lastIndex > index) {
    lastElement = lastElement->prev;
    --lastIndex;
  }

  return PTrue;
}


PListElement::PListElement(PObject * theData)
{
  next = prev = NULL;
  data = theData;
}


///////////////////////////////////////////////////////////////////////////////

PAbstractSortedList::PAbstractSortedList()
{
  info = new PSortedListInfo;
  PAssert(info != NULL, POutOfMemory);
}


PSortedListInfo::PSortedListInfo()
{
  root = &nil;
  nil.parent = nil.left = nil.right = &nil;
  nil.subTreeSize = 0;
  nil.colour = Element::Black;
  nil.data = NULL;
}


void PAbstractSortedList::DestroyContents()
{
  RemoveAll();
  delete info;
  info = NULL;
}


void PAbstractSortedList::CopyContents(const PAbstractSortedList & list)
{
  info = list.info;
}


void PAbstractSortedList::CloneContents(const PAbstractSortedList * list)
{
  PSortedListInfo * otherInfo = list->info;

  info = new PSortedListInfo;
  PAssert(info != NULL, POutOfMemory);
  reference->size = 0;

  // Have to do this in this manner rather than just doing a for() loop
  // as "this" and "list" may be the same object and we just changed info in
  // "this" so we need to use the info in "list" saved previously.
  Element * element = otherInfo->OrderSelect(otherInfo->root, 1);
  while (element != &otherInfo->nil) {
    Append(element->data->Clone());
    element = otherInfo->Successor(element);
  }
}


PBoolean PAbstractSortedList::SetSize(PINDEX)
{
  return PTrue;
}


PObject::Comparison PAbstractSortedList::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PAbstractSortedList), PInvalidCast);
  Element * elmt1 = info->root;
  while (elmt1->left != &info->nil)
    elmt1 = elmt1->left;

  Element * elmt2 = ((const PAbstractSortedList &)obj).info->root;
  while (elmt2->left != &info->nil)
    elmt2 = elmt2->left;

  while (elmt1 != &info->nil && elmt2 != &info->nil) {
    if (elmt1 == &info->nil)
      return LessThan;
    if (elmt2 == &info->nil)
      return GreaterThan;
    if (*elmt1->data < *elmt2->data)
      return LessThan;
    if (*elmt1->data > *elmt2->data)
      return GreaterThan;
    elmt1 = info->Successor(elmt1);
    elmt2 = info->Successor(elmt2);
  }
  return EqualTo;
}


PINDEX PAbstractSortedList::Append(PObject * obj)
{
  if (PAssertNULL(obj) == NULL)
    return P_MAX_INDEX;

  Element * z = new Element;
  z->parent = z->left = z->right = &info->nil;
  z->colour = Element::Black;
  z->subTreeSize = 1;
  z->data = obj;

  Element * x = info->root;
  Element * y = &info->nil;
  while (x != &info->nil) {
    x->subTreeSize++;
    y = x;
    x = *z->data < *x->data ? x->left : x->right;
  }
  z->parent = y;
  if (y == &info->nil)
    info->root = z;
  else if (*z->data < *y->data)
    y->left = z;
  else
    y->right = z;

  PSortedListElement * lastElement = x = z;
  PINDEX lastIndex;

  x->colour = Element::Red;
  while (x != info->root && x->parent->colour == Element::Red) {
    if (x->parent == x->parent->parent->left) {
      y = x->parent->parent->right;
      if (y->colour == Element::Red) {
        x->parent->colour = Element::Black;
        y->colour = Element::Black;
        x->parent->parent->colour = Element::Red;
        x = x->parent->parent;
      }
      else {
        if (x == x->parent->right) {
          x = x->parent;
          LeftRotate(x);
        }
        x->parent->colour = Element::Black;
        x->parent->parent->colour = Element::Red;
        RightRotate(x->parent->parent);
      }
    }
    else {
      y = x->parent->parent->left;
      if (y->colour == Element::Red) {
        x->parent->colour = Element::Black;
        y->colour = Element::Black;
        x->parent->parent->colour = Element::Red;
        x = x->parent->parent;
      }
      else {
        if (x == x->parent->left) {
          x = x->parent;
          RightRotate(x);
        }
        x->parent->colour = Element::Black;
        x->parent->parent->colour = Element::Red;
        LeftRotate(x->parent->parent);
      }
    }
  }

  info->root->colour = Element::Black;

  x = lastElement;
  lastIndex = x->left->subTreeSize;
  while (x != info->root) {
    if (x != x->parent->left)
      lastIndex += x->parent->left->subTreeSize+1;
    x = x->parent;
  }

  reference->size++;
  return lastIndex;
}


PBoolean PAbstractSortedList::Remove(const PObject * obj)
{
  PSortedListElement * lastElement;
  if (GetObjectsIndex(obj, lastElement) == P_MAX_INDEX)
    return PFalse;

  RemoveElement(lastElement);
  return PTrue;
}


PObject * PAbstractSortedList::RemoveAt(PINDEX index)
{
  Element * node = info->OrderSelect(info->root, index+1);
  if (node == &info->nil)
    return NULL;

  PObject * data = node->data;
  RemoveElement(node);
  return reference->deleteObjects ? (PObject *)NULL : data;
}


void PAbstractSortedList::RemoveAll()
{
  if (info->root != &info->nil) {
    DeleteSubTrees(info->root, reference->deleteObjects);
    delete info->root;
    info->root = &info->nil;
    reference->size = 0;
  }
}


PINDEX PAbstractSortedList::Insert(const PObject &, PObject * obj)
{
  return Append(obj);
}


PINDEX PAbstractSortedList::InsertAt(PINDEX, PObject * obj)
{
  return Append(obj);
}


PBoolean PAbstractSortedList::SetAt(PINDEX, PObject *)
{
  return PFalse;
}


PObject * PAbstractSortedList::GetAt(PINDEX index) const
{
  if (index >= GetSize())
    return NULL;

  PSortedListElement * lastElement = info->OrderSelect(info->root, index+1);
  return PAssertNULL(lastElement)->data;
}


PINDEX PAbstractSortedList::GetObjectsIndex(const PObject * obj) const
{
  PSortedListElement * lastElement;
  return PAbstractSortedList::GetObjectsIndex(obj, lastElement);
}

PINDEX PAbstractSortedList::GetObjectsIndex(const PObject * obj, PSortedListElement * & lastElement) const
{
  Element * elmt = NULL;
  PINDEX pos = ValueSelect(info->root, *obj, (const Element **)&elmt);
  if (pos == P_MAX_INDEX)
    return P_MAX_INDEX;

  if (elmt->data != obj) {
    PINDEX savePos = pos;
    Element * saveElmt = elmt;
    while (elmt->data != obj &&
            (elmt = info->Predecessor(elmt)) != &info->nil &&
            *obj == *elmt->data)
      pos--;
    if (elmt->data != obj) {
      pos = savePos;
      elmt = saveElmt;
      while (elmt->data != obj &&
              (elmt = info->Successor(elmt)) != &info->nil &&
              *obj == *elmt->data)
        pos++;
      if (elmt->data != obj)
        return P_MAX_INDEX;
    }
  }

  lastElement = elmt;

  return pos;
}


PINDEX PAbstractSortedList::GetValuesIndex(const PObject & obj) const
{
  PSortedListElement * lastElement;
  PINDEX lastIndex = ValueSelect(info->root, obj, (const Element **)&lastElement);
  if (lastIndex == P_MAX_INDEX)
    return P_MAX_INDEX;

  Element * prev;
  while ((prev = info->Predecessor(lastElement)) != &info->nil &&
                                  prev->data->Compare(obj) == EqualTo) {
    lastElement = prev;
    lastIndex--;
  }

  return lastIndex;
}


void PAbstractSortedList::RemoveElement(Element * node)
{
  // Don't try an remove one of the special leaf nodes!
  if (PAssertNULL(node) == &info->nil)
    return;

  if (node->data != NULL && reference->deleteObjects)
    delete node->data;

  Element * y = node->left == &info->nil || node->right == &info->nil ? node : info->Successor(node);

  Element * t = y;
  while (t != &info->nil) {
    t->subTreeSize--;
    t = t->parent;
  }

  Element * x = y->left != &info->nil ? y->left : y->right;
  x->parent = y->parent;

  if (y->parent == &info->nil)
    info->root = x;
  else if (y == y->parent->left)
    y->parent->left = x;
  else
    y->parent->right = x;

  if (y != node)
    node->data = y->data;

  if (y->colour == Element::Black) {
    while (x != info->root && x->colour == Element::Black) {
      if (x == x->parent->left) {
        Element * w = x->parent->right;
        if (w->colour == Element::Red) {
          w->colour = Element::Black;
          x->parent->colour = Element::Red;
          LeftRotate(x->parent);
          w = x->parent->right;
        }
        if (w->left->colour == Element::Black && w->right->colour == Element::Black) {
          w->colour = Element::Red;
          x = x->parent;
        }
        else {
          if (w->right->colour == Element::Black) {
            w->left->colour = Element::Black;
            w->colour = Element::Red;
            RightRotate(w);
            w = x->parent->right;
          }
          w->colour = x->parent->colour;
          x->parent->colour = Element::Black;
          w->right->colour = Element::Black;
          LeftRotate(x->parent);
          x = info->root;
        }
      }
      else {
        Element * w = x->parent->left;
        if (w->colour == Element::Red) {
          w->colour = Element::Black;
          x->parent->colour = Element::Red;
          RightRotate(x->parent);
          w = x->parent->left;
        }
        if (w->right->colour == Element::Black && w->left->colour == Element::Black) {
          w->colour = Element::Red;
          x = x->parent;
        }
        else {
          if (w->left->colour == Element::Black) {
            w->right->colour = Element::Black;
            w->colour = Element::Red;
            LeftRotate(w);
            w = x->parent->left;
          }
          w->colour = x->parent->colour;
          x->parent->colour = Element::Black;
          w->left->colour = Element::Black;
          RightRotate(x->parent);
          x = info->root;
        }
      }
    }
    x->colour = Element::Black;
  }

  delete y;

  reference->size--;
}


void PAbstractSortedList::LeftRotate(Element * node)
{
  Element * pivot = PAssertNULL(node)->right;
  node->right = pivot->left;
  if (pivot->left != &info->nil)
    pivot->left->parent = node;
  pivot->parent = node->parent;
  if (node->parent == &info->nil)
    info->root = pivot;
  else if (node == node->parent->left)
    node->parent->left = pivot;
  else
    node->parent->right = pivot;
  pivot->left = node;
  node->parent = pivot;
  pivot->subTreeSize = node->subTreeSize;
  node->subTreeSize = node->left->subTreeSize + node->right->subTreeSize + 1;
}


void PAbstractSortedList::RightRotate(Element * node)
{
  Element * pivot = PAssertNULL(node)->left;
  node->left = pivot->right;
  if (pivot->right != &info->nil)
    pivot->right->parent = node;
  pivot->parent = node->parent;
  if (node->parent == &info->nil)
    info->root = pivot;
  else if (node == node->parent->right)
    node->parent->right = pivot;
  else
    node->parent->left = pivot;
  pivot->right = node;
  node->parent = pivot;
  pivot->subTreeSize = node->subTreeSize;
  node->subTreeSize = node->left->subTreeSize + node->right->subTreeSize + 1;
}


PSortedListElement * PSortedListInfo::Successor(const PSortedListElement * node) const
{
  Element * next;
  if (node->right != &nil) {
    next = node->right;
    while (next->left != &nil)
      next = next->left;
  }
  else {
    next = node->parent;
    while (next != &nil && node == next->right) {
      node = next;
      next = node->parent;
    }
  }
  return next;
}


PSortedListElement * PSortedListInfo::Predecessor(const PSortedListElement * node) const
{
  Element * pred;
  if (node->left != &nil) {
    pred = node->left;
    while (pred->right != &nil)
      pred = pred->right;
  }
  else {
    pred = node->parent;
    while (pred != &nil && node == pred->left) {
      node = pred;
      pred = node->parent;
    }
  }
  return pred;
}


PSortedListElement * PSortedListInfo::OrderSelect(PSortedListElement * node, PINDEX index) const
{
  PINDEX r = node->left->subTreeSize+1;
  if (index == r)
    return node;

  if (index < r) {
    if (node->left != &nil)
      return OrderSelect(node->left, index);
  }
  else {
    if (node->right != &nil)
      return OrderSelect(node->right, index - r);
  }

  PAssertAlways2("PAbstractSortedList::Element", "Order select failed!");
  return (Element *)&nil;
}


PINDEX PAbstractSortedList::ValueSelect(const Element * node,
                                        const PObject & obj,
                                        const Element ** lastElement) const
{
  if (node != &info->nil) {
    switch (node->data->Compare(obj)) {
      case PObject::LessThan :
      {
        PINDEX index = ValueSelect(node->right, obj, lastElement);
        if (index != P_MAX_INDEX)
          return node->left->subTreeSize + index + 1;
        break;
      }

      case PObject::GreaterThan :
        return ValueSelect(node->left, obj, lastElement);

      default :
        *lastElement = node;
        return node->left->subTreeSize;
    }
  }

  return P_MAX_INDEX;
}


void PAbstractSortedList::DeleteSubTrees(Element * node, PBoolean deleteObject)
{
  if (node->left != &info->nil) {
    DeleteSubTrees(node->left, deleteObject);
    delete node->left;
    node->left = &info->nil;
  }
  if (node->right != &info->nil) {
    DeleteSubTrees(node->right, deleteObject);
    delete node->right;
    node->right = &info->nil;
  }
  if (deleteObject) {
    delete node->data;
    node->data = NULL;
  }
}


///////////////////////////////////////////////////////////////////////////////

PObject * POrdinalKey::Clone() const
{
  return new POrdinalKey(theKey);
}


PObject::Comparison POrdinalKey::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, POrdinalKey), PInvalidCast);
  const POrdinalKey & other = (const POrdinalKey &)obj;
  
  if (theKey < other.theKey)
    return LessThan;

  if (theKey > other.theKey)
    return GreaterThan;

  return EqualTo;
}


PINDEX POrdinalKey::HashFunction() const
{
  return PABSINDEX(theKey)%23;
}


void POrdinalKey::PrintOn(ostream & strm) const
{
  strm << theKey;
}


///////////////////////////////////////////////////////////////////////////////

void PHashTableInfo::DestroyContents()
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    Element * list = GetAt(i);
    if (list != NULL) {
      Element * elmt = list;
      do {
        Element * nextElmt = elmt->next;
        if (elmt->data != NULL && reference->deleteObjects)
          delete elmt->data;
        if (deleteKeys)
          delete elmt->key;
        delete elmt;
        elmt = nextElmt;
      } while (elmt != list);
    }
  }
  PAbstractArray::DestroyContents();
}


PINDEX PHashTableInfo::AppendElement(PObject * key, PObject * data)
{
  PINDEX bucket = PAssertNULL(key)->HashFunction();
  Element * list = GetAt(bucket);
  Element * element = new Element;
  PAssert(element != NULL, POutOfMemory);
  element->key = key;
  element->data = data;
  if (list == NULL) {
    element->next = element->prev = element;
    SetAt(bucket, element);
  }
  else if (list == list->prev) {
    list->next = list->prev = element;
    element->next = element->prev = list;
  }
  else {
    element->next = list;
    element->prev = list->prev;
    list->prev->next = element;
    list->prev = element;
  }
  return bucket;
}


PObject * PHashTableInfo::RemoveElement(const PObject & key)
{
  PObject * obj = NULL;
  Element * lastElement = GetElementAt(key);
  if (lastElement != NULL) {
    if (lastElement == lastElement->prev)
      SetAt(key.HashFunction(), NULL);
    else {
      lastElement->prev->next = lastElement->next;
      lastElement->next->prev = lastElement->prev;
      SetAt(key.HashFunction(), lastElement->next);
    }
    obj = lastElement->data;
    if (deleteKeys)
      delete lastElement->key;
    delete lastElement;
  }
  return obj;
}


PBoolean PHashTableInfo::SetLastElementAt(PINDEX index, PHashTableElement * & lastElement)
{
  PINDEX lastBucket = 0;
  while ((lastElement = GetAt(lastBucket)) == NULL) {
    if (lastBucket >= GetSize())
      return FALSE;
    lastBucket++;
  }

  PINDEX lastIndex = 0;

  if (lastIndex < index) {
    while (lastIndex != index) {
      if (lastElement->next != operator[](lastBucket))
        lastElement = lastElement->next;
      else {
        do {
          if (++lastBucket >= GetSize())
            return PFalse;
        } while ((lastElement = operator[](lastBucket)) == NULL);
      }
      lastIndex++;
    }
  }
  else {
    while (lastIndex != index) {
      if (lastElement != operator[](lastBucket))
        lastElement = lastElement->prev;
      else {
        do {
          if (lastBucket-- == 0)
            return PFalse;
        } while ((lastElement = operator[](lastBucket)) == NULL);
        lastElement = lastElement->prev;
      }
      lastIndex--;
    }
  }

  return PTrue;
}


PHashTableElement * PHashTableInfo::GetElementAt(const PObject & key)
{
  Element * list = GetAt(key.HashFunction());
  if (list != NULL) {
    Element * element = list;
    do {
      if (*element->key == key) 
        return element;
      element = element->next;
    } while (element != list);
  }
  return NULL;
}


PINDEX PHashTableInfo::GetElementsIndex(
                           const PObject * obj, PBoolean byValue, PBoolean keys) const
{
  PINDEX index = 0;
  for (PINDEX i = 0; i < GetSize(); i++) {
    Element * list = operator[](i);
    if (list != NULL) {
      Element * element = list;
      do {
        PObject * keydata = keys ? element->key : element->data;
        if (byValue ? (*keydata == *obj) : (keydata == obj))
          return index;
        element = element->next;
        index++;
      } while (element != list);
    }
  }
  return P_MAX_INDEX;
}


///////////////////////////////////////////////////////////////////////////////

PHashTable::PHashTable()
  : hashTable(new PHashTable::Table)
{
  PAssert(hashTable != NULL, POutOfMemory);
}


void PHashTable::DestroyContents()
{
  if (hashTable != NULL) {
    hashTable->reference->deleteObjects = reference->deleteObjects;
    delete hashTable;
    hashTable = NULL;
  }
}


void PHashTable::CopyContents(const PHashTable & hash)
{
  hashTable = hash.hashTable;
}

  
void PHashTable::CloneContents(const PHashTable * hash)
{
  PINDEX sz = PAssertNULL(hash)->GetSize();
  PHashTable::Table * original = PAssertNULL(hash->hashTable);

  hashTable = new PHashTable::Table(original->GetSize());
  PAssert(hashTable != NULL, POutOfMemory);
  hashTable->deleteKeys = original->deleteKeys;

  for (PINDEX i = 0; i < sz; i++) {
    Element * lastElement = NULL;
    original->SetLastElementAt(i, lastElement);
    PObject * data = lastElement->data;
    if (data != NULL)
      data = data->Clone();
    hashTable->AppendElement(lastElement->key->Clone(), data);
  }
}


PObject::Comparison PHashTable::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PHashTable), PInvalidCast);
  return reference != ((const PHashTable &)obj).reference
                                                      ? GreaterThan : EqualTo;
}


PBoolean PHashTable::SetSize(PINDEX)
{
  return PTrue;
}


PObject & PHashTable::AbstractGetDataAt(PINDEX index) const
{
  Element * lastElement;
  PAssert(hashTable->SetLastElementAt(index, lastElement), PInvalidArrayIndex);
  return *lastElement->data;
}


const PObject & PHashTable::AbstractGetKeyAt(PINDEX index) const
{
  Element * lastElement;
  PAssert(hashTable->SetLastElementAt(index, lastElement), PInvalidArrayIndex);
  return *lastElement->key;
}


///////////////////////////////////////////////////////////////////////////////

void PAbstractSet::DestroyContents()
{
  hashTable->deleteKeys = reference->deleteObjects;
  PHashTable::DestroyContents();
}


void PAbstractSet::CopyContents(const PAbstractSet & )
{
}

  
void PAbstractSet::CloneContents(const PAbstractSet * )
{
}


PINDEX PAbstractSet::Append(PObject * obj)
{
  if (AbstractContains(*obj)) {
    if (reference->deleteObjects)
      delete obj;
    return P_MAX_INDEX;
  }

  reference->size++;
  return hashTable->AppendElement(obj, NULL);
}


PINDEX PAbstractSet::Insert(const PObject &, PObject * obj)
{
  return Append(obj);
}


PINDEX PAbstractSet::InsertAt(PINDEX, PObject * obj)
{
  return Append(obj);
}


PBoolean PAbstractSet::Remove(const PObject * obj)
{
  if (PAssertNULL(obj) == NULL)
    return PFalse;

  if (hashTable->GetElementAt(*obj) == NULL)
    return PFalse;

  hashTable->deleteKeys = hashTable->reference->deleteObjects = reference->deleteObjects;
  hashTable->RemoveElement(*obj);
  reference->size--;
  return PTrue;
}


PObject * PAbstractSet::RemoveAt(PINDEX index)
{
  Element * lastElement;
  if (!hashTable->SetLastElementAt(index, lastElement))
    return NULL;

  PObject * obj = lastElement->key;
  hashTable->deleteKeys = hashTable->reference->deleteObjects = reference->deleteObjects;
  hashTable->RemoveElement(*obj);
  reference->size--;
  return obj;
}


PINDEX PAbstractSet::GetObjectsIndex(const PObject * obj) const
{
  return hashTable->GetElementsIndex(obj, PFalse, PTrue);
}


PINDEX PAbstractSet::GetValuesIndex(const PObject & obj) const
{
  return hashTable->GetElementsIndex(&obj, PTrue, PTrue);
}


PObject * PAbstractSet::GetAt(PINDEX index) const
{
  return (PObject *)&AbstractGetKeyAt(index);
}


PBoolean PAbstractSet::SetAt(PINDEX, PObject * obj)
{
  return Append(obj);
}


bool PAbstractSet::Union(const PAbstractSet & set)
{
  bool something = false;
  for (PINDEX i = 0; i < set.GetSize(); ++i) {
    const PObject & obj = set.AbstractGetKeyAt(i);
    if (!AbstractContains(obj)) {
      something = true;
      Append(obj.Clone());
    }
  }
  return something;
}


bool PAbstractSet::Intersection(const PAbstractSet & set1,
                                const PAbstractSet & set2,
                                PAbstractSet * intersection)
{
  bool something = false;
  for (PINDEX i = 0; i < set1.GetSize(); ++i) {
    const PObject & obj = set1.AbstractGetKeyAt(i);
    if (set2.AbstractContains(obj)) {
      something = true;
      if (intersection == NULL)
        break;
      intersection->Append(obj.Clone());
    }
  }
  return something;
}


///////////////////////////////////////////////////////////////////////////////

PINDEX PAbstractDictionary::Append(PObject *)
{
  PAssertAlways(PUnimplementedFunction);
  return 0;
}


PINDEX PAbstractDictionary::Insert(const PObject & before, PObject * obj)
{
  AbstractSetAt(before, obj);
  return 0;
}


PINDEX PAbstractDictionary::InsertAt(PINDEX index, PObject * obj)
{
  AbstractSetAt(AbstractGetKeyAt(index), obj);
  return index;
}
 
 
PBoolean PAbstractDictionary::Remove(const PObject * obj)
{
  PINDEX idx = GetObjectsIndex(obj);
  if (idx == P_MAX_INDEX)
    return PFalse;

  RemoveAt(idx);
  return PTrue;
}


PObject * PAbstractDictionary::RemoveAt(PINDEX index)
{
  PObject & obj = AbstractGetDataAt(index);
  AbstractSetAt(AbstractGetKeyAt(index), NULL);
  return &obj;
}


PINDEX PAbstractDictionary::GetObjectsIndex(const PObject * obj) const
{
  return hashTable->GetElementsIndex(obj, PFalse, PFalse);
}


PINDEX PAbstractDictionary::GetValuesIndex(const PObject & obj) const
{
  return hashTable->GetElementsIndex(&obj, PTrue, PFalse);
}


PBoolean PAbstractDictionary::SetAt(PINDEX index, PObject * val)
{
  return AbstractSetAt(AbstractGetKeyAt(index), val);
}


PObject * PAbstractDictionary::GetAt(PINDEX index) const
{
  Element * lastElement;
  PAssert(hashTable->SetLastElementAt(index, lastElement), PInvalidArrayIndex);
  return lastElement->data;
}
 
 
PBoolean PAbstractDictionary::SetDataAt(PINDEX index, PObject * val)
{
  return AbstractSetAt(AbstractGetKeyAt(index), val);
}


PBoolean PAbstractDictionary::AbstractSetAt(const PObject & key, PObject * obj)
{
  if (obj == NULL) {
    obj = hashTable->RemoveElement(key);
    if (obj != NULL) {
      if (reference->deleteObjects)
        delete obj;
      reference->size--;
    }
  }
  else {
    Element * element = hashTable->GetElementAt(key);
    if (element == NULL) {
      hashTable->AppendElement(key.Clone(), obj);
      reference->size++;
    }
    else {
      if (reference->deleteObjects) 
    		delete element->data;
      element->data = obj;
    }
  }
  return PTrue;
}


PObject * PAbstractDictionary::AbstractGetAt(const PObject & key) const
{
  Element * element = hashTable->GetElementAt(key);
  return element != NULL ? element->data : (PObject *)NULL;
}


PObject & PAbstractDictionary::GetRefAt(const PObject & key) const
{
  Element * element = hashTable->GetElementAt(key);
  return *PAssertNULL(element)->data;
}


void PAbstractDictionary::AbstractGetKeys(PArrayObjects & keys) const
{
  keys.SetSize(GetSize());
  for (PINDEX i = 0; i < GetSize(); i++)
    keys.SetAt(i, AbstractGetKeyAt(i).Clone());
}


void PAbstractDictionary::PrintOn(ostream &strm) const
{
  char separator = strm.fill();
  if (separator == ' ')
    separator = '\n';

  for (PINDEX i = 0; i < GetSize(); i++) {
    if (i > 0)
      strm << separator;
    strm << AbstractGetKeyAt(i) << '=' << AbstractGetDataAt(i);
  }

  if (separator == '\n')
    strm << separator;
}


// End Of File ///////////////////////////////////////////////////////////////
