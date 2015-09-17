/***************************************************************************
 *                                                                         *
 * Copyright (C) 2015 by frePPLe bvba                                      *
 *                                                                         *
 * All information contained herein is, and remains the property of        *
 * frePPLe.                                                                *
 * You are allowed to use and modify the source code, as long as the       *
 * software is used within your company.                                   *
 * You are not allowed to distribute the software, either in the form of   *
 * source code or in the form of compiled binaries.                        *
 *                                                                         *
 ***************************************************************************/

#include "webserver.h"

#include <openssl/sha.h>
#include <openssl/evp.h>

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

namespace module_webserver
{

string WebToken::secret;
map<string, WebToken::signFunc> WebToken::algorithms;
const string WebToken::base64_chars =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";


void WebToken::initialize()
{
  // Initialize the list of algorithms
  algorithms["HS256"] = &WebToken::signHS256;
  algorithms["HS384"] = &WebToken::signHS384;
  algorithms["HS512"] = &WebToken::signHS512;

  /*
  WebToken t;  // TODO create unit test for JWT
  t.setSecret("Holy crows");
  t.setSub("johan");
  t.setExp(Date::now());
  t.setAlg(string("HS512"));
  t.setAud("*");
  logger << t.getToken() << endl;
  */
}


void WebToken::setAlg(string& s)
{
  if (!algorithms.count(s))
    throw DataException("Unsupported signing algorithm");
  alg = s;
  claimschanged = true;
  tokenchanged = true;
}


void WebToken::encode()
{
  stringstream result;

  // Build and encode the header
  stringstream strm;
  strm << "{\"alg\": \"" << alg << "\"}";
  string tmp = strm.str();
  base64_encode(
    result,
    reinterpret_cast<const unsigned char*>(tmp.c_str()),
    static_cast<unsigned int>(tmp.length())
    );
  result << ".";

  // Build and encode the payload
  strm.str("");
  strm << "{\"sub\":\"" << sub << "\","
    << "\"exp\":" << exp.getTicks() << ","
    << "\"aud\":\"" << aud << "\"}";
  tmp = strm.str();
  base64_encode(result, reinterpret_cast<const unsigned char*>(tmp.c_str()), static_cast<unsigned int>(tmp.length()));

  // Build and encode the signature for the header and payload
  tmp = result.str();
  stringstream strm2;
  (this->*algorithms[alg])(strm2, tmp);
  result << "." << strm2.str();

  // Store the result
  token = result.str();
}


void WebToken::decode() // TODO
{
}


void WebToken::signHS256(stringstream& out, const string& val)
{
  unsigned int len = SHA256_DIGEST_LENGTH;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, secret.c_str(), static_cast<int>(secret.length()), EVP_sha256(), NULL);
  HMAC_Update(&ctx, (unsigned char*)(val.c_str()), strlen(val.c_str()));
  HMAC_Final(&ctx, hash, &len);
  HMAC_CTX_cleanup(&ctx);
  base64_encode(out, hash, len);
}


void WebToken::signHS384(stringstream& out, const string& val)
{
  unsigned int len = SHA384_DIGEST_LENGTH;
  unsigned char hash[SHA384_DIGEST_LENGTH];
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, secret.c_str(), static_cast<int>(secret.length()), EVP_sha384(), NULL);
  HMAC_Update(&ctx, (unsigned char*)(val.c_str()), strlen(val.c_str()));
  HMAC_Final(&ctx, hash, &len);
  HMAC_CTX_cleanup(&ctx);
  base64_encode(out, hash, len);
}


void WebToken::signHS512(stringstream& out, const string& val)
{
  unsigned int len = SHA512_DIGEST_LENGTH;
  unsigned char hash[SHA512_DIGEST_LENGTH];
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, secret.c_str(), static_cast<int>(secret.length()), EVP_sha512(), NULL);
  HMAC_Update(&ctx, (unsigned char*)(val.c_str()), strlen(val.c_str()));
  HMAC_Final(&ctx, hash, &len);
  HMAC_CTX_cleanup(&ctx);
  base64_encode(out, hash, len);
}


void WebToken::base64_encode(
  stringstream& output, unsigned char const* bytes_to_encode, unsigned int in_len
  )
{
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--)
  {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3)
    {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      for (i = 0; i < 4 ; i++)
        output << base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; j < i + 1; j++)
      output << base64_chars[char_array_4[j]];

    // Uncomment these lines if you need padding
    // while((i++ < 3))
    //  ret += '=';
  }
}


void WebToken::base64_decode(
  stringstream& output, string const& encoded_string
  )
{
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  // Original version ignores the padding characters:
  // while (in_len-- && encoded_string[in_] != '=' && is_base64(encoded_string[in_]))
  while (in_len-- && is_base64(encoded_string[in_]))
  {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i == 4)
    {
      for (i = 0; i < 4; i++)
        char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        output << char_array_3[i];
      i = 0;
    }
  }

  if (i)
  {
    for (j = i; j < 4; j++)
      char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; j < i - 1; j++)
      output << char_array_3[j];
  }
}

}       // end namespace
