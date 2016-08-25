/*
 * factory.h
 *
 * Abstract Factory Classes
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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

#ifndef PTLIB_FACTORY_H
#define PTLIB_FACTORY_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <string>
#include <map>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(disable:4786)
#endif

/**
 *
 * These templates implement an Abstract Factory that allows
 * creation of a class "factory" that can be used to create
 * "concrete" instance that are descended from a abstract base class
 *
 * Given an abstract class A with a descendant concrete class B, the 
 * concrete class is registered by instantiating the PFactory template
 * as follows:
 *
 *       PFactory<A>::Worker<B> aFactory("B");
 *
 * To instantiate an object of type B, use the following:
 *
 *       A * b = PFactory<A>::CreateInstance("B");
 *
 * A vector containing the names of all of the concrete classes for an
 * abstract type can be obtained as follows:
 *
 *       PFactory<A>::KeyList_T list = PFactory<A>::GetKeyList()
 *
 * Note that these example assumes that the "key" type for the factory
 * registration is of the default type PString. If a different key type
 * is needed, then it is necessary to specify the key type:
 *
 *       PFactory<C, unsigned>::Worker<D> aFactory(42);
 *       C * d = PFactory<C, unsigned>::CreateInstance(42);
 *       PFactory<C, unsigned>::KeyList_T list = PFactory<C, unsigned>::GetKeyList()
 *
 * The factory functions also allow the creation of "singleton" factories that return a 
 * single instance for all calls to CreateInstance. This can be done by passing a "true"
 * as a second paramater to the factory registration as shown below, which will cause a single
 * instance to be minted upon the first call to CreateInstance, and then returned for all
 * subsequent calls. 
 *
 *      PFactory<A>::Worker<E> eFactory("E", true);
 *
 * It is also possible to manually set the instance in cases where the object needs to be created non-trivially.
 *
 * The following types are defined as part of the PFactory template class:
 *
 *     KeyList_T    a vector<> of the key type (usually std::string)
 *     Worker       an abstract factory for a specified concrete type
 *     KeyMap_T     a map<> that converts from the key type to the Worker instance
 *                  for each concrete type registered for a specific abstract type
 *
 * As a side issue, note that the factory lists are all thread safe for addition,
 * creation, and obtaining the key lists.
 *
 */

// this define the default class to be used for keys into PFactories
//typedef PString PDefaultPFactoryKey;
typedef std::string PDefaultPFactoryKey;


/** Base class for generic factories.
    This classes reason for existance and the FactoryMap contained within it
    is to resolve issues with static global construction order and Windows DLL
    multiple instances issues. THis mechanism guarantees that the one and one
    only global variable (inside the GetFactories() function) is initialised
    before any other factory related instances of classes.
  */
class PFactoryBase
{
  protected:
    PFactoryBase()
    { }
  public:
    virtual ~PFactoryBase()
    { }

    virtual void DestroySingletons() = 0;

    class FactoryMap : public std::map<std::string, PFactoryBase *>
    {
      public:
        FactoryMap() { }
        ~FactoryMap();
    };

    static FactoryMap & GetFactories();
    static PMutex & GetFactoriesMutex();

  protected:
    PMutex m_mutex;

  private:
    PFactoryBase(const PFactoryBase &) {}
    void operator=(const PFactoryBase &) {}
};


/** Template class for generic factories of an abstract class.
  */
template <class AbstractClass, typename KeyType = PDefaultPFactoryKey>
class PFactory : PFactoryBase
{
  public:
    typedef KeyType       Key_T;
    typedef AbstractClass Abstract_T;

    class WorkerBase
    {
      protected:
        enum Types {
          NonSingleton,
          StaticSingleton,
          DynamicSingleton
        } m_type;

        Abstract_T * m_singletonInstance;

        WorkerBase(bool singleton = false)
          : m_type(singleton ? DynamicSingleton : NonSingleton)
          , m_singletonInstance(NULL)
        { }

