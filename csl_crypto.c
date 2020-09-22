//
// Created by kerry on 8/3/20.
//

//#include <sys/types.h>
#include <unistd.h>
#include "CryticaMonitor.h"
#include "csl_crypto.h"


/****************************************************************
 *      Prototype: void ENCTest(void)
 *
 *      Name:       ENCTest (Encryption test)
 *
 *      Purpose:    To show that .h and .c of the encryption modules are
 *                  loaded.
 *
 *      Parameters: None
 *
 *      Returns:
 *          Success - void
 *          Failure - void
 *          (internal display)
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       This is kept in to make sure the build is
 *                      connected.
 *
 ****************************************************************/
void EncTest()
{
    printf("EncryptionLibrary.c, Linked to .h file calling in...\n");
}


/****************************************************************
 *      Prototype:  void ENChandler(int flow_select, char *item, int debug_verbosity)   //todo change the type
 *
 *      Name:       ENChandler
 *
 *      Purpose:    Handler module that relays the information of
 *                  sub modules back to the caller.
 *
 *      Parameters: flow_select:        choice for switch statement
 *                           1 - AES encryption sub module
 *                           2 - N/A
 *                  item:               item to hand to enc or dec
 *                  debug_verbosity:    sets verbose output
 *
 *      Returns:
 *          Success - will return the enc or dec key
 *          Failure - returns negative number
 *          (internal display)
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       Handler will need to return a byte
 *                      array of the available enc or dec
 *                      string.
 *
 ****************************************************************/
void ENChandler(int* key, int flow_select, char *item, int debug_verbosity)
{
    switch(flow_select)
    {
        case 1: AESKeyGenerator();
            //       case 2: AESmodule(key, item, debug_verbosity);
            //           break;

        case 2: AESmoduleENC(key, item, debug_verbosity);
        default: printf("Error: Encryption handler was given improper flow choice.");

    }
}

/****************************************************************
 *      Prototype:  void AESmodule(char* item, int debug_verbosity);
 *
 *      Name:       AES encryption module
 *
 *      Purpose:    To encrypt or decrypt data and return the value
 *
 *      Parameters: item:               item to encrypt, decrypt
 *                  debug_verbosity:    sets verbose output
 *
 *      Returns:
 *          Success - the ENC or DEC string in a byte array
 *          Failure - returns negative number
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       This is set with one key for now
 *                      Does both enc and dec.
 *                      THe const above is the key.
 *
 ****************************************************************/
//Symmetric Encryption///////////////////////////////////////////////////////////

/*      Name:       AESmodule + TMP KEY HOLDER
 *      Date:       7/9/19
 *      Purpose:    Hand item, returns enc string and de-enc string
 *      Author:     T. Hudson
 *      Version:    1.0
 */
static const unsigned char aes_key[] =      //to be moved at a later date to an independent function
        {
                0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        };

void AESmoduleENC(int* key, char* item, int debug_verbosity)
{
    unsigned char *text = calloc(strlen(item), sizeof(unsigned char));
    memcpy(text, item, strlen(item));
    unsigned char enc_out[80];  //PROBLEM WITH SIZE OF ITEM
    unsigned char dec_out[80];

    AES_KEY enc_key, dec_key;

    AES_set_encrypt_key(aes_key, 128, &enc_key);
    AES_encrypt(text, enc_out, &enc_key);

    AES_set_decrypt_key(aes_key,128,&dec_key);
    AES_decrypt(enc_out, dec_out, &dec_key);

    int i;

    printf("original:\t");
    for(i=0;*(text+i)!=0x00;i++)
        printf("%X ",*(text+i));
    printf("\nencrypted:\t");
    for(i=0;*(enc_out+i)!=0x00;i++)
        printf("%X ",*(enc_out+i));
    printf("\ndecrypted:\t");
    for(i=0;*(dec_out+i)!=0x00;i++)
        printf("%X ",*(dec_out+i));
    printf("\n");

    if(debug_verbosity == 1)
    {
        // todo - we NEED something here!
    }
}

void AESmoduleDEC(int* key, char* item, int debug_verbosity)
{
    unsigned char *text = calloc(strlen(item), sizeof(unsigned char));
    memcpy(text, item, strlen(item));
    unsigned char enc_out[80];  //PROBLEM WITH SIZE OF ITEM
    unsigned char dec_out[80];

    AES_KEY enc_key, dec_key;


    AES_set_decrypt_key(aes_key, 128, &dec_key);
    AES_decrypt(enc_out, dec_out, &dec_key);

    int i;

    for (i = 0; *(dec_out + i) != 0x00; i++) {
        printf("%X ", *(dec_out + i));
        printf("\n");
    }
}
//Asymmetric Encryption///////////////////////////////////////////////////////////
/****************************************************************
 *      Prototype:
 *
 *      Name:
 *
 *      Purpose:
 *
 *      Parameters:
 *
 *
 *      Returns:
 *          Success -
 *          Failure -
 *
 *      Version Number: 00.00.00
 *      Author:
 *      Comments:
 *
 ****************************************************************/

