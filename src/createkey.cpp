/***************************************************************************
 *                                                                         *
 * Copyright (C) 2012-2013 by Johan De Taeye, frePPLe bvba                 *
 *                                                                         *
 * You should never have received this file!                               *
 *                                                                         *
 ***************************************************************************/

#include "frepple/utils.h"

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

using namespace frepple;
using namespace frepple::utils;


/** This program creates a RSA key pair and writes out the C-code to restore
  * the pair in a memory structure.
  * You'll normally run this program only once, and then paste the related
  * code into your encryption/decryption code.
  */
int main(int argc, char* argv[])
{
  // Temporary variables
	int len;
	unsigned char buf[4096];
  unsigned char *p;

  // Create an RSA key pair
	RSA *rsa = RSA_generate_key(2048,RSA_F4,NULL,NULL);

	// Echo the public key from a memory buffer.
  p = buf;
	len = i2d_RSAPublicKey(rsa,&p);
  printf ("unsigned char public_key[%d]={", len);
  for (int y=0; y<len; y++)
	{
	  if (y%16 == 0) printf("\n");
	  printf("0x%02X,",buf[y]);
	}
	printf("\n};\n");

  // Echo the public key from a memory buffer.
  p = buf;
	len = i2d_RSAPrivateKey(rsa,&p);
  printf ("unsigned char private_key[%d]={", len);
	for (int y=0; y<len; y++)
	{
	  if (y%16 == 0) printf("\n");
	  printf("0x%02X,",buf[y]);
	}
	printf("\n};\n");

  return true;
}

