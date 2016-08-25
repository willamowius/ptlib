/*
 * psoap.cxx
 *
 * SOAP client / server classes.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2003 Andreas Sikkema
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
 * The Initial Developer of the Original Code is Andreas Sikkema
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "psoap.h"
#endif


#include <ptlib.h>


#if P_SOAP

#include <ptclib/psoap.h>


#define new PNEW



/*
 SOAP message classes
 ####################
 */


PSOAPMessage::PSOAPMessage( int options ) : 
  PXML( options ),
  pSOAPBody( 0 ),
  pSOAPMethod( 0 ),
  faultCode( PSOAPMessage::NoFault )
{
}

PSOAPMessage::PSOAPMessage( const PString & method, const PString & nameSpace ) :
  PXML( PXMLParser::Indent + PXMLParser::NewLineAfterElement ),
  pSOAPBody( 0 ),
  pSOAPMethod( 0 ),
  faultCode( PSOAPMessage::NoFault )
{
    SetMethod( method, nameSpace );
}



void PSOAPMessage::SetMethod( const PString & name, const PString & nameSpace, const PString & methodPrefix )
{
  PXMLElement* rtElement = 0;
  
  if ( pSOAPBody == 0 )
  {
    SetRootElement("SOAP-ENV:Envelope");
    
    rtElement = GetRootElement();

    rtElement->SetAttribute("xmlns:SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", PTrue );
    rtElement->SetAttribute("xmlns:xsi", "http://www.w3.org/1999/XMLSchema-instance", PTrue );
    rtElement->SetAttribute("xmlns:xsd", "http://www.w3.org/1999/XMLSchema", PTrue );
    rtElement->SetAttribute("xmlns:SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", PTrue );

    pSOAPBody = new PXMLElement( rtElement, "SOAP-ENV:Body");

    rtElement->AddChild( pSOAPBody, PTrue );
  }

  if ( pSOAPMethod == 0 )
  {
    rtElement = GetRootElement();

    pSOAPMethod = new PXMLElement(rtElement, methodPrefix + name);
    if (!nameSpace.IsEmpty())
    {
      if (methodPrefix.IsEmpty())
        pSOAPMethod->SetAttribute("xmlns", nameSpace, true);
      else
        pSOAPMethod->SetAttribute("xmlns:m", nameSpace, true);
    }
    pSOAPBody->AddChild( pSOAPMethod, PTrue );
  }

}

void PSOAPMessage::GetMethod( PString & name, PString & nameSpace )
{
  PString fullMethod = pSOAPMethod->GetName();
  PINDEX sepLocation = fullMethod.Find(':');
  if (sepLocation != P_MAX_INDEX) {
    PString methodID = fullMethod.Left(sepLocation);
    name = fullMethod.Right(fullMethod.GetSize() - 2 - sepLocation);
    nameSpace = pSOAPMethod->GetAttribute( "xmlns:" + methodID );
  }
}


void PSOAPMessage::AddParameter( PString name, PString type, PString value )
{
  if ( pSOAPMethod )
  {
    PXMLElement* rtElement = GetRootElement();
    
    PXMLElement* pParameter = new PXMLElement( rtElement, name);
    PXMLData* pParameterData = new PXMLData( pParameter, value);
    
    if ( type != "" )
    {
      pParameter->SetAttribute( "xsi:type", PString( "xsd:" ) + type );
    }
    
    pParameter->AddChild( pParameterData, PTrue );

    AddParameter( pParameter, PTrue );
  }
}

void PSOAPMessage::AddParameter( PXMLElement* parameter, PBoolean dirty )
{
  if ( pSOAPMethod )
  {
    pSOAPMethod->AddChild( parameter, dirty );
  }
}


PString faultCodeToString( PINDEX faultCode )
{
  PString faultCodeStr;
  switch ( faultCode )
  {
  case PSOAPMessage::VersionMismatch:
    faultCodeStr = "VersionMisMatch";
    break;
  case PSOAPMessage::MustUnderstand:
    faultCodeStr = "MustUnderstand";
    break;
  case PSOAPMessage::Client:
    faultCodeStr = "Client";
    break;
  case PSOAPMessage::Server:
    faultCodeStr = "Server";
    break;
  default:
    // Default it's the server's fault. Can't blame it on the customer, because he/she is king ;-)
    faultCodeStr = "Server";
    break;
  }

  return faultCodeStr;
}

