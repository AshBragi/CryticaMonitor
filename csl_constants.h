//
// Created by C. Kerry Nemovicher, Ph.D. on 6/18/20.
//

#ifndef CRYTICAMONITOR_CSL_CONSTANTS_H
#define CRYTICAMONITOR_CSL_CONSTANTS_H

/************************************************
 * Standard includes
 *      This may not be the place for this, but since csl_constants.h is included
 *      in ALL of the other csl headers and code files, I figured it was a good place to start
 *      - Kerry     2020-07-11      (no free slurpees today because of COVID-19)
 ************************************************/

/*************************************
* // todo - We need to fix the include path for CLion so that
* // todo - we do not need to provide full path names for all of the includes
* ************************************/

#include </usr/include/stdio.h>
#include </usr/include/stdlib.h>
#include </usr/include/string.h>
#include </usr/include/time.h>
//#include </usr/include/stdbool.h>
#include </usr/lib/gcc/x86_64-linux-gnu/7/include/stdbool.h>

/**** Needed for MAC Address ****/
//#include <sys/socket.h>
//#include <sys/ioctl.h>
//#include <linux/if.h>
//#include <netdb.h>
#include </usr/include/linux/socket.h>
#include </usr/include/linux/ioctl.h>
#include </usr/include/netdb.h>

/************************************************
 * defines for commenting out code
 ************************************************/
#define DEPRECATED              0       // This exists so that we can comment out old code
#define DEBUG                   0       // This exists so that we can comment out debug code
#define COMMENT_OUT_DB          0
#define COMMENT_OUT_COMM        0

/************************************************
 * Literal and Default Values
 ************************************************/
#define CRYTICA_MONITOR         "Crytica Monitor"
#define CRYTICA_COPYRIGHT       "Crytica Security, Inc. ©2020 - All Rights Reserved."

/**** File Names ********************************/
#define FILE_HOSTNAME                   "/etc/hostname"
#define FILE_IP_ADDRESS                 "myIPaddress"
#define FILE_MAC_ADDRESS                "myMACaddress"
#define FILE_MONITOR_EXTERNAL_CONFIG    "External_Config"
#define FILE_MONITOR_INTERNAL_CONFIG    "Internal_Config"

/**** MySQL DB Access Constants ************************/
#define CS_SQL_DEFAULT_GROUP     "CryticaMonitor"
#define CS_SQL_MONITOR_SCHEMA    "cs_monitor"
/**** MySQL DB Table/View Constants ************************/
#define CS_SQL_ALERT_TABLE                  "alert_log"
#define CS_SQL_ALERT_LOG_VIEW               "v_alert_log"
#define CS_SQL_DEVICE_TABLE                 "v_device"
#define CS_SQL_ELEMENT_NAMES_VIEW           "v_element_names"
#define CS_SQL_MONITOR_VIEW                 "v_monitor"
#define CS_SQL_MONITOR_DEVICE_VIEW          "v_monitor_device"
#define CS_SQL_ELEMENT_ADDED_NAMES_TABLE    "element_added_names"
#define CS_SQL_ELEMENT_ADDED_NAMES_VIEW     "element_added_names"
#define CS_SQL_STANDARD_VIEW                "v_crytica_standard"
#define CS_SQL_STANDARD_TABLE               "crytica_standard"

/**** CS Default Values *************************/
static char* DEFAULT_DATE       = "2001-02-03 04:05:06";
#define DEFAULT_ELEMENT_NAME    "Default Element Name"
#define ELEMENT_NAME_NOT_FOUND  "Element Name Not Found in Database"
#define DEFAULT_IP4_ADDRESS     "000.000.000.000"
#define DEFAULT_MAC_ADDRESS     "00.00.00.00.00.00"
#define DEFAULT_MONITOR_ID      1984L
#define DEFAULT_PORT_PREFIX     42

/************************************************
 * SUCCESS, WARNING, and ERROR Codes
 ************************************************/
#define CS_SUCCESS                      0
#define CS_END_OF_RUN                   9999

#define CS_DATA_NOT_AVAILABLE           "Data Not Available"

#define CS_BAD_COMPARE                  -9021
#define CS_DEVICE_NOT_FOUND             -1
#define CS_ERROR                        -2
#define CS_ERROR_DB_QUERY               -2020
#define CS_FATAL_ERROR                  -1


/************************************************
 * ELEMENT (e.g., File) DEFINITIONS
 ************************************************/
static char *element_types[] =
        {
                "Executable File",
                "Data File",
                "Firmware"
        };

#define ELEMENT_DATA_FILE       1
#define ELEMENT_EXEC_FILE       0
#define ELEMENT_FIRMWARE        2

#define ELEMENT_NAME_SIZE       256
#define ELEMENT_PATH_SIZE       4096

/************************************************
 * Crytica Standard "CS" DEFINITIONS
 ************************************************/
/**** Old Values ********************************
static char *cs_standard_types[] =
        {
                "Bronze",
                "Silver",
                "Gold",
                "Platinum"
        };
#define CS_STANDARD_BRONZE      0
#define CS_STANDARD_SILVER      1
#define CS_STANDARD_GOLD        2
#define CS_STANDARD_PLATINUM    3
*************************************************/
static char *cs_standard_types[] =
        {
                "Self Defined",
                "Downloaded Code",
                "From Code Source",
                "From Code Source Signed"
        };
#define CS_CODE_SELF_DEFINED    0
#define CS_CODE_DOWNLOADED      1
#define CS_CODE_FROM_SOURCE     2
#define CS_CODE_SIGNED_SOURCE   3


