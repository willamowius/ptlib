/*
 * serchan.h
 *
 * Asynchronous Serial I/O channel class.
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

#ifndef PTLIB_SERIALCHANNEL_H
#define PTLIB_SERIALCHANNEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class PConfig;


///////////////////////////////////////////////////////////////////////////////
// Serial Channel

/**This class defines an I/O channel that communicates via a serial port. This
   is usually an RS-232 port.
 */
class PSerialChannel : public PChannel
{
  PCLASSINFO(PSerialChannel, PChannel);

  public:
  /**@name Construction */
  //@{
    /// Create a new serial channel object, but do not open it.
    PSerialChannel();

    /// Configuration of serial port parity options.
    enum Parity {
      /// Use the default parity, ie do not change it.
      DefaultParity,    
      /// Set the port for no parity bit.
      NoParity,         
      /// Set the port to generate parity and make it even.
      EvenParity,       
      /// Set the port to generate parity and make it odd.
      OddParity,        
      /// Set the port parity bit to mark only.
      MarkParity,       
      /// Set the port parity bit to space only.
      SpaceParity       
    };

    /// Configuration of serial port flow control options.
    enum FlowControl {
      /// Use the default flow control, ie do not change it.
      DefaultFlowControl,
      /// Set the port for no flow control.
      NoFlowControl,    
      /// Set the port for software or XON/XOFF flow control.
      XonXoff,         
      /// Set the port for hardware or RTS/CTS flow control.
      RtsCts           
    };

    /**Create a serial channal.
       The channel is opened it on the specified port and with the specified
       attributes.
     */
    PSerialChannel(
      /**The name of the serial port to connect to. This is a platform
         dependent string and woiuld rarely be a literal. The static function
         <code>GetPortNames()</code> can be used to find the platforms serial ports.
       */
      const PString & port,
      /**Serial port speed or baud rate. The actual values possible here are
         platform dependent, but the standard value of 300, 1200, 2400, 4800,
         9600, 19200, 38400 always be legal.
       */
      DWORD speed = 0,
      /**Number of data bits for serial port. The actual values possible here
         are platform dependent, but 7 and 8 should always be legal.
       */
      BYTE data = 0,
      /**Parity for serial port. The actual values possible here are platform
         dependent, but <code>NoParity</code>, <code>OddParity</code> and
         <code>EvenParity</code> should always be legal.
       */
      Parity parity = DefaultParity,
      /**Number of stop bits for serial port. The actual values possible here
         are platform dependent, but 1 and 2 should always be legal.
       */
      BYTE stop = 0,
      /**Flow control for data from the remote system into this conputer.*/
      FlowControl inputFlow = DefaultFlowControl,
      /**Flow control for data from this conputer out to remote system. */
      FlowControl outputFlow = DefaultFlowControl      
    );

#if P_CONFIG_FILE
    /**Create a serial channal.
       The channel is opened using attributes obtained from standard variables
       in the configuration file. Note that it assumed that the correct
       configuration file section is already set.
     */
    PSerialChannel(
      PConfig & cfg  ///< Configuration file to read serial port attributes from.
    );
#endif // P_CONFIG_FILE

    /// Close the serial channel on destruction.
    ~PSerialChannel();
  //@}


  /**@name Open functions */
  //@{
    /**Open a serial channal.
       The channel is opened it on the specified port and with the specified
       attributes.
     */
    virtual PBoolean Open(
      /**The name of the serial port to connect to. This is a platform
         dependent string and woiuld rarely be a literal. The static function
         <code>GetPortNames()</code> can be used to find the platforms serial ports.
       */
      const PString & port,
      /**Serial port speed or baud rate. The actual values possible here are
         platform dependent, but the standard value of 300, 1200, 2400, 4800,
         9600, 19200, 38400 always be legal.
       */
      DWORD speed = 0,
      /**Number of data bits for serial port. The actual values possible here
         are platform dependent, but 7 and 8 should always be legal.
       */
      BYTE data = 0,
      /**Parity for serial port. The actual values possible here are platform
         dependent, but <code>NoParity</code>, <code>OddParity</code> and
         <code>EvenParity</code> should always be legal.
       */
      Parity parity = DefaultParity,
      /**Number of stop bits for serial port. The actual values possible here
         are platform dependent, but 1 and 2 should always be legal.
       */
      BYTE stop = 0,
      /**Flow control for data from the remote system into this conputer.*/
      FlowControl inputFlow = DefaultFlowControl,
      /**Flow control for data from this conputer out to remote system. */
      FlowControl outputFlow = DefaultFlowControl      
    );

#if P_CONFIG_FILE
    /**Open a serial channal.
       The channel is opened using attributes obtained from standard variables
       in the configuration file. Note that it assumed that the correct
       configuration file section is already set.
     */
    virtual PBoolean Open(
      PConfig & cfg  ///< Configuration file to read serial port attributes from.
    );
#endif // P_CONFIG_FILE

