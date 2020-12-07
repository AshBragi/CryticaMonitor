//
// Created by kerry on 7/20/20.
//

#ifndef CRYTICAMONITOR_CRYTICAMONITOR_H
#define CRYTICAMONITOR_CRYTICAMONITOR_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <linux/limits.h>
#include <net/if.h>
#include <openssl/md5.h>
#include <czmq.h>

#include <time.h>
#include "csl_crypto.h"
#include "csl_constants.h"
#include "csl_utilities.h"

/************************************************
 * **********************************************
 * Project typedefs
 * ================
 *
 * **********************************************
 ************************************************/


/**** Crypto Key for messaging ******************/
typedef struct
{
    char   accessor_identifier[ACCESSOR_ID_SIZE];
    byte   accessor_encrypted_sym_key [SYM_KEY_SIZE];

} csl_crypto_accessor;

/**** A Structure for received heat beats *******/
typedef struct
{
    unsigned int  probe_id;
    char          *heart_beat_details;                      // This should probably be a CS serialized message
} heartBeatMessage;

/************************************************
 * Scan Related Typedefs
 * =====================
 *
 ************************************************/
/************************************************
 * csl_scan_record
 * This is one record from the array scan that the monitor receives from the probe
 * There is one row for the data of each scanned element
 * For some messages, the csl_scan_record comprises the entire messageBody
 ************************************************/
typedef struct
{
    unsigned int    scan_id;                                        // ordinal assigned by the probe
    double          probe_id;                                       // ordinal assigned by the monitor
    unsigned short  device_index;
    byte            probe_uuid[SIZE_ZUUID];                         // 33 Bytes
    byte            device_ip[SIZE_IP4_ADDRESS];                    // 16 Bytes
    byte            device_mac_address[SIZE_DEVICE_IDENTIFIER];     // Unique Identifier for sending device
    unsigned short  element_type;
    char            element_name[SIZE_ELEMENT_NAME+1];                // Fully Qualified Name
    byte            element_name_hash[SIZE_HASH_NAME+1];              // SIZE_HASH_32_BYTES
    byte            scan_value[SIZE_HASH_ELEMENT+1];
    unsigned short  element_attributes;
    time_t          scan_date;

} csl_scan_record;

#ifndef DEPRECATED
typedef struct
{
    unsigned int    scan_id;                        // ordinal assigned by the probe
    double          probe_id;                       // ordinal assigned by the monitor
    byte            *probe_uuid;                    // 33 Bytes
    byte            *device_ip;                     // 16 Bytes
    byte            *device_mac_address;            // Unique Identifier for sending device
    char            *element_name;                  // Fully Qualified Name
    byte            *element_name_hash;             // SIZE_HASH_32_BYTES
    byte            *scan_value;
    unsigned short  element_attributes;
    time_t          scan_date;

} csl_scan_record;
#endif
/************************************************
 * A Table for Scans
 * There is a counter for the number of scans in
 * the table, and then an array consisting of one
 * row per scan
 * **********************************************/
typedef struct
{
    unsigned short      scan_element_ctr;
    short               device_index;
    csl_scan_record     scan_elements[MAX_ELEMENTS];
} scan_structure;



/************************************************
 * Messaging Typedefs
 * ==================
 *
 ************************************************/

/**** Message Address Format ********************/
typedef struct
{
    byte            device_identifier[SIZE_DEVICE_IDENTIFIER];
    char            device_ip_address[SIZE_IP4_ADDRESS];
    char            misc_info[SIZE_DEVICE_MISC_INFO];
} csl_message_address;

/**** Message Header ****************************/
typedef struct
{
    csl_message_address to_address;
    csl_message_address from_address;
    unsigned short  message_type;
    unsigned short  message_length;
//    unsigned short  message_header_digsig;                         // future use ... not Version 1
} csl_message_header;

/**** Message Body ******************************/
/* @note:   Originally the message body was designed to be a complex structure
 *          including its own digsig. However, for version 1.0 the message body is
 *          simply a "serialized_message" is essentially a CS_2d_byte_array with a specialized purpose.
 *
 *          When the message body was more complex, expecting multiple scans per message, the
 *          message body was a CS_3d_byte_array, with each being a CS_2d_byte_array "row" (or table)
 *          In the current implementation, however, each message contains only one scan, so the
 *          C2_2d_byte_array can be used as the entire message body
 ************************************************/