PINDEX stringToFaultCode( PString & faultStr )
{
  if ( faultStr == "VersionMisMatch" )
    return PSOAPMessage::VersionMismatch;

  if ( faultStr == "MustUnderstand" )
    return PSOAPMessage::MustUnderstand;

  if ( faultStr == "Client" )
    return PSOAPMessage::Client;
  
  if ( faultStr == "Server" )
    return PSOAPMessage::Server;

  return PSOAPMessage::Server;
}

PBoolean PSOAPMessage::GetParameter( const PString & name, PString & value )
{
  PXMLElement* pElement = GetParameter( name );
  if(pElement == NULL)
    return PFalse;


  if ( pElement->GetAttribute( "xsi:type") == "xsd:string" )
  {
    value = pElement->GetData();
    return PTrue;
  }

  value.MakeEmpty();
  return PFalse;
}

PBoolean PSOAPMessage::GetParameter( const PString & name, int & value )
{
  PXMLElement* pElement = GetParameter( name );
  if(pElement == NULL)
    return PFalse;

  if ( pElement->GetAttribute( "xsi:type") == "xsd:int" )
  {
    value = pElement->GetData().AsInteger();
    return PTrue;
  }

  value = -1;
  return PFalse;
}

PXMLElement* PSOAPMessage::GetParameter( const PString & name )
{
  if ( pSOAPMethod )
  {
    return pSOAPMethod->GetElement( name, 0 );
  }
  else
  {
    return 0;
  }
}

PBoolean PSOAPMessage::Load( const PString & str )
{
  if ( !PXML::Load( str ) )
    return PFalse;
 
  if ( rootElement != NULL )
  {
    PString soapEnvelopeName = rootElement->GetName();
    PString soapEnvelopeID = soapEnvelopeName.Left( soapEnvelopeName.Find(':') );
    
    pSOAPBody = rootElement->GetElement( soapEnvelopeID + ":Body", 0 );
    
    if ( pSOAPBody != NULL )
    {
      PXMLObjectArray  subObjects = pSOAPBody->GetSubObjects() ;

      PINDEX idx;
      PINDEX size = subObjects.GetSize();
      
      for ( idx = 0; idx < size; idx++ ) {
        if ( subObjects[ idx ].IsElement() ) {
          // First subobject being an element is the method
          pSOAPMethod = ( PXMLElement * ) &subObjects[ idx  ];

          PString method;
          PString nameSpace;

          GetMethod( method, nameSpace );

          // Check if method name is "Fault"
          if ( method == "Fault" )
          {
            // The SOAP server has signalled an error
            PString faultCodeData = GetParameter( "faultcode" )->GetData();
            faultCode = stringToFaultCode( faultCodeData );
            faultText = GetParameter( "faultstring" )->GetData();
          }
          else
          {
            return PTrue;
          }
        }
      }
    }
  }
  return PFalse;
}

void PSOAPMessage::SetFault( PINDEX code, const PString & text) 
{ 
  faultCode = code; 
  faultText = text; 

  PString faultCodeStr = faultCodeToString( code );

  SetMethod( "Fault", "" );

  AddParameter( "faultcode", "", faultCodeStr );
  AddParameter( "faultstring", "", text );

}



/*
 SOAP server classes
 ####################
 */



PSOAPServerResource::PSOAPServerResource()
  : PHTTPResource( DEFAULT_SOAP_URL ),
  soapAction( " " )
{
}

PSOAPServerResource::PSOAPServerResource(
      const PHTTPAuthority & auth )    // Authorisation for the resource.
  : PHTTPResource( DEFAULT_SOAP_URL, auth ),
  soapAction( " " )
{
}
PSOAPServerResource::PSOAPServerResource(
      const PURL & url )               // Name of the resource in URL space.
  : PHTTPResource(url )
{
}

PSOAPServerResource::PSOAPServerResource(
      const PURL & url,              // Name of the resource in URL space.
      const PHTTPAuthority & auth    // Authorisation for the resource.
    )
  : PHTTPResource( url, auth )
{
}

