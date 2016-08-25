/*
 * telnet.h
 *
 * TELNET Socket class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_TELNET_H
#define PTLIB_TELNET_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/sockets.h>


/** A TCP/IP socket for the TELNET high level protocol.
 */
class PTelnetSocket : public PTCPSocket
{
  PCLASSINFO(PTelnetSocket, PTCPSocket)

  public:
    PTelnetSocket();
    // Create an unopened TELNET socket.

    PTelnetSocket(
      const PString & address  ///< Address of remote machine to connect to.
    );
    // Create an opened TELNET socket.


  // Overrides from class PChannel
    /** Low level read from the channel. This function may block until the
       requested number of characters were read or the read timeout was
       reached. The GetLastReadCount() function returns the actual number
       of bytes read.

       The GetErrorCode() function should be consulted after Read() returns
       false to determine what caused the failure.

       The TELNET channel intercepts and escapes commands in the data stream to
       implement the TELNET protocol.

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the channel. This function will block until the
       requested number of characters are written or the write timeout is
       reached. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       The TELNET channel intercepts and escapes commands in the data stream to
       implement the TELNET protocol.

       Returns true if at least len bytes were written to the channel.
     */
    PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
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


    /** Connect a socket to a remote host on the specified port number. This is
       typically used by the client or initiator of a communications channel.
       This connects to a "listening" socket at the other end of the
       communications channel.

       The port number as defined by the object instance construction or the
       <A>PIPSocket::SetPort()</A> function.

       @return
       true if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address   ///< Address of remote machine to connect to.
    );


    /** Open a socket to a remote host on the specified port number. This is an
       "accepting" socket. When a "listening" socket has a pending connection
       to make, this will accept a connection made by the "connecting" socket
       created to establish a link.

       The port that the socket uses is the one used in the <A>Listen()</A>
       command of the <CODE>socket</CODE> parameter.

       Note that this function will block until a remote system connects to the
       port number specified in the "listening" socket.

       @return
       true if the channel was successfully opened.
     */
    virtual PBoolean Accept(
      PSocket & socket          ///< Listening socket making the connection.
    );


    /** This is callback function called by the system whenever out of band data
       from the TCP/IP stream is received. A descendent class may interpret
       this data according to the semantics of the high level protocol.

       The TELNET socket uses this for sychronisation.
     */
    virtual void OnOutOfBand(
      const void * buf,   ///< Data to be received as URGENT TCP data.
      PINDEX len          ///< Number of bytes pointed to by <CODE>buf</CODE>.
    );


  // New functions
    enum Command {
      IAC           = 255,    ///< Interpret As Command - escape character.
      DONT          = 254,    ///< You are not to use option.
      DO            = 253,    ///< Request to use option.
      WONT          = 252,    ///< Refuse use of option.
      WILL          = 251,    ///< Accept the use of option.
      SB            = 250,    ///< Subnegotiation begin.
      GoAhead       = 249,    ///< Function GA, you may reverse the line.
      EraseLine     = 248,    ///< Function EL, erase the current line.
      EraseChar     = 247,    ///< Function EC, erase the current character.
      AreYouThere   = 246,    ///< Function AYT, are you there?
      AbortOutput   = 245,    ///< Function AO, abort output stream.
      InterruptProcess = 244, ///< Function IP, interrupt process, permanently.
      Break         = 243,    ///< NVT character break.
      DataMark      = 242,    ///< Marker for connection cleaning.
      NOP           = 241,    ///< No operation.
      SE            = 240,    ///< Subnegotiation end.
      EndOfReccord  = 239,    ///< End of record for transparent mode.
      AbortProcess  = 238,    ///< Abort the entire process
      SuspendProcess= 237,    ///< Suspend the process.
      EndOfFile     = 236     ///< End of file marker.
    };
    // Defined telnet commands codes

    /** Send an escaped IAC command. The <CODE>opt</CODE> parameters meaning
       depends on the command being sent:

       <DL>
       <DT>DO, DONT, WILL, WONT    <DD><CODE>opt</CODE> is Options code.

       <DT>AbortOutput             <DD>true is flush buffer.

       <DT>InterruptProcess,
          Break, AbortProcess,
          SuspendProcess           <DD>true is synchronise.
       </DL>

       Synchronises the TELNET streams, inserts the data mark into outgoing
       data stream and sends an out of band data to the remote to flush all
       data in the stream up until the syncronisation command.

       @return
       true if the command was successfully sent.
     */
    PBoolean SendCommand(
      Command cmd,  ///< Command code to send
      int opt = 0  ///< Option for command code.
    );