/************************************************
 * Currently not used - maybe in the future

typedef struct
{
    unsigned short      number_of_accessors;                        // future use ... not Version 1
    csl_crypto_accessor message_accessor[ACCESSORS_MAX_NUMBER];     // future use ... not Version 1
    unsigned short      message_length;
    CS_2d_byte_array    serialized_message;
    unsigned short      message_body_digsig;                        // future use ... not Version 1
} csl_message_body;
 ************************************************/
typedef struct
{
    csl_scan_record      message_scan;
    // fields from the zmessage structure that are not included in the message_scan
    uint32_t    message_total_size; //4 bytes for the message total size
    char        license_key[SIZE_LICENSE_KEY]; //65 bytes for the License Key (64 + 1 null term)
    char        hostname[HOST_NAME_MAX]; // 64 - 255 bytes on some systems
    uint32_t    probe_event; // 4 Bytes

} csl_message_body;

/**** Message Overall Structure *****************/
typedef struct
{
    csl_message_header  message_header;
    csl_message_body    message_body;
} csl_complete_message;


/************************************************
 * The following are used for both ZeroMQ and
 * by the CS monitor_info_table
 ************************************************/

/************************************************
 * This is for the CS monitor_info_table
 ************************************************/

/************************************************
 * Zero MQ Typedefs
 * =====================
 *
 ************************************************/

typedef struct
{
    // CurveMQ auth actor for tcp encryption
    zactor_t *auth;
    // Monitor server key (private/public), TODO: persistent key storage on the disk
    zcert_t *server_cert;
    // Command publisher (ZMQ_PUB), should not broadcast commands until probes finish handshake
    zsock_t  *broadcaster;
    // Probe handshake request responder (ZMQ_RESP)
    zsock_t *responder;
    // Probe file scan receiver (ZMQ_PULL)
    zsock_t *scan_receiver;
    // A zmq poller is just a way to combine multiple sockets and switch between them checking for incoming
    // incoming data on a socket
    zpoller_t *poller;

    char public_cert_file_name[256];
    char private_cert_file_name[256];

    const char *monitor_public_key;

    char broadcasting_address[256];
    char responder_address[256];
    char scan_address[256];

    bool verbose_auth;
    bool verbose_message;

} monitor_comms_t;

/**** To be replaced ASAP Todo !!!!!!!!! ****/
typedef struct
{
    //unsigned char probe_id[20];
    //unsigned char send_ip_addr[30];
    //unsigned char recv_ip_addr[30];
    size_t      message_total_size; //4 bytes for the message total size : WRONG!!!! size_t is 8 bytes!
    char        license_key[SIZE_LICENSE_KEY]; //65 bytes for the License Key (64 + 1 null term)
    uint32_t    file_name_size; //4 bytes for the file_name size (variable msg size)
//    char        file_name[PATH_MAX]; // 4096 bytes
//    char        *file_name; //  Max is 4096 bytes
    char        file_name[SIZE_ELEMENT_NAME];
    char        hostname[HOST_NAME_MAX]; // 64 - 255 bytes on some systems
    char        hash[SIZE_HASH_ELEMENT_Z]; // 32 bytes + 1 for '\0'
    char        probe_uuid[ZUUID_STR_LEN + 1]; // 32 Char = 32 * Bytes + 1 null char
    char        probe_ip[IFNAMSIZ]; // 16 Bytes
    byte        probe_mac_address[IFHWADDRLEN]; // 6 Bytes (or unsigned char)
    uint32_t    probe_event; // 4 Bytes
    uint32_t    probe_id; // 4 Bytes - Database ID of probe eg. crytica.probe.probe_id
    __mode_t    file_attribute;
} csl_zmessage;

/************************************************
 * This is for the CS monitor_info_table
 ************************************************/
typedef struct
{
    char monitor_hostname [SIZE_HOST_NAME];
    char monitor_public_key [SIZE_PUBLIC_KEY];
    char monitor_external_ip_address[SIZE_IP4_ADDRESS];
    char monitor_internal_ip_address[SIZE_IP4_ADDRESS];
    char monitor_mac_address[SIZE_MAC_ADDRESS];
    char broadcasting_address [SIZE_PORT_ADDRESS];
    char responder_address [SIZE_PORT_ADDRESS];
    char scan_address [SIZE_PORT_ADDRESS];
    char data_pipeline_address [SIZE_PORT_ADDRESS];
    char public_cert_file_name [SIZE_ELEMENT_IDENTIFIER];
} monitor_comm_params;


