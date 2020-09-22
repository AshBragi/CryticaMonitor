
/************************************************
 * CS_Utility.h
 * ============
 *
 * This is the header file for the CS_Utility Library
 *
 * Author:      Kerry
 * Version:     00.00.00
 * Comments:    This is the first cut
 ************************************************/
//
// Created by C. Kerry Nemovicher, Ph.D. on 6/23/20.
//

#ifndef CRYTICAMONITOR_CSL_UTILITIES_H
#define CRYTICAMONITOR_CSL_UTILITIES_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <limits.h>
#include "csl_constants.h"
//#include "CryticaMonitor.h"
//#include "csl_message.h"

/************************************************
 * #defines
 * ========
 * See csl_constants.h
 ************************************************/

/************************************************
 * Typedefs and other structure definitions
 * ========================================
 *
 * These are the typedefs and structure definitions required for the utility functions
 *
 ************************************************/

/************************************************
 * Crytica Byte Array Structure Definitions
 ************************************************/

/**** 1-Dimensional Array Structure *************/
typedef struct
{
    unsigned int        number_of_element_bytes;
    byte                *byte_array_element;
}CS_1d_byte_array;

/**** 2-Dimensional Array Structure *************/
typedef struct
{
    unsigned int        number_of_rows;
    CS_1d_byte_array    *array_row[];

} CS_2d_byte_array;

/**** 3-Dimensional Array Structure *************/
typedef struct
{
    unsigned int        number_of_tables;
    CS_2d_byte_array    *array_table[];

} CS_3d_byte_array;

/************************************************
 * Basic Utilities (Project Agnostic)
 * ===============
 *
 * These are utility functions that are useful, but are independent of THE Project
 *
 ************************************************/
char                *csl_Hash2String(byte *hash_in, short hash_length);
byte                *csl_String2Hash(byte *string_in, short hash_length);

double              csl_AssignProbeID(unsigned long long monitor_id, unsigned short device_index);

bool                csl_IsEven(int my_number);

byte                *csl_MacAddressExpand(byte *short_address);

char                *csl_Time2String(time_t in_time);

int                 csl_get_last_octet(char *ip_address_internal);

char                *csl_Byte2String(byte* in_byte, short size);


#ifndef DEPRECATED
int                 csl_ByteArrayCompare(const byte *array_one, unsigned int array_one_size,
                                         const byte *array_two, unsigned int array_two_size);
bool                csl_ByteCopyNFrom(byte *target_array, unsigned int target_length, unsigned int target_start,
                                      byte *source_array, unsigned int source_length, unsigned int source_start,
                                      unsigned int byte_to_copy);

int                 *parseMacFile(char *mac_address, char *command_line);

byte                *csl_String2Byte(char* in_string, short size);
// unsigned int        csl_charArray2UnsignedInt(char *in_array);

byte                *csl_UInt2Byte(unsigned int returnLength);

bool                csl_ZeroByteArray(byte *inArray, unsigned short size);

#endif

#ifndef DEPRECATED
/************************************************
 * Crytica Byte Array Utilities
 * ============================
 *
 * These are utility functions that manage the CS Byte Arrays used mostly in Crytica Messaging
 * CS Byte Arrays are all variable length, consisting of:
 *      unsigned int    array length
 *      byte   *array elements
 *
 * These can be embedded into each other to form 2-dimensional and even 3-dimensional arrays
 * The functions included here are for both array manipulation (e.g., copying an array)
 * as well as conversion to/from array <=> standard data types
 *
 ************************************************/


/************************************************
 * CS_1d_byte_array Functions
 ************************************************/
CS_1d_byte_array    *csl_String2ByteArray(char* in_string);
char*               csl_ByteArray2String(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_UChar2ByteArray(byte* in_array, unsigned int bytes);
byte*               csl_ByteArray2UChar(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_Int2ByteArray(int in_int);
int                 csl_ByteArray2Int(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_UInt2ByteArray(unsigned int in_int);
unsigned int        csl_ByteArray2UInt(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_Long2ByteArray(long long in_long);
long long           csl_ByteArray2Long(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_ULong2ByteArray(unsigned long long in_long);
unsigned long long   csl_ByteArray2ULong(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_Short2ByteArray(short in_short);
short               csl_ByteArray2Short(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_UShort2ByteArray(unsigned short in_short);
unsigned short      csl_ByteArray2UShort(CS_1d_byte_array *in_array);

CS_1d_byte_array    *csl_Time2ByteArray(time_t in_time);
time_t              csl_ByteArray2Time(CS_1d_byte_array *in_array);




/************************************************
 * CS_2d_byte_array Functions
 ************************************************/
CS_2d_byte_array    *csl_2dByteArrayAddRow(CS_2d_byte_array *out_array, CS_1d_byte_array *in_array);
CS_1d_byte_array    *csl_2dByteArrayRetrieveRow(CS_2d_byte_array *in_array, unsigned int out_row);
//CS_1d_byte_array    *csl_2dByteArrayRetrieveRow(CS_1d_byte_array *out_array, CS_2d_byte_array *in_array, unsigned int out_row);
CS_2d_byte_array    *csl_2dByteArrayCopy2dArray(CS_2d_byte_array *out_array, CS_2d_byte_array *in_array);
CS_2d_byte_array    *csl_2dByteArrayBuildFromTable(CS_2d_byte_array *out_array,
                                                   CS_1d_byte_array in_array[],
                                                   unsigned int in_rows);

/************************************************
 * CS_3d_byte_array Functions
 ************************************************/
CS_3d_byte_array    *csl_3dByteArrayAddTable(CS_3d_byte_array *out_array, CS_2d_byte_array *in_array);
CS_2d_byte_array    *csl_3dByteArrayRetrieveTable(CS_3d_byte_array *in_array, unsigned int out_row);

/********************************************************************************************************************/



/* Debug Functions */
//void    csl_printCS_Element(char *callingFunction, struct CS_arrayElement *inElement);
void    csl_printArray(char *callingFunction, char *inArray, unsigned int start, unsigned int end);
//void    csl_printStringArrays(char *callingFunction, char *inArray[], unsigned int start, unsigned int end);
void    csl_printStringArray(char *inArray[], unsigned int arrayLength);
void    csl_printByteStruct(char *callingFunction, CS_2d_byte_array *inStruct);


/* Serialize/Deserialize String Function Definitions */
char              *csl_Serialize2StringD(char *inArray[], unsigned int inArraySize, char delim);
int               csl_Deserialize2StringD(char *outArray[], byte *inBuffer, byte delim, int outArraySize);

/* Serialize/Deserialize Byte Array Function Definitions */
byte              *csl_serialize2ByteArrayFromStringArray(char* inArray[], unsigned int inArraySize);
byte              *csl_serialize2ByteArrayFromByteArray(CS_1d_byte_array *inStruct);
CS_1d_byte_array *csl_deserialize2ByteArrays(char *inBuffer);

/* String to Byte Array and Byte Array to String Utilities */
char              *csl_byteStruct2String(CS_1d_byte_array *inArray);
char              *csl_byteString2DString(char *inString);
CS_1d_byte_array  *csl_string2ByteStruct(char *inString);
CS_1d_byte_array  *csl_serializedArray2ByteStruct(char *inArray);

/**** Utility Library ***************************/
char              *csl_MacAddressGetLocal(char *network_interface_name);
#endif

#endif //CRYTICAMONITOR_CSL_UTILITIES_H
