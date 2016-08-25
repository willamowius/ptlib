// cegps.cxx
/*
 * GPS Implementation for the PTLib Library.
 *
 * Copyright (c) 2008 ISVO (Asia) Pte Ltd. All Rights Reserved.
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
 * The Original Code is derived from and used in conjunction with the 
 * H323plus (www.h323plus.org) and OpalVoip (www.opalvoip.org) Project.
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Portions taken from Windows Mobile 6 SDK GPS sample
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *
 * Contributor(s): ______________________________________.
 *
 *
 */

#include "ptlib.h"

#if _WIN32_WCE > 0x500
#include <ptlib/wm/cegps.h>
#include <GPSApi.h>

#pragma comment(lib, "Gpsapi.lib")


#define MAX_WAIT    5000
#define MAX_AGE     3000
#define GPS_CONTROLLER_EVENT_COUNT 3


PGPS::PGPS()
: PThread(10000, AutoDeleteThread, LowPriority, "GPS")
{
	s_hGPS_Device = NULL;
	s_hNewLocationData = NULL;
	s_hDeviceStateChange = NULL;
	s_hExitThread = NULL;

	gpsRunning = false;
	closeThread = false;
	Resume();
}

PGPS::~PGPS()
{
   Close();
}

void PGPS::Main()
{
    
    DWORD dwRet = 0;
    GPS_POSITION gps_Position = {0};

    HANDLE gpsHandles[GPS_CONTROLLER_EVENT_COUNT] = {s_hNewLocationData, 
        s_hDeviceStateChange,
        s_hExitThread
        };
    gps_Position.dwSize = sizeof(gps_Position);
    gps_Position.dwVersion = GPS_VERSION_1;

  while (!closeThread) 
  {
	  if (gpsRunning) {
			dwRet = WaitForMultipleObjects(GPS_CONTROLLER_EVENT_COUNT,gpsHandles,FALSE,INFINITE);
			if (dwRet == WAIT_OBJECT_0){
              dwRet = GPSGetPosition(s_hGPS_Device,&gps_Position,MAX_AGE,0);
				if (ERROR_SUCCESS != dwRet)
					continue;
				else
				  CurrentPosition(gps_Position.dblLatitude,
					gps_Position.dblLongitude,gps_Position.flAltitudeWRTSeaLevel,
					gps_Position.flSpeed);

			} else if (dwRet == WAIT_OBJECT_0 + 1) {
                // We don't collect device information at this stage so continue
						
			} else if (dwRet == WAIT_OBJECT_0 + 2)
				// We have got an indication to stop collecting GPS information
				gpsRunning = false;
            else
			   ASSERT(0);	    
	  } else {
	     if (!closeThread)
		     runningSync.Wait();
	  }
  }
}

PBoolean PGPS::Start()
{
  PWaitAndSignal m(gpsMutex);

  if (closeThread) {
      PTRACE(4,"CEGPS\tCannot start GPS as Thread is closed");
	  return false;
  }

  if (gpsRunning)
	  return true;

  if (!s_hGPS_Device) {
    s_hGPS_Device = GPSOpenDevice(s_hNewLocationData,s_hDeviceStateChange,NULL,NULL);
    if (!s_hGPS_Device) {
	   PTRACE(4,"CEGPS\tError Opening GPS Device");
    } else {
	  runningSync.Signal();
      gpsRunning = true;
    }
  }

  return gpsRunning;
}

PBoolean PGPS::Stop()
{
  PWaitAndSignal m(gpsMutex);

  if (!gpsRunning)
	  return true;

  GPSCloseDevice(s_hGPS_Device);
  s_hGPS_Device = NULL;

  if (s_hNewLocationData) {
    CloseHandle(s_hNewLocationData);
    s_hNewLocationData = NULL;
  }

  if (s_hDeviceStateChange) {
    CloseHandle(s_hDeviceStateChange);
    s_hDeviceStateChange = NULL;
  }

  if (s_hExitThread) {
    CloseHandle(s_hExitThread);
	s_hExitThread = NULL;
  }

  gpsRunning = false;

  return gpsRunning;
}

void PGPS::Close()
{
	closeThread = true;
	runningSync.Signal();

	// If the device exists then stop it.
	// This will close the thread.
	if  (s_hGPS_Device)
		Stop();
}


#endif // _WIN32_WCE
