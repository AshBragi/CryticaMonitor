//
// Created by C. Kerry Nemovicher, Ph.D. on 6/23/20.
//

#include "csl_constants.h"
#include "csl_utilities.h"

/************************************************
 * Basic Utilities (Project Agnostic)
 * ===============
 *
 * These are utility functions that are useful, but are independent of THE Project
 *
 ************************************************/

#ifndef DEPRECATED
/************************************************
 * int csl_serializedArrayCompare ()
 *  @param
 *          byte            *arrayOne
 *          unsigned int    arrayOneSize
 *          byte            *arrayTwo
 *          unsigned int    arrayTwoSize
 *
 * @brief   This is a CS Library Function. Its purpose is to compare two byte arrays
 *
 * @author  Kerry
 *
 * @returns
 *			Success -	CS_SUCCESS      - the arrays are identical
 *			Failure -	CS_BAD_COMPARE  - the arrays are not identical sizes
 *			Failure -   A positive int value indicating the ordinal of the byte where the first mismatch occurred
 ************************************************/
int   csl_ByteArrayCompare(const byte *arrayOne, unsigned int arrayOneSize, const byte *arrayTwo, unsigned int arrayTwoSize)
{
    if (arrayOneSize != arrayTwoSize)
        return (short) CS_BAD_COMPARE;
    for (int i = 0; i < arrayOneSize; i++)
    {
        if (arrayOne[i] != arrayTwo[i])
            return (short) CS_BAD_COMPARE;
    }

    return CS_SUCCESS;
}


/************************************************
 * bool csl_ByteCopyNFrom()
 *
 *  @param  byte*           target_array    -the array into which the bytes are to be copied
 *          unsigned int    target_length   -the size of the target array
 *          unsigned int    target_start    -the byte index into which the copying will start
 *          byte*           source_array    -the array from which the bytes are to be copied
 *          unsigned int    source_length   -the size of the source array
 *          unsigned int    source_start    -the byte index from which the copying will start
 *          unsigned int    bytes_to_copy   -the number of bytes to copy from the source to the target
 *
 *  @brief  Copies "N" Bytes from one byte array to another
 *
 *  @note:  If either the source or the target is too small, the operation continues, but
 *          but truncates the number of bytes copied so as not to overrun either the source or the target
 *
 *  @return true    if the operation is successful
 *          false   if the operation encounters a problem
 ************************************************/
bool   csl_ByteCopyNFrom(byte *target_array, unsigned int target_length, unsigned int target_start,
                         byte *source_array, unsigned int source_length, unsigned int source_start,
                         unsigned int bytes_to_copy)
{
    bool return_flag = true;

    if ((target_start + bytes_to_copy) > target_length)
    {
        // todo - Issue error message
        bytes_to_copy   = target_length - target_start;
        return_flag = false;
    }
    if ((source_start + bytes_to_copy > source_length))
    {
        // todo - Issue error message
        bytes_to_copy   = source_length - source_start;
        return_flag = false;
    }

    memcpy(&target_array[target_start], &source_array[source_start], bytes_to_copy);
    return return_flag;
}
#endif

/************************************************
 * Specific Data Type Conversion Functions
 * ============================================
 *
 ************************************************/

char *csl_Byte2String(byte* in_byte, short size)
{
    char *return_string = calloc(size + 1, sizeof(char));
    memcpy(return_string, in_byte, size);
    return_string[size] = END_OF_STRING;
    return return_string;
}

//char *csl_Time2String(char *out_string, time_t in_time)
char *csl_Time2String(time_t in_time)
{
    char *out_string = calloc(SIZE_OF_TIME, sizeof(char));
    struct tm *ts;
    ts = localtime(&in_time);
//    strftime(out_string, sizeof(out_string), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    strftime(out_string, SIZE_OF_TIME, "%Y-%m-%d %H:%M:%S", ts); // YYYY-MM-DD HH:MM:SS
    return out_string;
}

