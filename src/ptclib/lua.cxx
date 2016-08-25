/*
 * lua.cxx
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

#ifdef __GNUC__
#pragma implementation "lua.h"
#endif

#include <ptlib.h>

#include <ptbuildopts.h>

#if P_LUA

#include <ptclib/lua.h>
#include <lua.hpp>


#ifdef _MSC_VER
  #pragma comment(lib, P_LUA_LIBRARY)
  #pragma message("Lua scripting support enabled")
#endif


///////////////////////////////////////////////////////////////////////////////

PLua::PLua()
  : m_lua(luaL_newstate())
{
  luaL_openlibs(m_lua);
}


PLua::~PLua()
{
  lua_close(m_lua);
}


bool PLua::LoadFile(const char * filename)
{
  int err;
  if ((err = luaL_loadfile(m_lua, filename)) == 0)
    return true;

  if (err == LUA_ERRFILE) {
    stringstream strm;
    strm << "Cannot load/open file '" << filename << "'";
    OnError(err, strm.str());
    return false;
  }

  OnError(err, lua_tostring(m_lua, -1));
  lua_pop(m_lua, 1);
  return false;
}


bool PLua::LoadString(const char * string)
{
  int err;
  if ((err = luaL_loadstring(m_lua, string)) == 0)
    return true;

  OnError(err, lua_tostring(m_lua, -1));
  lua_pop(m_lua, 1);
  return false;
}


bool PLua::Run(const char * program)
{
  int err;
  if ((program == NULL) || ((err = luaL_loadstring(m_lua, program)) == 0)) {
    if ((err = lua_pcall(m_lua, 0, 0, 0)) == 0)
      return true;
  }

  OnError(err, lua_tostring(m_lua, -1));
  lua_pop(m_lua, 1);
  
  return false;
}


void PLua::OnError(int code, const PString & str)
{
  PStringStream err;
  err << "Error (" << code << " - " << str;
  m_lastErrorText = err;
  PTRACE(2, "Lua\t" << m_lastErrorText);
}


void PLua::SetValue(const char * name, const char * value)
{
  lua_pushstring(*this, value);
  lua_setglobal(*this, name);
}

PString PLua::GetValue(const char * name)
{
  lua_getglobal(*this, name);
  PString result = lua_tostring(*this, -1);
  lua_pop(*this, 1);
  return result;
}

void PLua::SetFunction(const char * name_, CFunction func)
{
  PString name(name_);
  PINDEX pos = name.Find('.');
  if (pos == P_MAX_INDEX) {
    lua_register(*this, name, func);
    return;
  }

  PString table(name.Left(pos));
  name = name.Mid(pos+1);

  lua_getglobal(*this, table);
  if (lua_istable(*this, -1)) {
    lua_pushcfunction(*this, func);
    lua_setfield(*this, -1, name);
  }
}

bool PLua::CallLuaFunction(const char * name_)
{
  PString name(name_);
  PINDEX pos = name.Find('.');
  if (pos == P_MAX_INDEX) {
    lua_getglobal(*this, name);
    return lua_pcall(*this, 0, 0, 0) == 0;
  }

  PString table(name.Left(pos));
  name = name.Mid(pos+1);

  lua_getglobal(*this, table);
  if (lua_istable(*this, -1)) {
    lua_getfield(*this, -1, name);
    if (lua_isfunction(*this, -1))
      return lua_pcall(*this, 0, 0, 0) == 0;
  }

  return false;
}

bool PLua::CallLuaFunction(const char * name, const char * sig, ...)
{
  va_list args;
  va_start(args, sig);
  lua_getglobal(*this, name);

  if (lua_isnil(*this, -1))
    return false;

  int nargs;
  for (nargs = 0; *sig != '\0'; ++nargs) {
    switch (*sig++) {
      case 'd':
        lua_pushnumber(*this, va_arg(args, double));
        break;
      case 'i':
        lua_pushinteger(*this, va_arg(args, int));
        break;
      case 's':
        lua_pushstring(*this, va_arg(args, char *));
        break;
      case '>':
        goto endargs;
      default:
        PTRACE(1, "LUA\tInvalid argument in call " << *(sig -1));
        return false;
    }
  }
  endargs:

  int nresults = strlen(sig);

  if (lua_pcall(*this, nargs, nresults, 0) != 0) 
    return false;

  nresults = -nresults;

  while (*sig) {
    switch (*sig++) {
      case 'd':
        if (lua_isnumber(*this, nresults)) {
          PTRACE(1, "LUA\tInvalid result from call " << *(sig -1));
          return false;
        }
        *va_arg(args, double *) = lua_tonumber(*this, nresults);
        break;
      case 'i':
        if (lua_isnumber(*this, nresults)) {
          PTRACE(1, "LUA\tInvalid result from call " << *(sig -1));
          return false;
        }
        *va_arg(args, int *) = lua_tointeger(*this, nresults);
        break;
      case 's':
        if (lua_isstring(*this, nresults)) {
          PTRACE(1, "LUA\tInvalid result from call " << *(sig -1));
          return false;
        }
        *va_arg(args, const char **) = lua_tostring(*this, nresults);
        break;
      default:
        PTRACE(1, "LUA\tInvalid result from call " << *(sig -1));
        return false;
    }
    nresults++;
  }

  va_end(args);

  return true;
}


void PLua::BindToInstanceStart(const char * instanceName)
{
  /* create a new metatable and set the __index table */
  luaL_newmetatable(m_lua, instanceName);
  lua_pushvalue(m_lua, -1);
  lua_setfield(m_lua, -2, "__index");
}


void PLua::BindToInstanceFunc(const char * lua_name, void * obj, CFunction func)
{
  /* set member function */
  lua_pushlightuserdata(m_lua, obj);
  lua_pushcclosure(m_lua, func, 1);
  lua_setfield(m_lua, -2, lua_name);
}


void PLua::BindToInstanceEnd(const char * instanceName)
{
  /* assign metatable */
  lua_newtable(m_lua);
  luaL_getmetatable(m_lua, instanceName);
  lua_setmetatable(m_lua, -2);
  lua_setglobal(m_lua, instanceName);
}


void * PLua::GetInstance(lua_State * L)
{
  return lua_touserdata(L, lua_upvalueindex(1));
}


int PLua::TraceFunction(lua_State * L)
{
  if (!lua_isnumber(L, -2))
    return 0;
  if (!lua_isstring(L, -1))
    return 0;
  PTRACE(lua_tointeger(L, -2), lua_tostring(L, -1));
  return 0;
}


#else // P_LUA

  #ifdef _MSC_VER
    #pragma message("Lua scripting support DISABLED")
  #endif

#endif // P_LUA
