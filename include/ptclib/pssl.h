/*
 * pssl.h
 *
 * Secure Sockets Layer channel interface class.
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

#ifndef PTLIB_PSSL_H
#define PTLIB_PSSL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <ptlib/sockets.h>


struct ssl_st;
struct ssl_ctx_st;
struct x509_st;
struct evp_pkey_st;
struct dh_st;

enum PSSLFileTypes {
  PSSLFileTypePEM,
  PSSLFileTypeASN1,
  PSSLFileTypeDEFAULT
};


/**Private key for SSL.
   This class embodies a common environment for all private keys used by the
   PSSLContext and PSSLChannel classes.
  */
class PSSLPrivateKey : public PObject
{
  PCLASSINFO(PSSLPrivateKey, PObject);
  public:
    /**Create an empty private key.
      */
    PSSLPrivateKey();

    /**Create a new RSA private key.
      */
    PSSLPrivateKey(
      unsigned modulus,   ///< Number of bits
      void (*callback)(int,int,void *) = NULL,  ///< Progress callback function
      void *cb_arg = NULL                       ///< Argument passed to callback
    );

    /**Create a new private key given the file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLPrivateKey(
      const PFilePath & keyFile,  ///< Private key file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create private key from the binary ASN1 DER encoded data specified.
      */
    PSSLPrivateKey(
      const BYTE * keyData,   ///< Private key data
      PINDEX keySize          ///< Size of private key data
    );

    /**Create private key from the binary ASN1 DER encoded data specified.
      */
    PSSLPrivateKey(
      const PBYTEArray & keyData  ///< Private key data
    );

    /**Create a copy of the private key.
      */
    PSSLPrivateKey(
      const PSSLPrivateKey & privKey
    );

    /**Create a copy of the private key.
      */
    PSSLPrivateKey & operator=(
      const PSSLPrivateKey & privKay
    );

    /**Destroy and release storage for private key.
      */
    ~PSSLPrivateKey();

    /**Get internal OpenSSL private key structure.
      */
    operator evp_pkey_st *() const { return key; }

    /**Create a new private key.
     */
    PBoolean Create(
      unsigned modulus,   ///< Number of bits
      void (*callback)(int,int,void *) = NULL,  ///< Progress callback function
      void *cb_arg = NULL                       ///< Argument passed to callback
    );

    /**Get the certificate as binary ASN1 DER encoded data.
      */
    PBYTEArray GetData() const;

    /**Get the certificate as ASN1 DER base64 encoded data.
      */
    PString AsString() const;

    /**Load private key from file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & keyFile,  ///< Private key file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Save private key to file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Save(
      const PFilePath & keyFile,  ///< Private key file
      PBoolean append = false,        ///< Append to file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to write
    );


  protected:
    evp_pkey_st * key;
};


/**Certificate for SSL.
   This class embodies a common environment for all certificates used by the
   PSSLContext and PSSLChannel classes.
  */
class PSSLCertificate : public PObject
{
  PCLASSINFO(PSSLCertificate, PObject);
  public:
    /**Create an empty certificate.
      */
    PSSLCertificate();

    /**Create a new certificate given the file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLCertificate(
      const PFilePath & certFile, ///< Certificate file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create certificate from the binary ASN1 DER encoded data specified.
      */
    PSSLCertificate(
      const BYTE * certData,  ///< Certificate data
      PINDEX certSize        ///< Size of certificate data
    );

    /**Create certificate from the binary ASN1 DER encoded data specified.
      */
    PSSLCertificate(
      const PBYTEArray & certData  ///< Certificate data
    );

    /**Create certificate from the ASN1 DER base64 encoded data specified.
      */
    PSSLCertificate(
      const PString & certString  ///< Certificate data as string
    );

    /**Create a copy of the certificate.
      */
    PSSLCertificate(
      const PSSLCertificate & cert
    );

    /**Create a copy of the certificate.
      */
    PSSLCertificate & operator=(
      const PSSLCertificate & cert
    );