PBoolean PSOAPServerResource::SetMethod(const PString & methodName, const PNotifier & func)
{
  // Set the method for the notifier function and add it to the list
  PWaitAndSignal m( methodMutex );

  // Find the method, or create a new one
  PSOAPServerMethod * methodInfo;
  
  PINDEX pos = methodList.GetValuesIndex( methodName );
  if (pos != P_MAX_INDEX)
  {
    methodInfo = ( PSOAPServerMethod *) methodList.GetAt( pos );
  }
  else 
  {
    methodInfo = new PSOAPServerMethod( methodName );
    methodList.Append( methodInfo );
  }

  // set the function
  methodInfo->methodFunc = func;

  return PTrue;
}

PBoolean PSOAPServerResource::LoadHeaders( PHTTPRequest& /* request */ )    // Information on this request.
{
  return PTrue;
}

PBoolean PSOAPServerResource::OnPOSTData( PHTTPRequest & request,
                                const PStringToString & /*data*/)
{
  PTRACE(4, "PSOAPServerResource\tReceived post data, request: " << request.entityBody );

  PString reply;

  PBoolean ok = PFalse;

  // Check for the SOAPAction header
  PString* pSOAPAction = request.inMIME.GetAt( "SOAPAction" );
  if ( pSOAPAction )
  {
    // If it's available check if we are expecting a special header value
    if ( soapAction.IsEmpty() || soapAction == " " )
    {
      // A space means anything goes
      ok = OnSOAPRequest( request.entityBody, reply );
    }
    else
    {
      // Check if the incoming header is the same as we expected
      if ( *pSOAPAction == soapAction )
      {
        ok = OnSOAPRequest( request.entityBody, reply );
      }
      else
      {
        ok = PFalse;
        reply = FormatFault( PSOAPMessage::Client, "Incorrect SOAPAction in HTTP Header: " + *pSOAPAction ).AsString();
      }
    }
  }
  else
  {
    ok = PFalse;
    reply = FormatFault( PSOAPMessage::Client, "SOAPAction is missing in HTTP Header" ).AsString();
  }

  // If everything went OK, reply with ReturnCode 200 (OK)
  if ( ok )
    request.code = PHTTP::RequestOK;
  else
    // Reply with InternalServerError (500)
    request.code = PHTTP::InternalServerError;

  // Set the correct content-type
  request.outMIME.SetAt(PHTTP::ContentTypeTag(), "text/xml");

  // Start constructing the response
  PINDEX len = reply.GetLength();
  request.server.StartResponse( request.code, request.outMIME, len );
  
  // Write the reply to the client
  return request.server.Write( (const char* ) reply, len );
}


PBoolean PSOAPServerResource::OnSOAPRequest( const PString & body, PString & reply )
{
  // Load the HTTP body into the SOAP (XML) parser
  PSOAPMessage request;
  PBoolean ok = request.Load( body );

  // If parsing the XML to SOAP failed reply with an error
  if ( !ok ) 
  { 
    reply = FormatFault( PSOAPMessage::Client, "XML error:" + request.GetErrorString() ).AsString();
    return PFalse;
  }


  PString method;
  PString nameSpace;

  // Retrieve the method from the SOAP messsage
  request.GetMethod( method, nameSpace );

  PTRACE(4, "PSOAPServerResource\tReceived SOAP message for method " << method);

  return OnSOAPRequest( method, request, reply );
}

PBoolean PSOAPServerResource::OnSOAPRequest( const PString & methodName, 
                                            PSOAPMessage & request,
                                            PString & reply )
{
  methodMutex.Wait();

  // Find the method information
  PINDEX pos = methodList.GetValuesIndex( methodName );

  if ( pos == P_MAX_INDEX ) 
  {
    reply = FormatFault( PSOAPMessage::Client, "Unknown method = " + methodName ).AsString();
    return PFalse;
  }
  
  PSOAPServerMethod * methodInfo = ( PSOAPServerMethod * )methodList.GetAt( pos );
  PNotifier notifier = methodInfo->methodFunc;
  
  methodMutex.Signal();

  // create a request/response container to be passed to the notifier function
  PSOAPServerRequestResponse p( request );

  // call the notifier
  notifier( p, 0 );

  // get the reply

  reply = p.response.AsString();

  return p.response.GetFaultCode() == PSOAPMessage::NoFault;
}