/************************************************
 * Device and Monitor TypeDefs
 * ===========================
 *
 ************************************************/
/************************************************
 * Device Table
 * The device_table has one record for each device being monitored
 * **********************************************/
typedef struct
{
    unsigned long long  device_id;
    byte                device_identifier[SIZE_DEVICE_IDENTIFIER];
    short               device_index;
    byte                device_mac_address[SIZE_MAC_ADDRESS];
    unsigned short      status_element_ctr;
    double              probe_id;           // currently the device_id
    bool                cs_standard_flag;
    int64_t             last_heartbeat;
    bool                currently_scanning;
    time_t              last_scan;
} device_record;

/************************************************
 * Monitor Information Table
 * The monitor_info_table is where the monitor specific data are stored
 ************************************************/
typedef struct
{
    unsigned long long  monitor_id;         // Value obtained from the monitor table in the config database
    short               device_ctr;         // The number of devices currently being monitored
    short               max_devices;        // Maximum number of devices this monitor can support (later feature)
    short               monitor_sync;       // Flag indicating whether the monitor needs a config update
    monitor_comm_params comm_params;        // The information the monitor needs to communicate via ZeroMQ

} monitor_info_table;


/************************************************
 * Database Record TypeDefs
 * ========================
 *
 ************************************************/

/************************************************
 * Device Alert Record
 ************************************************/
typedef struct
{
    short               alert_type;
    time_t              alert_date;
    unsigned long long  monitor_id;
    char                device_identifier[SIZE_MAC_ADDRESS + 1];
    char                device_from_ip_address[SIZE_IP4_ADDRESS + 1];
    char                device_from_info[SIZE_DEVICE_MISC_INFO];
    char                device_to_ip_address[SIZE_IP4_ADDRESS + 1];
    char                device_to_info[SIZE_DEVICE_MISC_INFO];
} device_alert_record;

/************************************************
 * Scan Alert Record
 ************************************************/
typedef struct
{
    short               alert_type;
    time_t              alert_date;
    unsigned long long  monitor_id;
    unsigned long long  device_id;
    char                device_identifier[SIZE_DEVICE_IDENTIFIER+1];
    double              probe_id;
    unsigned short      element_type;               // currently, this is only a file
    char                element_name[SIZE_ELEMENT_NAME];
    unsigned int        scan_id;                    // not yet used
    byte                scan_value[SIZE_HASH_ELEMENT];
    time_t              scan_date;                  // not yet used
} scan_alert_record;

/************************************************
 * Crytica Standard Record
 ************************************************/
typedef struct
{
    unsigned short      cs_standard_type;            // i.e., Gold, Silver, Copper ...
    time_t              cs_date;
    unsigned long long  cs_monitor_id;
    unsigned long long  cs_device_id;
    unsigned short      cs_element_type;
    byte                cs_element_identifier[SIZE_HASH_NAME];
    char                *cs_element_name;
    byte                cs_scan_value[SIZE_HASH_ELEMENT];
    time_t              cs_previous_date;
} cs_standard_record;

/************************************************
 * Monitor Record
 ************************************************/
typedef struct
{
    unsigned long long  monitor_id;
    char                monitor_name[SIZE_HOST_NAME];
    byte                monitor_identifier[SIZE_DEVICE_IDENTIFIER];
    unsigned short      monitor_sync;
} csl_monitor_record;

/************************************************
 * Monitor Device Record
 ************************************************/
typedef struct
{
    unsigned long long  monitor_id;
    unsigned long long  device_id;
    byte                device_identifier[SIZE_DEVICE_IDENTIFIER];
    time_t              cs_standard_date;
} csl_monitor_device_record;


/************************************************
 * Status Quo Table
 * The status_quo_table is where the most recent scan for each device is stored.
 * There is one column (status_quo_column) per device
 * There is one record (status_quo_record) in each column for the data of a scanned element
 ************************************************/
