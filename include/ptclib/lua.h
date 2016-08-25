/*
 * lua.h
 *
 * Interface library for Lua interpreter
 *
 * Portable Tools Library]
 *
 * Copyright (C) 2010 by Post Increment
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): Craig Southeren
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_LUA_H
#define PTLIB_LUA_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>
#include <ptbuildopts.h>

#if P_LUA

struct lua_State;


//////////////////////////////////////////////////////////////

class PLua
{
  public:
    PLua();
    ~PLua();

    virtual bool LoadString(const char * text);

    virtual bool LoadFile(const char * filename);

    virtual bool Run(const char * program = NULL);

    virtual void OnError(int code, const PString & str);

    operator lua_State * () { return m_lua; }

    virtual void SetValue(const char * name, const char * value);
    virtual PString GetValue(const char * name);

    typedef int (*CFunction)(lua_State *L);
    virtual void SetFunction(const char * name, CFunction func);

    bool CallLuaFunction(const char * name);
    bool CallLuaFunction(const char * name, const char * sig, ...);

    static int TraceFunction(lua_State * L);

    PString GetLastErrorText() const 
    { return m_lastErrorText; }

    void BindToInstanceStart(const char * instanceName);
    void BindToInstanceFunc(const char * lua_name, void * obj, CFunction func);
    void BindToInstanceEnd(const char * instanceName);

    static void * GetInstance(lua_State * L);

  protected:
    lua_State * m_lua;
    PString m_lastErrorText;
};

#define PLUA_BINDING_START(class_type) \
  typedef class_type PLua_InstanceType; \
  void UnbindFromInstance(PLua &, const char *) { } \
  void BindToInstance(PLua & lua, const char * instanceName) \
  { \
    lua.BindToInstanceStart(instanceName);

#define PLUA_BINDING2(cpp_name, lua_name) \
    lua.BindToInstanceFunc(lua_name, (void *)this, &PLua_InstanceType::cpp_name##_callback);

#define PLUA_BINDING(fn_name) \
  PLUA_BINDING2(fn_name, #fn_name)

#define PLUA_BINDING_END() \
    lua.BindToInstanceEnd(instanceName); \
  }

#define PLUA_FUNCTION_DECL(fn_name) \
  static int fn_name##_callback(lua_State * L) \
  { \
    return ((PLua_InstanceType *)PLua::GetInstance(L))->fn_name(L); \
  }

#define PLUA_FUNCTION(fn_name) \
  PLUA_FUNCTION_DECL(fn_name) \
  int fn_name(lua_State * L) \

#define PLUA_FUNCTION_NOARGS(fn_name) \
  PLUA_FUNCTION_DECL(fn_name) \
  int fn_name(lua_State *) \

#define PLUA_DECLARE_FUNCTION(fn_name) \
  PLUA_FUNCTION_DECL(fn_name) \
  int fn_name(lua_State * L); \


//////////////////////////////////////////////////////////////

#endif // P_LUA

#endif  // PTLIB_LUA_H

