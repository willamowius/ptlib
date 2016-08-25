/*
 * xmpp_c2s.h
 *
 * Extensible Messaging and Presence Protocol (XMPP) Core
 * Client to Server communication classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 2004 Reitek S.p.A.
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_XMPP_C2S_H
#define PTLIB_XMPP_C2S_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptclib/xmpp.h>

#if P_EXPAT

#include <ptclib/psasl.h>
#include <ptlib/sockets.h>


///////////////////////////////////////////////////////

namespace XMPP
{
  namespace C2S
  {

    /** XMPP client to server TCP transport
     */
    class TCPTransport : public Transport
    {
      PCLASSINFO(TCPTransport, Transport);

    public:
      TCPTransport(const PString& hostname);
      TCPTransport(const PString& hostname, WORD port);
      ~TCPTransport();

      const PString&  GetServerHost() const   { return m_Hostname; }
      WORD            GetServerPort() const   { return m_Port; }

      virtual PBoolean Open();
      virtual PBoolean Close();

    protected:
      PString         m_Hostname;
      WORD            m_Port;
      PTCPSocket *    m_Socket;
    };
  

    /** This class handles the client side of a C2S (Client to Server)
        XMPP stream.
     */
    class StreamHandler : public BaseStreamHandler
    {
      PCLASSINFO(StreamHandler, BaseStreamHandler);

    public:
      StreamHandler(const JID& jid, const PString& pwd, PBoolean newAccount = false);
      ~StreamHandler();

      virtual PBoolean IsEstablished() const        { return m_State == Established; }

      virtual PBoolean Start(Transport * transport = 0);

      /** Request the delivery of the specified stanza
      NOTE: the StreamHandler takes ownership of the stanza
      and will take care of deleting it.
      BIG NOTE: use this method and not Write() if you want to
      get a notification when an answer to an iq arrives
      */
      PBoolean    Send(Stanza * stanza);

      void    SetVersion(WORD major, WORD minor);
      void    GetVersion(WORD& major, WORD& minor) const;

      const JID&  GetJID() const  { return m_JID; }

      /** These notifier lists after when a client session is
      established (i.e. after the handshake and authentication
      steps are completed) or is released. The parameter passed
      to the notifiers is a reference to the stream handler
      */
      PNotifierList&  SessionEstablishedHandlers()  { return m_SessionEstablishedHandlers; }
      PNotifierList&  SessionReleasedHandlers()     { return m_SessionReleasedHandlers; }

      /** These notifier lists are fired when a XMPP stanza or a
      stream error is received. For the notifier lists to be fired
      the stream must be already in the established state (i.e.
      after the bind and the session state). The parameter passed
      to the notifiers is a reference to the received pdu
      */
      PNotifierList&  ErrorHandlers()     { return m_ErrorHandlers; }
      PNotifierList&  MessageHandlers()   { return m_MessageHandlers; }
      PNotifierList&  PresenceHandlers()  { return m_PresenceHandlers; }
      PNotifierList&  IQHandlers()        { return m_IQHandlers; }

      /** A notifier list for a specific namespace. The list will
      be fired only upon receiving an IQ with the child element
      of the specified namespace
      */
      PNotifierList&  IQNamespaceHandlers(const PString& xml_namespace);

      /** A notifier list for a particular message originator. The list will
      be fired only upon receiving a message from the specified jid.
      NOTE: if a matching notifier list is found and it's not emnpty, the
      generic MessageHandlers list IS NOT fired.
      */
      PNotifierList&  MessageSenderHandlers(const JID& from);

      /** JEP-0030 Service Discovery access methods.
          The response handler will receive a PIQ stanza (a smart
          pointer to a XMPP::IQ)
       */
      virtual PBoolean DiscoverItems(
        const PString& jid,           ///< JID to which a query will be send
        PNotifier * responseHandler,  ///< Handler function for responses
        const PString& node = PString::Empty() ///< Optional node
      );
      virtual PBoolean DiscoverInfo(
        const PString& jid,           ///< JID to which a query will be send
        PNotifier * responseHandler,  ///< Handler function for responses
        const PString& node = PString::Empty() ///< Optional node
      );

    protected:
      virtual void    OnOpen(Stream& stream, INT);
      virtual void    OnClose(Stream& stream, INT);
      virtual void    StartRegistration();
      virtual void    StartAuthNegotiation();

      virtual void    OnSessionEstablished();
      virtual void    OnSessionReleased();
      virtual void    OnElement(PXML& pdu);
      virtual void    OnError(PXML& pdu);

      virtual void    OnMessage(XMPP::Message& pdu);
      virtual void    OnPresence(XMPP::Presence& pdu);
      virtual void    OnIQ(XMPP::IQ& pdu);

      // State handlers
      virtual void    HandleNullState(PXML& pdu);
      virtual void    HandleRegStartedState(PXML& pdu);
      virtual void    HandleTLSStartedState(PXML& pdu);
#if P_SASL
      virtual void    HandleSASLStartedState(PXML& pdu);
#endif
      virtual void    HandleNonSASLStartedState(PXML& pdu);
      virtual void    HandleStreamSentState(PXML& pdu);
      virtual void    HandleBindSentState(PXML& pdu);
      virtual void    HandleSessionSentState(PXML& pdu);
      virtual void    HandleEstablishedState(PXML& pdu);

      virtual PBoolean    Discover(const PString& xmlns,
                               const PString& jid,
                               PNotifier * responseHandler,
                               const PString& node);

      WORD                m_VersionMajor;
      WORD                m_VersionMinor;
      PString             m_StreamID;
      PBoolean                m_NewAccount;
      JID                 m_JID;
      const PString       m_Password;
#if P_SASL
      PSASLClient         m_SASL;
      PString             m_Mechanism;
#endif
      PBoolean                m_HasBind;
      PBoolean                m_HasSession;

      PNotifierList       m_SessionEstablishedHandlers;
      PNotifierList       m_SessionReleasedHandlers;
      PNotifierList       m_ErrorHandlers;
      PNotifierList       m_MessageHandlers;
      PNotifierList       m_PresenceHandlers;
      PNotifierList       m_IQHandlers;
      PDictionary<PString, PNotifierList> m_IQNamespaceHandlers;
      PDictionary<JID, PNotifierList> m_MessageSenderHandlers;

      PMutex              m_PendingIQsLock;
      StanzaList          m_PendingIQs;

      enum StreamState
      {
        Null,
        RegStarted,
        TLSStarted,
        SASLStarted,
        NonSASLStarted, ///< non SASL authentication (JEP-0078)
        StreamSent,
        BindSent,
        SessionSent,
        Established
      };

      virtual void SetState(StreamState s);

      StreamState m_State;
    };

  }  // namespace C2S
} // namespace XMPP


#endif  // P_EXPAT

#endif  // PTLIB_XMPP_C2S_H

// End of File ///////////////////////////////////////////////////////////////