typedef struct
{
    byte            element_identifier[SIZE_HASH_NAME];                 // unique element identifier - hash of element_name
    unsigned short  element_type;
//    char            element_name[ELEMENT_NAME_SIZE];    // fully qualified path name included for DEBUG ONLY todo - REMOVE after DEBUG!!!!
    byte            scan_value[SIZE_HASH_ELEMENT];                         // value of the most recent scan
    unsigned short  element_attributes;                 // value of the element's attributes
    unsigned char   alert_code;                         // if alert, type of alert, otherwise zero
//    char            alerting_probe;     not yet needed     // This is bit array in the one byte char
} status_quo_record;


/**************************************************************************************************
 * ************************************************************************************************
 * Typedefs Above & Function Prototypes Below
 *
 * ************************************************************************************************
 **************************************************************************************************/




/************************************************
 * **********************************************
 * main function declarations
 * ==========================
 *
 * **********************************************
 ************************************************/


/************************************************
 *      Alert Processing Functions
 ************************************************/

/************************************************
 * int alertDeviceWriteRecord()
 *  @param
 *          device_alert_record *alert_record
 *
 *  @brief  Write a device alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int     alertDeviceWriteRecord(device_alert_record *alert_record);

/************************************************
 * bool alertOnDevice()
 *  @param
 *          char    *identifier_string
 *          short   alert_type
 *
 *  @brief
 *          Populates the alert record fields when an unauthorized device is detected
 *          Sends the alert record to the database to be written
 *
 *  @author Kerry
 *
 *  @note: this function uses the current_message in order to get info about device
 *
 *  @return true on success, false on failure
 ************************************************/
bool    alertOnDevice (char *identifier_string, short alert_type);


/************************************************
 * bool alertOnElementAddition
 *  @param
 *          unsigned short  device_index
 *          csl_scan_record scan_element
 *
 *  @brief
 *          Populates the alert record fields when a scan detects that an element had not been there before
 *          Sends the alert record to the database to be written
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool alertOnElementAddition(unsigned short device_index, csl_scan_record scan_element);

/************************************************
 * bool alertOnElementDeletion
 *  @param
 *          status_quo_record   deleted_record
 *          unsigned long long  device_id
 *          time_t              scan_date
 *
 *  @brief
 *          Populates the alert record fields when a scan detects that an element has been deleted
 *          Sends the alert record to the database to be written
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool alertOnElementDeletion (status_quo_record deleted_record, unsigned long long device_id, time_t scan_date);

/************************************************
 * bool alertOnElementModification()
 *  @param
 *          unsigned short  device_index
 *          csl_scan_record scan_element
 *          short           alert_type
 *
 *  @brief
 *          Populates the alert record fields when an element modification detected
 *          Sends the alert record to the database to be written
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool    alertOnElementModification(unsigned short device_index, csl_scan_record scan_elements, short alert_type);

/************************************************
 * int alertScanWriteRecord()
 *  @param
 *          device_alert_record *alert_record
 *
 *  @brief  Write a scan alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int     alertScanWriteRecord (scan_alert_record *alert_record);

/************************************************
 *      Config File Processing Functions
 ************************************************/
/************************************************
 * int configFileCreate()
 * @param
 *          monitor_comms_t *comms  - A structure containing all of the communication data
 *
 * @brief   Create (write) the configuration file that goes out to all
 *          probe installations. The probe reads the file and then
 *          has the information necessary to communicate back to this monitor.
 *          Each monitor has its own config file, one that can be used
 *          by any probe anywhere.
 *
 * @author Kerry
 *
 * @return true on success or false on error ... we may want to change this ... (yet to be determined)
 ************************************************/
bool    configFileCreate(monitor_comm_params *comms);


/************************************************
 *      Crytica Standard Functions
 ************************************************/

/************************************************
 * int cStandardNeeded()
 *  @param  - None. All needed fields are either global or local variables
 *
 *  @brief  Query the config for devices that require a new Crytica Standard
 *          and update the flag in the device table accordingly
 *
 *  @author Kerry
 *
 *  @note   In the database, a DEFAULT_DATE in
 *              the c_standard_date field in
 *              the cs_monitor.v_monitor_config table
 *          indicates that a new Crytica Standard is required for the device.
 *          Otherwise that field will contain the date when the last Crytica Standard was taken.
 *
 *          This function queries the cs_monitor database to find any devices that require a new Crytica Standard.
 *          For those that do, it sets:
 *              device_table->cs_standard_flag = true   // the device needs a new Crytica Standard
 *
 *  @return Success - number of devices found
 *          Failure - ERROR Code
 ************************************************/
