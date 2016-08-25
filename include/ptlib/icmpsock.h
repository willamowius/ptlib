/*
 * icmpsock.h
 *
 * Internet Control Message Protocol socket I/O channel class.
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

#ifndef PTLIB_ICMPSOCKET_H
#define PTLIB_ICMPSOCKET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/pprocess.h>

/**Create a socket channel that uses allows ICMP commands in the Internal
   Protocol.
 */
class PICMPSocket : public PIPDatagramSocket
{
  PCLASSINFO(PICMPSocket, PIPDatagramSocket);

  public:
  /**@name Construction */
  //@{
    /**Create a TCP/IP protocol socket channel. If a remote machine address or
       a "listening" socket is specified then the channel is also opened.
     */
    PICMPSocket();
  //@}

  /**@name Status & Information */
  //@{
    /// Results of ICMP operation.
    enum PingStatus {
      PingSuccess,         // don't use Success - X11 defines this!
      NetworkUnreachable,
      HostUnreachable,
      PacketTooBig,
      RequestTimedOut,
      BadRoute,
      TtlExpiredTransmit,
      TtlExpiredReassembly,
      SourceQuench,
      MtuChange,
      GeneralError,
      NumStatuses
    };

    /// Information used by and obtained by the ping operation.
    class PingInfo {
      public:
        /// Create Ping information structure.
        PingInfo(WORD id = (WORD)PProcess::GetCurrentProcessID());

        /**@name Supplied data */
        //@{
        /// Arbitrary identifier for the ping.
        WORD identifier;         
        /// Sequence number for ping packet.
        WORD sequenceNum;        
        /// Time To Live for packet.
        BYTE ttl;                
        /// Send buffer (if NULL, defaults to 32 bytes).
        const BYTE * buffer;     
        /// Size of buffer (< 64k).
        PINDEX bufferSize;       
        //@}

        /**@name Returned data */
        //@{
        /// Time for packet to make trip.
        PTimeInterval delay;     
        /// Source address of reply packet.
        Address remoteAddr;      
        /// Destination address of reply packet.
        Address localAddr;       
        /// Status of the last ping operation
        PingStatus status;       
        //@}
    };
  //@}

  /**@name Ping */
  //@{
    /**Send an ECHO_REPLY message to the specified host and wait for a reply
       to be sent back.

       @return
       false if host not found or no response.
     */
    PBoolean Ping(
      const PString & host   ///< Host to send ping.
    );
    /**Send an ECHO_REPLY message to the specified host and wait for a reply
       to be sent back.

       @return
       false if host not found or no response.
     */
    PBoolean Ping(
      const PString & host,   ///< Host to send ping.
      PingInfo & info         ///< Information on the ping and reply.
    );
  //@}

  protected:
    const char * GetProtocolName() const;
    virtual PBoolean OpenSocket();
    virtual PBoolean OpenSocket(int ipAdressFamily);


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/icmpsock.h"
#else
#include "unix/ptlib/icmpsock.h"
#endif
};


#endif // PTLIB_ICMPSOCKET_H


// End Of File ///////////////////////////////////////////////////////////////
