/*
 * xmpp.h
 *
 * Extensible Messaging and Presence Protocol (XMPP) Core
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

#ifndef PTLIB_XMPP_H
#define PTLIB_XMPP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib.h>

#if P_EXPAT

#include <ptclib/pxml.h>
#include <ptlib/notifier_ext.h>


///////////////////////////////////////////////////////

namespace XMPP
{
  /** Various constant strings
   */
  extern const PCaselessString & LanguageTag();
  extern const PCaselessString & NamespaceTag();
  extern const PCaselessString & MessageStanzaTag();
  extern const PCaselessString & PresenceStanzaTag();
  extern const PCaselessString & IQStanzaTag();
  extern const PCaselessString & IQQueryTag();

  class JID : public PObject
  {
    PCLASSINFO(JID, PObject);

  public:
    JID(const char * jid = 0);
    JID(const PString& jid);
    JID(const PString& user, const PString& server, const PString& resource = PString::Empty());

    virtual Comparison Compare(
      const PObject & obj   ///< Object to compare against.
      ) const;

    JID& operator=(
      const PString & jid  ///< New JID to assign.
      );

    operator const PString&() const;

    virtual PObject * Clone() const { return new JID(m_JID); }

    PString   GetUser() const         { return m_User; }
    PString   GetServer() const       { return m_Server; }

    virtual PString GetResource() const { return m_Resource; }

    virtual void SetUser(const PString& user);
    virtual void SetServer(const PString& server);
    virtual void SetResource(const PString& resource);

    virtual PBoolean IsBare() const       { return m_Resource.IsEmpty(); }
    virtual void PrintOn(ostream & strm) const;

  protected:
    virtual void ParseJID(const PString& jid);
    virtual void BuildJID() const;

    PString   m_User;
    PString   m_Server;
    PString   m_Resource;

    mutable PString m_JID;
    mutable PBoolean    m_IsDirty;
  };

  // A bare jid is a jid with no resource
  class BareJID : public JID
  {
    PCLASSINFO(BareJID, JID);

  public:
    BareJID(const char * jid = 0) : JID(jid) { }
    BareJID(const PString& jid) : JID(jid) { }
    BareJID(const PString& user, const PString& server, const PString& resource = PString::Empty())
      : JID(user, server, resource) { }

    virtual Comparison Compare(
      const PObject & obj   ///< Object to compare against.
      ) const;

    BareJID& operator=(
      const PString & jid  ///< New JID to assign.
      );

    virtual PObject * Clone() const { return new BareJID(m_JID); }
    virtual PString GetResource() const { return PString::Empty(); }
    virtual void SetResource(const PString&) { }
    virtual PBoolean IsBare() const { return true; }
  };

  /** This interface is the base class of each XMPP transport class

  Derived classes might include an XMPP TCP transport as well as
  classes to handle XMPP incapsulated in SIP messages.
  */
  class Transport : public PIndirectChannel
  {
    PCLASSINFO(Transport, PIndirectChannel);

  public:
    virtual PBoolean Open() = 0;
    virtual PBoolean Close() = 0;
  };