int    cStandardNeeded();

/************************************************
 * cs_standard_record *cStandardReadRecord()
 *  @param
 *          unsigned long long   monitor_id
 *          unsigned long long   device_id
 *          unsigned short       element_identifier
 *
 *  @brief  Queries the cs_monitor.v_cs_standard View to find the current record of a CS-Standard for
 *          a specific element on a specific device.
 *
 *  @note   At the moment, all we are retrieving are the element_name and element_type
 *
 *  @author Kerry
 *
 *  @return Success - A pointer to a populated CS_Standard Record
 *          Failure - A NULL pointer
 ************************************************/
cs_standard_record *cStandardReadRecord(
        unsigned long long  monitor_id,
        unsigned long long  device_id,
        byte                *element_identifier);

/************************************************
 * int cStandardWriteRecord()
 *  @param
 *          csl_monitor_scan_record new_record
 *
 *  @brief  Writes a new Crytica Standard record to the cs_monitor.v_cs_standard view
 *
 *  @author Kerry
 *
 *  @note   If need be for speed, we can populate the "fixed" portions of the query statement and then
 *          cycle through the variable portions. My feeling is that this will make the code more complex
 *          and not buy use much, if any, time. But it IS a possibility.
 *
 *  @return CS_SUCCESS
 *          error code
 ************************************************/
int cStandardWriteRecord(cs_standard_record *new_record);

/************************************************
 * bool cStandardWriteToDB(deviceIndex)
 *  @param
 *          short   device_index    - indicates for which device the new Crytica Standard is being written
 *
 *  @brief  For a specific device, writes a new Crytica Standard (CS) to the database
 *
 *  @author Kerry
 *
 *  @note  Assumes that a successful scan has just been taken
 *          Uses the most recent scan as the source of
 *          the CS standard database records
 *
 *          Reads through the StatusQuoTable Column for the device
 *          Writes each record to the DigSig Ledger
 *          ... with the current date time in cs_date
 *          ... with replaced_by blank
 *          ... with its cs_date in the replaced_on field of its predecessor
 *
 *          NOTE: To save time this can be combined with statusQuoTableBuild, but I would prefer not to do that
 *          ====
 *
 * @return true on success, false on failure
 ************************************************/
bool    cStandardWriteToDB(unsigned short deviceIndex);


/************************************************
 *      Device Processing Functions
 ************************************************/

/************************************************
 * long long deviceAssignedToMonitor()
 *  @param
 *          byte                *device_identifier  - the device identifier to lookup
 *          char*               device_mac_address  - the device MAC address found in the lookup
 *
 *  @brief  Function to query the database and find out if a specific device is assigned to this monitor
 *          and if so, return the device_id and the device_mac_address
 *
 *  @author Kerry
 *
 *  @note   The function looks up the device in the database to see whether
 *          (1) it is defined there
 *          (2) it is assigned to this monitor
 *          If either of these conditions fail the database query will come up empty and an alert is issued
 *
 * @return
 *          device_id               - if found, i.e., if assigned, device_id of found device
 *          CS_DEVICE_NOT_FOUND     - if not, i.e., device is not assigned to this monitor
 ************************************************/
long long deviceAssignedToMonitor(byte *device_identifier, byte *device_mac_address);

/************************************************
 * short deviceBadActorAdd(byte *device_identifier)
 *  @param
 *          char *device_identifier
 *
 *  @brief  Function to add a device in the device bad actor table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a bad actor device_table in which each entry
 *          contains the device identifier of those unauthorized devices that have attempted to connect
 *          to the monitor.
 *          In the event that the table is about to overflow, this function re-initializes the table and
 *          starts again from position zero.
 *
 *  @return the size of the device bad actor device table
 ************************************************/
short deviceBadActorAdd(char *device_identifier);

/************************************************
 * short deviceBadActorFind()
 *  @param
 *          char *device_identifier
 *
 *  @brief  Function to find a device in the device bad actor table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a bad actor device_table in which each entry
 *          contains the device identifier of those unauthorized devices that have attempted to connect
 *          to the monitor
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique device_id
 *
 * @return the row number (the device_index) of the found device or the code: CS_DEVICE_NOT_FOUND
 ************************************************/
