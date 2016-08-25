/*
 * tcpsock.h
 *
 * Transmission Control Protocol socket channel class.
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

#ifndef PTLIB_TCPSOCKET_H
#define PTLIB_TCPSOCKET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


/** A socket that uses the TCP transport on the Internet Protocol.
 */
class PTCPSocket : public PIPSocket
{
  PCLASSINFO(PTCPSocket, PIPSocket);
  public:
  /**@name Construction. */
  //@{
    /**Create a TCP/IP protocol socket channel. If a remote machine address or
       a "listening" socket is specified then the channel is also opened.

       Note that what looks like a "copy" constructor here is really a
       the accept of a "listening" socket the same as the PSocket & parameter
       version constructor.
     */
    PTCPSocket(
      WORD port = 0             ///< Port number to use for the connection.
    );
    PTCPSocket(
      const PString & service   ///< Service name to use for the connection.
    );
    PTCPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      WORD port                 ///< Port number to use for the connection.
    );
    PTCPSocket(
      const PString & address,  ///< Address of remote machine to connect to.
      const PString & service   ///< Service name to use for the connection.
    );
    PTCPSocket(
      PSocket & socket          ///< Listening socket making the connection.
    );
    PTCPSocket(
      PTCPSocket & tcpSocket    ///< Listening socket making the connection.
    );
  //@}

  /**@name Overrides from class PObject. */
  //@{
    /** Create a copy of the class on the heap. The exact semantics of the
       descendent class determine what is required to make a duplicate of the
       instance. Not all classes can even {\b do} a clone operation.
       
       The main user of the clone function is the <code>PDictionary</code> class as
       it requires copies of the dictionary keys.

       The default behaviour is for this function to assert.

       @return
       pointer to new copy of the class instance.
     */
    virtual PObject * Clone() const;
  //@}

  /**@name Overrides from class PChannel. */
  //@{
    /** Low level write to the channel. This function will block until the
       requested number of characters are written or the write timeout is
       reached. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       This override repeatedly writes if there is no error until all of the
       requested bytes have been written.

       @return
       true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );
  //@}

  /**@name Overrides from class PSocket. */
  //@{
    /** Listen on a socket for a remote host on the specified port number. This
       may be used for server based applications. A "connecting" socket begins
       a connection by initiating a connection to this socket. An active socket
       of this type is then used to generate other "accepting" sockets which
       establish a two way communications channel with the "connecting" socket.

       The Listen() method is only used once in the entire lifetime of
       the socket. In contrast, the Accept() may be used many more
       times. Thus, if you have a socket to collect incoming
       connections, do a Listen() on it to get things ready, and then
       Accept(). With each incoming connection, you will do Accept
       again.

       If the <code>port</code> parameter is zero then the port number as
       defined by the object instance construction or the
       <code>PIPSocket::SetPort()</code> function.

       @return
       true if the channel was successfully opened.
     */
    virtual PBoolean Listen(
      unsigned queueSize = 5,  ///< Number of pending accepts that may be queued.
      WORD port = 0,           ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Can't listen more than once.
    );
    virtual PBoolean Listen(
      const Address & bind,     ///< Local interface address to bind to.
      unsigned queueSize = 5,   ///< Number of pending accepts that may be queued.
      WORD port = 0,            ///< Port number to use for the connection.
      Reusability reuse = AddressIsExclusive ///< Can/Can't listen more than once.
    );

    /** Open a socket to a remote host on the specified port number. This is an
       "accepting" socket. When a "listening" socket has a pending connection
       to make, this will accept a connection made by the "connecting" socket
       created to establish a link.

       The port that the socket uses is the one used in the <code>Listen()</code>
       command of the <code>socket</code> parameter.

       Note that this function will block until a remote system connects to the
       port number specified in the "listening" socket.

       @return
       true if the channel was successfully opened.
     */
    virtual PBoolean Accept(
      PSocket & socket          ///< Listening socket making the connection.
    );
  //@}

  /**@name New functions for class. */
  //@{
    /** Write out of band data from the TCP/IP stream. This data is sent as TCP
       URGENT data which does not follow the usual stream sequencing of the
       normal channel data.

       This is subject to the write timeout and sets the
       <code>lastWriteCount</code> member variable in the same way as usual
       <code>PChannel::Write()</code> function.
       
       @return
       true if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteOutOfBand(
      const void * buf,   ///< Data to be written as URGENT TCP data.
      PINDEX len          ///< Number of bytes pointed to by <code>buf</code>.
    );

    /** This is callback function called by the system whenever out of band data
       from the TCP/IP stream is received. A descendent class may interpret
       this data according to the semantics of the high level protocol.

       The default behaviour is for the out of band data to be ignored.
     */
    virtual void OnOutOfBand(
      const void * buf,   ///< Data to be received as URGENT TCP data.
      PINDEX len          ///< Number of bytes pointed to by <code>buf</code>.
    );
  //@}


  protected:
    // Open an IPv4 socket (for backward compatibility)
    virtual PBoolean OpenSocket();

    // Open an IPv4 or IPv6 socket
    virtual PBoolean OpenSocket(
      int ipAdressFamily
    );

    virtual const char * GetProtocolName() const;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/tcpsock.h"
#else
#include "unix/ptlib/tcpsock.h"
#endif
};

#endif // PTLIB_TCPSOCKET_H


// End Of File ///////////////////////////////////////////////////////////////
