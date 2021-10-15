/*
 * pssl.cxx
 *
 * SSL implementation for PTLib using the SSLeay package
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
 * Portions bsed upon the file crypto/buffer/bss_sock.c 
 * Original copyright notice appears below
 *
 * $Id$
 * $Revision$
 * $Author$
 * $Date$
 */

/* crypto/buffer/bss_sock.c */
/* Copyright (C) 1995-1996 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 * 
 * This file is part of an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL
 * specification.  This library and applications are
 * FREE FOR COMMERCIAL AND NON-COMMERCIAL USE
 * as long as the following conditions are aheared to.
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.  If this code is used in a product,
 * Eric Young should be given attribution as the author of the parts used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Eric Young (eay@mincom.oz.au)
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#ifdef __GNUC__
#pragma implementation "pssl.h"
#endif

#include <ptlib.h>

#include <ptclib/pssl.h>
#include <ptclib/mime.h>

#include <ptbuildopts.h>

#if P_SSL

#define USE_SOCKETS

extern "C" {

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

};

#if defined(__cpp_threadsafe_static_init) && (__cpp_threadsafe_static_init+0) >= 200806
#if !defined(P_THREADSAFE_STATIC_INIT)
#define P_THREADSAFE_STATIC_INIT
#endif
#endif

#if defined(P_THREADSAFE_STATIC_INIT) && defined(P_EXCEPTIONS)
#include <stdexcept>
#elif defined(P_PTHREADS)
#include <stdlib.h> // atexit
#include <pthread.h>
#else
#include <stdlib.h> // atexit
#include <ptlib/mutex.h>
#include <ptlib/psync.h>
#endif

#ifdef _MSC_VER
  #pragma comment(lib, P_SSL_LIB1)
  #pragma comment(lib, P_SSL_LIB2)
  #pragma message("SSL support (via OpenSSL) enabled")
#endif


// On Windows, use a define from the header to guess the API type
#ifdef _WIN32
#ifdef SSL_OP_NO_QUERY_MTU
#define P_SSL_USE_CONST 1
#endif
#endif


class PSSLInitialiser : public PProcessStartup
{
  PCLASSINFO(PSSLInitialiser, PProcessStartup)
  public:
    virtual void OnStartup();
    virtual void OnShutdown();
    void LockingCallback(int mode, int n);

    PFACTORY_GET_SINGLETON(PProcessStartupFactory, PSSLInitialiser);

  private:
    vector<PMutex> mutexes;
};

PFACTORY_CREATE_SINGLETON(PProcessStartupFactory, PSSLInitialiser);


///////////////////////////////////////////////////////////////////////////////

class PSSL_BIO
{
  public:
#if OPENSSL_VERSION_NUMBER >= 0x10100000L || LIBRESSL_VERSION_NUMBER >= 0x3040100fL
    PSSL_BIO(const BIO_METHOD *method = BIO_s_file())
#else
    PSSL_BIO(BIO_METHOD *method = BIO_s_file_internal())
#endif
      { bio = BIO_new(method); }

    ~PSSL_BIO()
      { BIO_free(bio); }

    operator BIO*() const
      { return bio; }

    bool OpenRead(const PFilePath & filename)
      { return BIO_read_filename(bio, (char *)(const char *)filename) > 0; }

    bool OpenWrite(const PFilePath & filename)
      { return BIO_write_filename(bio, (char *)(const char *)filename) > 0; }

    bool OpenAppend(const PFilePath & filename)
      { return BIO_append_filename(bio, (char *)(const char *)filename) > 0; }

  protected:
    BIO * bio;
};


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

PSSLPrivateKey::PSSLPrivateKey()
{
  key = NULL;
}


PSSLPrivateKey::PSSLPrivateKey(unsigned modulus,
                               void (*callback)(int,int,void *),
                               void *cb_arg)
{
  key = NULL;
  Create(modulus, callback, cb_arg);
}


PSSLPrivateKey::PSSLPrivateKey(const PFilePath & keyFile, PSSLFileTypes fileType)
{
  key = NULL;
  Load(keyFile, fileType);
}


PSSLPrivateKey::PSSLPrivateKey(const BYTE * keyData, PINDEX keySize)
{
#if P_SSL_USE_CONST
  key = d2i_AutoPrivateKey(NULL, &keyData, keySize);
#else
  key = d2i_AutoPrivateKey(NULL, (BYTE **)&keyData, keySize);
#endif
}


PSSLPrivateKey::PSSLPrivateKey(const PBYTEArray & keyData)
{
  const BYTE * keyPtr = keyData;
#if P_SSL_USE_CONST
  key = d2i_AutoPrivateKey(NULL, &keyPtr, keyData.GetSize());
#else
  key = d2i_AutoPrivateKey(NULL, (BYTE **)&keyPtr, keyData.GetSize());
#endif
}


PSSLPrivateKey::PSSLPrivateKey(const PSSLPrivateKey & privKey)
{
  key = privKey.key;
}


PSSLPrivateKey & PSSLPrivateKey::operator=(const PSSLPrivateKey & privKey)
{
  if (key != NULL)
    EVP_PKEY_free(key);

  key = privKey.key;

  return *this;
}


PSSLPrivateKey::~PSSLPrivateKey()
{
  if (key != NULL)
    EVP_PKEY_free(key);
}


PBoolean PSSLPrivateKey::Create(unsigned modulus,
                            void (*callback)(int,int,void *),
                            void *cb_arg)
{
  if (key != NULL) {
    EVP_PKEY_free(key);
    key = NULL;
  }

  if (modulus < 384) {
    return PFalse;
  }

  key = EVP_PKEY_new();
  if (key == NULL)
    return PFalse;

  if (EVP_PKEY_assign_RSA(key, RSA_generate_key(modulus, 0x10001, callback, cb_arg)))
    return PTrue;

  EVP_PKEY_free(key);
  key = NULL;
  return PFalse;
}


PBYTEArray PSSLPrivateKey::GetData() const
{
  PBYTEArray data;

  if (key != NULL) {
    BYTE * keyPtr = data.GetPointer(i2d_PrivateKey(key, NULL));
    i2d_PrivateKey(key, &keyPtr);
  }

  return data;
}


PString PSSLPrivateKey::AsString() const
{
  return PBase64::Encode(GetData());
}


PBoolean PSSLPrivateKey::Load(const PFilePath & keyFile, PSSLFileTypes fileType)
{
  if (key != NULL) {
    EVP_PKEY_free(key);
    key = NULL;
  }

  PSSL_BIO in;
  if (!in.OpenRead(keyFile)) {
    SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE,ERR_R_SYS_LIB);
    return PFalse;
  }

  if (fileType == PSSLFileTypeDEFAULT)
    fileType = keyFile.GetType() == ".pem" ? PSSLFileTypePEM : PSSLFileTypeASN1;

  switch (fileType) {
    case PSSLFileTypeASN1 :
      key = d2i_PrivateKey_bio(in, NULL);
      if (key != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE, ERR_R_ASN1_LIB);
      break;

    case PSSLFileTypePEM :
      key = PEM_read_bio_PrivateKey(in, NULL, NULL, NULL);
      if (key != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE, ERR_R_PEM_LIB);
      break;

    default :
      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE,SSL_R_BAD_SSL_FILETYPE);
  }

  return PFalse;
}


PBoolean PSSLPrivateKey::Save(const PFilePath & keyFile, PBoolean append, PSSLFileTypes fileType)
{
  if (key == NULL)
    return PFalse;

  PSSL_BIO out;
  if (!(append ? out.OpenAppend(keyFile) : out.OpenWrite(keyFile))) {
    SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE,ERR_R_SYS_LIB);
    return PFalse;
  }

  if (fileType == PSSLFileTypeDEFAULT)
    fileType = keyFile.GetType() == ".pem" ? PSSLFileTypePEM : PSSLFileTypeASN1;

  switch (fileType) {
    case PSSLFileTypeASN1 :
      if (i2d_PrivateKey_bio(out, key))
        return PTrue;

      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE, ERR_R_ASN1_LIB);
      break;

    case PSSLFileTypePEM :
      if (PEM_write_bio_PrivateKey(out, key, NULL, NULL, 0, 0, NULL))
        return PTrue;

      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE, ERR_R_PEM_LIB);
      break;

    default :
      SSLerr(SSL_F_SSL_USE_PRIVATEKEY_FILE,SSL_R_BAD_SSL_FILETYPE);
  }

  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

PSSLCertificate::PSSLCertificate()
{
  certificate = NULL;
}


PSSLCertificate::PSSLCertificate(const PFilePath & certFile, PSSLFileTypes fileType)
{
  certificate = NULL;
  Load(certFile, fileType);
}


PSSLCertificate::PSSLCertificate(const BYTE * certData, PINDEX certSize)
{
#if P_SSL_USE_CONST
  certificate = d2i_X509(NULL, &certData, certSize);
#else
  certificate = d2i_X509(NULL, (unsigned char **)&certData, certSize);
#endif
}


PSSLCertificate::PSSLCertificate(const PBYTEArray & certData)
{
  const BYTE * certPtr = certData;
#if P_SSL_USE_CONST
  certificate = d2i_X509(NULL, &certPtr, certData.GetSize());
#else
  certificate = d2i_X509(NULL, (unsigned char **)&certPtr, certData.GetSize());
#endif
}


PSSLCertificate::PSSLCertificate(const PString & certStr)
{
  PBYTEArray certData;
  PBase64::Decode(certStr, certData);
  if (certData.GetSize() > 0) {
    const BYTE * certPtr = certData;
#if P_SSL_USE_CONST
    certificate = d2i_X509(NULL, &certPtr, certData.GetSize());
#else
    certificate = d2i_X509(NULL, (unsigned char **)&certPtr, certData.GetSize());
#endif
  }
  else
    certificate = NULL;
}


PSSLCertificate::PSSLCertificate(const PSSLCertificate & cert)
{
  if (cert.certificate == NULL)
    certificate = NULL;
  else
    certificate = X509_dup(cert.certificate);
}


PSSLCertificate & PSSLCertificate::operator=(const PSSLCertificate & cert)
{
  if (certificate != NULL)
    X509_free(certificate);
  if (cert.certificate == NULL)
    certificate = NULL;
  else
    certificate = X509_dup(cert.certificate);

  return *this;
}


PSSLCertificate::~PSSLCertificate()
{
  if (certificate != NULL)
    X509_free(certificate);
}


PBoolean PSSLCertificate::CreateRoot(const PString & subject,
                                 const PSSLPrivateKey & privateKey)
{
  if (certificate != NULL) {
    X509_free(certificate);
    certificate = NULL;
  }

  if (privateKey == NULL)
    return PFalse;

  POrdinalToString info;
  PStringArray fields = subject.Tokenise('/', PFalse);
  PINDEX i;
  for (i = 0; i < fields.GetSize(); i++) {
    PString field = fields[i];
    PINDEX equals = field.Find('=');
    if (equals != P_MAX_INDEX) {
      int nid = OBJ_txt2nid((char *)(const char *)field.Left(equals));
      if (nid != NID_undef)
        info.SetAt(nid, field.Mid(equals+1));
    }
  }
  if (info.IsEmpty())
    return PFalse;

  certificate = X509_new();
  if (certificate == NULL)
    return PFalse;

  if (X509_set_version(certificate, 2)) {
    /* Set version to V3 */
    ASN1_INTEGER_set(X509_get_serialNumber(certificate), 0L);

    X509_NAME * name = X509_NAME_new();
    for (i = 0; i < info.GetSize(); i++)
      X509_NAME_add_entry_by_NID(name,
                                 info.GetKeyAt(i),
                                 MBSTRING_ASC,
                                 (unsigned char *)(const char *)info.GetDataAt(i),
                                 -1,-1, 0);
    X509_set_issuer_name(certificate, name);
    X509_set_subject_name(certificate, name);
    X509_NAME_free(name);

    X509_gmtime_adj(X509_get_notBefore(certificate), 0);
    X509_gmtime_adj(X509_get_notAfter(certificate), (long)60*60*24*365*5);

    X509_PUBKEY * pubkey = X509_PUBKEY_new();
    if (pubkey != NULL) {
      X509_PUBKEY_set(&pubkey, privateKey);
      EVP_PKEY * pkey = X509_PUBKEY_get(pubkey);
      X509_set_pubkey(certificate, pkey);
      EVP_PKEY_free(pkey);
      X509_PUBKEY_free(pubkey);

      if (X509_sign(certificate, privateKey, EVP_md5()) > 0)
        return PTrue;
    }
  }

  X509_free(certificate);
  certificate = NULL;
  return PFalse;
}