//Post-Quantum safe///////////////////////////////////////////////////////////////
/****************************************************************
 *      Prototype:
 *
 *      Name:
 *
 *      Purpose:
 *
 *      Parameters:
 *
 *
 *      Returns:
 *          Success -
 *          Failure -
 *
 *      Version Number: 00.00.00
 *      Author:
 *      Comments:
 *
 ****************************************************************/

/****************************************************************************************************************/

/****************************************************************
 *      Prototype:  void HashTest();
 *
 *      Name:       HashTest
 *
 *      Purpose:    Tests to see if hash .c and .h are connected
 *
 *      Parameters: none
 *
 *      Returns:
 *          Success - void
 *          Failure - void
 *          (see internal display)
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:      Test to see if the .h and .c file are connected
 *
 ****************************************************************/
void HashTest()
{
    printf("HashLibrary.c, Linked to .h file calling in...\n");

}

/**
    @brief
    @param paramName[in] :
    @return
**/
/****************************************************************
 *      Prototype:  char* HashHandler(int flow_select, char *fp, int debug_verbosity);
 *
 *      Name:       Hash Handler
 *
 *      Purpose:    Called on with options to return a specific hash
 *
 *      Parameters: flow_select:        select which type of hash that you want.
 *                                          1 - SHA256  (buggy)
 *                                          2 - MD5     (stable)
 *                                          3 - Blake2  (not implemented)
 *                  *fp:                File pointer
 *                  debug_verbosity:    sets level of debug output
 *
 *      Returns:
 *          Success - Valid hash string in a byte container.
 *          Failure - returns garbage * for now, later and error code.
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       This will be the module you call on when you want a hash
 *
 ****************************************************************/
unsigned char* HashHandler(int flow_select, char *fp, int debug_verbosity)
{
    char *Hand_Back = "Hand Back is empty!";
    switch(flow_select)
    {
        case 1: Hand_Back = Sha256Module(fp, debug_verbosity);
            break;
            //case 2: Hand_Back = MD5Module(fp, debug_verbosity);
            break;
        case 3: Hand_Back = Blake2Module(fp, debug_verbosity);
            break;
        default: printf("Error: Hash handler was given improper flow choice, check your function call!.\n");

    }
    unsigned char *return_array = calloc(strlen(Hand_Back), sizeof(unsigned char));
    memset(return_array, NULL_BINARY, strlen(Hand_Back));
    memcpy(return_array,Hand_Back, strlen(Hand_Back));
    return return_array;
}


/**
    @brief
    @param paramName[in] :
    @return
**/
/****************************************************************
 *      Prototype:  char* Sha256Module(char *fp, int debug_verbosity);
 *
 *      Name:       SHA 256 Module
 *
 *      Purpose:    Called by handler to perform the hash with SHA256
 *
 *      Parameters: *fp:                File Pointer
 *                  debug_verbosity:    sets debug output
 *
 *      Returns:
 *          Success - Valid hash string in a byte container, return to handler
 *          Failure - returns garbage * for now, later and error code.
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       This will be the module you call on when you want a hash
 *
 ****************************************************************/
char* Sha256Module(char *fp, int debug_verbosity)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)fp, strlen(fp), (unsigned char*)&digest);
    char mdString[SHA256_DIGEST_LENGTH*2+1];

    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(&mdString[i * 2], "%02x", (unsigned int) digest[i]);
    }

    if(debug_verbosity == 1)
    {
        printf("SHA256 digest: %s", mdString);
        printf("\t\t\t\t\t :%s\n", fp);
    }

    char *buffer = calloc(260, sizeof(char));        //to be optimized at a later date
    strcpy(buffer, mdString);

    return buffer;
}

/**
    @brief
    @param paramName[in] :
    @return
**/
/****************************************************************
 *      Prototype:  int* MD5Module(char *fp, int debug_verbosity);
 *
 *      Status: Functional, but needs to be cleaned up
 *
 *      Name:       MD5 Module
 *
 *      Purpose:    Called by handler to perform the hash with MD5
 *
 *      Parameters: *fp:                File Pointer
 *                  debug_verbosity:    sets debug output
 *
 *      Returns:
 *          Success - Valid hash string in an int container.
 *          Failure - error code depending on the fail type
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       This will be the module you call on when you want a hash
 *
 ****************************************************************/
