/*
 * ftp.h
 *
 * File Transfer Protocol Server/Client channel classes
 *  As per RFC 959 and RFC 1123
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

#ifndef PTLIB_FTP_H
#define PTLIB_FTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/inetprot.h>
#include <ptlib/sockets.h>


class PURL;


/**
File Transfer Protocol base class.
*/
class PFTP : public PInternetProtocol
{
  PCLASSINFO(PFTP, PInternetProtocol);
  public:
    /// FTP commands
    enum Commands { 
      USER, PASS, ACCT, CWD, CDUP, SMNT, QUIT, REIN, PORT, PASV, TYPE,
      STRU, MODE, RETR, STOR, STOU, APPE, ALLO, REST, RNFR, RNTO, ABOR,
      DELE, RMD, MKD, PWD, LIST, NLST, SITE, SYST, STATcmd, HELP, NOOP,
      NumCommands
    };

    /// Types for file transfer
    enum RepresentationType {
      ASCII,
      EBCDIC,
      Image
    };

    /// File transfer mode on data channel
    enum DataChannelType {
      NormalPort,
      Passive
    };

    /// Listing types
    enum NameTypes {
      ShortNames,
      DetailedNames
    };

    enum {
      DefaultPort = 21
    };

    /** Send the PORT command for a transfer.
     @return Boolean indicated PORT command was successful
    */
    PBoolean SendPORT(
      const PIPSocket::Address & addr, ///< Address for PORT connection. IP address to connect back to
      WORD port                        ///< Port number for PORT connection.
    );


  protected:
    /// Construct an ineternal File Transfer Protocol channel.
    PFTP();
};


/**
File Transfer Protocol client channel class.
*/
class PFTPClient : public PFTP
{
  PCLASSINFO(PFTPClient, PFTP);
  public:
    /// Declare an FTP client socket.
    PFTPClient();

    /// Delete and close the socket.
    ~PFTPClient();


  /**@name Overrides from class PSocket. */
  //@{
    /** Close the socket, and if connected as a client, QUITs from server.

       @return
       true if the channel was closed and the QUIT accepted by the server.
     */
    virtual PBoolean Close();

  //@}

  /**@name New functions for class */
  //@{
    /**Open host using TCP
     */
    bool OpenHost(
      const PString & host,
      WORD port = DefaultPort
    );

    /** Log in to the remote host for FTP.

       @return
       true if the log in was successfull.
     */
    PBoolean LogIn(
      const PString & username,   ///< User name for FTP log in.
      const PString & password    ///< Password for the specified user name.
    );

    /** Get the type of the remote FTP server system, eg Unix, WindowsNT etc.

       @return
       String for the type of system.
     */
    PString GetSystemType();

    /** Set the transfer type.

       @return
       true if transfer type set.
     */
    PBoolean SetType(
      RepresentationType type   ///< RepresentationTypeof file to transfer
    );

    /** Change the current directory on the remote FTP host.

       @return
       true if the log in was successfull.
     */
    PBoolean ChangeDirectory(
      const PString & dirPath     ///< New directory
    );

    /** Get the current working directory on the remote FTP host.

       @return
       String for the directory path, or empty string if an error occurred.
     */
    PString GetCurrentDirectory();

    /** Get a list of files from the current working directory on the remote
       FTP host.

       @return
       String array for the files in the directory.
     */
    PStringArray GetDirectoryNames(
      NameTypes type = ShortNames,        ///< Detail level on a directory entry.
      DataChannelType channel = Passive   ///< Data channel type.
    );
    /** Get a list of files from the current working directory on the remote
       FTP host.

       @return
       String array for the files in the directory.
     */
    PStringArray GetDirectoryNames(
      const PString & path,               ///< Name to get details for.
      NameTypes type = ShortNames,        ///< Detail level on a directory entry.
      DataChannelType channel = Passive   ///< Data channel type.
    );

    /** Create a directory on the remote FTP host.

       @return
       true if the directory was created successfully.
     */
    PBoolean CreateDirectory(
      const PString & path                ///< Name of the directory to create.
    );

    /** Get status information for the file path specified.

       @return
       String giving file status.
     */
    PString GetFileStatus(
      const PString & path,                ///< Path to get status for.
      DataChannelType channel = Passive    ///< Data channel type.
    );