    enum Options {
      TransmitBinary      = 0,    ///< Assume binary 8 bit data is transferred.
      EchoOption          = 1,    ///< Automatically echo characters sent.
      ReconnectOption     = 2,    ///< Prepare to reconnect
      SuppressGoAhead     = 3,    ///< Do not use the GA protocol.
      MessageSizeOption   = 4,    ///< Negatiate approximate message size
      StatusOption        = 5,    ///< Status packets are understood.
      TimingMark          = 6,    ///< Marker for synchronisation.
      RCTEOption          = 7,    ///< Remote controlled transmission and echo.
      OutputLineWidth     = 8,    ///< Negotiate about output line width.
      OutputPageSize      = 9,    ///< Negotiate about output page size.
      CRDisposition       = 10,   ///< Negotiate about CR disposition.
      HorizontalTabsStops = 11,   ///< Negotiate about horizontal tabstops.
      HorizTabDisposition = 12,   ///< Negotiate about horizontal tab disposition
      FormFeedDisposition = 13,   ///< Negotiate about formfeed disposition.
      VerticalTabStops    = 14,   ///< Negotiate about vertical tab stops.
      VertTabDisposition  = 15,   ///< Negotiate about vertical tab disposition.
      LineFeedDisposition = 16,   ///< Negotiate about output LF disposition.
      ExtendedASCII       = 17,   ///< Extended ascic character set.
      ForceLogout         = 18,   ///< Force logout.
      ByteMacroOption     = 19,   ///< Byte macro.
      DataEntryTerminal   = 20,   ///< data entry terminal.
      SupDupProtocol      = 21,   ///< supdup protocol.
      SupDupOutput        = 22,   ///< supdup output.
      SendLocation        = 23,   ///< Send location.
      TerminalType        = 24,   ///< Provide terminal type information.
      EndOfRecordOption   = 25,   ///< Record boundary marker.
      TACACSUID           = 26,   ///< TACACS user identification.
      OutputMark          = 27,   ///< Output marker or banner text.
      TerminalLocation    = 28,   ///< Terminals physical location information.
      Use3270RegimeOption = 29,   ///< 3270 regime.
      UseX3PADOption      = 30,   ///< X.3 PAD
      WindowSize          = 31,   ///< NAWS - Negotiate About Window Size.
      TerminalSpeed       = 32,   ///< Provide terminal speed information.
      FlowControl         = 33,   ///< Remote flow control.
      LineModeOption      = 34,   ///< Terminal in line mode option.
      XDisplayLocation    = 35,   ///< X Display location.
      EnvironmentOption   = 36,   ///< Provide environment information.
      AuthenticateOption  = 37,   ///< Authenticate option.
      EncriptionOption    = 38,   ///< Encryption option.
      EncryptionOption    = 38,   ///< Duplicate to fix spelling mistake and remain backwards compatible.
      ExtendedOptionsList = 255,  ///< Code for extended options.
      MaxOptions
    };
    // Defined TELNET options.


    /** Send DO request.

       @return
       true if the command was successfully sent.
     */
    virtual PBoolean SendDo(
      BYTE option    ///< Option to DO
    );

    /** Send DONT command.

       @return
       true if the command was successfully sent.
     */
    virtual PBoolean SendDont(
      BYTE option    ///< Option to DONT
    );

    /** Send WILL request.

       @return
       true if the command was successfully sent.
     */
    virtual PBoolean SendWill(
      BYTE option    ///< Option to WILL
    );

    /** Send WONT command.

       @return
       true if the command was successfully sent.
     */
    virtual PBoolean SendWont(
      BYTE option    ///< Option to WONT
    );

    enum SubOptionCodes {
      SubOptionIs       = 0,  ///< Sub-option is...
      SubOptionSend     = 1,  ///< Request to send option.
    };
    // Codes for sub option negotiation.

    /** Send a sub-option with the information given.

       @return
       true if the command was successfully sent.
     */
    PBoolean SendSubOption(
      BYTE code,          ///< Suboptions option code.
      const BYTE * info,  ///< Information to send.
      PINDEX len,         ///< Length of information.
      int subCode = -1    ///< Suboptions sub-code, -1 indicates no sub-code.
    );

