//
// hello.cxx
//
// Equivalence Pty. Ltd.
//

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptclib/lua.h>

#if P_LUA
#else
#error Cannot compile Lua test program without Lua support!
#endif

class LuaProcess : public PProcess
{
  PCLASSINFO(LuaProcess, PProcess)
  public:
    void Main();
};


class MyClass {
  public:
    MyClass(const char * str)
      : m_str(str)
    { }
    const char * m_str;

    PLUA_BINDING_START(MyClass)
      PLUA_BINDING(print)
    PLUA_BINDING_END()

    PLUA_FUNCTION_NOARGS(print)
    {
      cerr << m_str << endl; 
      return 0;
    }
};

PCREATE_PROCESS(LuaProcess)

void LuaProcess::Main()
{
  PLua lua;

  // populate the table
  MyClass class1("class1");
  class1.BindToInstance(lua, "class1");

  MyClass class2("class2");
  class2.BindToInstance(lua, "class2");

  lua.SetValue("a", "12");

  if (!lua.Run("class1.print()\nclass2.print()\na=17"))
    cout << lua.GetLastErrorText() << endl;

  cout << "New value for a=" << lua.GetValue("a") << endl;
}

// End of hello.cxx
