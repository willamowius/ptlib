/*
 * inetprot.h
 *
 * Internet Protocol ancestor channel class
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

#ifndef PTLIB_INETPROT_H
#define PTLIB_INETPROT_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


class PSocket;
class PIPSocket;


/** A TCP/IP socket for process/application layer high level protocols. All of
   these protocols execute commands and responses in a standard manner.

   A command consists of a line starting with a short, case insensitive command
   string terminated by a space or the end of the line. This may be followed
   by optional arguments.

   A response to a command is usually a number and/or a short string eg "OK".
   The response may be followed by additional information about the response
   but this is not typically used by the protocol. It is only for user
   information and may be tracked in log files etc.

   All command and reponse lines of the protocol are terminated by a CR/LF
   pair. A command or response line may be followed by additional data as
   determined by the protocol, but this data is "outside" the protocol
   specification as defined by this class.

   The default read timeout is to 10 minutes by the constructor.
 */
class PInternetProtocol : public PIndirectChannel
{
  PCLASSINFO(PInternetProtocol, PIndirectChannel)

  protected:
    PInternetProtocol(
      const char * defaultServiceName, ///< Service name for the protocol.
      PINDEX cmdCount,                 ///< Number of command strings.
      char const * const * cmdNames    ///< Strings for each command.
    );
    // Create an unopened TCP/IP protocol socket channel.


  public:
  // Overrides from class PChannel.
    /** Low level read from the channel.

       This override also supports the mechanism in the <A>UnRead()</A>
       function allowing characters to be be "put back" into the data stream.
       This allows a look-ahead required by the logic of some protocols. This
       is completely independent of the standard iostream mechanisms which do
       not support the level of timeout control required by the protocols.

       @return
       true if at least len bytes were written to the channel.
     */
    virtual PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /** Low level write to the channel.

       This override assures that the sequence CR/LF/./CR/LF does not occur by
       byte stuffing an extra '.' character into the data stream, whenever a
       line begins with a '.' character.

       Note that this only occurs if the member variable
       <CODE>stuffingState</CODE> has been set to some value other than
       <CODE>DontStuff</CODE>, usually <CODE>StuffIdle</CODE>. Also, if the
       <CODE>newLineToCRLF</CODE> member variable is true then all occurrences
       of a '\\n' character will be translated to a CR/LF pair.

       @return
       true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );

     /** Set the maximum timeout between characters within a line. Default
        value is 10 seconds.
      */
     void SetReadLineTimeout(
       const PTimeInterval & t
     );

  // New functions for class.
    /** Connect a socket to a remote host for the internet protocol.

       @return
       true if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Connect(
      const PString & address,    ///< Address of remote machine to connect to.
      WORD port = 0               ///< Port number to use for the connection.
    );
    virtual PBoolean Connect(
      const PString & address,    ///< Address of remote machine to connect to.
      const PString & service     ///< Service name to use for the connection.
    );

    /** Accept a server socket to a remote host for the internet protocol.

       @return
       true if the channel was successfully connected to the remote host.
     */
    virtual PBoolean Accept(
      PSocket & listener    ///< Address of remote machine to connect to.
    );

    /** Get the default service name or port number to use in socket
       connections.

       @return
       string for the default service name.
     */
    const PString & GetDefaultService() const;

    /** Get the eventual socket for the series of indirect channels that may
       be between the current protocol and the actual I/O channel.

       This will assert if the I/O channel is not an IP socket.

       @return
       true if the string and CR/LF were completely written.
     */
    PIPSocket * GetSocket() const;

    /** Write a string to the socket channel followed by a CR/LF pair. If there
       are any lone CR or LF characters in the <CODE>line</CODE> parameter
       string, then these are translated into CR/LF pairs.

       @return
       true if the string and CR/LF were completely written.
     */
    virtual PBoolean WriteLine(
      const PString & line ///< String to write as a command line.
    );

    /** Read a string from the socket channel up to a CR/LF pair.
    
       If the <CODE>unstuffLine</CODE> parameter is set then the function will
       remove the '.' character from the start of any line that begins with
       two consecutive '.' characters. A line that has is exclusively a '.'
       character will make the function return false.

       Note this function will block for the time specified by the
       <A>PChannel::SetReadTimeout()</A> function for only the first character
       in the line. The rest of the characters must each arrive within the time
       set by the <CODE>readLineTimeout</CODE> member variable. The timeout is
       set back to the original setting when the function returns.

       @return
       true if a CR/LF pair was received, false if a timeout or error occurred.
     */
    virtual PBoolean ReadLine(
      PString & line,             ///< String to receive a CR/LF terminated line.
      PBoolean allowContinuation = false  ///< Flag to handle continued lines.
    );

    /** Put back the characters into the data stream so that the next
       <A>Read()</A> function call will return them first.
     */
    virtual void UnRead(
      int ch                ///< Individual character to be returned.
    );
    virtual void UnRead(
      const PString & str   ///< String to be put back into data stream.
    );
    virtual void UnRead(
      const void * buffer,  ///< Characters to be put back into data stream.
      PINDEX len            ///< Number of characters to be returned.
    );

