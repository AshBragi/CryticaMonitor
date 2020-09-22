//
// Created by C. Kerry Nemovicher, Ph.D. on 6/19/20.
//

#ifndef CRYTICAMONITOR_CSL_CRYPTO_H
#define CRYTICAMONITOR_CSL_CRYPTO_H

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include "openssl/sha.h"
#include "blake2.h"         //sudo apt-get install libb2-dev

#include "csl_constants.h"


//Global Defines




#define SYM_KEY_SIZE            5012


//Global Variables


//Dependency Prototypes (debugging)
void AllLink();
void HashLink();
void HashShowCase();
void ENCShowCase();
void MACShowCase();


//Encryption Prototypes:
void EncTest();
void ENChandler(int* key, int flow_select, char *item, int debug_verbosity);
void AESmoduleENC(int* key, char* item, int debug_verbosity);
void AESmoduleDEC(int* key, char* item, int debug_verbosity);

//Hash Modules
void HashTest();
unsigned char* HashHandler(int flow_select, char *fp, int debug_verbosity);
char* Sha256Module(char *fp, int debug_verbosity);
int MD5Module(char *fp, char* hash, int debug_verbosity);
int MD5CharModule(char* fp, char* hash);
char* Blake2Module(char *fp, int debug_verbosity);
int convertToHash(const unsigned char *object, char* hash);

//Key prototype:
int KeyTest();
char* KeyHandler(int flow_select, char *item, int debug_verbosity);
int AESKeyGenerator();
int AESEncKey();
int AESDecKey();

#endif //CRYTICAMONITOR_CSL_CRYPTO_H