int csl_get_last_octet(char *ip_address_internal)
{
    int return_value    = 0;
    char *final_octet = calloc(4,sizeof(char));
    int dot_ctr         = 0;
    int string_length   = (int) strlen(ip_address_internal);
    for (int i = 0, j = 0; i < string_length; i++)
    {
        if (dot_ctr < 3)
        {
            if (ip_address_internal[i] == DOT)
            {
                dot_ctr++;
            }
        }
        else
        {
            final_octet[j] = ip_address_internal[i];
            j++;
        }
    }
    return_value = (int) strtol(final_octet,NULL, BASE_TEN);
    free (final_octet);
    return return_value;
}

double csl_AssignProbeID(unsigned long long monitor_id, unsigned short device_index)
{
    double return_probe_id  = (double)(monitor_id * MAX_DEVICES) + device_index + 1;
    return_probe_id         = return_probe_id / MAX_DEVICES;
    return return_probe_id;
}

/************************************************
 * bool csl_IsEven()
 * =================
 *  @param
 *          int my_number
 *
 *  @brief  Determines if an Int is even or odd
 *
 *  @return true if my_number is even
 *          false if my number is odd
 ************************************************/
bool csl_IsEven(int my_number)
{
    if ((my_number/2)*2 == my_number)
        return true;
    return false;
}

byte *csl_MacAddressExpand(byte *short_address)
{
    byte *long_address = calloc(SIZE_MAC_ADDRESS, sizeof(byte));
    memset(long_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    char tmp_address [SIZE_MAC_ADDRESS + 1];
    memset(tmp_address, NULL_BINARY, SIZE_MAC_ADDRESS + 1);
    sprintf(tmp_address,"%02x:%02x:%02x:%02x:%02x:%02x",
            short_address[0],
            short_address[1],
            short_address[2],
            short_address[3],
            short_address[4],
            short_address[5]);
    memcpy(long_address, tmp_address, SIZE_MAC_ADDRESS);
    return long_address;
}

char *csl_Hash2String(byte *hash_in, short hash_length)
{
    char *hash_string = calloc(hash_length + 1, sizeof(char));
    memcpy(hash_string, hash_in, hash_length);
    hash_string[hash_length]  = END_OF_STRING;
    return hash_string;
}

byte *csl_String2Hash(byte *string_in, short hash_length)
{
    byte *hash_out  = calloc(hash_length, sizeof(byte));
    memcpy(hash_out, string_in, hash_length);
    return hash_out;
}


/************************************************
 * Specific Data Type Conversion Functions
 * ============================================
 * Conversions between various CS data types ... and also
 * between CS data types and standard data types
 ************************************************/

#ifndef DEPRECATED
/************************************************
 * CS_1d_byte_array Functions
 ************************************************/

/**** char/byte Conversions *********************/
CS_1d_byte_array *csl_String2ByteArray(char* in_string)
{
    CS_1d_byte_array *out_array = calloc (1, sizeof  (CS_1d_byte_array));
    out_array->number_of_element_bytes  = strlen(in_string);
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, in_string, out_array->number_of_element_bytes);
    return out_array;
}

char*   csl_ByteArray2String(CS_1d_byte_array *in_array)
{
    char *out_string = calloc(in_array->number_of_element_bytes + 1,sizeof(char));
    memcpy(out_string, in_array->byte_array_element, in_array->number_of_element_bytes);
    out_string[in_array->number_of_element_bytes] = END_OF_STRING;

    return out_string;
}

CS_1d_byte_array *csl_UChar2ByteArray(byte* in_array, unsigned int bytes)
{
//    CS_1d_byte_array *out_array = calloc(1, sizeof(CS_1d_byte_array));
    CS_1d_byte_array *out_array = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = bytes;
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, in_array, bytes);
    return out_array;
}


byte*   csl_ByteArray2UChar(CS_1d_byte_array *in_array)
{
    byte *out_array = calloc(in_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array, in_array->byte_array_element, in_array->number_of_element_bytes);

    return out_array;
}

