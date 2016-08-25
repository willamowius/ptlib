/*
 * remconn.h
 *
 * Remote networking connection class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_REMOTECONNECTION_H
#define PTLIB_REMOTECONNECTION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/pipechan.h>

#ifdef _WIN32
#include <ras.h>
#include <raserror.h>
#endif

/** Remote Access Connection class.
*/
class PRemoteConnection : public PObject
{
  PCLASSINFO(PRemoteConnection, PObject);

  public:
  /**@name Construction */
  //@{
    /// Create a new remote connection.
    PRemoteConnection();

    /**Create a new remote connection.
       This will initiate the connection using the specified settings.
     */
    PRemoteConnection(
      const PString & name  ///< Name of RAS configuration.
    );

    /// Disconnect remote connection.
    ~PRemoteConnection();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Compare two connections.
      @return EqualTo of same RAS connectionconfiguration.
     */
    virtual Comparison Compare(
      const PObject & obj     ///< Another connection instance.
    ) const;

    /** Get has value for the connection
        @return Hash value of the connection name string.
      */
    virtual PINDEX HashFunction() const;
  //@}

  /**@name Dial/Hangup functions */
  //@{
    /** Open the remote connection.
     */
    PBoolean Open(
      PBoolean existing = false  ///< Flag for open only if already connected.
    );

    /** Open the remote connection.
     */
    PBoolean Open(
      const PString & name,   ///< RAS name of of connection to open.
      PBoolean existing = false   ///< Flag for open only if already connected.
    );

    /** Open the remote connection.
     */
    PBoolean Open(
      const PString & name,     ///< RAS name of of connection to open.
      const PString & username, ///< Username for remote log in.
      const PString & password, ///< password for remote log in.
      PBoolean existing = false     ///< Flag for open only if already connected.
    );

    /** Close the remote connection.
        This will hang up/dosconnect the connection, net access will no longer
        be available to this site.
      */
    void Close();
  //@}

  /**@name Error/Status functions */
  //@{
    /// Status codes for remote connection.
    enum Status {
      /// Connection has not been made and no attempt is being made.
      Idle,
      /// Connection is completed and active.
      Connected,
      /// Connection is in progress.
      InProgress,
      /// Connection failed due to the line being busy.
      LineBusy,
      /// Connection failed due to the line havin no dial tone.
      NoDialTone,
      /// Connection failed due to the remote not answering.
      NoAnswer,
      /// Connection failed due to the port being in use.
      PortInUse,
      /// Connection failed due to the RAS setting name/number being incorrect.
      NoNameOrNumber,
      /// Connection failed due to insufficient privilege.
      AccessDenied,
      /// Connection failed due to a hardware failure.
      HardwareFailure,
      /// Connection failed due to a general failure.
      GeneralFailure,
      /// Connection was lost after successful establishment.
      ConnectionLost,
      /// The Remote Access Operating System support is not installed.
      NotInstalled,
      NumStatuses
    };

    /**Get the current status of the RAS connection.

       @return
       Status code.
     */
    Status GetStatus() const;

    /**Get the error code for the last operation.

       @return
       Operating system error code.
     */
    DWORD GetErrorCode() const { return osError; }
  //@}

  /**@name Information functions */
  //@{
    /**Get the name of the RAS connection.

       @return
       String for IP address, or empty string if none.
     */
    const PString & GetName() const { return remoteName; }

    /**Get the IP address in dotted decimal form for the RAS connection.

       @return
       String for IP address, or empty string if none.
     */
    PString GetAddress();

    /**Get an array of names for all of the available remote connections on
       this system.

       @return
       Array of strings for remote connection names.
     */
    static PStringArray GetAvailableNames();
  //@}

  /**@name Configuration functions */
  //@{
    /// Structure for a RAS configuration.
    struct Configuration {
      /// Device name for connection eg /dev/modem
      PString device;
      /// Telephone number to call to make the connection.
      PString phoneNumber;
      /// IP address of local machine after connection is made.
      PString ipAddress;
      /// DNS host on remote site.
      PString dnsAddress;
      /// Script name for doing remote log in.
      PString script;
      /// Sub-entry number when Multi-link PPP is used.
      PINDEX  subEntries;
      /// Always establish maximum bandwidth when Multi-link PPP is used.
      PBoolean    dialAllSubEntries;
    };

    /**Get the configuration of the specified remote access connection.

       @return
       <code>Connected</code> if the configuration information was obtained,
       <code>NoNameOrNumber</code> if the particular RAS name does not exist,
       <code>NotInstalled</code> if there is no RAS support in the operating system,
       <code>GeneralFailure</code> on any other error.
     */
    Status GetConfiguration(
      Configuration & config  ///< Configuration of remote connection
    );

    /**Get the configuration of the specified remote access connection.

       @return
       <code>Connected</code> if the configuration information was obtained,
       <code>NoNameOrNumber</code> if the particular RAS name does not exist,
       <code>NotInstalled</code> if there is no RAS support in the operating system,
       <code>GeneralFailure</code> on any other error.
     */
    static Status GetConfiguration(
      const PString & name,   ///< Remote connection name to get configuration
      Configuration & config  ///< Configuration of remote connection
    );

    /**Set the configuration of the specified remote access connection.

       @return
       <code>Connected</code> if the configuration information was set,
       <code>NoNameOrNumber</code> if the particular RAS name does not exist,
       <code>NotInstalled</code> if there is no RAS support in the operating system,
       <code>GeneralFailure</code> on any other error.
     */
    Status SetConfiguration(
      const Configuration & config,  ///< Configuration of remote connection
      PBoolean create = false            ///< Flag to create connection if not present
    );

    /**Set the configuration of the specified remote access connection.

       @return
       <code>Connected</code> if the configuration information was set,
       <code>NoNameOrNumber</code> if the particular RAS name does not exist,
       <code>NotInstalled</code> if there is no RAS support in the operating system,
       <code>GeneralFailure</code> on any other error.
     */
    static Status SetConfiguration(
      const PString & name,          ///< Remote connection name to configure
      const Configuration & config,  ///< Configuration of remote connection
      PBoolean create = false            ///< Flag to create connection if not present
    );

    /**Remove the specified remote access connection.

       @return
       <code>Connected</code> if the configuration information was removed,
       <code>NoNameOrNumber</code> if the particular RAS name does not exist,
       <code>NotInstalled</code> if there is no RAS support in the operating system,
       <code>GeneralFailure</code> on any other error.
     */
    static Status RemoveConfiguration(
      const PString & name          ///< Remote connection name to configure
    );
  //@}
    
  protected:
    PString remoteName;
    PString userName;
    PString password;
    DWORD osError;

  private:
    PRemoteConnection(const PRemoteConnection &) { }
    void operator=(const PRemoteConnection &) { }
    void Construct();


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/remconn.h"
#else
#include "unix/ptlib/remconn.h"
#endif
};


#endif // PTLIB_REMOTECONNECTION_H


// End Of File ///////////////////////////////////////////////////////////////
