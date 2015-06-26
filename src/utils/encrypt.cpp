/***************************************************************************
 *                                                                         *
 * Copyright (C) 2013 by frePPLe bvba                                      *
 *                                                                         *
 * You should never have received this file!                               *
 *                                                                         *
 ***************************************************************************/

#define FREPPLE_CORE
#include "frepple/utils.h"
#include "frepple/xml.h"

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace frepple
{
namespace utils
{

DECLARE_EXPORT int flags = 836125;

const MetaClass *LicenseValidator::metadata = NULL;


const unsigned char Decryptor::key[270] = {
  0x30,0x82,0x01,0x0A,0x02,0x82,0x01,0x01,0x00,0xCC,0x9F,0x8B,0xF9,0xEF,0xC1,0x06,
  0x81,0x5E,0x88,0xBF,0x94,0xE3,0xB0,0x86,0x05,0x1E,0x57,0xFE,0xBC,0x5D,0x6D,0x79,
  0xD6,0xAA,0xC7,0x15,0x2A,0x49,0x89,0xCF,0x39,0x6D,0x80,0xF7,0xA4,0xD3,0x4D,0xD6,
  0xFA,0xED,0x0E,0x7B,0xE8,0x19,0xBF,0x3A,0x8D,0xF9,0xB5,0x0D,0xC4,0x75,0xC6,0x7A,
  0xAE,0x59,0x30,0x06,0x37,0x49,0x25,0x7B,0xF9,0xE8,0x95,0x00,0x71,0x77,0x11,0x5D,
  0x55,0xC7,0xC3,0xFA,0x5A,0x24,0xC2,0x25,0x15,0xCB,0x7E,0x32,0xE0,0x1B,0xA7,0x6F,
  0x75,0xFF,0x84,0xE4,0x91,0x04,0xC2,0x58,0xF0,0x42,0x06,0x48,0xA0,0x09,0x5B,0xB6,
  0xDF,0x19,0xAF,0x62,0x6F,0x2C,0x8D,0xA9,0x8A,0x5E,0xEF,0x6D,0x54,0x7E,0x58,0x56,
  0x31,0xB7,0xAA,0x5A,0xAD,0xD2,0x86,0x7C,0xD4,0xFA,0xE3,0x17,0xDE,0x09,0x8F,0x54,
  0xD3,0xEA,0x27,0xF0,0x97,0x4C,0x49,0xE5,0x41,0xD2,0xC7,0xF6,0x85,0x92,0x20,0xA5,
  0x06,0x85,0x4E,0x14,0x10,0xAF,0x76,0x3A,0xAE,0xD4,0x20,0x5C,0xF0,0x88,0xB5,0xDC,
  0x99,0x73,0xC1,0x30,0x58,0xBF,0xF5,0x88,0x87,0xA6,0x8B,0x06,0x65,0x50,0xD2,0x59,
  0x33,0x46,0x59,0x6D,0x12,0x02,0xF0,0xA3,0x01,0x15,0xFB,0x1E,0x9E,0xA4,0xDE,0x4B,
  0xAC,0x59,0xBC,0x88,0x0A,0x85,0x91,0x7C,0x0A,0x3B,0x8B,0x35,0x1E,0x68,0x5F,0x2F,
  0x3E,0x00,0xB9,0x29,0x53,0xDA,0x6F,0x54,0x71,0x48,0x97,0xE1,0xBA,0x2E,0xD5,0xAE,
  0xE4,0x60,0x58,0x86,0xAD,0x27,0xE9,0xFB,0x0F,0x85,0xCF,0xA3,0xEC,0x66,0x2D,0x2A,
  0x72,0x66,0xC6,0x48,0x93,0x30,0x6B,0xAD,0x01,0x02,0x03,0x01,0x00,0x01,
};


string Decryptor::base64(const unsigned char *input, int length)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;
  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);
  string x( bptr->data, bptr->length-1);
  BIO_free_all(b64);
  return x;
}


unsigned char* Decryptor::unbase64(string input)
{
  BIO *b64, *bmem;
  size_t length = input.size();
  unsigned char *buffer = (unsigned char *)malloc(length);
  memset(buffer, 0, length);
  b64 = BIO_new(BIO_f_base64());
  //BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new_mem_buf(const_cast<char*>(input.c_str()), static_cast<int>(length));
  bmem = BIO_push(b64, bmem);
  BIO_read(bmem, buffer, static_cast<int>(input.size()));
  BIO_free_all(bmem);
  return buffer;
}


void LicenseValidator::valid()
{
  static const Keyword tag_customer("customer");
  static const Keyword tag_email("email");
  static const Keyword tag_valid_from("valid_from");
  static const Keyword tag_valid_till("valid_till");
  static const Keyword tag_signature("signature");
  if (!metadata)
  {
    metadata = MetaClass::registerClass<LicenseValidator>();
    const_cast<MetaClass*>(metadata)->addStringField<LicenseValidator>(
      tag_customer, &LicenseValidator::getCustomer, &LicenseValidator::setCustomer
      );
    const_cast<MetaClass*>(metadata)->addStringField<LicenseValidator>(
      tag_email, &LicenseValidator::getEmail, &LicenseValidator::setEmail
      );
    const_cast<MetaClass*>(metadata)->addStringField<LicenseValidator>(
      tag_valid_from, &LicenseValidator::getValidFrom, &LicenseValidator::setValidFrom
      );
    const_cast<MetaClass*>(metadata)->addStringField<LicenseValidator>(
      tag_valid_till, &LicenseValidator::getValidTill, &LicenseValidator::setValidTill
      );
    const_cast<MetaClass*>(metadata)->addStringField<LicenseValidator>(
      tag_signature, &LicenseValidator::getSignature, &LicenseValidator::setSignature
      );
  }

  // Parse the license file
  XMLInputFile(Environment::searchFile("license.xml")).parse(this, false);

  // Validate the fields
  Date now = Date::now();
  if (customer.empty() || email.empty() || now < valid_from_date
    || now > valid_till_date)
    throw RuntimeException("Invalid license file");

  // Build public key.
  RSA *rsa;
  const unsigned char *p = Decryptor::key;
  rsa = d2i_RSAPublicKey(NULL, &p, sizeof(Decryptor::key));
  if (!rsa) throw RuntimeException("Invalid license file");

  // Decode the signature from its base64 encoding
  unsigned char *sig_buf = Decryptor::unbase64(signature);

  // Intialize the signature
  EVP_MD_CTX ctx;
  EVP_PKEY *evpKey = 0;
  evpKey = EVP_PKEY_new();
  EVP_PKEY_set1_RSA( evpKey, rsa );
  EVP_VerifyInit(&ctx, EVP_sha1());

  // Add all data to be verified
  EVP_VerifyUpdate(&ctx, customer.c_str(), customer.size());
  EVP_VerifyUpdate(&ctx, email.c_str(), email.size());
  EVP_VerifyUpdate(&ctx, valid_from.c_str(), valid_from.size());
  EVP_VerifyUpdate(&ctx, valid_till.c_str(), valid_till.size());

  // Finalize the signature verification
  int err = EVP_VerifyFinal(&ctx, sig_buf, RSA_size(rsa), evpKey);
  EVP_PKEY_free (evpKey);
  free(sig_buf);
  if (err != 1) throw RuntimeException("Invalid license file");

  // Set a "secret" flag to determine we are running in enterprise mode or not.
  flags = Keyword::hash(customer.c_str());
}

} // End namespace frepple_enterprise::utils
} // End namespace frepple_enterprise