/**** int Conversions ***************************/
CS_1d_byte_array *csl_Int2ByteArray(int in_int)
{
//    CS_1d_byte_array *out_array = calloc(1, sizeof(CS_1d_byte_array));
    CS_1d_byte_array *out_array = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = sizeof(int);
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_int, out_array->number_of_element_bytes);
    return out_array;
}

int csl_ByteArray2Int(CS_1d_byte_array *in_array)
{
    int out_int = 0;
    memcpy(&out_int, in_array->byte_array_element, sizeof(int));
    return out_int;
}

/**** unsigned int Conversions ******************/
CS_1d_byte_array    *csl_UInt2ByteArray(unsigned int in_int)
{
    CS_1d_byte_array *out_array = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes = sizeof(unsigned int);
    out_array->byte_array_element      = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_int, out_array->number_of_element_bytes);
//        unsigned int test = csl_ByteArray2UInt(out_array);          // debug code
    return out_array;
}

unsigned int        csl_ByteArray2UInt(CS_1d_byte_array *in_array)
{
    unsigned int out_int = 0;
    memcpy(&out_int, in_array->byte_array_element, sizeof(unsigned int));
    return out_int;
}

/**** long long Conversions *********************/
CS_1d_byte_array    *csl_Long2ByteArray(long long in_long)
{
    CS_1d_byte_array *out_array = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes = sizeof(long long);
    out_array->byte_array_element      = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_long, out_array->number_of_element_bytes);
    return out_array;
}

long long           csl_ByteArray2Long(CS_1d_byte_array *in_array)
{
    //long long out_long = 0L;
    char *tmp_char      = calloc(in_array->number_of_element_bytes + 1, sizeof(char));
    memcpy(tmp_char, in_array->byte_array_element, in_array->number_of_element_bytes);
    tmp_char[in_array->number_of_element_bytes] = END_OF_STRING;
    return strtoll(tmp_char, NULL, BASE_TEN);
}

CS_1d_byte_array    *csl_ULong2ByteArray(unsigned long long in_long)
{
    CS_1d_byte_array *out_array = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes = sizeof(unsigned long long);
    out_array->byte_array_element      = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_long, out_array->number_of_element_bytes);
    return out_array;
}

unsigned long long   csl_ByteArray2ULong(CS_1d_byte_array *in_array)
{
    unsigned long long out_long = 0L;

    char *byte_address  = (char*) in_array->byte_array_element; // this only for clarity
    out_long = strtoull(byte_address,NULL, BASE_TEN);

    return out_long;
}

/**** short Conversions *************************/
CS_1d_byte_array    *csl_Short2ByteArray(short in_short)
{
    CS_1d_byte_array *out_array         = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = sizeof(short);
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_short, out_array->number_of_element_bytes);
    return out_array;
}

short               csl_ByteArray2Short(CS_1d_byte_array *in_array)
{
    short out_short = 0;
    memcpy(&out_short, in_array->byte_array_element, sizeof(short));
    return out_short;
}

/**** unsigned short Conversions ****************/
CS_1d_byte_array    *csl_UShort2ByteArray(unsigned short in_short)
{
    CS_1d_byte_array *out_array         = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = sizeof(unsigned short);
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_short, out_array->number_of_element_bytes);
    return out_array;
}

unsigned short      csl_ByteArray2UShort(CS_1d_byte_array *in_array)
{
    unsigned short out_short = 0;
    memcpy(&out_short, in_array->byte_array_element, sizeof(unsigned short));
    return out_short;
};

/**** time Conversions **************************/
CS_1d_byte_array    *csl_Time2ByteArray(time_t in_time)
{
    CS_1d_byte_array *out_array         = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = sizeof(time_t);
    out_array->byte_array_element       = calloc(out_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->byte_array_element, &in_time, out_array->number_of_element_bytes);
    return out_array;
}

time_t              csl_ByteArray2Time(CS_1d_byte_array *in_array)
{
    time_t out_time = 0;
    memcpy(&out_time, in_array->byte_array_element, sizeof(time_t));
    return out_time;
}


