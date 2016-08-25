/*
 * modem.h
 *
 * AT command set modem on asynchonous port class.
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

#ifndef PTLIB_MODEM_H
#define PTLIB_MODEM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#include <ptlib/serchan.h>


/** A class representing a modem attached to a serial port. This adds the usual
   modem operations to the basic serial port.
   
   A modem object is always in a particular state. This state determines what
   operations are allowed which then move the object to other states. The
   operations are the exchange of strings in "chat" script.
   
   The following defaults are used for command strings:
       initialise         <CODE>ATZ\\r\\w2sOK\\w100m</CODE>
       deinitialise       <CODE>\\d2s+++\\d2sATH0\\r</CODE>
       pre-dial           <CODE>ATDT</CODE>
       post-dial          <CODE>\\r</CODE>
       busy reply         <CODE>BUSY</CODE>
       no carrier reply   <CODE>NO CARRIER</CODE>
       connect reply      <CODE>CONNECT</CODE>
       hang up            <CODE>\\d2s+++\\d2sATH0\\r</CODE>

 */
class PModem : public PSerialChannel
{
  PCLASSINFO(PModem, PSerialChannel)

  public:
    /** Create a modem object on the serial port specified. If no port was
       specified do not open it. It does not initially have a valid port name.
       
       See the <A>PSerialChannel</A> class for more information on the
       parameters.
     */
    PModem();
    PModem(
      const PString & port,   ///< Serial port name to open.
      DWORD speed = 0,        ///< Speed of serial port.
      BYTE data = 0,          ///< Number of data bits for serial port.
      Parity parity = DefaultParity,  ///< Parity for serial port.
      BYTE stop = 0,          ///< Number of stop bits for serial port.
      FlowControl inputFlow = DefaultFlowControl,   ///< Input flow control.
      FlowControl outputFlow = DefaultFlowControl   ///< Output flow control.
    );

#if P_CONFIG_FILE
    /** Open the modem serial channel obtaining the parameters from standard
       variables in the configuration file. Note that it assumed that the
       correct configuration file section is already set.
     */
    PModem(
      PConfig & cfg   ///< Configuration file to read parameters from.
    );
#endif // P_CONFIG_FILE


  // Overrides from class PChannel
    virtual PBoolean Close();
    // Close the modem serial port channel.


  // Overrides from class PSerialChannel
    /** Open the modem serial channel on the specified port.
       
       See the <A>PSerialChannel</A> class for more information on the
       parameters.
       
       @return
       true if the modem serial port was successfully opened.
     */
    virtual PBoolean Open(
      const PString & port,   ///< Serial port name to open.
      DWORD speed = 0,        ///< Speed of serial port.
      BYTE data = 0,          ///< Number of data bits for serial port.
      Parity parity = DefaultParity,  ///< Parity for serial port.
      BYTE stop = 0,          ///< Number of stop bits for serial port.
      FlowControl inputFlow = DefaultFlowControl,   ///< Input flow control.
      FlowControl outputFlow = DefaultFlowControl   ///< Output flow control.
    );

#if P_CONFIG_FILE
    /** Open the modem serial port obtaining the parameters from standard
       variables in the configuration file. Note that it assumed that the
       correct configuration file section is already set.

       @return
       true if the modem serial port was successfully opened.
     */
    virtual PBoolean Open(
      PConfig & cfg   ///< Configuration file to read parameters from.
    );

    virtual void SaveSettings(
      PConfig & cfg   ///< Configuration file to write parameters to.
    );
    // Save the current modem serial port settings into the configuration file.
#endif // P_CONFIG_FILE


  // New member functions
    /** Set the modem initialisation meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\s</CODE> before the string.
     */
    void SetInitString(
      const PString & str   ///< New initialisation command string.
    );

    /** Get the modem initialisation meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for initialisation command.
     */
    PString GetInitString() const;

    /** The modem is in a state that allows the initialise to start.
    
       @return
       true if the <A>Initialise()</A> function may proceeed.
     */
    PBoolean CanInitialise() const;

    /** Send the initialisation meta-command string to the modem. The return
       value indicates that the conditions for the operation to start were met,
       ie the serial port was open etc and the command was successfully
       sent with all replies met.

       @return
       true if command string sent successfully and the objects state has
       changed.
     */
    PBoolean Initialise();

    /** Set the modem de-initialisation meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\s</CODE> before the string.
     */
    void SetDeinitString(
      const PString & str   ///< New de-initialisation command string.
    );

    /** Get the modem de-initialisation meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for de-initialisation command.
     */
    PString GetDeinitString() const;

    /** The modem is in a state that allows the de-initialise to start.
    
       @return
       true if the <A>Deinitialise()</A> function may proceeed.
     */
    PBoolean CanDeinitialise() const;

    /** Send the de-initialisation meta-command string to the modem. The return
       value indicates that the conditions for the operation to start were met,
       ie the serial port was open etc and the command was successfully
       sent with all replies met.

       @return
       true if command string sent successfully and the objects state has
       changed.
     */
    PBoolean Deinitialise();

    /** Set the modem pre-dial meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\s</CODE> before the string.
     */
    void SetPreDialString(
      const PString & str   ///< New pre-dial command string.
    );

    /** Get the modem pre-dial meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for pre-dial command.
     */
    PString GetPreDialString() const;