PBYTEArray PSSLCertificate::GetData() const
{
  PBYTEArray data;

  if (certificate != NULL) {
    BYTE * certPtr = data.GetPointer(i2d_X509(certificate, NULL));
    i2d_X509(certificate, &certPtr);
  }

  return data;
}


PString PSSLCertificate::AsString() const
{
  return PBase64::Encode(GetData());
}


PBoolean PSSLCertificate::Load(const PFilePath & certFile, PSSLFileTypes fileType)
{
  if (certificate != NULL) {
    X509_free(certificate);
    certificate = NULL;
  }

  PSSL_BIO in;
  if (!in.OpenRead(certFile)) {
    SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE,ERR_R_SYS_LIB);
    return PFalse;
  }

  if (fileType == PSSLFileTypeDEFAULT)
    fileType = certFile.GetType() == ".pem" ? PSSLFileTypePEM : PSSLFileTypeASN1;

  switch (fileType) {
    case PSSLFileTypeASN1 :
      certificate = d2i_X509_bio(in, NULL);
      if (certificate != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE, ERR_R_ASN1_LIB);
      break;

    case PSSLFileTypePEM :
      certificate = PEM_read_bio_X509(in, NULL, NULL, NULL);
      if (certificate != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE, ERR_R_PEM_LIB);
      break;

    default :
      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE,SSL_R_BAD_SSL_FILETYPE);
  }

  return PFalse;
}