    /**Get a list of the available serial ports. This returns a set of
       platform dependent strings which describe the serial ports of the
       computer. For example under unix it may be "ttyS0", under MS-DOS or
       NT it would be "COM1" and for the Macintosh it could be "Modem".

       @return
       list of strings for possible serial ports.
     */
    static PStringList GetPortNames();
  //@}


  /**@name Configuration functions */
  //@{
    /**Set the speed (baud rate) of the serial channel.

       @return
       true if the change was successfully made.
     */
    PBoolean SetSpeed(
      DWORD speed   ///< New speed for serial channel.
    );

    /**Get the speed (baud rate) of the serial channel.

       @return
       current setting.
     */
    DWORD GetSpeed() const;

    /**Set the data bits (5, 6, 7 or 8) of the serial port.

       @return
       true if the change was successfully made.
     */
    PBoolean SetDataBits(
      BYTE data   ///< New number of data bits for serial channel.
    );

    /**Get the data bits (5, 6, 7 or 8) of the serial port.

       @return
       current setting.
     */
    BYTE GetDataBits() const;

    /**Set the parity of the serial port.

       @return
       true if the change was successfully made.
     */
    PBoolean SetParity(
      Parity parity   ///< New parity option for serial channel.
    );

    /**Get the parity of the serial port.

       @return
       current setting.
     */
    Parity GetParity() const;

    /**Set the stop bits (1 or 2) of the serial port.

       @return
       true if the change was successfully made.
     */
    PBoolean SetStopBits(
      BYTE stop   ///< New number of stop bits for serial channel.
    );

    /**Get the stop bits (1 or 2) of the serial port.

       @return
       current setting.
     */
    BYTE GetStopBits() const;

    /**Set the flow control (handshaking) protocol of the input to the serial
       port.

       @return
       true if the change was successfully made.
     */
    PBoolean SetInputFlowControl(
      FlowControl flowControl   ///< New flow control for serial channel input.
    );

    /**Get the flow control (handshaking) protocol of the input to the serial
       port.

       @return
       current setting.
     */
    FlowControl GetInputFlowControl() const;

    /**Set the flow control (handshaking) protocol of the output to the serial
       port.

       @return
       true if the change was successfully made.
     */
    PBoolean SetOutputFlowControl(
      FlowControl flowControl   ///< New flow control for serial channel output.
    );

    /**Get the flow control (handshaking) protocol of the output from the
       serial port.

       @return
       current setting.
     */
    FlowControl GetOutputFlowControl() const;

#if P_CONFIG_FILE
    /**Save the current port settings into the configuration file. Note that
       it assumed that the correct configuration file section is already set.
     */
    virtual void SaveSettings(
      PConfig & cfg   ///< Configuration file to save setting into.
    );
#endif // P_CONFIG_FILE
  //@}


  /**@name Status functions */
  //@{
    /** Set the Data Terminal Ready signal of the serial port. */
    void SetDTR(
      PBoolean state = true   ///< New state of the DTR signal.
    );

    /**Clear the Data Terminal Ready signal of the serial port. This is
       equivalent to <code>SetDTR(false)</code>.
     */
    void ClearDTR();

    /**Set the Request To Send signal of the serial port. */
    void SetRTS(
      PBoolean state = true   ///< New state of the RTS signal.
    );

    /**Clear the Request To Send signal of the serial port. This is equivalent
       to <code>SetRTS(false)</code>.
     */
    void ClearRTS();

    /** Set the break condition of the serial port. */
    void SetBreak(
      PBoolean state = true   ///< New state of the serial port break condition.
    );

    /**Clear the break condition of the serial port. This is equivalent to
       <code>SetBreak(false)</code>.
     */
    void ClearBreak();

    /**Get the Clear To Send signal of the serial port.
    
       @return
       true if the CTS signal is asserted.
     */
    PBoolean GetCTS();

    /**Get the Data Set Ready signal of the serial port.
    
       @return
       true if the DSR signal is asserted.
     */
    PBoolean GetDSR();

    /**Get the Data Carrier Detect signal of the serial port.
    
       @return
       true if the DCD signal is asserted.
     */
    PBoolean GetDCD();

    /**Get the Ring Indicator signal of the serial port.
    
       @return
       true if the RI signal is asserted.
     */
    PBoolean GetRing();
  //@}


  private:
    void Construct();
    // Platform dependent construct of the serial channel.


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/serchan.h"
#else
#include "unix/ptlib/serchan.h"
#endif
};


#endif // PTLIB_SERIALCHANNEL_H


// End Of File ///////////////////////////////////////////////////////////////