    /**Destroy and release storage for certificate.
      */
    ~PSSLCertificate();

    /**Get internal OpenSSL X509 structure.
      */
    operator x509_st *() const { return certificate; }

    /**Create a new root certificate.
       The subject name is a string of the form "/name=value/name=value" where
       name is a short name for the field and value is a string value for the
       field for example:
          "/C=ZA/SP=Western Cape/L=Cape Town/O=Thawte Consulting cc"
          "/OU=Certification Services Division/CN=Thawte Server CA"
          "/Email=server-certs@thawte.com"
     */
    PBoolean CreateRoot(
      const PString & subject,    ///< Subject name for certificate
      const PSSLPrivateKey & key  ///< Key to sign certificate with
    );

    /**Get the certificate as binary ASN1 DER encoded data.
      */
    PBYTEArray GetData() const;

    /**Get the certificate as ASN1 DER base64 encoded data.
      */
    PString AsString() const;

    /**Load certificate from file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & certFile, ///< Certificate file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Save certificate to file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Save(
      const PFilePath & keyFile,  ///< Certificate key file
      PBoolean append = false,        ///< Append to file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to write
    );


  protected:
    x509_st * certificate;
};


/**Diffie-Hellman parameters for SSL.
   This class embodies a set of Diffie Helman parameters as used by
   PSSLContext and PSSLChannel classes.
  */
class PSSLDiffieHellman : public PObject
{
  PCLASSINFO(PSSLDiffieHellman, PObject);
  public:
    /**Create an empty set of Diffie-Hellman parameters.
      */
    PSSLDiffieHellman();

    /**Create a new set of Diffie-Hellman parameters given the file.
       The type of the file can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLDiffieHellman(
      const PFilePath & dhFile, ///< Diffie-Hellman parameters file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create a set of Diffie-Hellman parameters.
      */
    PSSLDiffieHellman(
      const BYTE * pData, ///< P data
      PINDEX pSize,       ///< Size of P data
      const BYTE * gData, ///< G data
      PINDEX gSize        ///< Size of G data
    );

    /**Create a copy of the Diffie-Hellman parameters.
      */
    PSSLDiffieHellman(
      const PSSLDiffieHellman & dh
    );

    /**Create a copy of the Diffie-Hellman parameters.
      */
    PSSLDiffieHellman & operator=(
      const PSSLDiffieHellman & dh
    );

    /**Destroy and release storage for Diffie-Hellman parameters.
      */
    ~PSSLDiffieHellman();

    /**Get internal OpenSSL DH structure.
      */
    operator dh_st *() const { return dh; }

    /**Load Diffie-Hellman parameters from file.
       The type of the file can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & dhFile, ///< Diffie-Hellman parameters file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

  protected:
    dh_st * dh;
};


/**Context for SSL channels.
   This class embodies a common environment for all connections made via SSL
   using the PSSLChannel class. It includes such things as the version of SSL
   and certificates, CA's etc.
  */
class PSSLContext {
  public:
    enum Method {
      SSLv23,
      SSLv3,
      TLSv1
    };

    /**Create a new context for SSL channels.
       An optional session ID may be provided in the context. This is used
       to identify sessions across multiple channels in this context. The
       session ID is a completely arbitrary block of data. If sessionId is
       non NULL and idSize is zero, then sessionId is assumed to be a pointer
       to a C string.
       The default SSL method is SSLv23
      */
    PSSLContext(
      const void * sessionId = NULL,  ///< Pointer to session ID
      PINDEX idSize = 0               ///< Size of session ID
    );
    PSSLContext(
      Method method,                  ///< SSL connection method
      const void * sessionId = NULL,  ///< Pointer to session ID
      PINDEX idSize = 0               ///< Size of session ID
    );

    /**Clean up the SSL context.
      */
    ~PSSLContext();

    /**Get the internal SSL context structure.
      */
    operator ssl_ctx_st *() const { return context; }

    /**Set the path to locate CA certificates.
      */
    PBoolean SetCAPath(
      const PDirectory & caPath   ///< Directory for CA certificates
    );