PBoolean PSSLCertificate::Save(const PFilePath & certFile, PBoolean append, PSSLFileTypes fileType)
{
  if (certificate == NULL)
    return PFalse;

  PSSL_BIO out;
  if (!(append ? out.OpenAppend(certFile) : out.OpenWrite(certFile))) {
    SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE,ERR_R_SYS_LIB);
    return PFalse;
  }

  if (fileType == PSSLFileTypeDEFAULT)
    fileType = certFile.GetType() == ".pem" ? PSSLFileTypePEM : PSSLFileTypeASN1;

  switch (fileType) {
    case PSSLFileTypeASN1 :
      if (i2d_X509_bio(out, certificate))
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE, ERR_R_ASN1_LIB);
      break;

    case PSSLFileTypePEM :
      if (PEM_write_bio_X509(out, certificate))
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE, ERR_R_PEM_LIB);
      break;

    default :
      SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_FILE,SSL_R_BAD_SSL_FILETYPE);
  }

  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

PSSLDiffieHellman::PSSLDiffieHellman()
{
  dh = NULL;
}


PSSLDiffieHellman::PSSLDiffieHellman(const PFilePath & dhFile,
                                     PSSLFileTypes fileType)
{
  dh = NULL;
  Load(dhFile, fileType);
}


PSSLDiffieHellman::PSSLDiffieHellman(const BYTE * pData, PINDEX pSize,
                                     const BYTE * gData, PINDEX gSize)
{
  dh = DH_new();
  if (dh == NULL)
    return;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
  DH_set0_pqg (dh, BN_bin2bn(pData, pSize, NULL), NULL, BN_bin2bn(gData, gSize, NULL));
  const BIGNUM *p, *g;
  DH_get0_pqg(dh, &p, NULL, &g);
  if (p != NULL && g != NULL)
    return;
#else
  dh->p = BN_bin2bn(pData, pSize, NULL);
  dh->g = BN_bin2bn(gData, gSize, NULL);
  if (dh->p != NULL && dh->g != NULL)
    return;
#endif

  DH_free(dh);
  dh = NULL;
}


