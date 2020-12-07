//
// Created by C. Kerry Nemovicher, Ph.D. on 6/19/20.
//

#ifndef CRYTICAMONITOR_CSL_MESSAGE_H
#define CRYTICAMONITOR_CSL_MESSAGE_H

#include <linux/limits.h>
#include <net/if.h>
#include <openssl/md5.h>
#include <czmq.h>

#include <time.h>
#include "csl_crypto.h"
#include "csl_utilities.h"
#include "CryticaMonitor.h"


/************************************************
 * #defines
 * ========
 * See csl_constants.h ... included in csl_utilities.h
 ************************************************/


// Byte size of all CSL message struct data members
// UPDATE EVERY TIME
// This is platform dependent
#define FILE_HASH_LENGTH MD5_DIGEST_LENGTH*2

//#define CSL_MESSAGE_LENGTH (PATH_MAX + HOST_NAME_MAX + (MD5_DIGEST_LENGTH*2) + 1 + ZUUID_STR_LEN + 1 + IFNAMSIZ + IFHWADDRLEN + sizeof (uint32_t) + sizeof (uint32_t) ) //4256
//#define CSL_MAX_MESSAGE_LENGTH (sizeof(size_t) +LICENSE_KEY_LENGTH + sizeof(unsigned int) +PATH_MAX + HOST_NAME_MAX + (MD5_DIGEST_LENGTH*2) + 1 + ZUUID_STR_LEN + 1 + IFNAMSIZ + IFHWADDRLEN + sizeof (uint32_t) + sizeof (uint32_t)  ) //4333
//#define CSL_MAX_MESSAGE_LENGTH (sizeof(size_t) + sizeof(unsigned int) +PATH_MAX + HOST_NAME_MAX + (MD5_DIGEST_LENGTH*2) + 1 + ZUUID_STR_LEN + 1 + IFNAMSIZ + IFHWADDRLEN + sizeof (uint32_t) + sizeof (uint32_t)  ) //4333


//#define CSL_MAX_MESSAGE_LENGTH (sizeof(size_t) +LICENSE_KEY_LENGTH + sizeof(unsigned int) +PATH_MAX + HOST_NAME_MAX + (MD5_DIGEST_LENGTH*2) + 1 + ZUUID_STR_LEN + 1 + IFNAMSIZ + IFHWADDRLEN + sizeof (uint32_t) + sizeof (uint32_t)  ) //4333
//#define CSL_MAX_MESSAGE_LENGTH (sizeof(size_t) +LICENSE_KEY_LENGTH + sizeof(unsigned int) +PATH_MAX + HOST_NAME_MAX + (MD5_DIGEST_LENGTH*2) + 1 + ZUUID_STR_LEN + 1 + IFNAMSIZ + IFHWADDRLEN + sizeof (uint32_t) + sizeof (uint32_t)  + sizeof(mode_t) ) //4333 + 4


#define MONITOR_PUB_KEY_FILE_NAME   "monitor.pub"
#define BROADCASTER_PORT            43
#define DATA_PIPELINE_PORT          44 //Port for handshakes, heartbeats, metadata, etc. Lower load than scan_push
#define RESPONDER_PORT              44   // This is what had been called the DATA_PIPELINE_PORT
#define DATA_PIPELINE_PORT_POSTFIX  44
#define SCAN_PORT                   45 //High load port for sending filename/filehash pairs
#define SCAN_PORT_POSTFIX           45
//#define COMM_DEFAULT_PREFIX         "42"
#define HEARTBEAT_TIMEOUT           5 //Arbitrary timeout for heartbeats (seconds)
// Refer to ZeroMQ documentation about its use of high water marks to prevent overflowing sockets
#define RECEIVE_HIGH_WATER_MARK     10000
#define LICENSE_KEY_LENGTH          65
#define DEFAULT_HOST_NAME           "DefaultHostName"

/************************************************
 * struct Definitions
 * ==================
 *  - Moved to CryticaMonitor.h
 ************************************************/


/************************************************
 * Function Declarations
 * =====================
 *
 ************************************************/

//monitor_comm_params *comm_params_initialize(monitor_comm_params *returnStruct,
int comm_params_initialize(monitor_comm_params *returnStruct,
                                            unsigned short broadcaster_port,
                                            unsigned short data_pipeline_port,
                                            unsigned short scan_port);

short               csl_zmessage_get(csl_zmessage *current_zmessage, monitor_comms_t *comms, csl_complete_message *cs_message);
int                 monitor_comms_bind_command_publisher(monitor_comms_t* self);
int                 monitor_comms_destroy(monitor_comms_t* self);
monitor_comms_t*    monitor_comms_new(int broadcaster_port, int responder_port, int scan_port, char* hostname_in);
void                monitor_comms_set_verbose_auth(monitor_comms_t *self, bool verbosity);
void                monitor_comms_set_verbose_messaging(monitor_comms_t *self, bool verbosity);

//void                *zsys_init(void);

void                csl_createNewMessage (csl_zmessage* message,
                                          char license_key[LICENSE_KEY_LENGTH],
                                          char file_name[PATH_MAX],
                                          __mode_t file_attribute,
                                          char hostname[HOST_NAME_MAX],
                                          char probe_uuid[ZUUID_STR_LEN + 1],
                                          char probe_ip[IFNAMSIZ],
                                          unsigned char probe_mac_address[IFHWADDRLEN],
                                          uint32_t probe_event,
                                          uint32_t probe_id);

int                 csl_AcknowledgeHandshake(csl_zmessage *current_message, monitor_comms_t *comms);
int                 csl_RequestScan(monitor_comms_t *comms, csl_zmessage *pcurrMessage, device_record *device_entry);


int csl_ProcessHeartbeat (monitor_comms_t *comms, csl_zmessage *pcurrMessage, device_record *device_table_row);
//int                 csl_serializeMessageByte (csl_zmessage* message, byte* buffer);

/****************************************************************************************
 * CKN Functions
 * =============
 *
 * To work with and eventually replace all of the above functions
 *
 *
 ****************************************************************************************/

csl_complete_message    *csl_ConvertFromZMessage(csl_complete_message *new_message, csl_zmessage *z_source_message);
int                     csl_MessageGetType(csl_complete_message *in_message);

csl_scan_record         *csl_MessageToScanRecord(csl_message_body *in_message);
//csl_scan_record         *generateScanRecord(csl_scan_record *newRecord, int rec_ctr);
//csl_scan_record         *csl_MessageToScanRecord(csl_scan_record *out_record, csl_message_body *in_array);
//CS_2d_byte_array        *csl_ScanRecordToMessage(CS_2d_byte_array *out_array, csl_scan_record *in_record);
csl_message_body        *csl_ScanRecordToMessage(csl_message_body *message_out, csl_scan_record *in_record);
//csl_scan_record         *csl_MessageToScanRecord(csl_scan_record *out_record, CS_2d_byte_array *in_array);
void                    print_csl_scan_record(csl_scan_record *my_record, char* heading, int rev);

// **** Specialized Functions **** //
int csl_serializeMessageByte (csl_zmessage* message, byte* buffer);
int csl_deserializeMessageByte (csl_zmessage* message, byte* buffer);


/************************************************
 * DEBUG and other utilities - todo - REPLACE!!!!
 ************************************************/
//static void csl_print_message(csl_zmessage *message);
//static void csl_print_message_raw_bin_as_hex(byte *buffer);

char *csl_get_probe_event_name(int probe_event_index);

#endif //CRYTICAMONITOR_CSL_MESSAGE_H

