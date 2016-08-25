/*
 * svngetrev.cxx
 *
 * Get the current SVN revision for source tree
 *
 * Portable Tools Library
 *
 * Copyright (c) 2010 Vox Lucida Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#pragma warning(disable:4786)
#pragma warning(disable:4996)

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>


#define VERSION "1.02"

using namespace std;


int main(int argc, char* argv[])
{
  cout << "SVN Get Revision " VERSION << endl;

  if (argc < 3) {
    cerr << "usage: svn_getrev infile outfile [ define ]" << endl;
    return 1;
  }

  if (strcmp(argv[1], argv[2]) == 0) {
    cerr << "Input and output files must be different." << endl;
    return 1;
  }

  const char * define = argc < 4 ? "SVN_REVISION" : argv[3];

  char revision[20];
  revision[0] = '\0';


  ifstream file(".svn\\entries", ios::in);
  if (!file.is_open()) {
    file.clear();
    file.open("_svn\\entries", ios::in);
  }

  if (file.is_open()) {
    file.ignore(1000, '\n');
    file.ignore(1000, '\n');
    file.ignore(1000, '\n');

    file.getline(revision, sizeof(revision));

    file.close();
  }

  if (revision[0] == '\0')
    cout << "Cannot determine revision, using default." << endl;
  else
    cout << "Changing to revision " << revision << endl;


  file.clear();
  file.open(argv[1], ios::in);
  if (!file.is_open()) {
    cerr << "Could not open \"" << argv[1] << '"' << endl;
    return 1;
  }

  ofstream out(argv[2], ios::out);
  if (!out.is_open()) {
    cerr << "Could not open \"" << argv[2] << '"' << endl;
    return 1;
  }

  while (!file.eof()) {
    char line[250];
    file.getline(line, sizeof(line));
    if (revision[0] == '\0' && strstr(line, "$Revision: ") != NULL) {
      char * digits = line;
      while (!isdigit(*digits))
        ++digits;
      char * end = digits;
      while (isdigit(*end))
        ++end;
      *end = '\0';
      strcpy(revision, digits);
    }
    if (revision[0] != '\0' && strstr(line, define) != NULL)
      out << "#define " << define << ' ' << revision << '\n';
    else
      out << line << '\n';
  }

  return 0;
}