        WorkerBase(Abstract_T * instance, bool delSingleton = true)
          : m_type(delSingleton ? DynamicSingleton : StaticSingleton)
          , m_singletonInstance(instance)
        { }

        virtual ~WorkerBase()
        {
          DestroySingleton();
        }

        Abstract_T * CreateInstance(const Key_T & key)
        {
          if (m_type == NonSingleton)
            return Create(key);

          if (m_singletonInstance == NULL)
            m_singletonInstance = Create(key);
          return m_singletonInstance;
        }

        virtual Abstract_T * Create(const Key_T & /*key*/) const
        {
          PAssert(this->m_type == StaticSingleton, "Incorrect factory worker descendant");
          return this->m_singletonInstance;
        }

        virtual void DestroySingleton()
        {
          if (m_type == DynamicSingleton) {
            delete m_singletonInstance;
            m_singletonInstance = NULL;
          }
        }

        bool IsSingleton() const { return m_type != NonSingleton; }

      friend class PFactory<Abstract_T, Key_T>;
    };

    template <class ConcreteClass>
    class Worker : WorkerBase
    {
      public:
        Worker(const Key_T & key, bool singleton = false)
          : WorkerBase(singleton)
        {
          PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
          PFactory<Abstract_T, Key_T>::Register(key, this);
        }

      protected:
        virtual Abstract_T * Create(const Key_T & /*key*/) const
        {
          return new ConcreteClass;
        }
    };

    typedef std::map<Key_T, WorkerBase *> KeyMap_T;
    typedef std::vector<Key_T> KeyList_T;

    static bool Register(const Key_T & key, WorkerBase * worker)
    {
      return GetInstance().Register_Internal(key, worker);
    }

    static bool Register(const Key_T & key, Abstract_T * instance, bool autoDeleteInstance = true)
    {
      WorkerBase * worker = PNEW WorkerBase(instance, autoDeleteInstance);
      if (GetInstance().Register_Internal(key, worker))
        return true;
      delete worker;
      return false;
    }

    static PBoolean RegisterAs(const Key_T & newKey, const Key_T & oldKey)
    {
      return GetInstance().RegisterAs_Internal(newKey, oldKey);
    }

    static void Unregister(const Key_T & key)
    {
      GetInstance().Unregister_Internal(key);
    }

    static void UnregisterAll()
    {
      GetInstance().UnregisterAll_Internal();
    }

    static bool IsRegistered(const Key_T & key)
    {
      return GetInstance().IsRegistered_Internal(key);
    }

    static Abstract_T * CreateInstance(const Key_T & key)
    {
      return GetInstance().CreateInstance_Internal(key);
    }

    template <class Derived_T>
    static Derived_T * CreateInstanceAs(const Key_T & key)
    {
      return dynamic_cast<Derived_T *>(GetInstance().CreateInstance_Internal(key));
    }

    static PBoolean IsSingleton(const Key_T & key)
    {
      return GetInstance().IsSingleton_Internal(key);
    }

    static KeyList_T GetKeyList()
    { 
      return GetInstance().GetKeyList_Internal();
    }

    static KeyMap_T & GetKeyMap()
    { 
      return GetInstance().m_keyMap;
    }

    static PMutex & GetMutex()
    {
      return GetInstance().m_mutex;
    }

    virtual void DestroySingletons()
    {
      for (typename KeyMap_T::const_iterator it = m_keyMap.begin(); it != m_keyMap.end(); ++it)
        it->second->DestroySingleton();
    }

  protected:
    PFactory()
    { }

    ~PFactory()
    {
      DestroySingletons();
    }

    static PFactory & GetInstance()
    {
      std::string className = typeid(PFactory).name();
      PWaitAndSignal m(GetFactoriesMutex());
      FactoryMap & factories = GetFactories();
      FactoryMap::const_iterator entry = factories.find(className);
      if (entry != factories.end()) {
        PAssert(entry->second != NULL, "Factory map returned NULL for existing key");
        PFactoryBase * b = entry->second;
        // don't use the following dynamic cast, because gcc does not like it
        //PFactory * f = dynamic_cast<PFactory*>(b);
        return *(PFactory *)b;
      }

      PMEMORY_IGNORE_ALLOCATIONS_FOR_SCOPE;
      PFactory * factory = new PFactory;
      factories[className] = factory;
      return *factory;
    }