    /**Set the CA certificate file.
      */
    PBoolean SetCAFile(
      const PFilePath & caFile    ///< CA certificate file
    );

    /**Use the certificate specified.
      */
    PBoolean UseCertificate(
      const PSSLCertificate & certificate
    );

    /**Use the private key specified.
      */
    PBoolean UsePrivateKey(
      const PSSLPrivateKey & key
    );

    /**Use the Diffie-Hellman parameters specified.
      */
    PBoolean UseDiffieHellman(
      const PSSLDiffieHellman & dh
    );

    /**Set the available ciphers to those listed.
      */
    PBoolean SetCipherList(
      const PString & ciphers   ///< List of cipher names.
    );

  protected:
    void Construct(Method method, const void * sessionId, PINDEX idSize);
    ssl_ctx_st * context;
};


/**This class will start a secure SSL based channel.
  */
class PSSLChannel : public PIndirectChannel
{
  PCLASSINFO(PSSLChannel, PIndirectChannel)
  public:
    /**Create a new channel given the context.
       If no context is given a default one is created.
      */
    PSSLChannel(
      PSSLContext * context = NULL,   ///< Context for SSL channel
      PBoolean autoDeleteContext = false  ///< Flag for context to be automatically deleted.
    );
    PSSLChannel(
      PSSLContext & context           ///< Context for SSL channel
    );

    /**Close and clear the SSL channel.
      */
    ~PSSLChannel();

    // Overrides from PChannel
    virtual PBoolean Read(void * buf, PINDEX len);
    virtual PBoolean Write(const void * buf, PINDEX len);
    virtual PBoolean Close();
    virtual PBoolean Shutdown(ShutdownValue) { return true; }
    virtual PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
    virtual PBoolean ConvertOSError(int error, ErrorGroup group = LastGeneralError);

    // New functions
    /**Accept a new inbound connection (server).
       This version expects that the indirect channel has already been opened
       using Open() beforehand.
      */
    PBoolean Accept();

    /**Accept a new inbound connection (server).
      */
    PBoolean Accept(
      PChannel & channel  ///< Channel to attach to.
    );

    /**Accept a new inbound connection (server).
      */
    PBoolean Accept(
      PChannel * channel,     ///< Channel to attach to.
      PBoolean autoDelete = true  ///< Flag for if channel should be automatically deleted.
    );


    /**Connect to remote server.
       This version expects that the indirect channel has already been opened
       using Open() beforehand.
      */
    PBoolean Connect();

    /**Connect to remote server.
      */
    PBoolean Connect(
      PChannel & channel  ///< Channel to attach to.
    );

    /**Connect to remote server.
      */
    PBoolean Connect(
      PChannel * channel,     ///< Channel to attach to.
      PBoolean autoDelete = true  ///< Flag for if channel should be automatically deleted.
    );

    /**Use the certificate specified.
      */
    PBoolean UseCertificate(
      const PSSLCertificate & certificate
    );

    /**Use the private key file specified.
      */
    PBoolean UsePrivateKey(
      const PSSLPrivateKey & key
    );

    enum VerifyMode {
      VerifyNone,
      VerifyPeer,
      VerifyPeerMandatory,
    };

    void SetVerifyMode(
      VerifyMode mode
    );

    PSSLContext * GetContext() const { return context; }

    virtual PBoolean RawSSLRead(void * buf, PINDEX & len);

  protected:
    /**This callback is executed when the Open() function is called with
       open channels. It may be used by descendent channels to do any
       handshaking required by the protocol that channel embodies.

       The default behaviour "connects" the channel to the OpenSSL library.

       @return
       Returns true if the protocol handshaking is successful.
     */
    virtual PBoolean OnOpen();

  protected:
    PSSLContext * context;
    PBoolean          autoDeleteContext;
    ssl_st      * ssl;
};

#endif // PTLIB_PSSL_H


// End Of File ///////////////////////////////////////////////////////////////