    /**Begin retreiving a file from the remote FTP server. The second
       parameter indicates that the transfer is on a normal or passive data
       channel. In short, a normal transfer the server connects to the
       client and in passive mode the client connects to the server.

       @return
       Socket to read data from, or NULL if an error occurred.
     */
    PTCPSocket * GetFile(
      const PString & filename,         ///< Name of file to get
      DataChannelType channel = Passive ///< Data channel type.
    );

    /**Begin storing a file to the remote FTP server. The second parameter
       indicates that the transfer is on a normal or passive data channel.
       In short, a normal transfer the server connects to the client and in
       passive mode the client connects to the server.

       @return
       Socket to write data to, or NULL if an error occurred.
     */
    PTCPSocket * PutFile(
      const PString & filename,         ///< Name of file to get
      DataChannelType channel = Passive ///< Data channel type.
    );

    /**Begin retreiving a file from the remote FTP server. The second
       parameter indicates that the transfer is on a normal or passive data
       channel. In short, a normal transfer the server connects to the
       client and in passive mode the client connects to the server.

       @return
       Socket to read data from, or NULL if an error occurred.
     */
    PTCPSocket * GetURL(
      const PURL & url,                 ///< URL of file to get
      RepresentationType type,          ///< Type of transfer (text/binary)
      DataChannelType channel = Passive ///< Data channel type.
    );

  //@}

  protected:
    /// Call back to verify open succeeded in an PInternetProtocol class
    virtual PBoolean OnOpen();

    PTCPSocket * NormalClientTransfer(
      Commands cmd,
      const PString & args
    );
    PTCPSocket * PassiveClientTransfer(
      Commands cmd,
      const PString & args
    );

    /// Port number on remote system
    WORD remotePort;
};


/**
File Transfer Protocol server channel class.
*/
class PFTPServer : public PFTP
{
  PCLASSINFO(PFTPServer, PFTP);
  public:
    enum { MaxIllegalPasswords = 3 };

    /// declare a server socket
    PFTPServer();
    PFTPServer(
      const PString & readyString   ///< Sign on string on connection ready.
    );

    /// Delete the server, cleaning up passive sockets.
    ~PFTPServer();


  // New functions for class
    /**
    Get the string printed when a user logs in default value is a string
    giving the user name
    */
    virtual PString GetHelloString(const PString & user) const;

    /// return the string printed just before exiting
    virtual PString GetGoodbyeString(const PString & user) const;

    /// return the string to be returned by the SYST command
    virtual PString GetSystemTypeString() const;

    /// return the thirdPartyPort flag, allowing 3 host put and get.
    PBoolean GetAllowThirdPartyPort() const { return thirdPartyPort; }

    /// Set the thirdPartyPort flag.
    void SetAllowThirdPartyPort(PBoolean state) { thirdPartyPort = state; }

    /** Process commands, dispatching to the appropriate virtual function. This
       is used when the socket is acting as a server.

       @return
       true if more processing may be done, false if the QUIT command was
       received or the <code>OnUnknown()</code> function returns false.
     */
    PBoolean ProcessCommand();

    /** Dispatching to the appropriate virtual function. This is used when the
       socket is acting as a server.

       @return
       true if more processing may be done, false if the QUIT command was
       received or the <code>OnUnknown()</code> function returns false.
     */
    virtual PBoolean DispatchCommand(
      PINDEX code,          ///< Parsed command code.
      const PString & args  ///< Arguments to command.
    );


    /** Check to see if the command requires the server to be logged in before
       it may be processed.

       @return
       true if the command required the user to be logged in.
     */
    virtual PBoolean CheckLoginRequired(
      PINDEX cmd    ///< Command to check if log in required.
    );

    /** Validate the user name and password for access. After three invalid
       attempts, the socket will close and false is returned.

       Default implementation returns true for all strings.

       @return
       true if user can access, otherwise false
     */
    virtual PBoolean AuthoriseUser(
      const PString & user,     ///< User name to authorise.
      const PString & password, ///< Password supplied for the user.
      PBoolean & replied            ///< Indication that a reply was sent to client.
    );