short   deviceBadActorFind(char *device_identifier);

/************************************************
 * bool deviceBadActorTableInitialize()
 *  @param  none
 *
 *  @brief  To initialize the device bad actor table
 *
 *  @author Kerry
 *
 * @return  true
 ************************************************/
bool deviceBadActorTableInitialize();

/************************************************
 * short deviceFindByDeviceID()
 *  @param
 *          unsigned long long device_id
 *
 *  @brief  Function to find a device in the device_table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a device_table in which each entry
 *          contains information about a device the monitor is monitoring.
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique device_id
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short   deviceFindByDeviceID(unsigned long long device_id);

/************************************************
 * short deviceFindByDeviceIdentifier()
 *  @param
 *          char *device_identifier
 *
 *  @brief  Function to find a device in the device_table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a device_table in which each entry
 *          contains information about a device the monitor is monitoring.
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique device identifier
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short   deviceFindByDeviceIdentifier(byte *device_identifier);

/************************************************
 * short deviceFindByProbeID()
 *  @param
 *          double probe_id
 *
 *  @brief  Function to find a device in the device_table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a device_table in which each entry
 *          contains information about a device the monitor is monitoring.
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique probe_id
 *
 *          The probe_id is monitor_id + ((device_index + 1) / MAX_DEVICES)
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short   deviceFindByProbeID(double probe_id);

/************************************************
 * short    deviceRegisterNew()
 *  @param  byte                *device_identifier  - The identifier of the new device being added to the device table
 *  @param  unsigned long long  device_id           - The device_id of the new device being added to the device table
 *  @param  char                *device_mac_address - The MAC address of the new device being added
 *
 *  @brief  To add a new device to the monitor's internal device_table. It is assumed at this point that the device
 *          identifier and id were checked with the config database, just to be sure. The new device is added to the
 *          table and the device counter is incremented.
 *
 *  @author Kerry
 *
 *  @return the value of the device counter, i.e., how many devices are in the table
 ************************************************/
short   deviceRegisterNew(byte *device_identifier, unsigned long long device_id, byte *device_mac_address);


/************************************************
 * char *elementNameRetrieve(()
 *  @param
 *          unsigned long long   monitor_id
 *          unsigned long long   device_id
 *          unsigned short       element_identifier
 *
 *  @brief  Queries the cs_monitor.v_element_names View to find the full element name fo
 *          a specific element on a specific device.
 *
 *  @note   This is not necessary for releases prior to 0.5.0, since we are storing the element name in those
 *
 *  @author Kerry
 *
 *  @return a point to the element name or NULL
 ************************************************/
char *elementNameRetrieve(
        unsigned long long  monitor_id,
        unsigned long long  device_id,
        byte                *element_identifier);


/************************************************
 *      Message Process Functions
 *      =========================
 ************************************************/

/************************************************
 * int messageCheckOrigin()
 *  @param - None
 *
 *  @brief
 *      Checks to see if a message has originated from:
 *          A device the monitor has already registered
 *              - the global variable current_device_index is set, and CS_SUCCESS is returned
 *          An authorized device the monitor has not yet registered
 *              - the new device is registered, current_device_index is set and CS_SUCCESS is returned
 *          An unauthorized device
 *              - an alert is issued (CS_ALERT_DEVICE_UNKNOWN) and CS_DEVICE_UNKNOWN is returned
 *
 *  @author Kerry
 *
 *  @return
 *      CS_SUCCESS
 *      CS_DEVICE_UNKNOWN
 ************************************************/
int     messageCheckOrigin();

/************************************************
 * int messageHandshakeProccess()
 *  @param csl_zmessage     *current_message    - The incoming message
 *  @param monitor_comms_t  *comms              - The CZMQ parameter file
 *
 *  @brief  Calls the csl_AcknowledgeHandshake function after a handshake message is received
 *
 *  @author Kerry
 *
 *  @return the result of the csl_AcknowledgeHandshake function
 ************************************************/
int     messageHandshakeProccess(csl_zmessage *current_message, monitor_comms_t *comms);

/************************************************
 * bool messageHeartBeatProcess()
 * @param   - short device_index
 *
 * @brief   Calls the csl_ProcessHeartbeat function after a heartbeat message is received
 *
 * @author  Kerry
 *
 * @return True on success, failure on failure
 ************************************************/