int MD5Module(char *fp, char* hash, int debug_verbosity)
{
    //variables
    unsigned char object[MD5_DIGEST_LENGTH];
    FILE *inFile = fopen (fp, "rb");

    //handoff to context function
    MD5_CTX mdContext;
    unsigned int bytes;
    int result = 0;
    unsigned char data[1024];

    //File related error checking
    if (inFile == NULL)
    {
        printf("%s can't be opened. error = %s\n", fp, strerror(errno));
        return -1;
    }

    MD5_Init (&mdContext);

    while ((bytes = fread (data, 1, 1024, inFile)) != 0)
    {
        MD5_Update (&mdContext, data, bytes);
    }

    MD5_Final (object, &mdContext); //MD5 Context is the actual hash

    //Print the formatted version to the screen

    //object[16] - hash

    result = convertToHash(object, hash);

    if(result == -1)
    {
        printf("Failed to convert hash");
        return -1;
    }

    if(debug_verbosity)
    {
        printf("\nMD5 (MODULE HASH): %s", hash);
    }

    //printf("FP: %s  -- ", fp);
    //printf("Hash: %s\n", hash);

    /*
    if(debug_verbosity == 1)
    {
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        {
            buffer[i] = object[i];

            printf("%02x", object[i]);
        }
        printf("\t\t:%s\n", fp);

    */

    fclose(inFile);


    return 0;
}

int MD5CharModule(char* fp, char* hash)
{
    //variables
    unsigned char object[MD5_DIGEST_LENGTH];

    //handoff to context function
    MD5_CTX mdContext;
    int result = 0;
    //File related error checking
    if (fp == NULL)
    {
        return CS_ERROR;
    }


    MD5_Init (&mdContext);
    for( int i = 0; i < strlen(fp); i++ ){
        MD5_Update (&mdContext, fp + i, sizeof(char));
    }
    MD5_Final (object, &mdContext); //MD5 Context is the actual hash
    for(int i = 0; i < 16; ++i){
        sprintf(&hash[i*2], "%02x", (unsigned int)object[i]);
    }
    return CS_SUCCESS;
}


/**
    @brief
    @param paramName[in] :
    @return
**/
/****************************************************************
 *      Prototype:  char* Blake2Module(char *fp, int debug_verbosity)
 *
 *      Name:       Blake v.2 module
 *
 *      Purpose:    Apply a blake hash to an item
 *
 *      Parameters: *fp:                File Pointer
 *                  debug_verbosity:    sets debug output
 *
 *      Returns:
 *          Success - Valid hash string in a byte container.
 *          Failure - returns garbage * for now, later and error code.
 *
 *      Version Number: 00.00.00
 *      Author:         T. Hudson
 *      Comments:       Not implemented yet
 *
 ****************************************************************/
char* Blake2Module(char *fp, int debug_verbosity)
{

    if(debug_verbosity == 1)
    {
        printf("Blake (MODULE) is not available in this version\n");
    }

    return fp;
}



int convertToHash(const unsigned char *object, char *hash)
{
    if(object == NULL || hash == NULL)
    {
        return -1;
    }

    const char * hex = "0123456789ABCDEF";

    for(int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        hash[2*i] = hex[(object[i]) >> 4 & 0xF];
        hash[(2*i) + 1] = hex[object[i] & 0xF];
    }

    return 0;
}


/**
    @brief
    @param paramName[in] :
    @return
**/
/*      Name:       KeyTest
 *      Date:       7/15/19
 *      Purpose:    To test key modules
 *      Author:     T. Hudson
 *      Version:    0.0.1 (building)
 *      Bugs:
 */
int KeyTest()
{
    printf("KeyLibrary.c, Linked to .h file calling in...\n");
    return 0;
}

/**
    @brief
    @param paramName[in] :
    @return
**/
/*      Name:       KeyHandler
 *      Date:       7/15/19
 *      Purpose:    To handle key generation functions
 *      Author:     T. Hudson
 *      Version:    0.0.1 (building)
 *      Bugs:
 */
char* KeyHandler(int flow_select, char *item, int debug_verbosity)
{
    char *Hand_Back = "Hand Back is empty!";
    switch(flow_select)
    {
        //Generate a key
        case 1: AESKeyGenerator();
            break;
            //ENC item
        case 2:
            break;
            //DEC item
        case 3:
            break;
        default: printf("Error: Hash handler was given improper flow choice, check your function call!.\n");
    }
    return Hand_Back;
}


//AES related

/*      Name:       AESKeyGenerator
 *      Date:       7/15/19
 *      Purpose:    To generate keys for AES encryption/decryption
 *      Author:     T. Hudson
 *      Version:    0.0.1 (building)
 *      Bugs:
 *      Note:       Will have a return type of the key to the original caller
 *      TEMP:       need to gen key that is hex
 */

/**
    @brief
    @param paramName[in] :
    @return
**/
int AESKeyGenerator()
{
    int length = 32;                //generated key needs 32 numbers
    char str[] = "0123456789ABCDEF";
    char holder[32];

    /* Seed number for rand() */
    srand((unsigned int) time(0) + getpid());

    for(int i = 0; i < length; i++)
    {
        holder[i] = putchar(str[rand() % 16]);
        srand(rand());
    }
    printf("\n");
    printf("Key: %s \n", holder);

    return EXIT_SUCCESS;

    return 0;
}

/**
    @brief
    @param paramName[in] :
    @return
**/
int AESEncKey()
{
    return 0;
}

/**
    @brief
    @param paramName[in] :
    @return
**/
int AESDecKey()
{
    return 0;
}