/************************************************
 * CS_2d_byte_array Functions
 ************************************************/

CS_2d_byte_array    *csl_2dByteArrayAddRow(CS_2d_byte_array *old_array, CS_1d_byte_array *in_array)
{
    CS_2d_byte_array *out_array     = calloc(sizeof(CS_2d_byte_array), sizeof(byte));
    out_array->number_of_rows       = old_array->number_of_rows + 1;
    for (unsigned int i = 0; i <old_array->number_of_rows; i++)
    {
        out_array->array_row[i]                            = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
        out_array->array_row[i]->number_of_element_bytes   = old_array->array_row[i]->number_of_element_bytes;
        out_array->array_row[i]->byte_array_element        = calloc(out_array->array_row[i]->number_of_element_bytes,sizeof(byte));
        memcpy(out_array->array_row[i]->byte_array_element, old_array->array_row[i]->byte_array_element, in_array->number_of_element_bytes);
    }
    out_array->array_row[old_array->number_of_rows]                            = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->array_row[old_array->number_of_rows]->number_of_element_bytes   = in_array->number_of_element_bytes;
    out_array->array_row[old_array->number_of_rows]->byte_array_element        = calloc(in_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->array_row[old_array->number_of_rows]->byte_array_element, in_array->byte_array_element, in_array->number_of_element_bytes);

    free (old_array);
    return out_array;
}

CS_2d_byte_array    *csl_2dByteArrayAddRow(CS_2d_byte_array *out_array, CS_1d_byte_array *in_array)
{
    unsigned int new_row                                     = out_array->number_of_rows;

    out_array->array_row[new_row]                            = calloc(1,sizeof(CS_1d_byte_array));
    out_array->array_row[new_row]->number_of_element_bytes   = in_array->number_of_element_bytes;
    out_array->array_row[new_row]->byte_array_element        = calloc(in_array->number_of_element_bytes,sizeof(byte));
    memcpy(out_array->array_row[new_row]->byte_array_element, in_array->byte_array_element, in_array->number_of_element_bytes);
    out_array->number_of_rows                                = new_row + 1;

    return out_array;

/*****************************************
    unsigned test_int0  = 0;
    unsigned test_short0    =0;
    char *test_string0;
    unsigned test_int1   = 0;
    unsigned test_short1    =0;
    char *test_string1;
    switch(new_row)
    {
        case SCAN_ORDINAL_SCAN_ID:
        case SCAN_ORDINAL_PROBE_ID:
        case SCAN_ORDINAL_SCAN_VALUE:
        case SCAN_ORDINAL_ELEMENT_NAME_HASH:
            test_int0 = csl_ByteArray2UInt(out_array->array_row[new_row]);
            test_int1 = csl_ByteArray2UInt(in_array);
            break;

        case SCAN_ORDINAL_ELEMENT_ATTRIBS:
            test_short0 = csl_ByteArray2UShort(out_array->array_row[new_row]);
            test_short1 = csl_ByteArray2UShort(in_array);

        case SCAN_ORDINAL_SCAN_DATE:
            break;

        default:
            test_string0 = csl_ByteArray2String(out_array->array_row[new_row]);
            test_string1 = csl_ByteArray2String(in_array);
            break;
    }

******************************************/
}

CS_1d_byte_array    *csl_2dByteArrayRetrieveRow(CS_2d_byte_array *in_array, unsigned int out_row)
{
//    CS_1d_byte_array *out_array         = calloc(1, sizeof(CS_1d_byte_array));
    CS_1d_byte_array *out_array         = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
    out_array->number_of_element_bytes  = in_array->array_row[out_row]->number_of_element_bytes;
    out_array->byte_array_element       = calloc (out_array->number_of_element_bytes, sizeof(byte));
    memcpy(out_array->byte_array_element, in_array->array_row[out_row]->byte_array_element, out_array->number_of_element_bytes);

    return out_array;
}