    /** Write a single line for a command. The command name for the command
       number is output, then a space, the the <CODE>param</CODE> string
       followed at the end with a CR/LF pair.

       If the <CODE>cmdNumber</CODE> parameter is outside of the range of
       valid command names, then the function does not send anything and
       returns false.

       This function is typically used by client forms of the socket.

       @return
       true if the command was completely written.
     */
    virtual PBoolean WriteCommand(
      PINDEX cmdNumber       ///< Number of command to write.
    );
    virtual PBoolean WriteCommand(
      PINDEX cmdNumber,      ///< Number of command to write.
      const PString & param  ///< Extra parameters required by the command.
    );

    /** Read a single line of a command which ends with a CR/LF pair. The
       command number for the command name is parsed from the input, then the
       remaining text on the line is returned in the <CODE>args</CODE>
       parameter.

       If the command does not match any of the command names then the entire
       line is placed in the <CODE>args</CODE> parameter and a value of
       P_MAX_INDEX is returned.

       Note this function will block for the time specified by the
       <A>PChannel::SetReadTimeout()</A> function.

       This function is typically used by server forms of the socket.

       @return
       true if something was read, otherwise an I/O error occurred.
     */
    virtual PBoolean ReadCommand(
      PINDEX & num,
       ///< Number of the command parsed from the command line, or P_MAX_INDEX
       ///< if no match.
      PString & args  ///< String to receive the arguments to the command.
    );

    /** Write a response code followed by a text string describing the response
       to a command. The form of the response is to place the code string,
       then the info string.
       
       If the <CODE>info</CODE> parameter has multiple lines then each line
       has the response code at the start. A '-' character separates the code
       from the text on all lines but the last where a ' ' character is used.

       The first form assumes that the response code is a 3 digit numerical
       code. The second form allows for any arbitrary string to be the code.

       This function is typically used by server forms of the socket.

       @return
       true if the response was completely written.
     */
    virtual PBoolean WriteResponse(
      unsigned numericCode, ///< Response code for command response.
      const PString & info  ///< Extra information available after response code.
    );
    virtual PBoolean WriteResponse(
      const PString & code, ///< Response code for command response.
      const PString & info  ///< Extra information available after response code.
    );

    /** Read a response code followed by a text string describing the response
       to a command. The form of the response is to have the code string,
       then the info string.
       
       The response may have multiple lines in it. A '-' character separates
       the code from the text on all lines but the last where a ' ' character
       is used. The <CODE>info</CODE> parameter will have placed in it all of
       the response lines separated by a single '\\n' character.

       The first form places the response code and info into the protected
       member variables <CODE>lastResponseCode</CODE> and
       <CODE>lastResponseInfo</CODE>.

       This function is typically used by client forms of the socket.

       @return
       true if the response was completely read without a socket error.
     */
    virtual PBoolean ReadResponse();
    virtual PBoolean ReadResponse(
      int & code,      ///< Response code for command response.
      PString & info   ///< Extra information available after response code.
    );

    /** Write a command to the socket, using <CODE>WriteCommand()</CODE> and
       await a response using <CODE>ReadResponse()</CODE>. The first character
       of the response is returned, as well as the entire response being saved
       into the protected member variables <CODE>lastResponseCode</CODE> and
       <CODE>lastResponseInfo</CODE>.

       This function is typically used by client forms of the socket.

       @return
       First character of response string or '\\0' if a socket error occurred.
     */
    virtual int ExecuteCommand(
      PINDEX cmdNumber       ///< Number of command to write.
    );
    virtual int ExecuteCommand(
      PINDEX cmdNumber,      ///< Number of command to write.
      const PString & param  ///< Extra parameters required by the command.
    );

    /** Return the code associated with the last response received by the
       socket.

       @return
       Response code
    */
    int GetLastResponseCode() const;

    /** Return the last response received by the socket.

       @return
       Response as a string
    */
    PString GetLastResponseInfo() const;


  protected:
    /** Parse a response line string into a response code and any extra info
       on the line. Results are placed into the member variables
       <CODE>lastResponseCode</CODE> and <CODE>lastResponseInfo</CODE>.

       The default bahaviour looks for a space or a '-' and splits the code
       and info either side of that character, then returns false.

       @return
       Position of continuation character in response, 0 if no continuation
       lines are possible.
     */
    virtual PINDEX ParseResponse(
      const PString & line ///< Input response line to be parsed
    );


    PString defaultServiceName;
    // Default Service name to use for the internet protocol socket.

    PStringArray commandNames;
    // Names of each of the command codes.

    PCharArray unReadBuffer;
    // Buffer for characters put back into the data stream.

    PINDEX unReadCount;
    // Buffer count for characters put back into the data stream.

    PTimeInterval readLineTimeout;
    // Time for characters in a line to be received.

    enum StuffState {
      DontStuff, StuffIdle, StuffCR, StuffCRLF, StuffCRLFdot, StuffCRLFdotCR
    } stuffingState;
    // Do byte stuffing of '.' characters in output to the socket channel.

    PBoolean newLineToCRLF;
    // Translate \\n characters to CR/LF pairs.

    int     lastResponseCode;
    PString lastResponseInfo;
    // Responses

  private:
    PBoolean AttachSocket(PIPSocket * socket);
};



#endif // PTLIB_INETPROT_H


// End Of File ///////////////////////////////////////////////////////////////