bool    messageHeartBeatProcess(short device_index);

/************************************************
 * int messageTimeoutProcess()
 * @param   - None
 *
 * @brief   - Handles a Timeout Message from the probe message handler
 *
 * @author  - Kerry
 *
 * @note    - This is still rudimentary // todo We need to expand this function!!!!
 *
 * @return  - CS_SUCCESS
 ************************************************/
int     messageTimeoutProcess();

/************************************************
 * int messageUnknownProcess()
 * @param   - int message type
 *            short device_id
 *
 * @brief   - Handles a Message of unknown type from the probe message handler
 *
 * @author  - Kerry
 *
 * @note    - This is still rudimentary // todo We need to expand this function!!!!
 *
 * @return  - CS_SUCCESS
 ************************************************/
int     messageUnknownProcess(int message_type, short device_id);


/************************************************
 *      Monitor Start-up, Config and Shutdown Functions
 *      ===============================================
 ************************************************/

int     monitorConfigDBQuery (bool db_sync_just_launched);

int monitorConfigUpdate();

/************************************************
 * int monitorInitialize()
 *  @param  None
 *
 *  @brief  When the monitor first launches, there are many start-up tasks
 *          that need to be performed. Rather than clog up the main() routine
 *          with them, they are collected into this support function.
 *
 *  @author Kerry
 *
 *  @note   The logic is:
 *              Setup the monitor_table with information about the monitor and its environment, e.g.,
 *                  - IP Address(s)
 *                  - MAC Address
 *                  - Hostname
 *              Setup the Monitor's Communications Protocols
 *              Connect to the local Crytica MySQL database to verify the Monitor
 *              Initialize the Monitor's Important Tables
 *                  - Device Table
 *                  - Status Quo Table
 *
 *  @return CS_SUCCESS or error code
 ************************************************/
int    monitorInitialize();

/************************************************
 * bool monitorShutdown()
 *  @param  None
 *
 *  @brief  When the monitor finishes its run, for whatever reason, there are clean-up and shutdown tasks
 *          that need to be performed. Rather than clog up the main() routine
 *          with them, they are collected into this support function.
 *
 *  @author Kerry
 *
 *  @Note   At the moment, all we have is to close the connection to the DB
 *
 * @return  true on success, false on failure
 ************************************************/
bool    monitorShutDown();


/************************************************
 *      Scan Functions
 *      ==============
 ************************************************/
short scan_get_next_index(short previous_scan_index);

/************************************************
 * bool scanEvaluate()
 *  @param  short   device_index
 *
 *  @brief  After a scan has been received, it needs to be "evaluated" that is,
 *          We need to compare it to the appropriate column in the Status Quo Table
 *          (the column associated with the device from which the scan came)
 *          Once this function determines the appropriate column, it calls
 *          statusQuoTableSearch(), which then compares the most recent scan
 *          with the values in the Status Quo Table.
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool scanEvaluate(short device_index);

/************************************************
 * bool scanRequest()
 *  @param
 *          short device_index
 *
 *  @brief  After a scan has been received from a device's probe, new scan should immediately be requested
 *          This functions sends such a request to the device.
 *
 *  @author Kerry
 *
 *  @note   Although it is true that this function could use the global variable "current_device_index", if we
 *          ever move to a more asynchronous environment, being explicit about from which device we are
 *          requesting the scan cannnot hurt us.
 *
 *  @return true on success, false on failure
 ************************************************/
bool    scanRequest(short device_index);

/************************************************
 * int scanTableAddRow()
 *  @param  csl_scan_record *new_scan
 *
 *  @brief  Adds a row to the scan table
 *
 *  @author Kerry
 *
 * @return CS_SUCCESS
 ************************************************/
int scanTableAddRow(csl_scan_record *new_scan, short device_index);

/************************************************
 * short scanTableInitialize()
 *  @param  - None
 *
 *  @brief  Initializes all of the values in the scan_table
 *
 *  @author Kerry
 *
 *  @note   This is called each time a new scan is received from a probe. It is possible that
 *          we can get away with setting:
 *              scan_table.scan_element_ctr = ZERO;
 *          or perhaps setting
 *              scan_table.scan_elements[i].scan_id = 0L;
 *          but we shall see
 *
 * @return CS_SUCCESS
 ************************************************/
int scanTableInitialize(short device_index);