CS_1d_byte_array    *csl_2dByteArrayRetrieveRow(CS_1d_byte_array *out_array, CS_2d_byte_array *in_array, unsigned int out_row)
{
    out_array->number_of_element_bytes  = in_array->array_row[out_row]->number_of_element_bytes;
    out_array->byte_array_element       = calloc (out_array->number_of_element_bytes, sizeof(byte));
    memcpy(out_array->byte_array_element, in_array->array_row[out_row]->byte_array_element, out_array->number_of_element_bytes);

/**** DEBUG **********************************
    unsigned test_int0  = 0;
    unsigned test_short0    =0;
    char *test_string0;
    unsigned test_int1   = 0;
    unsigned test_short1    =0;
    char *test_string1;
    switch(out_row)
    {
        case SCAN_ORDINAL_SCAN_ID:
        case SCAN_ORDINAL_PROBE_ID:
        case SCAN_ORDINAL_SCAN_VALUE:
        case SCAN_ORDINAL_ELEMENT_NAME_HASH:
            test_int0 = csl_ByteArray2UInt(in_array->array_row[out_row]);
            test_int1 = csl_ByteArray2UInt(out_array);
            break;

        case SCAN_ORDINAL_ELEMENT_ATTRIBS:
            test_short0 = csl_ByteArray2UShort(in_array->array_row[out_row]);
            test_short1 = csl_ByteArray2UShort(out_array);

        case SCAN_ORDINAL_SCAN_DATE:
            break;

        default:
            test_string0 = csl_ByteArray2String(in_array->array_row[out_row]);
            test_string1 = csl_ByteArray2String(out_array);
            break;
    }

    ******************************************/
    return out_array;
}

CS_1d_byte_array    *csl_2dByteArrayFindRow(CS_2d_byte_array *in_array, unsigned int out_row)
{
    return in_array->array_row[out_row];
}

CS_2d_byte_array    *csl_2dByteArrayCopy2dArray(CS_2d_byte_array *out_array, CS_2d_byte_array *in_array)
{
    for (int i = 0; i < in_array->number_of_rows; i++)
    {
        CS_1d_byte_array *tmpArray  = csl_2dByteArrayRetrieveRow(in_array, i);
        out_array                   = csl_2dByteArrayAddRow(out_array, tmpArray);
    }
    return out_array;
}

CS_2d_byte_array    *csl_2dByteArrayBuildFromTable(CS_2d_byte_array *out_array, CS_1d_byte_array in_array[], unsigned int in_rows)
{
    out_array->number_of_rows   = in_rows;
    for (int i = 0; i < in_rows; i++)
    {
//        out_array->array_row[i]                          = calloc(1, sizeof(CS_1d_byte_array));
        out_array->array_row[i]                          = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
        out_array->array_row[i]->number_of_element_bytes = in_array[i].number_of_element_bytes;
        out_array->array_row[i]->byte_array_element      = calloc(in_array[i].number_of_element_bytes, sizeof(char));
        memcpy(out_array->array_row[i]->byte_array_element, in_array[i].byte_array_element, in_array[i].number_of_element_bytes);
    }
    return out_array;
}


/************************************************
 * CS_3d_byte_array Functions
 ************************************************/

CS_3d_byte_array    *csl_3dByteArrayAddTable(CS_3d_byte_array *old_array, CS_2d_byte_array *in_array)
{
    CS_3d_byte_array *new_array  = calloc(sizeof(CS_3d_byte_array), sizeof(byte));
    if (old_array != NULL)
    {
        new_array->number_of_tables = old_array->number_of_tables + 1;
    }
    else
    {
        new_array ->number_of_tables = 1;
    }
    // copy the old tables in
    for (unsigned int i = 0; i < old_array->number_of_tables; i++)
    {
        new_array ->array_table[i]   = calloc(sizeof(CS_2d_byte_array), sizeof(byte));
        new_array ->array_table[i]   = csl_2dByteArrayCopy2dArray(new_array->array_table[i], old_array->array_table[i]);
    }
    // add the new table
    new_array->array_table[old_array->number_of_tables]   = calloc(sizeof(CS_2d_byte_array), sizeof(byte));
    new_array->array_table[old_array->number_of_tables]   = csl_2dByteArrayCopy2dArray
            (new_array->array_table[old_array->number_of_tables], in_array);

    free (old_array);
    return new_array;
}