PSSLDiffieHellman::PSSLDiffieHellman(const PSSLDiffieHellman & diffie)
{
  dh = diffie.dh;
}


PSSLDiffieHellman & PSSLDiffieHellman::operator=(const PSSLDiffieHellman & diffie)
{
  if (dh != NULL)
    DH_free(dh);
  dh = diffie.dh;
  return *this;
}


PSSLDiffieHellman::~PSSLDiffieHellman()
{
  if (dh != NULL)
    DH_free(dh);
}

#if defined(P_d2i_DHparams_bio_OLD) && OPENSSL_VERSION_NUMBER < 0x100020efL
// 2/21/04 Yuri Kiryanov - fix for compiler choke on BeOS for usage of
// SSL function d2i_DHparams_bio below in PSSLDiffieHellman::Load
// 5/26/06 Hannes Friederich - Mac OS X seems to need that fix too...
// 3/15/08 Hannes Friederich - Mac OS X 10.5 (Darwin 9.X) no longer needs this
#undef  d2i_DHparams_bio
#define d2i_DHparams_bio(bp,x) \
 (DH *)ASN1_d2i_bio( \
         (char *(*)(...))(void *)DH_new, \
         (char *(*)(...))(void *)d2i_DHparams, \
         (bp), \
         (unsigned char **)(x) \
)
#endif

PBoolean PSSLDiffieHellman::Load(const PFilePath & dhFile,
                             PSSLFileTypes fileType)
{
  if (dh != NULL) {
    DH_free(dh);
    dh = NULL;
  }

  PSSL_BIO in;
  if (!in.OpenRead(dhFile)) {
    SSLerr(SSL_F_SSL_CTX_USE_PRIVATEKEY_FILE,ERR_R_SYS_LIB);
    return PFalse;
  }

  if (fileType == PSSLFileTypeDEFAULT)
    fileType = dhFile.GetType() == ".pem" ? PSSLFileTypePEM : PSSLFileTypeASN1;

  switch (fileType) {
    case PSSLFileTypeASN1 :
      dh = d2i_DHparams_bio(in, NULL);
      if (dh != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_PRIVATEKEY_FILE, ERR_R_ASN1_LIB);
      break;

    case PSSLFileTypePEM :
      dh = PEM_read_bio_DHparams(in, NULL, NULL, NULL);
      if (dh != NULL)
        return PTrue;

      SSLerr(SSL_F_SSL_CTX_USE_PRIVATEKEY_FILE, ERR_R_PEM_LIB);
      break;

    default :
      SSLerr(SSL_F_SSL_CTX_USE_PRIVATEKEY_FILE,SSL_R_BAD_SSL_FILETYPE);
  }

  return PFalse;
}


