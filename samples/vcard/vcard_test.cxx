/*
 * vcard_test.cxx
 *
 * Test program for vCard parsing.
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
#include <ptclib/vcard.h>


static const char * const TestData[10] =
{
   "BEGIN:vCard\r\n"
   "VERSION:3.0\r\n"
   "FN:Frank Dawson\r\n"
   "ORG:Lotus Development Corporation\r\n"
   "ADR;TYPE=WORK,POSTAL,PARCEL:;;6544 Battleford Drive\r\n"
   " ;Raleigh;NC;27613-3502;U.S.A.\r\n"
   "TEL;TYPE=VOICE,MSG,WORK:+1-919-676-9515\r\n"
   "TEL;TYPE=FAX,WORK:+1-919-676-9564\r\n"
   "EMAIL;TYPE=INTERNET,PREF:Frank_Dawson@Lotus.com\r\n"
   "EMAIL;TYPE=INTERNET:fdawson@earthlink.net\r\n"
   "URL:http://home.earthlink.net/~fdawson\r\n"
   "END:vCard\r\n",

   "BEGIN:vCard\r\n"
   "VERSION:3.0\r\n"
   "FN:Tim Howes\r\n"
   "ORG:Netscape Communications Corp.\r\n"
   "ADR;TYPE=WORK:;;501 E. Middlefield Rd.;Mountain View;\r\n"
   " CA; 94043;U.S.A.\r\n"
   "TEL;TYPE=VOICE,MSG,WORK:+1-415-937-3419\r\n"
   "TEL;TYPE=FAX,WORK:+1-415-528-4164\r\n"
   "EMAIL;TYPE=INTERNET:howes@netscape.com\r\n"
   "END:vCard",
};


class Test : public PProcess
{
  PCLASSINFO(Test, PProcess)
  public:
    void Main();
};


PCREATE_PROCESS(Test)

void Test::Main()
{
  cout << "vCard Test Utility" << endl;

  PArgList & args = GetArguments();
  if (!args.Parse("-P-photo:")) {
    cerr << "usage: " << GetFile().GetTitle() << " [ options] <filename> | '-'\n"
            "options:\n"
            "  -P --photo fname    Output PHOTO field to filename\n";
    return;
  }

  PvCard card;
  if (args[0] == "-") {
    cin >> card;
    if (cin.fail()) {
      cerr << "Could not parse vCard from stdin\n";
      return;
    }
  }
  else if (args[0].GetLength() == 1 && isdigit(args[0][0])) {
    PStringStream strm(TestData[args[0].AsUnsigned()]);
    strm >> card;
    if (strm.fail()) {
      cerr << "Could not parse vCard from Test Data\n";
      return;
    }
  }
  else {
    PTextFile file;
    if (!file.Open(args[0], PFile::ReadOnly)) {
      cerr << "Could not open \"" << args[0] << '"' << endl;
      return;
    }
    file >> card;
    if (file.fail()) {
      cerr << "Could not parse vCard from \"" << args[0] << '"' << endl;
      return;
    }
  }

  cout << card;

  if (args.HasOption('P')) {
    PBYTEArray data;
    if (card.m_photo.LoadResource(data)) {
      PFile photo;
      if (photo.Open(args.GetOptionString('P'), PFile::WriteOnly))
        photo.Write(data, data.GetSize());
    }
  }
}


// End of file