    /** Set if the option on our side is possible, this does not mean it is set
       it only means that in response to a DO we WILL rather than WONT.
     */
    void SetOurOption(
      BYTE code,          ///< Option to check.
      PBoolean state = true   ///< New state for for option.
    ) { option[code].weCan = state; }

    /** Set if the option on their side is desired, this does not mean it is set
       it only means that in response to a WILL we DO rather than DONT.
     */
    void SetTheirOption(
      BYTE code,          ///< Option to check.
      PBoolean state = true  ///< New state for for option.
    ) { option[code].theyShould = state; }

    /** Determine if the option on our side is enabled.

       @return
       true if option is enabled.
     */
    PBoolean IsOurOption(
      BYTE code    ///< Option to check.
    ) const { return option[code].ourState == OptionInfo::IsYes; }

    /** Determine if the option on their side is enabled.

       @return
       true if option is enabled.
     */
    PBoolean IsTheirOption(
      BYTE code    ///< Option to check.
    ) const { return option[code].theirState == OptionInfo::IsYes; }

    void SetTerminalType(
      const PString & newType   ///< New terminal type description string.
    );
    // Set the terminal type description string for TELNET protocol.

    const PString & GetTerminalType() const { return terminalType; }
    // Get the terminal type description string for TELNET protocol.

    void SetWindowSize(
      WORD width,   ///< New window width.
      WORD height   ///< New window height.
    );
    // Set the width and height of the Network Virtual Terminal window.

    void GetWindowSize(
      WORD & width,   ///< Old window width.
      WORD & height   ///< Old window height.
    ) const;
    // Get the width and height of the Network Virtual Terminal window.


  protected:
    void Construct();
    // Common construct code for TELNET socket channel.

    /** This callback function is called by the system when it receives a DO
       request from the remote system.
       
       The default action is to send a WILL for options that are understood by
       the standard TELNET class and a WONT for all others.

       @return
       true if option is accepted.
     */
    virtual void OnDo(
      BYTE option   ///< Option to DO
    );

    /** This callback function is called by the system when it receives a DONT
       request from the remote system.
       
       The default action is to disable options that are understood by the
       standard TELNET class. All others are ignored.
     */
    virtual void OnDont(
      BYTE option   ///< Option to DONT
    );

    /** This callback function is called by the system when it receives a WILL
       request from the remote system.
       
       The default action is to send a DO for options that are understood by
       the standard TELNET class and a DONT for all others.
     */
    virtual void OnWill(
      BYTE option   ///< Option to WILL
    );

    /** This callback function is called by the system when it receives a WONT
       request from the remote system.

       The default action is to disable options that are understood by the
       standard TELNET class. All others are ignored.
     */
    virtual void OnWont(
      BYTE option   ///< Option to WONT
    );

    /** This callback function is called by the system when it receives a
       sub-option command from the remote system.
     */
    virtual void OnSubOption(
      BYTE code,          ///< Option code for sub-option data.
      const BYTE * info,  ///< Extra information being sent in the sub-option.
      PINDEX len          ///< Number of extra bytes.
    );


    /** This callback function is called by the system when it receives an
       telnet command that it does not do anything with.

       The default action displays a message to the <A>PError</A> stream
       (when <CODE>debug</CODE> is true) and returns true;

       @return
       true if next byte is not part of the command.
     */
    virtual PBoolean OnCommand(
      BYTE code  ///< Code received that could not be precessed.
    );


  // Member variables.
    struct OptionInfo {
      enum {
        IsNo, IsYes, WantNo, WantNoQueued, WantYes, WantYesQueued
      };
      unsigned weCan:1;      // We can do the option if they want us to do.
      unsigned ourState:3;
      unsigned theyShould:1; // They should if they will.
      unsigned theirState:3;
    };
    
    OptionInfo option[MaxOptions];
    // Information on protocol options.

    PString terminalType;
    // Type of terminal connected to telnet socket, defaults to "UNKNOWN"

    WORD windowWidth, windowHeight;
    // Size of the "window" used by the NVT.


  private:
    enum State {
      StateNormal,
      StateCarriageReturn,
      StateIAC,
      StateDo,
      StateDont,
      StateWill,
      StateWont,
      StateSubNegotiations,
      StateEndNegotiations
    };
    // Internal states for the TELNET decoder

    State state;
    // Current state of incoming characters.

    PBYTEArray subOption;
    // Storage for sub-negotiated options

    unsigned synchronising;
};


#endif // PTLIB_TELNET_H


// End Of File ///////////////////////////////////////////////////////////////
