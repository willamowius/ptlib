/*
 * url_test.cxx
 *
 * Test program for URL loader.
 *
 * Portable Tools Library
 *
 * Copyright (c) 2011 Vox Lucida Pty. Ltd.
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
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <ptlib/pprocess.h>
#include <ptclib/url.h>


class Test : public PProcess
{
  PCLASSINFO(Test, PProcess)
  public:
    void Main();
};


PCREATE_PROCESS(Test)

void Test::Main()
{
  cout << "URL Test Utility" << endl;

  PArgList & args = GetArguments();
  if (args.GetCount() < 1) {
    cerr << "usage: " << GetFile().GetTitle() << " <url> [ <filename> ]\n";
    return;
  }

  PURL url;
  if (!url.Parse(args[0], "http")) {
    cerr << "Could not parse URL \"" << args[0] << '"' << endl;
    return;
  }

  if (args.GetCount() == 1) {
    PString str;
    if (url.LoadResource(str))
      cout << str << endl;
    else
      cerr << "Could not load text URL \"" << args[0] << '"' << endl;
  }
  else {
    PBYTEArray data;
    if (url.LoadResource(data)) {
      PFile out;
      if (out.Open(args[1], PFile::WriteOnly))
        out.Write((const BYTE *)data, data.GetSize());
      else
        cerr << "Could not open file \"" << args[1] << '"' << endl;
    }
    else
      cerr << "Could not load binary URL \"" << args[0] << '"' << endl;
  }
}


// End of file