#define CS_DEVICE_UNKNOWN       -101
#define CS_TABLE_OVERFLOW       -111
#define PROBE_NEW               105
#define CS_NULL                 '\0'
#define SCAN_RECEIVED           120
#define MESSAGE_TIMEOUT         1001
#define CSL_MAX_MESSAGE_LENGTH  5012
#define MAX_ELEMENTS            15000
#define MAX_DEVICES             1000      // todo - this for testing only and MUST be changed
#define MAX_PROBES_PER_DEVICE   1       // this is a version 1 constraint

/************************************************
 * Miscellaneous Constants
 ************************************************/
#define BASE_TEN                10
#define BLANK                   ' '
#define DOT                     '.'
#define COLON                   ':'
#define CS_CALL_FROM_FORMAT     "Function: %s() and line: %d"
#define DELIMITER               0xFF
#define EMPTY_STRING            ""
#define	END_OF_STRING   		'\0'
#define HEX_FF                  0xFF
#define NULL_BINARY             '\0'
#define ONE                     1
#define ZERO                    0

/************************************************
 * Alert Defines & Masks
 ************************************************/
#define ALERT_DEVICE_UNKNOWN    -200

#define ALERT_ADD_ELEMENT       0x2
#define ALERT_MOD_CONTENTS      0x4
#define ALERT_DEL_ELEMENT       0x8
#define ALERT_MOD_ATTRIBS       0x10

/**** Bit-wise operators - which #define does not handle well ****/
static unsigned char MASK_COMPARED      = 0x1;
static unsigned char MASK_ADD_ELEMENT   = 0x2;
static unsigned char MASK_MOD_CONTENTS  = 0x4;
static unsigned char MASK_DEL_ELEMENT   = 0x8;
static unsigned char MASK_MOD_ATTRIBS   = 0x10;



/**** Message Related Defines *******************/
#define ACCESSOR_ID_SIZE                64
#define ACCESSORS_MAX_NUMBER            512
#define ADDRESS_MAX_SIZE                128
#define MESSAGE_HANDSHAKE               10
#define MESSAGE_HEARTBEAT               20
#define MESSAGE_LICENSE_KEY_LENGTH      64
#define MESSAGE_MAX_SIZE                5012
#define MESSAGE_SCAN_REQUEST            30
#define MESSAGE_SCAN_RESULT             40

/**** Probe Related Defines *********************/
#define PROBE_READY                     100
#define PROBE_HANDSHAKE                 101
#define PROBE_REGISTERED                102
#define PROBE_GOLD_STANDARD             103
#define PROBE_END_GOLD_STANDARD         104
#define PROBE_RECURRING_SCAN            105
#define PROBE_END_RECURRING_SCAN        106
#define PROBE_END_SCAN                  107
#define PROBE_WAITING                   169
#define PROBE_START_SCAN                202
#define PROBE_TERMINATE                 203
#define PROBE_HEARTBEAT                 204
#define PROBE_TIMEOUT                   208
#define PROBE_INVALID_LICENSE           600

/**** Scan Related Defines **********************/
#define SCAN_RECORD_NUMBER_OF_FIELDS    10
#define SCAN_ORDINAL_SCAN_ID            0
#define SCAN_ORDINAL_PROBE_ID           1
#define SCAN_ORDINAL_PROBE_UUID         2
#define SCAN_ORDINAL_DEVICE_IP          3
#define SCAN_ORDINAL_DEVICE_MAC_ADDRESS 4
#define SCAN_ORDINAL_ELEMENT_NAME       5
#define SCAN_ORDINAL_ELEMENT_NAME_HASH  6
#define SCAN_ORDINAL_SCAN_VALUE         7
#define SCAN_ORDINAL_ELEMENT_ATTRIBS    8
#define SCAN_ORDINAL_SCAN_DATE          9

#define SIZE_ALERT_DEVICE_DATA          256     //todo - this is slightly but needs to be flexible
#define SIZE_DB_LICENSE_KEY             65
#define SIZE_DEVICE_IDENTIFIER          64      // For Version 1, this is identical to SIZE_MAC_ADDRESS - not necessarily forever
#define SIZE_DEVICE_MISC_INFO           64
#define SIZE_ELEMENT_IDENTIFIER         64
#define SIZE_ELEMENT_NAME               5120
#define SIZE_FIXED_SIZE                 256     // todo - We need to calculate this out more precisely - this is just a guess
//#define SIZE_HASH_ELEMENT               33      // (MD5_DIGEST_LENGTH*2) +1
#define SIZE_HASH_ELEMENT_Z             33
#define SIZE_HASH_ELEMENT               32
#define SIZE_HASH_NAME                  32
#define SIZE_HOST_NAME                  64
#define SIZE_IP4_ADDRESS                16
#define SIZE_LICENSE_KEY                65
#define SIZE_LINUX_COMMAND              256
#define SIZE_MAC_ADDRESS                64
#define SIZE_MAC_SHORT                  6       // This is essentially IFHWADDRLEN 6         // defined in if.h
#define SIZE_CS_SQL_COMMAND             6144
#define SIZE_PROBE_EVENT_NAME           32
#define SIZE_OF_TIME                    64
#define SIZE_PORT_ADDRESS               8
#define SIZE_PUBLIC_KEY                 516
#define SIZE_SCAN_RECORD                5012
//#define SIZE_SCAN_TABLE                 15000
#define SIZE_ZUUID                      33      // need to verify this
#define SIZE_IP_PREFIX                  9

#define IP_PREFIX                       "192.168.2"
#define LOCAL_HOST_IP                   "127.0.0.1"
#define SIZE_LOCAL_HOST_IP              9

/* Byte Array Global Defines */

/* Essential typedef Specifications */
/* ================================ */
typedef unsigned char byte;

#endif //CRYTICAMONITOR_CSL_CONSTANTS_H