    bool Register_Internal(const Key_T & key, WorkerBase * worker)
    {
      PWaitAndSignal mutex(m_mutex);
      if (m_keyMap.find(key) != m_keyMap.end())
        return false;
      m_keyMap[key] = PAssertNULL(worker);
      return true;
    }

    PBoolean RegisterAs_Internal(const Key_T & newKey, const Key_T & oldKey)
    {
      PWaitAndSignal mutex(m_mutex);
      if (m_keyMap.find(oldKey) == m_keyMap.end())
        return false;
      m_keyMap[newKey] = m_keyMap[oldKey];
      return true;
    }

    void Unregister_Internal(const Key_T & key)
    {
      m_mutex.Wait();
      m_keyMap.erase(key);
      m_mutex.Signal();
    }

    void UnregisterAll_Internal()
    {
      m_mutex.Wait();
      m_keyMap.clear();
      m_mutex.Signal();
    }

    bool IsRegistered_Internal(const Key_T & key)
    {
      PWaitAndSignal mutex(m_mutex);
      return m_keyMap.find(key) != m_keyMap.end();
    }

    Abstract_T * CreateInstance_Internal(const Key_T & key)
    {
      PWaitAndSignal mutex(m_mutex);
      typename KeyMap_T::const_iterator entry = m_keyMap.find(key);
      if (entry != m_keyMap.end())
        return entry->second->CreateInstance(key);
      return NULL;
    }

    bool IsSingleton_Internal(const Key_T & key)
    {
      PWaitAndSignal mutex(m_mutex);
      if (m_keyMap.find(key) == m_keyMap.end())
        return false;
      return m_keyMap[key]->IsSingleton();
    }

    KeyList_T GetKeyList_Internal()
    { 
      PWaitAndSignal mutex(m_mutex);
      KeyList_T list;
      typename KeyMap_T::const_iterator entry;
      for (entry = m_keyMap.begin(); entry != m_keyMap.end(); ++entry)
        list.push_back(entry->first);
      return list;
    }

    KeyMap_T m_keyMap;

  private:
    PFactory(const PFactory &) {}
    void operator=(const PFactory &) {}
};


/** This macro is used to create a factory.
    This is mainly used for factories that exist inside a library and works in
    conjunction with the PFACTORY_LOAD() macro.

    When a factory is contained wholly within a single compilation module of
    a library, it is typical that a linker does not include ANY of the code in
    that module. To avoid this the header file that declares the abstract type
    should include a PFACTORY_LOAD() macro call for all concrete classes that
    are in the library. Then whan an application includes the abstract types
    header, it will force the load of all the possible concrete classes.
  */
#define PFACTORY_CREATE(factory, ConcreteClass, ...) \
  namespace PFactoryLoader { \
    int ConcreteClass##_link() { return 0; } \
    factory::Worker<ConcreteClass> ConcreteClass##_instance(__VA_ARGS__); \
  }

#define PFACTORY_CREATE_SINGLETON(factory, ConcreteClass) \
        PFACTORY_CREATE(factory, ConcreteClass, typeid(ConcreteClass).name(), true)

#define PFACTORY_GET_SINGLETON(factory, ConcreteClass) \
        static ConcreteClass & GetInstance() { \
          return *factory::CreateInstanceAs<ConcreteClass>(typeid(ConcreteClass).name()); \
        }




/* This macro is used to force linking of factories.
   See PFACTORY_CREATE() for more information
 */
#define PFACTORY_LOAD(ConcreteType) \
  namespace PFactoryLoader { \
    extern int ConcreteType##_link(); \
    int const ConcreteType##_loader = ConcreteType##_link(); \
  }


#endif // PTLIB_FACTORY_H


// End Of File ///////////////////////////////////////////////////////////////