PSOAPMessage PSOAPServerResource::FormatFault( PINDEX code, const PString & str )
{
  PTRACE(2, "PSOAPServerResource\trequest failed: " << str);

  PSOAPMessage reply;

  PString faultCodeStr = faultCodeToString( code );

  reply.SetMethod( "Fault", "" );

  reply.AddParameter( "faultcode", "", faultCodeStr );
  reply.AddParameter( "faultstring", "", str );

  return reply;
}


/*
 SOAP client classes
 ####################
 */


PSOAPClient::PSOAPClient( const PURL & _url )
  : url(_url),
  soapAction( " " )
{
  timeout = 10000;
}

PBoolean PSOAPClient::MakeRequest( const PString & method, const PString & nameSpace )
{
  PSOAPMessage request( method, nameSpace );
  PSOAPMessage response;

  return MakeRequest( request, response );
}

PBoolean PSOAPClient::MakeRequest( const PString & method, const PString & nameSpace, PSOAPMessage & response )
{
  PSOAPMessage request( method, nameSpace );

  return MakeRequest( request, response );
}

PBoolean PSOAPClient::MakeRequest( PSOAPMessage & request, PSOAPMessage & response )
{
  return  PerformRequest( request, response );
}

PBoolean PSOAPClient::PerformRequest( PSOAPMessage & request, PSOAPMessage & response )
{
  // create SOAP request
  PString soapRequest;

  PStringStream txt;
  
  if ( !request.Save( soapRequest ) ) 
  {
    
    txt << "Error creating request XML ("
        << request.GetErrorLine() 
        << ") :" 
        << request.GetErrorString();
    return PFalse;
  }

  // End with a newline
  soapRequest += "\n";

  PTRACE( 5, "SOAPClient\tOutgoing SOAP is " << soapRequest );

  // do the request
  PHTTPClient client;
  PMIMEInfo sendMIME, replyMIME;
  sendMIME.SetAt( "Server", url.GetHostName() );
  sendMIME.SetAt( PHTTP::ContentTypeTag(), "text/xml" );
  sendMIME.SetAt( "SOAPAction", soapAction );

  if(url.GetUserName() != "") {
      PStringStream SoapAuthToken;
      SoapAuthToken << url.GetUserName() << ":" << url.GetPassword();
      sendMIME.SetAt( "Authorization", PBase64::Encode(SoapAuthToken) );
  }

  // Set thetimeout
  client.SetReadTimeout( timeout );

  PString replyBody;

  // Send the POST request to the server
  bool ok = client.PostData( url, sendMIME, soapRequest, replyMIME, replyBody);

  // Check if the server really gave us something
  if ( !ok || replyBody.IsEmpty() ) 
    txt << "HTTP POST failed: "
        << client.GetLastResponseCode() << ' '
        << client.GetLastResponseInfo();
  else
    PTRACE( 5, "PSOAP\tIncoming SOAP is " << replyBody );

  // Parse the response only if the response code from the server
  // is either 500 (Internal server error) or 200 (RequestOK)

  if ( ( client.GetLastResponseCode() == PHTTP::RequestOK ) ||
       ( client.GetLastResponseCode() == PHTTP::InternalServerError ) )
  {
    if (!response.Load(replyBody)) 
    {
      txt << "Error parsing response XML ("
        << response.GetErrorLine() 
        << ") :" 
        << response.GetErrorString();
      
      PStringArray lines = replyBody.Lines();
      for ( int offset = -2; offset <= 2; offset++ ) {
        int line = response.GetErrorLine() + offset;
        
        if ( line >= 0 && line < lines.GetSize() )
          txt << lines[ ( PINDEX ) line ];
      }
    }
  }


  if ( ( client.GetLastResponseCode() != PHTTP::RequestOK ) ||
       ( !ok ) )
  {
    response.SetFault( PSOAPMessage::Server, txt );
    return PFalse;
  }


  return PTrue;
}


#endif // P_SOAP


// End of File ////////////////////////////////////////////////////////////////
