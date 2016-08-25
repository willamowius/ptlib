/*
 * mail.h
 *
 * Electronic Mail abstraction class.
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

#ifndef PTLIB_MAIL_H
#define PTLIB_MAIL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#if defined(_WIN32) && !defined(_WIN64)

#  ifndef P_HAS_MAPI
#  define P_HAS_MAPI 1
#  endif

#  ifndef P_HAS_CMC
#  define P_HAS_CMC 1
#  endif

#  if P_HAS_MAPI
#  include <mapi.h>
#  endif

#  if P_HAS_CMC
#  include <xcmc.h>
#  endif

#endif  // _WIN32


/**This class establishes a mail session with the platforms mail system.
*/
class PMail : public PObject
{
  PCLASSINFO(PMail, PObject);

  public:
  /**@name Construction */
  //@{
    /**Create a mail session. It is initially not logged in.
     */
    PMail();

    /**Create a mail session.
       Attempt to log in using the parameters provided.
     */
    PMail(
      const PString & username,  ///< User withing mail system to use.
      const PString & password   ///< Password for user in mail system.
    );

    /**Create a mail session.
       Attempt to log in using the parameters provided.
     */
    PMail(
      const PString & username,  ///< User withing mail system to use.
      const PString & password,  ///< Password for user in mail system.
      const PString & service
      /**A platform dependent string indicating the location of the underlying
         messaging service, eg the path to a message store or node name of the
         mail server.
       */
    );


    virtual ~PMail();
    /* Destroy the mail session, logging off the mail system if necessary.
     */
  //@}

  /**@name Log in/out functions */
  //@{
    /**Attempt to log on to the mail system using the parameters provided.

       @return
       true if successfully logged on.
     */
    PBoolean LogOn(
      const PString & username,  ///< User withing mail system to use.
      const PString & password   ///< Password for user in mail system.
    );

    /**Attempt to log on to the mail system using the parameters provided.

       @return
       true if successfully logged on.
     */
    PBoolean LogOn(
      const PString & username,  ///< User withing mail system to use.
      const PString & password,  ///< Password for user in mail system.
      const PString & service
      /**A platform dependent string indicating the location of the underlying
         messaging service, eg the path to a message store or node name of the
         mail server.
       */
    );

    /**Log off from the mail system.

       @return
       true if successfully logged off.
     */
    virtual PBoolean LogOff();

    /**Determine if the mail session is active and logged into the mail system.

       @return
       true if logged into the mail system.
     */
    PBoolean IsLoggedOn() const;
  //@}

  /**@name Send message functions */
  //@{
    /**Send a new simple mail message.

       @return
       true if the mail message was successfully queued. Note that this does
       {\b not} mean that it has been delivered.
     */
    PBoolean SendNote(
      const PString & recipient,  ///< Name of recipient of the mail message.
      const PString & subject,    ///< Subject name for the mail message.
      const char * body           ///< Text body of the mail message.
    );

    /**Send a new simple mail message.

       @return
       true if the mail message was successfully queued. Note that this does
       {\b not} mean that it has been delivered.
     */
    PBoolean SendNote(
      const PString & recipient,  ///< Name of recipient of the mail message.
      const PString & subject,    ///< Subject name for the mail message.
      const char * body,          ///< Text body of the mail message.
      const PStringList & attachments
                        ///< List of files to attach to the mail message.
    );

    /**Send a new simple mail message.

       @return
       true if the mail message was successfully queued. Note that this does
       {\b not} mean that it has been delivered.
     */
    PBoolean SendNote(
      const PString & recipient,  ///< Name of recipient of the mail message.
      const PStringList & carbonCopies, ///< Name of CC recipients.
      const PStringList & blindCarbons, ///< Name of BCC recipients.
      const PString & subject,        ///< Subject name for the mail message.
      const char * body,              ///< Text body of the mail message.
      const PStringList & attachments
                        ///< List of files to attach to the mail message.
    );
  //@}

  /**@name Read message functions */
  //@{
    /**Get a list of ID strings for all messages in the mail box.

       @return
       An array of ID strings.
     */
    PStringArray GetMessageIDs(
      PBoolean unreadOnly = true    ///< Only get the IDs for unread messages.
    );

    /// Message header for each mail item.
    struct Header {
      /// Subject for message.
      PString  subject;           
      /// Full name of message originator.
      PString  originatorName;    
      /// Return address of message originator.
      PString  originatorAddress; 
      /// Time message received.
      PTime    received;          
    };

    /**Get the header information for a message.

       @return
       true if header information was successfully obtained.
     */
    PBoolean GetMessageHeader(
      const PString & id,      ///< Identifier of message to get header.
      Header & hdrInfo         ///< Header info for the message.
    );

    /**Get the body text for a message into the <code>body</code> string
       parameter.

       Note that if the body text for the mail message is very large, the
       function will return false. To tell between an error getting the message
       body and having a large message body the <code>GetErrorCode()</code> function
       must be used.

       To get a large message body, the <code>GetMessageAttachments()</code> should
       be used with the <code>includeBody</code> parameter set to true so that
       the message body is placed into a disk file.

       @return
       true if the body text was retrieved, false if the body was too large or
       some other error occurred.
     */
    PBoolean GetMessageBody(
      const PString & id,      ///< Identifier of message to get body.
      PString & body,          ///< Body text of mail message.
      PBoolean markAsRead = false  ///< Mark the message as read.
    );

    /**Get all of the attachments for a message as disk files.

       @return
       true if attachments were successfully obtained.
     */
    PBoolean GetMessageAttachments(
      const PString & id,       ///< Identifier of message to get attachments.
      PStringArray & filenames, ///< File names for each attachment.
      PBoolean includeBody = false, ///< Include the message body as first attachment
      PBoolean markAsRead = false   ///< Mark the message as read
    );

    /**Mark the message as read.

       @return
       true if message was successfully marked as read.
     */
    PBoolean MarkMessageRead(
      const PString & id      ///< Identifier of message to get header.
    );

    /**Delete the message from the system.

       @return
       true if message was successfully deleted.
     */
    PBoolean DeleteMessage(
      const PString & id      ///< Identifier of message to get header.
    );
  //@}

  /**@name User look up functions */
  //@{
    /// Result of a lookup operation with the <code>LookUp()</code> function.
    enum LookUpResult {
      /// User name is unknown in mail system.
      UnknownUser,    
      /// User is ambiguous in mail system.
      AmbiguousUser,  
      /// User is a valid, unique name in mail system.
      ValidUser,      
      /// An error occurred during the look up
      LookUpError     
    };

    /**Look up the specified name and verify that they are a valid address in
       the mail system.

       @return
       result of the name lookup.
     */
    LookUpResult LookUp(
      const PString & name,  ///< Name to look up.
      PString * fullName = NULL
      /**<String to receive full name of user passed in <code>name</code>. If
         NULL then the full name is <b>not</b> returned.
       */
    );
  //@}

  /**@name Error functions */
  //@{
    /**Get the internal error code for the last error by a function in this
       mail session.

       @return
       integer error code for last operation.
     */
    int GetErrorCode() const;

    /**Get the internal error description for the last error by a function in
       this mail session.

       @return
       string error text for last operation.
     */
    PString GetErrorText() const;
  //@}


  protected:
    void Construct();
    // Common construction code.

    /// Flag indicating the session is active.
    PBoolean loggedOn;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/mail.h"
#else
#include "unix/ptlib/mail.h"
#endif
};


#endif // PTLIB_MAIL_H


// End Of File ///////////////////////////////////////////////////////////////