/************************************************
 *      Status Quo Table Functions
 *      ==========================
 ************************************************/

/************************************************
 * bool statusQuoTableAddRow ()
 *  @param
 *          unsigned short device_index
 *          unsigned short sq_table_row
 *          csl_scan_record scanRecord
 *
 *  @brief  This function adds a row to the column in the Status Quo Table associated with a specific device.
 *          It is used to both append a row to the "bottom" of the column (assuming a top-to-bottom configuration)
 *          as well as to create (or recreate) the column in the event of a new Crytica Standard being recorded.
 *
 *  @author Kerry
 *
 * @return  true on success, false on failure
 ************************************************/
bool    statusQuoTableAddRow(unsigned short device_index, unsigned short sq_table_position, csl_scan_record scanRecord);

/************************************************
 * bool statusQuoTableBuild()
 *  @param
 *          unsigned short device_index
 *
 *  @brief  Builds a column, from zero, in the Status Quo Table. The column is associated with a specific device.
 *          It is used whenever a new Crytica Standard scan is taken for the specific device, and hence that is
 *          when that device's column needs to be rebuilt
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool    statusQuoTableBuild(unsigned short device_index);

/************************************************
 * bool statusQuoTableRemoveRow()
 *  @param
 *          unsigned short device_index
 *          unsigned short row_number
 *
 *  @brief  Removes the specified row from the column in the Status Quo Table associated with a specific device.
 *          It then "closes up the gap" by moving all of the lower rows up.
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool    statusQuoTableRemoveRow(unsigned short device_index, unsigned short row_number);

/************************************************
 * short statusQuoTableSearch
 *  @param
 *          short           device_index    - The device column to search
 *          scan_structure  the_scan        - The scan data from the latest scan
 *
 *  @brief  This function is one of the main functions in the monitor code. (See note below) It compares the values
 *          in the new scan with those already in the Status Quo Table in order to determine
 *          if a change has occurred in the device being monitored.
 *
 *  @author Kerry
 *
 *  @note
 *      **** Caution: This function uses bit-wise operators ****
 *
 *      This function is one of the main functions in the monitor code. When a scan is received from a specific
 *      device, this function is called to search through that device's column in the Status Quo Table to compare
 *      the values and look for:
 *          * Modified Element (File) contents
 *          * Modified Element (File) attributes (permissions)
 *          * Added new Element
 *          * Deleted Element
 *      This is done in two passes through the Status Quo Table
 *
 *      Pass #1
 *      -------
 *      Modified Elements are determined by a direct comparison with the associated values in the Status Quo Table
 *      Additions are determined by there not being an associated value in the Status Quo Table
 *      For additions and modifications, each discrepancy is flagged and the alert_code field in the
 *      Status Quo Table is updated appropriately. The alert_code field is a single byte (unsigned char) field
 *      manipulated via bit masks. So far, its bits are:
 *          COMPARED       0x1   - This row been examined during this search                    0x0 = No, 0x1  = yes
 *          ADD_ELEMENT    0x2   - This row been added since the last search                    0x0 = No, 0x2  = yes
 *          MOD_CONTENTS   0x4   - This row's file contents been modified since the last search 0x0 = No, 0x4  = yes
 *          MOD_ATTRIBS    0x16  - This row's file attribs been modified since the last search  0x0 = No, 0x16 = yes
 *      For every row that is compared in the first pass, the COMPARED bit is set to 0x1
 *      For modifications, after the discrepancy is flagged and an alert requested, the row updated with the
 *      new value(s)
 *      For additions, a new row is added to the table (and flagged as having been compared)
 *
 *      Pass #2
 *      -------
 *      In the second pass, the COMPARED bit is examined. Any row in which the COMPARED bit is still 0x0 is one
 *      in for which there was no corresponding value in the scan; and hence it must have been deleted. When a
 *      deleted entry is found, an alert is sent and then the entry is deleted from the Status Quo Table.
 *      For every entry touched on Pass #2, when it is examined, its COMPARED bit is set back to 0x0.
 *
 * @return  on success, the number of alerts found
 *          On failure, an error code (not yet implemented)
 ************************************************/
short   statusQuoTableSearch(short device_index);
//short   statusQuoTableSearch(short device_index, scan_structure the_scan);

#endif //CRYTICAMONITOR_CRYTICAMONITOR_H