/** This class represents a XMPP stream, i.e. a XML message exchange
    between XMPP entities
 */
  class Stream : public PIndirectChannel
  {
    PCLASSINFO(Stream, PIndirectChannel);

  public:
    Stream(Transport * transport = 0);
    ~Stream();

    virtual PBoolean        OnOpen()            { return m_OpenHandlers.Fire(*this); }
    PNotifierList&      OpenHandlers()      { return m_OpenHandlers; }

    virtual PBoolean        Close();
    virtual void        OnClose()           { m_CloseHandlers.Fire(*this); }
    PNotifierList&      CloseHandlers()     { return m_CloseHandlers; }

    virtual PBoolean        Write(const void * buf, PINDEX len);
    virtual PBoolean        Write(const PString& data);
    virtual PBoolean        Write(const PXML& pdu);

    /** Read a XMPP stanza from the stream
    */
    virtual PXML *      Read();

    /** Reset the parser. The will delete and re-instantiate the
    XML stream parser.
    */
    virtual void        Reset();
    PXMLStreamParser *  GetParser()     { return m_Parser; }

  protected:
    PXMLStreamParser *  m_Parser;
    PNotifierList       m_OpenHandlers;
    PNotifierList       m_CloseHandlers;
  };


  class BaseStreamHandler : public PThread
  {
    PCLASSINFO(BaseStreamHandler, PThread);

  public:
    BaseStreamHandler();
    ~BaseStreamHandler();

    virtual PBoolean        Start(Transport * transport = 0);
    virtual PBoolean        Stop(const PString& error = PString::Empty());

    void                SetAutoReconnect(PBoolean b = true, long timeout = 1000);

    PNotifierList&      ElementHandlers()   { return m_ElementHandlers; }
    Stream *            GetStream()         { return m_Stream; }

    virtual PBoolean        Write(const void * buf, PINDEX len);
    virtual PBoolean        Write(const PString& data);
    virtual PBoolean        Write(const PXML& pdu);
    virtual void        OnElement(PXML& pdu);

    virtual void        Main();

  protected:
    PDECLARE_NOTIFIER(Stream, BaseStreamHandler, OnOpen);
    PDECLARE_NOTIFIER(Stream, BaseStreamHandler, OnClose);

    Stream *        m_Stream;
    PBoolean            m_AutoReconnect;
    PTimeInterval   m_ReconnectTimeout;

    PNotifierList   m_ElementHandlers;
  };


  /** XMPP stanzas: the following classes represent the three
    stanzas (PDUs) defined by the xmpp protocol
   */

  class Stanza : public PXML
  {
    PCLASSINFO(Stanza, PXML)

  public:
    /** Various constant strings
    */
    static const PCaselessString & IDTag();
    static const PCaselessString & FromTag();
    static const PCaselessString & ToTag();

    virtual PBoolean IsValid() const = 0;

    virtual PString GetID() const;
    virtual PString GetFrom() const;
    virtual PString GetTo() const;

    virtual void SetID(const PString& id);
    virtual void SetFrom(const PString& from);
    virtual void SetTo(const PString& to);

    virtual PXMLElement * GetElement(const PString& name, PINDEX i = 0);
    virtual void AddElement(PXMLElement * elem);

    static PString GenerateID();
  };

  PLIST(StanzaList, Stanza);


  class Message : public Stanza
  {
    PCLASSINFO(Message, Stanza)

  public:
    enum MessageType {
      Normal,
      Chat,
      Error,
      GroupChat,
      HeadLine,
      Unknown = 999
    };

    /** Various constant strings
    */
    static const PCaselessString & TypeTag();
    static const PCaselessString & SubjectTag();
    static const PCaselessString & BodyTag();
    static const PCaselessString & ThreadTag();

    /** Construct a new empty message
    */
    Message();

    /** Construct a message from a (received) xml PDU.
    The root of the pdu MUST be a message stanza.
    NOTE: the root of the pdu is cloned.
    */
    Message(PXML& pdu);
    Message(PXML * pdu);

    virtual PBoolean IsValid() const;
    static PBoolean IsValid(const PXML * pdu);

    virtual MessageType GetType(PString * typeName = 0) const;
    virtual PString     GetLanguage() const;

    /** Get the subject for the specified language. The default subject (if any)
    is returned in case no language is specified or a matching one cannot be
    found
    */
    virtual PString GetSubject(const PString& lang = PString::Empty());
    virtual PString GetBody(const PString& lang = PString::Empty());
    virtual PString GetThread();

    virtual PXMLElement * GetSubjectElement(const PString& lang = PString::Empty());
    virtual PXMLElement * GetBodyElement(const PString& lang = PString::Empty());

    virtual void SetType(MessageType type);
    virtual void SetType(const PString& type); // custom, possibly non standard, type
    virtual void SetLanguage(const PString& lang);

    virtual void SetSubject(const PString& subj, const PString& lang = PString::Empty());
    virtual void SetBody(const PString& body, const PString& lang = PString::Empty());
    virtual void SetThread(const PString& thrd);
  };


  class Presence : public Stanza
  {
    PCLASSINFO(Presence, Stanza)

  public:
    enum PresenceType {
      Available,
      Unavailable,
      Subscribe,
      Subscribed,
      Unsubscribe,
      Unsubscribed,
      Probe,
      Error,
      Unknown = 999
    };

    enum ShowType {
      Online,
      Away,
      Chat,
      DND,
      XA,
      Other = 999
    };

    /** Various constant strings
    */
    static const PCaselessString & TypeTag();
    static const PCaselessString & ShowTag();
    static const PCaselessString & StatusTag();
    static const PCaselessString & PriorityTag();

    /** Construct a new empty presence
    */
    Presence();

    /** Construct a presence from a (received) xml PDU.
    The root of the pdu MUST be a presence stanza.
    NOTE: the root of the pdu is cloned.
    */
    Presence(PXML& pdu);
    Presence(PXML * pdu);

    virtual PBoolean IsValid() const;
    static PBoolean IsValid(const PXML * pdu);

    virtual PresenceType GetType(PString * typeName = 0) const;
    virtual ShowType     GetShow(PString * showName = 0) const;
    virtual BYTE         GetPriority() const;

    /** Get the status for the specified language. The default status (if any)
    is returned in case no language is specified or a matching one cannot be
    found
    */
    virtual PString GetStatus(const PString& lang = PString::Empty());
    virtual PXMLElement * GetStatusElement(const PString& lang = PString::Empty());

    virtual void SetType(PresenceType type);
    virtual void SetType(const PString& type); // custom, possibly non standard, type
    virtual void SetShow(ShowType show);
    virtual void SetShow(const PString& show); // custom, possibly non standard, type
    virtual void SetPriority(BYTE priority);

    virtual void SetStatus(const PString& status, const PString& lang = PString::Empty());
  };


  class IQ : public Stanza
  {
    PCLASSINFO(IQ, Stanza)

  public:
    enum IQType {
      Get,
      Set,
      Result,
      Error,
      Unknown = 999
    };

    /** Various constant strings
    */
    static const PCaselessString & TypeTag();

    IQ(IQType type, PXMLElement * body = 0);
    IQ(PXML& pdu);
    IQ(PXML * pdu);
    ~IQ();

    virtual PBoolean IsValid() const;
    static PBoolean IsValid(const PXML * pdu);

    /** This method signals that the message was taken care of
    If the stream handler, after firing all the notifiers finds
    that an iq set/get pdu has not being processed, it returns
    an error to the sender
    */
    void SetProcessed()             { m_Processed = true; }
    PBoolean HasBeenProcessed() const   { return m_Processed; }

    virtual IQType        GetType(PString * typeName = 0) const;
    virtual PXMLElement * GetBody();

    virtual void SetType(IQType type);
    virtual void SetType(const PString& type); // custom, possibly non standard, type
    virtual void SetBody(PXMLElement * body);

    // If the this message is response, returns a pointer to the
    // original set/get message
    virtual IQ *  GetOriginalMessage() const      { return m_OriginalIQ; }
    virtual void  SetOriginalMessage(IQ * iq);

    /** Creates a new response iq for this message (that must
    be of the set/get type!)
    */
    virtual IQ *  BuildResult() const;

    /** Creates an error response for this message
    */
    virtual IQ *  BuildError(const PString& type, const PString& code) const;

    virtual PNotifierList GetResponseHandlers()   { return m_ResponseHandlers; }

  protected:
    PBoolean            m_Processed;
    IQ *            m_OriginalIQ;
    PNotifierList   m_ResponseHandlers;
  };
  /** JEP-0030 Service Discovery classes
   */
  namespace Disco
  {
    class Item : public PObject
    {
      PCLASSINFO(Item, PObject);
    public:
      Item(PXMLElement * item);
      Item(const PString& jid, const PString& node = PString::Empty());

      const JID&      GetJID() const    { return m_JID; }
      const PString&  GetNode() const   { return m_Node; }

      PXMLElement *   AsXML(PXMLElement * parent) const;

    protected:
      const JID     m_JID;
      const PString m_Node;
    };

    PDECLARE_LIST(ItemList, Item)
    public:
      ItemList(PXMLElement * list);
      PXMLElement * AsXML(PXMLElement * parent) const;
    };

    class Identity : public PObject
    {
      PCLASSINFO(Identity, PObject);
    public:
      Identity(PXMLElement * identity);
      Identity(const PString& category, const PString& type, const PString& name);

      const PString&  GetCategory() const   { return m_Category; }
      const PString&  GetType() const       { return m_Type; }
      const PString&  GetName() const       { return m_Name; }

      PXMLElement *   AsXML(PXMLElement * parent) const;

    protected:
      const PString m_Category;
      const PString m_Type;
      const PString m_Name;
    };

    PDECLARE_LIST(IdentityList, Identity)
    public:
      IdentityList(PXMLElement * list);
      PXMLElement * AsXML(PXMLElement * parent) const;
    };

    class Info : public PObject
    {
      PCLASSINFO(Info, PObject);
    public:
      Info(PXMLElement * info);
      
      IdentityList&   GetIdentities() { return m_Identities; }
      PStringSet&     GetFeatures()   { return m_Features; }

      PXMLElement *   AsXML(PXMLElement * parent) const;

    protected:
      IdentityList  m_Identities;
      PStringSet    m_Features;
    };
  } // namespace Disco

}; // class XMPP


#endif  // P_EXPAT

#endif  // PTLIB_XMPP_H

// End of File ///////////////////////////////////////////////////////////////
