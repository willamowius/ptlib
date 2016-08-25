/*
 * vidinput_names.h
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Portable Windows Library (V4L plugin).
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Derek Smithies (derek@indranet.co.nz)
 *                 Mark Cooke (mpc@star.sr.bham.ac.uk)
 *                 Nicola Orru' <nigu@itadinanta.it>
 *
 * $Revision$
 * $Author$
 * $Date$
 */
#ifndef _VIDINPUTNAMES_H
#define _VIDINPUTNAMES_H


#include <ptlib.h>
#include <ptlib/videoio.h>
#ifndef MAJOR
#define MAJOR(a) (int)((unsigned short) (a) >> 8)
#endif

#ifndef MINOR
#define MINOR(a) (int)((unsigned short) (a) & 0xFF)
#endif

class V4LXNames : public PObject
{ 
  PCLASSINFO(V4LXNames, PObject);

 public:

  V4LXNames() {/* nothing */};

  virtual void Update () = 0;
  
  PString GetUserFriendly(PString devName);

  PString GetDeviceName(PString userName);

  PStringList GetInputDeviceNames();

protected:

  void AddUserDeviceName(PString userName, PString devName);

  virtual PString BuildUserFriendly(PString devname) = 0;

  void PopulateDictionary();

  void ReadDeviceDirectory(PDirectory devdir, POrdinalToString & vid);

  PMutex          mutex;
  PStringToString deviceKey;
  PStringToString userKey;
  PStringList     inputDeviceNames;

};

#endif
