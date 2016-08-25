/*
 * indchan.h
 *
 * Indirect I/O channel class.
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

#ifndef PTLIB_INDIRECTCHANNEL_H
#define PTLIB_INDIRECTCHANNEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/channel.h>
#include <ptlib/syncthrd.h>

/**This is a channel that operates indirectly through another channel(s). This
   allows for a protocol to operate through a "channel" mechanism and for its
   low level byte exchange (Read and Write) to operate via a completely
   different channel, eg TCP or Serial port etc.
 */
class PIndirectChannel : public PChannel
{
  PCLASSINFO(PIndirectChannel, PChannel);

  public:
  /**@name Construction */
  //@{
    /**Create a new indirect channel without any channels to redirect to. If
       an attempt to read or write is made before Open() is called the the
       functions will assert.
     */
    PIndirectChannel();

    /// Close the indirect channel, deleting read/write channels if desired.
    ~PIndirectChannel();
  //@}


  /**@name Overrides from class PObject */
  //@{
    /**Determine if the two objects refer to the same indirect channel. This
       actually compares the channel pointers.

       @return
       EqualTo if channel pointer identical.
     */
    Comparison Compare(
      const PObject & obj   ///< Another indirect channel to compare against.
    ) const;
  //@}


  /**@name Overrides from class PChannel */
  //@{
    /**Get the name of the channel. This is a combination of the channel
       pointers names (or simply the channel pointers name if the read and
       write channels are the same) or empty string if both null.
    
       @return
       string for the channel names.
     */
    virtual PString GetName() const;

    /**Close the channel. This will detach itself from the read and write
       channels and delete both of them if they are auto delete.

       @return
       true if the channel is closed.
     */
    virtual PBoolean Close();

    /**Determine if the channel is currently open and read and write operations
       can be executed on it. For example, in the <code>PFile</code> class it returns
       if the file is currently open.

       @return
       true if the channel is open.
     */
    virtual PBoolean IsOpen() const;

    /**Low level read from the channel. This function may block until the
       requested number of characters were read or the read timeout was
       reached. The GetLastReadCount() function returns the actual number
       of bytes read.

       This will use the <code>readChannel</code> pointer to actually do the
       read. If <code>readChannel</code> is null the this asserts.

       The GetErrorCode() function should be consulted after Read() returns
       false to determine what caused the failure.

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    virtual PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /**Low level write to the channel. This function will block until the
       requested number of characters are written or the write timeout is
       reached. The GetLastWriteCount() function returns the actual number
       of bytes written.

       This will use the <code>writeChannel</code> pointer to actually do the
       write. If <code>writeChannel</code> is null the this asserts.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       @return
       true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );

    /**Close one or both of the data streams associated with a channel.

       The behavour here is to pass the shutdown on to its read and write
       channels.

       @return
       true if the shutdown was successfully performed.
     */
    virtual PBoolean Shutdown(
      ShutdownValue option   ///< Flag for shut down of read, write or both.
    );

    /**Set local echo mode.
       For some classes of channel, e.g. PConsoleChannel, data read by this
       channel is automatically echoed. This disables the function so things
       like password entry can work.

       Default behaviour does nothing and return true if the channel is open.
      */
    virtual bool SetLocalEcho(
      bool localEcho
    );


    /**This function returns the eventual base channel for reading of a series
       of indirect channels provided by descendents of <code>PIndirectChannel</code>.

       The behaviour for this function is to return "this".
       
       @return
       Pointer to base I/O channel for the indirect channel.
     */
    virtual PChannel * GetBaseReadChannel() const;

    /**This function returns the eventual base channel for writing of a series
       of indirect channels provided by descendents of <code>PIndirectChannel</code>.

       The behaviour for this function is to return "this".
       
       @return
       Pointer to base I/O channel for the indirect channel.
     */
    virtual PChannel * GetBaseWriteChannel() const;

    /** Get error message description.
        Return a string indicating the error message that may be displayed to
       the user. The error for the last I/O operation in this object is used.
      @return Operating System error description string.
     */
    virtual PString GetErrorText(
      ErrorGroup group = NumErrorGroups   ///< Error group to get
    ) const;
  //@}

  /**@name Channel establish functions */
  //@{
    /**Set the channel for both read and write operations. This then checks
       that they are open and then calls the OnOpen() virtual function. If
       it in turn returns true then the Open() function returns success.

       @return
       true if both channels are set, open and OnOpen() returns true.
     */
    PBoolean Open(
      PChannel & channel   ///< Channel to be used for both read and write operations.
    );

    /**Set the channel for both read and write operations. This then checks
       that they are open and then calls the OnOpen() virtual function. If
       it in turn returns true then the Open() function returns success.

       The channel pointed to by <code>channel</code> may be automatically deleted
       when the PIndirectChannel is destroyed or a new subchannel opened.

       @return
       true if both channels are set, open and OnOpen() returns true.
     */
    PBoolean Open(
      PChannel * channel,      ///< Channel to be used for both read and write operations.
      PBoolean autoDelete = true   ///< Automatically delete the channel
    );

    /**Set the channel for both read and write operations. This then checks
       that they are open and then calls the OnOpen() virtual function. If
       it in turn returns true then the Open() function returns success.

       The channels pointed to by <code>readChannel</code> and <code>writeChannel</code> may be
       automatically deleted when the PIndirectChannel is destroyed or a
       new subchannel opened.

       @return
       true if both channels are set, open and OnOpen() returns true.
     */
    PBoolean Open(
      PChannel * readChannel,      ///< Channel to be used for both read operations.
      PChannel * writeChannel,     ///< Channel to be used for both write operations.
      PBoolean autoDeleteRead = true,  ///< Automatically delete the read channel
      PBoolean autoDeleteWrite = true  ///< Automatically delete the write channel
    );

    /**Get the channel used for read operations.
    
       @return
       pointer to the read channel.
     */
    PChannel * GetReadChannel() const;

    /**Set the channel for read operations.

       @return
       Returns true if both channels are set and are both open.
     */
    bool SetReadChannel(
      PChannel * channel,         ///< Channel to be used for both read operations.
      bool autoDelete = true,     ///< Automatically delete the channel
      bool closeExisting = false  ///< Close (and auto-delete) the existing read channel
    );

    /**Get the channel used for write operations.
    
       @return
       pointer to the write channel.
     */
    PChannel * GetWriteChannel() const;

    /**Set the channel for read operations.

       @return
       Returns true if both channels are set and are both open.
    */
    PBoolean SetWriteChannel(
      PChannel * channel,         ///< Channel to be used for both read operations.
      bool autoDelete = true,     ///< Automatically delete the channel
      bool closeExisting = false  ///< Close (and auto-delete) the existing read channel
    );
  //@}


  protected:
    /**This callback is executed when the Open() function is called with
       open channels. It may be used by descendent channels to do any
       handshaking required by the protocol that channel embodies.

       The default behaviour is to simply return true.

       @return
       Returns true if the protocol handshaking is successful.
     */
    virtual PBoolean OnOpen();


  // Member variables
    /// Channel for read operations.
    PChannel * readChannel;

    /// Automatically delete read channel on destruction.
    PBoolean readAutoDelete;

    /// Channel for write operations.
    PChannel * writeChannel;

    /// Automatically delete write channel on destruction.
    PBoolean writeAutoDelete;

    /// Race condition prevention on closing channel
    PReadWriteMutex channelPointerMutex;
};


#endif // PTLIB_INDIRECTCHANNEL_H


// End Of File ///////////////////////////////////////////////////////////////