CS_3d_byte_array    *csl_3dByteArrayAddTable(CS_3d_byte_array *out_array, CS_2d_byte_array *in_array)
{
    // Note: We assume that for a new table, having used calloc, the initial table count will be zero ... but VERIFY!
    unsigned new_table                  = out_array->number_of_tables;
//    out_array->array_table[new_table]   = calloc(1,  sizeof(CS_2d_byte_array));
    out_array->array_table[new_table]   = calloc(sizeof(CS_2d_byte_array), sizeof(byte));
    out_array->array_table[new_table]   = csl_2dByteArrayCopy2dArray(out_array->array_table[new_table], in_array);
    out_array->number_of_tables         = new_table + 1;

    return out_array;
}

CS_2d_byte_array    *csl_3dByteArrayRetrieveTable(CS_3d_byte_array *in_array, unsigned int out_table)
{
    return in_array->array_table[out_table];
}

/************************************************
 * ==========================================================================
 * char *csl_Serialize2StringD(char* inArray[], int inArraySize, char* delim)
 * ==========================================================================
 *
 *      This is a CS Library Function
 *
 *      Purpose:	To convert an array of strings (character arrays) "**inArray" into a
 *							single delimited string "*outArray", using the delimiter "delim"
 *
 *		Parameters:
 * 		    char *inArray[]     - pointer to an array of strings (of *char)
 * 		    int  inArraySize    - Number of elements in inArray
 * 		    char delim          - the delimiter character
 *
 *		Returns:
 *			Success -	A pointer to the serialized array
 *			Failure -	A NULL Pointer
 *
 *
 *      Version Number: 00.00.00
 *		Author:		    Kerry
 *		Comments:       Currently this version is VERY rough ... no real error checking and reporting ... PoC
 *
 ************************************************/

char *csl_Serialize2StringD(char *inArray[], unsigned int inArraySize, char delim)
{
    /* Initialize the return value */
    char*	returnValue = NULL;

    /* Check the passed parameters */
    if (inArray == NULL)
    {
        /* Log the error - to be filled in*/
        return returnValue;
    }
    if (inArraySize <= 0)
    {
        /* Log the error - to be filled in*/
        return returnValue;
    }
    for (int i = 0; i < inArraySize; i++)
    {
        for (int j = 0; j < strlen(inArray[i]); j++)
        {
            if (inArray[i][j] == delim)
            {
                /* Log the error - to be filled in */
                return returnValue;
            }
        }
    }
    /* ***** NOTE ****************
     * If the above checks fail, we are safe bailing out there because no space has yet been allocated
     * ***************************/

    /* Initialize the space for the buffer */
    /* Count the size needed for the returnValue buffer */
    size_t bufferSize = 0;
    for (int i = 0; i < inArraySize; i++)
    {
        bufferSize += strlen(inArray[i]);
        bufferSize ++;  // for the END_OF_STRING or delimiter characters
    }

    /* Malloc the returnValue and initialize it with blanks spaces */
    returnValue = (char *) malloc(sizeof(char) * bufferSize);
    for (int i = 0; i < bufferSize; i++)
    {
        returnValue[i] = BLANK;
    }

    /* Start the read and copy loop */
    int endCtr  = 0;
    for (int i = 0; i < inArraySize; i++)
    {
        for (int j = 0; inArray[i][j] != END_OF_STRING; j++)
        {
            returnValue[endCtr] = inArray[i][j];
            endCtr++;
        }
        returnValue[endCtr] = delim;
        endCtr++;
    }
    endCtr--;
    returnValue[endCtr] = END_OF_STRING;
    printf("[%s]\nreturnValue = \"%s\"\n\n", __PRETTY_FUNCTION__, returnValue);

    return returnValue;
}
#endif