///////////////////////////////////////////////////////////////////////////////

static void LockingCallback(int mode, int n, const char * /*file*/, int /*line*/)
{
  PSSLInitialiser::GetInstance().LockingCallback(mode, n);
}


void PSSLInitialiser::OnStartup()
{
  SSL_library_init();
  SSL_load_error_strings();

  // Seed the random number generator
#if defined(P_LINUX) || defined (P_FREEBSD)
  RAND_load_file("/dev/urandom", 1024);
#else
  BYTE seed[128];
  for (size_t i = 0; i < sizeof(seed); i++)
    seed[i] = (BYTE)rand();
  RAND_seed(seed, sizeof(seed));
#endif

  // set up multithread stuff
  mutexes.resize(CRYPTO_num_locks());
  CRYPTO_set_locking_callback(::LockingCallback);
}


void PSSLInitialiser::OnShutdown()
{
  CRYPTO_set_locking_callback(NULL);
  ERR_free_strings();
}


void PSSLInitialiser::LockingCallback(int mode, int n)
{
  if ((mode & CRYPTO_LOCK) != 0)
    mutexes[n].Wait();
  else
    mutexes[n].Signal();
}


static int VerifyCallBack(int ok, X509_STORE_CTX * ctx)
{
  X509 * err_cert = X509_STORE_CTX_get_current_cert(ctx);
  //int err         = X509_STORE_CTX_get_error(ctx);

  // get the subject name, just for verification
  char buf[256];
  X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

  PTRACE(3, "SSL\tVerify callback depth "
         << X509_STORE_CTX_get_error_depth(ctx)
         << " : cert name = " << buf);

  return ok;
}


static void PSSLAssert(const char * msg)
{
  char buf[256];
  strncpy(buf, msg, sizeof(buf)-1);
  buf[sizeof(buf)-1] = '\0';
  ERR_error_string(ERR_peek_error(), &buf[strlen(msg)]);
  PTRACE(1, "SSL\t" << buf);
  PAssertAlways(buf);
}


///////////////////////////////////////////////////////////////////////////////

PSSLContext::PSSLContext(Method method, const void * sessionId, PINDEX idSize)
{
  Construct(method, sessionId, idSize);
}


PSSLContext::PSSLContext(const void * sessionId, PINDEX idSize)
{
  Construct(SSLv23, sessionId, idSize);
}