    /** Handle an unknown command.

       @return
       true if more processing may be done, false if the
       <code>ProcessCommand()</code> function is to return false.
     */
    virtual PBoolean OnUnknown(
      const PCaselessString & command  ///< Complete command line received.
    );

    /** Handle an error in command.

       @return
       true if more processing may be done, false if the
       <code>ProcessCommand()</code> function is to return false.
     */
    virtual void OnError(
      PINDEX errorCode, ///< Error code to use
      PINDEX cmdNum,    ///< Command that had the error.
      const char * msg  ///< Error message.
    );

    /// Called for syntax errors in commands.
    virtual void OnSyntaxError(
      PINDEX cmdNum   ///< Command that had the syntax error.
    );

    /// Called for unimplemented commands.
    virtual void OnNotImplemented(
      PINDEX cmdNum   ///< Command that was not implemented.
    );

    /// Called for successful commands.
    virtual void OnCommandSuccessful(
      PINDEX cmdNum   ///< Command that had was successful.
    );


    // the following commands must be implemented by all servers
    // and can be performed without logging in
    virtual PBoolean OnUSER(const PCaselessString & args);
    virtual PBoolean OnPASS(const PCaselessString & args);  // officially optional, but should be done
    virtual PBoolean OnQUIT(const PCaselessString & args);
    virtual PBoolean OnPORT(const PCaselessString & args);
    virtual PBoolean OnSTRU(const PCaselessString & args);
    virtual PBoolean OnMODE(const PCaselessString & args);
    virtual PBoolean OnTYPE(const PCaselessString & args);
    virtual PBoolean OnNOOP(const PCaselessString & args);
    virtual PBoolean OnSYST(const PCaselessString & args);
    virtual PBoolean OnSTAT(const PCaselessString & args);

    // the following commands must be implemented by all servers
    // and cannot be performed without logging in
    virtual PBoolean OnRETR(const PCaselessString & args);
    virtual PBoolean OnSTOR(const PCaselessString & args);
    virtual PBoolean OnACCT(const PCaselessString & args);
    virtual PBoolean OnAPPE(const PCaselessString & args);
    virtual PBoolean OnRNFR(const PCaselessString & args);
    virtual PBoolean OnRNTO(const PCaselessString & args);
    virtual PBoolean OnDELE(const PCaselessString & args);
    virtual PBoolean OnCWD(const PCaselessString & args);
    virtual PBoolean OnCDUP(const PCaselessString & args);
    virtual PBoolean OnRMD(const PCaselessString & args);
    virtual PBoolean OnMKD(const PCaselessString & args);
    virtual PBoolean OnPWD(const PCaselessString & args);
    virtual PBoolean OnLIST(const PCaselessString & args);
    virtual PBoolean OnNLST(const PCaselessString & args);
    virtual PBoolean OnPASV(const PCaselessString & args);

    // the following commands are optional and can be performed without
    // logging in
    virtual PBoolean OnHELP(const PCaselessString & args);
    virtual PBoolean OnSITE(const PCaselessString & args);
    virtual PBoolean OnABOR(const PCaselessString & args);

    // the following commands are optional and cannot be performed
    // without logging in
    virtual PBoolean OnSMNT(const PCaselessString & args);
    virtual PBoolean OnREIN(const PCaselessString & args);
    virtual PBoolean OnSTOU(const PCaselessString & args);
    virtual PBoolean OnALLO(const PCaselessString & args);
    virtual PBoolean OnREST(const PCaselessString & args);


    /// Send the specified file to the client.
    void SendToClient(
      const PFilePath & filename    ///< File name to send.
    );


  protected:
    /// Call back to verify open succeeded in an PInternetProtocol class
    PBoolean OnOpen();
    void Construct();

    PString readyString;
    PBoolean    thirdPartyPort;

    enum {
      NotConnected,
      NeedUser,
      NeedPassword,
      Connected,
      ClientConnect
    } state;

    PIPSocket::Address remoteHost;
    WORD remotePort;

    PTCPSocket * passiveSocket;

    char    type;
    char    structure;
    char    mode;
    PString userName;
    int     illegalPasswordCount;
};


PFACTORY_LOAD(PURL_FtpLoader);


#endif // PTLIB_FTP_H


// End of File ///////////////////////////////////////////////////////////////
