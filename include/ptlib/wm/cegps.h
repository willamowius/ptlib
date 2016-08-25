// cegps.h
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
 *
 * Contributor(s): ______________________________________.
 *
 *
 */

#if _WIN32_WCE > 0x500

/*
  This class accesses the WinCE GPS intermediate GPS driver to retrieve GPS information
  Using GPS can drain the power so it is advised to only Start and Stop as required to
  ensure the longevity of the battery.
*/

class PGPS : public PThread
{
  public:
     PGPS();
     ~PGPS();

	 /* Start the GPS service
	  */
	 PBoolean Start();

	 /* Stop the GPS service
	  */
	 PBoolean Stop();

	 /* Close the GPS service
	  */
	 void Close();

	 void Main();

	 /* GPS position information Event
	 */
	 virtual void CurrentPosition(double /*posLat*/, 
				                  double /*posLong*/, 
								  float /*posAlt*/, 
								  float /*posSpeed*/
								  ) {};

  protected:
	HANDLE s_hGPS_Device;
	HANDLE s_hNewLocationData;
	HANDLE s_hDeviceStateChange;
	HANDLE s_hExitThread;

	PMutex gpsMutex;
	PSyncPoint runningSync;
	bool gpsRunning;
	bool closeThread;

};

#endif // _WIN32_WCE