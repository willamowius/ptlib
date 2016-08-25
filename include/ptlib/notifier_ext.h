/*
 * notifier_ext.h
 *
 * Smart Notifiers and Notifier Lists
 *
 * Portable Windows Library
 *
 * Copyright (c) 2004 Reitek S.p.A.
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_NOTIFIER_EXT_H
#define PTLIB_NOTIFIER_EXT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/** Implements a function similar to the PNotifier, but uses an "id" to link the caller
  * and callee rather than using a pointer. This has the advantage that if the pointer
  * becomes invalid, the caller can gracefully fail the notification rather than
  * simply crashing due to an invalid pointer access.
  *
  * These classes were created to support of the XMPP classes
  */

class PSmartNotifieeRegistrar
{
  public:
    PSmartNotifieeRegistrar() : m_ID(P_MAX_INDEX) {}
    ~PSmartNotifieeRegistrar() { UnregisterNotifiee(m_ID); }

    void        Init(void * obj)        { if (m_ID == P_MAX_INDEX) m_ID = RegisterNotifiee(obj); }
    unsigned    GetID() const           { return m_ID; }

    static unsigned    RegisterNotifiee(void * obj);
    static PBoolean        UnregisterNotifiee(unsigned id);
    static PBoolean        UnregisterNotifiee(void * obj);
    static void *      GetNotifiee(unsigned id);

  protected:
    unsigned m_ID;
};

class PSmartNotifierFunction : public PNotifierFunction
{
    PCLASSINFO(PSmartNotifierFunction, PNotifierFunction);

  protected:
    unsigned m_NotifieeID;

  public:
    PSmartNotifierFunction(unsigned id) : PNotifierFunction(&id), m_NotifieeID(id) { }
    unsigned GetNotifieeID() const { return m_NotifieeID; }
    void * GetNotifiee() const { return PSmartNotifieeRegistrar::GetNotifiee(m_NotifieeID); }
    PBoolean IsValid() const { return GetNotifiee() != 0; }
};

#define PDECLARE_SMART_NOTIFIEE \
    PSmartNotifieeRegistrar   m_Registrar; \

#define PCREATE_SMART_NOTIFIEE m_Registrar.Init(this)

#define PDECLARE_SMART_NOTIFIER(notifier, notifiee, func) \
  class func##_PSmartNotifier : public PSmartNotifierFunction { \
    public: \
      func##_PSmartNotifier(unsigned id) : PSmartNotifierFunction(id) { } \
      virtual void Call(PObject & note, INT extra) const \
      { \
          void * obj = GetNotifiee(); \
          if (obj) \
            ((notifiee*)obj)->func((notifier &)note, extra); \
          else \
            PTRACE(2, "PWLib\tInvalid notifiee"); \
      } \
  }; \
  friend class func##_PSmartNotifier; \
  virtual void func(notifier & note, INT extra)

#define PCREATE_SMART_NOTIFIER(func) PNotifier(new func##_PSmartNotifier(m_Registrar.GetID()))


class PNotifierList : public PObject
{
  PCLASSINFO(PNotifierList, PObject);
  private:
    PLIST(_PNotifierList, PNotifier);

    _PNotifierList m_TheList;

    // Removes smart pointers to deleted objects
    void   Cleanup();

  public:
    PINDEX GetSize() const { return m_TheList.GetSize(); }

    void Add(PNotifier * handler)       { m_TheList.Append(handler); }
    void Remove(PNotifier * handler)    { m_TheList.Remove(handler); }
    PBoolean RemoveTarget(PObject * obj);
    PBoolean Fire(PObject& obj, INT val = 0);

    // Moves all the notifiers in "that" to "this"
    void  Move(PNotifierList& that);
};


#endif  // PTLIB_NOTIFIER_EXT_H


// End of File ///////////////////////////////////////////////////////////////