void PSSLContext::Construct(Method method, const void * sessionId, PINDEX idSize)
{
  // create the new SSL context
  context  = SSL_CTX_new(SSLv23_method());
  if (context == NULL)
    PSSLAssert("Error creating context: ");

  // Always disable unsafe SSLv2 and SSLv3 (the latter - unless specifically requested).
  // Always disable TLS compression as it is unsafe.
  // https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/
  switch (method) {
    case SSLv3:
      SSL_CTX_set_options(context, SSL_OP_NO_COMPRESSION | SSL_OP_NO_SSLv2);
      break;
    default:
      SSL_CTX_set_options(context, SSL_OP_NO_COMPRESSION | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
      break;
  }

  // Shutdown
  SSL_CTX_set_quiet_shutdown(context, 1);

  // Set default locations
  if (!SSL_CTX_load_verify_locations(context, NULL, ".") ||
      !SSL_CTX_set_default_verify_paths(context))
    PSSLAssert("Cannot set CAfile and path: ");

  if (sessionId != NULL) {
    if (idSize == 0)
      idSize = ::strlen((const char *)sessionId)+1;
    SSL_CTX_set_session_id_context(context, (const BYTE *)sessionId, idSize);
    SSL_CTX_sess_set_cache_size(context, 128);
  }

  // set default verify mode
  SSL_CTX_set_verify(context, SSL_VERIFY_NONE, VerifyCallBack);
}


PSSLContext::~PSSLContext()
{
  SSL_CTX_free(context);
}


PBoolean PSSLContext::SetCAPath(const PDirectory & caPath)
{
  PString path = caPath.Left(caPath.GetLength()-1);
  if (!SSL_CTX_load_verify_locations(context, NULL, path))
    return PFalse;

  return SSL_CTX_set_default_verify_paths(context);
}


PBoolean PSSLContext::SetCAFile(const PFilePath & caFile)
{
  if (!SSL_CTX_load_verify_locations(context, caFile, NULL))
    return PFalse;

  return SSL_CTX_set_default_verify_paths(context);
}


PBoolean PSSLContext::UseCertificate(const PSSLCertificate & certificate)
{
  return SSL_CTX_use_certificate(context, certificate) > 0;
}


PBoolean PSSLContext::UsePrivateKey(const PSSLPrivateKey & key)
{
  if (SSL_CTX_use_PrivateKey(context, key) <= 0)
    return PFalse;

  return SSL_CTX_check_private_key(context);
}


PBoolean PSSLContext::UseDiffieHellman(const PSSLDiffieHellman & dh)
{
  return SSL_CTX_set_tmp_dh(context, (dh_st *)dh) > 0;
}


PBoolean PSSLContext::SetCipherList(const PString & ciphers)
{
  if (ciphers.IsEmpty())
    return PFalse;

  return SSL_CTX_set_cipher_list(context, (char *)(const char *)ciphers);
}


/////////////////////////////////////////////////////////////////////////
//
//  SSLChannel
//

PSSLChannel::PSSLChannel(PSSLContext * ctx, PBoolean autoDel)
{
  if (ctx != NULL) {
    context = ctx;
    autoDeleteContext = autoDel;
  }
  else {
    context = new PSSLContext;
    autoDeleteContext = PTrue;
  }

  ssl = SSL_new(*context);
  if (ssl == NULL)
    PSSLAssert("Error creating channel: ");
}


PSSLChannel::PSSLChannel(PSSLContext & ctx)
{
  context = &ctx;
  autoDeleteContext = PFalse;

  ssl = SSL_new(*context);
}


PSSLChannel::~PSSLChannel()
{
  // free the SSL connection
  if (ssl != NULL)
    SSL_free(ssl);

  if (autoDeleteContext)
    delete context;
}


PBoolean PSSLChannel::Read(void * buf, PINDEX len)
{
  flush();

  channelPointerMutex.StartRead();

  lastReadCount = 0;

  PBoolean returnValue = PFalse;
  if (readChannel == NULL)
    SetErrorValues(NotOpen, EBADF, LastReadError);
  else if (readTimeout == 0 && SSL_pending(ssl) == 0)
    SetErrorValues(Timeout, ETIMEDOUT, LastReadError);
  else {
    readChannel->SetReadTimeout(readTimeout);

    int readResult = SSL_read(ssl, (char *)buf, len);
    lastReadCount = readResult;
    returnValue = readResult > 0;
    if (readResult < 0 && GetErrorCode(LastReadError) == NoError)
      ConvertOSError(-1, LastReadError);
  }

  channelPointerMutex.EndRead();

  return returnValue;
}

PBoolean PSSLChannel::Write(const void * buf, PINDEX len)
{
  flush();

  channelPointerMutex.StartRead();

  lastWriteCount = 0;

  PBoolean returnValue;
  if (writeChannel == NULL) {
    SetErrorValues(NotOpen, EBADF, LastWriteError);
    returnValue = PFalse;
  }
  else {
    writeChannel->SetWriteTimeout(writeTimeout);

    int writeResult = SSL_write(ssl, (const char *)buf, len);
    lastWriteCount = writeResult;
    returnValue = lastWriteCount >= len;
    if (writeResult < 0 && GetErrorCode(LastWriteError) == NoError)
      ConvertOSError(-1, LastWriteError);
  }

  channelPointerMutex.EndRead();

  return returnValue;
}


PBoolean PSSLChannel::Close()
{
  PBoolean ok = SSL_shutdown(ssl);
  return PIndirectChannel::Close() && ok;
}


PBoolean PSSLChannel::ConvertOSError(int error, ErrorGroup group)
{
  Errors lastError = NoError;
  DWORD osError = 0;
  if (SSL_get_error(ssl, error) != SSL_ERROR_NONE && (osError = ERR_peek_error()) != 0) {
    osError |= 0x80000000;
    lastError = Miscellaneous;
  }

  return SetErrorValues(lastError, osError, group);
}


PString PSSLChannel::GetErrorText(ErrorGroup group) const
{
  if ((lastErrorNumber[group]&0x80000000) == 0)
    return PIndirectChannel::GetErrorText(group);

  char buf[200];
  return ERR_error_string(lastErrorNumber[group]&0x7fffffff, buf);
}


PBoolean PSSLChannel::Accept()
{
  if (IsOpen())
    return ConvertOSError(SSL_accept(ssl));
  return PFalse;
}


PBoolean PSSLChannel::Accept(PChannel & channel)
{
  if (Open(channel))
    return ConvertOSError(SSL_accept(ssl));
  return PFalse;
}


PBoolean PSSLChannel::Accept(PChannel * channel, PBoolean autoDelete)
{
  if (Open(channel, autoDelete))
    return ConvertOSError(SSL_accept(ssl));
  return PFalse;
}


PBoolean PSSLChannel::Connect()
{
  if (IsOpen())
    return ConvertOSError(SSL_connect(ssl));
  return PFalse;
}


PBoolean PSSLChannel::Connect(PChannel & channel)
{
  if (Open(channel))
    return ConvertOSError(SSL_connect(ssl));
  return PFalse;
}


PBoolean PSSLChannel::Connect(PChannel * channel, PBoolean autoDelete)
{
  if (Open(channel, autoDelete))
    return ConvertOSError(SSL_connect(ssl));
  return PFalse;
}


PBoolean PSSLChannel::UseCertificate(const PSSLCertificate & certificate)
{
  return SSL_use_certificate(ssl, certificate);
}


void PSSLChannel::SetVerifyMode(VerifyMode mode)
{
  int verify;

  switch (mode) {
    default :
    case VerifyNone:
      verify = SSL_VERIFY_NONE;
      break;

    case VerifyPeer:
      verify = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
      break;

    case VerifyPeerMandatory:
      verify = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  }

  SSL_set_verify(ssl, verify, VerifyCallBack);
}


PBoolean PSSLChannel::RawSSLRead(void * buf, PINDEX & len)
{
  if (!PIndirectChannel::Read(buf, len)) 
    return PFalse;

  len = GetLastReadCount();
  return PTrue;
}


//////////////////////////////////////////////////////////////////////////
//
//  Low level interface to SSLEay routines
//


#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
#define PSSLCHANNEL(bio)      ((PSSLChannel *)(BIO_get_data (bio)))
#else
#define PSSLCHANNEL(bio)      ((PSSLChannel *)(bio->ptr))
#endif

extern "C" {

#if (OPENSSL_VERSION_NUMBER < 0x00906000)

typedef int (*ifptr)();
typedef long (*lfptr)();

#endif

static int Psock_new(BIO * bio)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
  BIO_set_init (bio, 0);
  BIO_set_data (bio, NULL);    // this is really (PSSLChannel *)
  BIO_set_flags (bio, 0);
#else
  bio->init     = 0;
  bio->num      = 0;
  bio->ptr      = NULL;    // this is really (PSSLChannel *)
  bio->flags    = 0;
#endif

  return(1);
}


static int Psock_free(BIO * bio)
{
  if (bio == NULL)
    return 0;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
  if (BIO_get_shutdown (bio)) {
    if (BIO_get_init (bio)) {
#else
  if (bio->shutdown) {
    if (bio->init) {
#endif
      PSSLCHANNEL(bio)->Shutdown(PSocket::ShutdownReadAndWrite);
      PSSLCHANNEL(bio)->Close();
    }
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
    BIO_set_init (bio, 0);
    BIO_set_flags (bio, 0);
#else
    bio->init  = 0;
    bio->flags = 0;
#endif
  }
  return 1;
}


static long Psock_ctrl(BIO * bio, int cmd, long num, void * /*ptr*/)
{
  switch (cmd) {
    case BIO_CTRL_SET_CLOSE:
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
      BIO_set_shutdown (bio, (int)num);
#else
      bio->shutdown = (int)num;
#endif
      return 1;

    case BIO_CTRL_GET_CLOSE:
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
      return BIO_get_shutdown (bio);
#else
      return bio->shutdown;
#endif

    case BIO_CTRL_FLUSH:
      return 1;
  }

  // Other BIO commands, return 0
  return 0;
}


static int Psock_read(BIO * bio, char * out, int outl)
{
  if (out == NULL)
    return 0;

  BIO_clear_retry_flags(bio);

  // Skip over the polymorphic read, want to do real one
  PINDEX len = outl;
  if (PSSLCHANNEL(bio)->RawSSLRead(out, len))
    return len;

  switch (PSSLCHANNEL(bio)->GetErrorCode(PChannel::LastReadError)) {
    case PChannel::Interrupted :
    case PChannel::Timeout :
      BIO_set_retry_read(bio);
      return -1;

    default :
      break;
  }

  return 0;
}


static int Psock_write(BIO * bio, const char * in, int inl)
{
  if (in == NULL)
    return 0;

  BIO_clear_retry_flags(bio);

  // Skip over the polymorphic write, want to do real one
  if (PSSLCHANNEL(bio)->PIndirectChannel::Write(in, inl))
    return PSSLCHANNEL(bio)->GetLastWriteCount();

  switch (PSSLCHANNEL(bio)->GetErrorCode(PChannel::LastWriteError)) {
    case PChannel::Interrupted :
    case PChannel::Timeout :
      BIO_set_retry_write(bio);
      return -1;

    default :
      break;
  }

  return 0;
}


static int Psock_puts(BIO * bio, const char * str)
{
  int n,ret;

  n   = strlen(str);
  ret = Psock_write(bio,str,n);

  return ret;
}

} // extern "C"

namespace {

#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)

inline BIO_METHOD* CreatePsockMethods()
{
  BIO_METHOD* methods_pSock = BIO_meth_new (BIO_TYPE_SOCKET, "PTLib-PSSLChannel");
  if (methods_pSock) {
    BIO_meth_set_write (methods_pSock, Psock_write);
    BIO_meth_set_read (methods_pSock, Psock_read);
    BIO_meth_set_puts (methods_pSock, Psock_puts);
    BIO_meth_set_ctrl (methods_pSock, Psock_ctrl);
    BIO_meth_set_create (methods_pSock, Psock_new);
    BIO_meth_set_destroy (methods_pSock, Psock_free);
  }
  return methods_pSock;
}

#if defined(P_THREADSAFE_STATIC_INIT) && defined(P_EXCEPTIONS)

struct PsockMethodsInitializer
{
  BIO_METHOD* methods;

  PsockMethodsInitializer() : methods(CreatePsockMethods())
  {
    if (!methods)
      throw std::runtime_error("Failed to create Psock methods");
  }
  ~PsockMethodsInitializer()
  {
    BIO_meth_free (methods);
  }
};

inline BIO_METHOD* GetPsockMethods()
{
  try {
    static const PsockMethodsInitializer meth_init;
    return meth_init.methods;
  }
  catch (...) {
    return NULL;
  }
}

#elif defined(P_PTHREADS)

static pthread_mutex_t g_methods_pSock_mutex = PTHREAD_MUTEX_INITIALIZER;
static BIO_METHOD *g_methods_pSock = NULL;

extern "C" void PTLibFreePsockMethods()
{
  BIO_meth_free (g_methods_pSock);
  g_methods_pSock = NULL;
}

inline BIO_METHOD* GetPsockMethods()
{
  pthread_mutex_lock(&g_methods_pSock_mutex);

  BIO_METHOD* methods = g_methods_pSock;
  if (!methods) {
    methods = CreatePsockMethods();
    if (methods) {
      g_methods_pSock = methods;
      atexit(&PTLibFreePsockMethods);
    }
  }

  pthread_mutex_unlock(&g_methods_pSock_mutex);

  return methods;
}

#else

static PMutex g_methods_pSock_mutex;
static BIO_METHOD *g_methods_pSock = NULL;

extern "C" void PTLibFreePsockMethods()
{
  BIO_meth_free (g_methods_pSock);
  g_methods_pSock = NULL;
}

inline BIO_METHOD* GetPsockMethods()
{
  PWaitAndSignal lock(g_methods_pSock_mutex);

  BIO_METHOD* methods = g_methods_pSock;
  if (!methods) {
    methods = CreatePsockMethods();
    if (methods) {
      g_methods_pSock = methods;
      atexit(&PTLibFreePsockMethods);
    }
  }

  return methods;
}

#endif

#else // OPENSSL_VERSION_NUMBER >= 0x10100000l

static BIO_METHOD g_methods_Psock =
{
  BIO_TYPE_SOCKET,
  "PTLib-PSSLChannel",
#if (OPENSSL_VERSION_NUMBER < 0x00906000)
  (ifptr)Psock_write,
  (ifptr)Psock_read,
  (ifptr)Psock_puts,
  NULL,
  (lfptr)Psock_ctrl,
  (ifptr)Psock_new,
  (ifptr)Psock_free
#else
  Psock_write,
  Psock_read,
  Psock_puts,
  NULL,
  Psock_ctrl,
  Psock_new,
  Psock_free
#endif
};

inline BIO_METHOD* GetPsockMethods()
{
  return &g_methods_Psock;
}

#endif // OPENSSL_VERSION_NUMBER >= 0x10100000l

} // namespace

PBoolean PSSLChannel::OnOpen()
{
  BIO_METHOD *methods_pSock = GetPsockMethods();
  if (methods_pSock == NULL)
    return PFalse;

  BIO *bio = BIO_new(methods_pSock);
  if (bio == NULL) {
    SSLerr(SSL_F_SSL_SET_FD,ERR_R_BUF_LIB);
    return PFalse;
  }

  // "Open" then bio
#if OPENSSL_VERSION_NUMBER >= 0x10100000L && !defined(LIBRESSL_VERSION_NUMBER)
  BIO_set_data (bio, this);
  BIO_set_init (bio, 1);
#else
  bio->ptr  = this;
  bio->init = 1;
#endif

  SSL_set_bio(ssl, bio, bio);
  return PTrue;
}


#else

  #ifdef _MSC_VER
    #pragma message("SSL support (via OpenSSL) DISABLED")
  #endif

#endif // P_SSL


// End of file ////////////////////////////////////////////////////////////////