    /** Set the modem post-dial meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is <EM>not</EM> an implied <CODE>\\s</CODE> before the
       string, unlike the pre-dial string.
     */
    void SetPostDialString(
      const PString & str   ///< New post-dial command string.
    );

    /** Get the modem post-dial meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for post-dial command.
     */
    PString GetPostDialString() const;

    /** Set the modem busy response meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\w120s</CODE> before the string. Also
       the <CODE>\\s</CODE> and <CODE>\\d</CODE> commands do not operate and
       will simply terminate the string match.
     */
    void SetBusyString(
      const PString & str   ///< New busy response command string.
    );

    /** Get the modem busy response meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for busy response command.
     */
    PString GetBusyString() const;

    /** Set the modem no carrier response meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\w120s</CODE> before the string. Also
       the <CODE>\\s</CODE> and <CODE>\\d</CODE> commands do not operate and
       will simply terminate the string match.
     */
    void SetNoCarrierString(
      const PString & str   ///< New no carrier response command string.
    );

    /** Get the modem no carrier response meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for no carrier response command.
     */
    PString GetNoCarrierString() const;

    /** Set the modem connect response meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\w120s</CODE> before the string. Also
       the <CODE>\\s</CODE> and <CODE>\\d</CODE> commands do not operate and
       will simply terminate the string match.
     */
    void SetConnectString(
      const PString & str   ///< New connect response command string.
    );

    /** Get the modem connect response meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for connect response command.
     */
    PString GetConnectString() const;

    /** The modem is in a state that allows the dial to start.
    
       @return
       true if the <A>Dial()</A> function may proceeed.
     */
    PBoolean CanDial() const;

    /** Send the dial meta-command strings to the modem. The return
       value indicates that the conditions for the operation to start were met,
       ie the serial port was open etc and the command was successfully
       sent with all replies met.

       The string sent to the modem is the concatenation of the pre-dial
       string, a <CODE>\\s</CODE>, the <CODE>number</CODE> parameter and the
       post-dial string.

       @return
       true if command string sent successfully and the objects state has
       changed.
     */
    PBoolean Dial(const PString & number);

    /** Set the modem hang up meta-command string.

       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       Note there is an implied <CODE>\\s</CODE> before the string.
     */
    void SetHangUpString(
      const PString & str   ///< New hang up command string.
    );

    /** Get the modem hang up meta-command string.
    
       See the <A>PChannel::SendCommandString()</A> function for more
       information on the format of the command string.

       @return
       string for hang up command.
     */
    PString GetHangUpString() const;

    /** The modem is in a state that allows the hang up to start.
    
       @return
       true if the <A>HangUp()</A> function may proceeed.
     */
    PBoolean CanHangUp() const;

    /** Send the hang up meta-command string to the modem. The return
       value indicates that the conditions for the operation to start were met,
       ie the serial port was open etc and the command was successfully
       sent with all replies met.

       @return
       true if command string sent successfully and the objects state has
       changed.
     */
    PBoolean HangUp();

    /** The modem is in a state that allows the user command to start.
    
       @return
       true if the <A>SendUser()</A> function may proceeed.
     */
    PBoolean CanSendUser() const;

    /** Send an arbitrary user meta-command string to the modem. The return
       value indicates that the conditions for the operation to start were met,
       ie the serial port was open etc and the command was successfully
       sent with all replies met.

       @return
       true if command string sent successfully.
     */
    PBoolean SendUser(
      const PString & str   ///< User command string to send.
    );

    void Abort();
    // Abort the current meta-string command operation eg dial, hang up etc.

    /** The modem is in a state that allows the user application to read from
       the channel. Reading while this is true can interfere with the operation
       of the meta-string processing. This function is only usefull when
       multi-threading is used.

       @return
       true if <A>Read()</A> operations are "safe".
     */
    PBoolean CanRead() const;

    enum Status {
      Unopened,           ///< Has not been opened yet
      Uninitialised,      ///< Is open but has not yet been initialised
      Initialising,       ///< Is currently initialising the modem
      Initialised,        ///< Has been initialised but is not connected
      InitialiseFailed,   ///< Initialisation sequence failed
      Dialling,           ///< Is currently dialling
      DialFailed,         ///< Dial failed
      AwaitingResponse,   ///< Dialling in progress, awaiting connection
      LineBusy,           ///< Dial failed due to line busy
      NoCarrier,          ///< Dial failed due to no carrier
      Connected,          ///< Dial was successful and modem has connected
      HangingUp,          ///< Is currently hanging up the modem
      HangUpFailed,       ///< The hang up failed
      Deinitialising,     ///< is currently de-initialising the modem
      DeinitialiseFailed, ///< The de-initialisation failed
      SendingUserCommand, ///< Is currently sending a user command
      NumStatuses
    };
    // Modem object states.

    /** Get the modem objects current state.

       @return
       modem status.
     */
    Status GetStatus() const;


  protected:
    // Member variables
    PString initCmd, deinitCmd, preDialCmd, postDialCmd,
            busyReply, noCarrierReply, connectReply, hangUpCmd;
      // Modem command meta-strings.

    Status status;
      // Current modem status
};


#endif // PTLIB_MODEM_H


// End Of File ///////////////////////////////////////////////////////////////
