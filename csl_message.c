//
// Created by C. Kerry Nemovicher, Ph.D. on 6/20/20.
//

#include <time.h>
#include "csl_message.h"


static bool   G_scan_in_process;
static short  G_current_device_index;
static short  G_current_scan_ctr;
/************************************************
 * int monitor_comm_params *comm_params_initialize()
 *  @params
 *          monitor_comm_params *returnStruct
 *          unsigned short      broadcaster_port
 *          unsigned short      data_pipeline_port
 *          unsigned short      scan_port
 *
 *  @brief: This initializes the communication parameters needed by ZeroMQ. These include
 *          the communication ports and the names of the local files required
 *
 *  @return CS_SUCCESS
 ************************************************/
//monitor_comm_params *comm_params_initialize(monitor_comm_params *returnStruct,
int comm_params_initialize(monitor_comm_params *returnStruct,
                                            unsigned short broadcaster_port,
                                            unsigned short data_pipeline_port,
                                            unsigned short scan_port)
{
    sprintf(returnStruct->monitor_hostname,"%s", DEFAULT_HOST_NAME);
    sprintf(returnStruct->public_cert_file_name,"%s", DEFAULT_HOST_NAME);
    sprintf(returnStruct->broadcasting_address,"%d", broadcaster_port);
    sprintf(returnStruct->responder_address,"%d", data_pipeline_port);
    sprintf(returnStruct->data_pipeline_address,"%d", data_pipeline_port);
    sprintf(returnStruct->scan_address,"%d", scan_port);

    G_scan_in_process     = false;
    G_current_device_index = -1;

    return CS_SUCCESS;
//    return returnStruct;
}

/************************************************
 *  short csl_zmessage_get()
 *  @params
 *          csl_zmessage            *current_zmessage
 *          monitor_comms_t         *comms
 *          csl_complete_message    *cs_message
 *
 *  @brief  This is the main messaging function. It runs in a loop until there is a "return condition". Each
 *          loop queries the ZeroMQ functions and returns either with a new message or not. If not, it continues
 *          to loop until a message is received. When a message is received, that message is processed.
 *
 *          There are two "message states"
 *              Scan Processing
 *              Non-Scan Processing
 *
 *          During Non-Scan Processing the possible message types are:
 *              Message from an authorized device:  log an alert and then ignore all further communications for the device
 *              PROBE_HANDSHAKE:                    return the handshake message to the calling routine
 *              PROBE_HEARTBEAT:                    return the heartbeat message to the calling routine
 *          During the Scan Processing the possible message types are:
 *              PROBE_START_SCAN
 *
 *
 *  @note   This is the main messaging function. It runs in a loop until there is a "return condition"
 *          Unlike most subroutines, and certainly in violation of "one entry point - one exit point" for
 *          subroutines, csl_zmessage_get() "returns" from a number of different locations.
 *
 *
 ***********************************************/

short csl_zmessage_get(csl_zmessage *current_zmessage, monitor_comms_t *comms, csl_complete_message *cs_message)
{
    // Set up the communications buffers
//    csl_message current_message;
//    byte buffer[CSL_MAX_MESSAGE_LENGTH] = {0x00};



//    int buff_size = sizeof(buffer);
    unsigned long long msg_bandwidth_bytes = 0L;

//    int debug_ctr   = 0;
    while (! zsys_interrupted)
    {
        byte *data;     // **** This is INTENTIONALLY NOT initialized. If we do, it causes a memory leak **** //

        memset(current_zmessage, 0, sizeof(csl_zmessage));       // todo check to see if we want a null character here

        // Switch between every responder (probe handshake) and scan_receiver every millisecond
        //todo: We need weights on the sockets so we prioritize things like heartbeats.
        zsock_t *which = (zsock_t *) zpoller_wait(comms->poller, ZMQ_POLL_MSEC);

        // If there is no data on the current socket, continue to loop through the other sockets
        if (which == NULL)
        {
            if (zpoller_terminated(comms->poller))
            {
                zsys_error("CryticaMonitor - main - Process terminated");
                break;
            }
            else if(zpoller_expired(comms->poller))
            {
                //               debug_ctr++;
                //               printf("Waiting on Probe [%d]\n", debug_ctr);
                continue;
            }
        }
        size_t msg_size = CSL_MAX_MESSAGE_LENGTH;       //was sizeof(csl_message);

        // Important note: The "b" here signals to czmq that we are expecting binary data.
        // And it won't try to append any null chars
        // or try and treat it like a c-string
        int resp = zsock_recv(which, "b", &data, &msg_size);

        if (resp == -1)
        {
            zsys_error("CryticaMonitor - main() - Failed to receive");
            continue; //We probably don't want to deserialize data that wasn't received properly
        }

        csl_deserializeMessageByte(current_zmessage, data);
        msg_bandwidth_bytes += (unsigned long long) current_zmessage->message_total_size;  //was msg_size; and earlier

        free(data);

//        csl_complete_message *cs_message = calloc(1, sizeof(csl_complete_message));
        cs_message = csl_ConvertFromZMessage(cs_message,current_zmessage);
// **** Check to provenance of the message ****
        int device_index = messageCheckOrigin();
        if (device_index == CS_DEVICE_UNKNOWN)
        {
            // If bad provenance, we issue an error code in the messageCheckOrigin() function and then continue the while loop

            // The following is a kluge - We need to respond to the bad device so that the monitor can continue
            csl_AcknowledgeHandshake(current_zmessage, comms);
            return CS_DEVICE_UNKNOWN;
        }
        // add the device index to the current cs_message
        cs_message->message_body.message_scan.device_index = device_index;

#ifndef NDEBUG
//        csl_print_message(current_zmessage);
#endif

        if (G_scan_in_process == false && which == comms->responder) // We do not want to drop in here in the middle of a scan
        {
            //Check probe event to determine what to do
            switch (cs_message->message_header.message_type)
            {
                case (PROBE_HANDSHAKE):
                    break;


                case PROBE_HEARTBEAT:
                {
                    //todo: Heartbeat needs it's own thread/socket. This solution gets overwhelemd, but is functional
                    //                   process_Heartbeat(comms, &current_zmessage, &device_table);
                    if (G_scan_in_process == true && cs_message->message_body.message_scan.device_index != G_current_device_index)
                    {
                        cs_message->message_header.message_type = PROBE_WAITING;
                    }
                    break;
                }

                default:
                {
                    fprintf(stderr, "Received unregistered probe event! Is probe a verified Crytica Probe?\n");
                    printf("\t<%s> Received unregistered probe event[%d]! Is probe a verified Crytica Probe?\n",
                           __PRETTY_FUNCTION__, cs_message->message_header.message_type);
                    break;
                }
            }
            return (short) cs_message->message_header.message_type;

        }

        if (which == comms->scan_receiver)
        {
            if (G_scan_in_process == true && cs_message->message_body.message_scan.device_index != G_current_device_index)
            {
                continue;
            }
            else
            {
                switch (cs_message->message_header.message_type)
                {
                    case PROBE_START_SCAN:
                        if (G_scan_in_process == true)
                        {
                            break;      // We don't want to start a new scan while one is in process
                        }
                        G_scan_in_process = true;
                        time_t start_time = time(NULL);
                        char *time_string = csl_Time2String(start_time);
                        G_current_device_index = (short) cs_message->message_body.message_scan.device_index;

                        zsys_info("\t<%s> Scan Started  at %s for probe_id: %.4f, hostname: %s, ip: %s",
                                  __PRETTY_FUNCTION__, time_string,
                                  cs_message->message_body.message_scan.probe_id,
                                  current_zmessage->hostname, current_zmessage->probe_ip);
                        free(time_string);
                        /**********
                        zsys_info("Scan started at %ld for probe_id: %u, hostname: %s, ip: %s", start_time,
                                  current_zmessage->probe_id, current_zmessage->hostname, current_zmessage->probe_ip,
                                  csl_Time2String(start_time));
                         **********/
                        // **** Zero Out the Scan Table and prepare it to start anew **** //
                        if (scanTableInitialize(G_current_device_index) != 0)
                        {
                            printf("\t<%s> Failed to initial scan table for device[%d]",
                                   __PRETTY_FUNCTION__, G_current_device_index);
                        }
                        G_current_scan_ctr = 0;
                        break;

                    case PROBE_RECURRING_SCAN:  // **** Build the scan record and add it to the scan_table **** //
                        if (G_scan_in_process != true)
                        {
                            break;      // We don't want to process a scan while one is not in process
                        }
/**** Debug Code ****/
                        if (cs_message->message_body.message_scan.device_index != G_current_device_index)
                        {
                            printf("\t<%s> Wrong Device[%d] in the midst of [%d] device scan\n",
                                   __PRETTY_FUNCTION__,
                                   cs_message->message_body.message_scan.device_index, G_current_device_index);
                            //break;
                        }
/**********************/
                        csl_scan_record *new_scan = csl_MessageToScanRecord(&cs_message->message_body);
                        scanTableAddRow(new_scan, (short) cs_message->message_body.message_scan.device_index);
                        G_current_scan_ctr++;
                        free(new_scan);
                        break;

                    case PROBE_END_SCAN:
                        if (G_scan_in_process != true)
                        {
                            break;      // We don't want to end a scan before one is started
                        }
                        start_time = time(NULL);
                        time_string = csl_Time2String(start_time);
                        zsys_info("\t<%s> Scan Finished at %s for probe_id: %.4f, hostname: %s, ip: %s\n",
                                  __PRETTY_FUNCTION__, time_string,
                                  cs_message->message_body.message_scan.probe_id,
                                  current_zmessage->hostname, current_zmessage->probe_ip);
//                        printf("\t<%s> Finished Scan of [%d] records for device [%d]\n",
//                               __PRETTY_FUNCTION__, G_current_scan_ctr, G_current_device_index);
                        G_scan_in_process = false;
                        G_current_device_index = -1;
                        return SCAN_RECEIVED;
                        break;

                    default:
                        fprintf(stderr, "Received unregistred probe event! Is probe a Crytica Probe?\n");
                        break;

                } //end switch
            }
        } //endif
#ifdef NDEBUG
        zsys_debug("Bandwidth usage: ~%u (KB) or ~%u (MB)", msg_bandwidth_bytes / 1000, msg_bandwidth_bytes / 1000000);
#endif
    }   // end while
    printf("\n\t<%s> We should NEVER get here!\n", __PRETTY_FUNCTION__ );
    return CS_FATAL_ERROR;          // We should NEVER get here
}

monitor_comms_t* monitor_comms_new(int broadcaster_port, int responder_port, int scan_port, char* hostname_in)
{
    assert(broadcaster_port >= 1 && broadcaster_port <= 65535);
    assert(responder_port >= 1 && responder_port <= 65535);
    assert(scan_port >= 1 && scan_port <= 65535);

    monitor_comms_t *self = (monitor_comms_t*) malloc(sizeof(monitor_comms_t));

    self->auth = zactor_new(zauth, NULL);
    assert(self->auth);

#ifndef NDEBUG
    zstr_sendx (self->auth, "VERBOSE", NULL);
    zsock_wait (self->auth);
#endif

    // TODO: These should be statically linked libraries libsodium, libzmq, libczmq to maintain portability
    if (! zsys_has_curve()) {
        zsys_error("monitor - communications - System does not support CurveMQ, network encryption might not function properly");
        zsys_error("monitor - communications - Is libsodium installed?");
    }

    char *hostname = zsys_hostname();
    if (hostname == NULL)
    {
        strcpy(hostname, hostname_in);
    }
    strcpy(self->public_cert_file_name, hostname);
    // czmq appends "_private" to the keyname
    strcpy(self->private_cert_file_name, hostname);
    strcat(self->private_cert_file_name, "_secret");
    zstr_free(&hostname);

    if (zsys_file_exists(self->public_cert_file_name) && zsys_file_exists(self->private_cert_file_name))
    {
        self->server_cert = zcert_load(self->public_cert_file_name);
#ifndef NDEBUG
        zcert_print(self->server_cert);
        zsys_info("monitor - communications - loading ECDH from file");
#endif
    }
    else
    {
        self->server_cert = zcert_new();
        zcert_save(self->server_cert, self->public_cert_file_name);

#ifndef NDEBUG
        zsys_info("monitor - communications - generating and saving ECDH keys");
#endif
    }

    assert(self->server_cert);

    self->monitor_public_key = zcert_public_txt(self->server_cert);
    assert(self->monitor_public_key);

    // printf("Probe Registration Key:%s\n", self->monitor_public_key); // Kerry moved to csm_main.c

    zcert_save_public (self->server_cert, MONITOR_PUB_KEY_FILE_NAME);

    // ZeroMQ takes the format "tcp://address:port" or "tcp://network_interface_name:port" or "tcp://*:port"
    sprintf(self->broadcasting_address, "tcp://*:%u", broadcaster_port);
    sprintf(self->responder_address, "tcp://*:%u", responder_port);
    sprintf(self->scan_address, "tcp://*:%u", scan_port);

    zstr_sendx (self->auth, "CURVE", CURVE_ALLOW_ANY, NULL);
    zsock_wait (self->auth);

    // We will not BIND the probe command broadcaster until later because of the "slow joiner problem"
    self->broadcaster = zsock_new(ZMQ_PUB);
    zcert_apply(self->server_cert, self->broadcaster);
    zsock_set_curve_server(self->broadcaster, 1);
    zsock_set_zap_domain(self->broadcaster, "global");

    self->responder = zsock_new(ZMQ_REP);
    zcert_apply(self->server_cert, self->responder);
    zsock_set_curve_server(self->responder, 1);
    zsock_set_zap_domain(self->responder, "global");
    int resp_bind_success = zsock_bind(self->responder, "%s", self->responder_address);
    if (resp_bind_success != -1)
    {
        printf("\t<%s> **** resp_bind_success == %d\n", __PRETTY_FUNCTION__, resp_bind_success);
    }

    self->scan_receiver = zsock_new(ZMQ_PULL);
    zcert_apply(self->server_cert, self->scan_receiver);
    zsock_set_curve_server(self->scan_receiver, 1);
    zsock_set_zap_domain(self->scan_receiver, "global");
    zsock_set_rcvhwm (self->scan_receiver, RECEIVE_HIGH_WATER_MARK);
    int scan_bind_success = zsock_bind(self->scan_receiver, "%s", self->scan_address);
    if (scan_bind_success != -1)
    {
        printf("\t<%s> **** scan_bind_success == %d\n", __PRETTY_FUNCTION__, scan_bind_success);
    }

    self->poller = zpoller_new(self->responder, self->scan_receiver, NULL);
    assert(self->poller);

    return self;
}

int monitor_comms_destroy(monitor_comms_t* self)
{
    zactor_destroy(&self->auth);
    zcert_destroy(&self->server_cert);

    zsock_destroy(&self->broadcaster);
    zsock_destroy(&self->responder);
    zsock_destroy(&self->scan_receiver);
    zpoller_destroy(&self->poller);

    free (self);
    return CS_SUCCESS; // todo
}

/**** CKN Removed ... Never Used ****************
int monitor_comms_bind_command_publisher(monitor_comms_t* self)
{
    int bind_err = zsock_bind(self->broadcaster, self->broadcasting_address);
    assert(bind_err != -1);
    return bind_err;
}
*************************************************/

void monitor_comms_set_verbose_auth(monitor_comms_t *self, bool verbosity)
{
    self->verbose_auth = verbosity;

    if (self->verbose_auth) {
        zstr_sendx (self->auth, "VERBOSE", NULL);
        zsock_wait (self->auth);
    }
}

void monitor_comms_set_verbose_messaging(monitor_comms_t *self, bool verbosity)
{
    self->verbose_message = verbosity;
    // TODO: think zero mq has a built in socket monitor to show connected waiting etc
}

void csl_createNewMessage (csl_zmessage* message,
                           char license_key[LICENSE_KEY_LENGTH],
                           char file_name[PATH_MAX],
                           __mode_t file_attribute,
                           char hostname[HOST_NAME_MAX],
                           char probe_uuid[ZUUID_STR_LEN + 1],
                           char probe_ip[IFNAMSIZ],
                           unsigned char probe_mac_address[IFHWADDRLEN],
                           uint32_t probe_event,
                           uint32_t probe_id)
{

    //Clear message data
    memset(message, 0, CSL_MAX_MESSAGE_LENGTH);
    uint32_t message_size   = 0;

    //Add message data to fresh message
    if (license_key!= NULL)
    {
        strcpy(message->license_key, license_key);
    }
    message_size += LICENSE_KEY_LENGTH;

    if (file_name!= NULL)
    {
        strcpy(message->file_name, file_name);
        message->file_name_size = strlen(message->file_name);
        message_size    += message->file_name_size;

        MD5Module(message->file_name, message->hash,0);
    }
    else
    {
        message->file_name_size = 0;
        memset(message->hash, NULL_BINARY, SIZE_HASH_ELEMENT_Z);
    }
    message_size    += (SIZE_HASH_ELEMENT_Z + sizeof(uint32_t));       // because we write the hash whether or not the filename exists

    if (hostname!= NULL)
    {
        memset(message->hostname, NULL_BINARY, HOST_NAME_MAX);
        memcpy(message->hostname, hostname, HOST_NAME_MAX);
    }
    message_size    += HOST_NAME_MAX;

    if (probe_uuid!= NULL)
    {
        memset(message->probe_uuid, NULL_BINARY, ZUUID_STR_LEN + 1);
        memcpy(message->probe_uuid, probe_uuid, ZUUID_STR_LEN + 1);
    }
    message_size    += ZUUID_STR_LEN + 1;

    if (probe_ip!= NULL)
    {
        memset(message->probe_ip, NULL_BINARY, IFNAMSIZ);
        memcpy(message->probe_ip, probe_ip, IFNAMSIZ);
    }
    message_size    += IFNAMSIZ;

    if (probe_mac_address!= NULL)
    {
        memset(message->probe_mac_address, NULL_BINARY, IFHWADDRLEN);
        memcpy(message->probe_mac_address, probe_mac_address, IFHWADDRLEN);
    }
    message_size    += IFHWADDRLEN;

    message->probe_event    = probe_event;
    message_size            += sizeof(uint32_t);

    message->probe_id       = probe_id;
    message_size            += sizeof(uint32_t);

    message->file_attribute = file_attribute;
    message_size            += sizeof(mode_t);

//    message->message_total_size = CSL_MAX_MESSAGE_LENGTH - FILENAME_MAX + message->file_name_size;
    message_size            += sizeof(size_t);
    message->message_total_size = message_size;
 //   printf("message_size[%d] and message_total_size[%lu]\n", message_size, message->message_total_size);
}


int csl_ProcessHeartbeat (monitor_comms_t *comms, csl_zmessage *pcurrMessage, device_record *device_table_row)
{
    int return_flag = CS_SUCCESS;
    byte *buffer    = calloc(CSL_MAX_MESSAGE_LENGTH, sizeof(byte));

//  TODO: Heartbeat needs it's own thread/socket. This solution gets overwhelmed, but is functional.  Comment from Madi
#ifdef NDEBUG
    fprintf(stderr, "Received heartbeat \u2665\n");
#endif

    device_table_row->last_heartbeat = zclock_mono();
//    registerHeartbeat(&device->probeTable, pcurrMessage->probe_id);

    if (device_table_row->currently_scanning == false )
    {
//        device_table[device_index].currently_scanning = true;
        pcurrMessage->probe_event = PROBE_RECURRING_SCAN;
    }
    else
    {
        pcurrMessage->probe_event = PROBE_READY;
    }

    csl_serializeMessageByte(pcurrMessage, buffer);

    if (zsock_send(comms->responder, "b", buffer, pcurrMessage->message_total_size))
    {
#ifndef DEBUG
        printf("\t<%s> WARNING: For device_index[%d] - Couldn't beat back </3\n", __PRETTY_FUNCTION__,
               device_table_row->device_index);
        fprintf(stderr, "Couldn't beat back </3\n");
#endif
        return_flag = CS_ERROR;
    }
    else
    {
#ifndef DEBUG
        fprintf(stderr, "Beat back \u2665\n");
#endif
    }
    free(buffer);

    return return_flag;
}

#ifndef DEPRECATED

int csl_ProcessHeartbeat (monitor_comms_t *comms, csl_zmessage *pcurrMessage, device_record device_table[])
{
    byte *probe_long_mac_address = csl_MacAddressExpand(pcurrMessage->probe_mac_address);
    short device_index = deviceFindByDeviceIdentifier(probe_long_mac_address);
    free (probe_long_mac_address);

    int return_flag = CS_SUCCESS;
    if (device_index < 0)
    {
        return_flag = CS_DEVICE_NOT_FOUND;
        return return_flag;
    }

//    byte buffer[CSL_MAX_MESSAGE_LENGTH] = {0x00};
    byte *buffer = calloc(CSL_MAX_MESSAGE_LENGTH, sizeof(byte));

//  TODO: Heartbeat needs it's own thread/socket. This solution gets overwhelmed, but is functional.  Comment from Madi
#ifdef NDEBUG
    fprintf(stderr, "Received heartbeat \u2665\n");
#endif

    device_table[device_index].last_heartbeat = zclock_mono();
//    registerHeartbeat(&device->probeTable, pcurrMessage->probe_id);

    if (device_table[device_index].currently_scanning == false )
    {
//        device_table[device_index].currently_scanning = true;
        pcurrMessage->probe_event = PROBE_RECURRING_SCAN;
    }
    else
    {
        pcurrMessage->probe_event = PROBE_READY;
    }

    csl_serializeMessageByte(pcurrMessage, buffer);

    if (zsock_send(comms->responder, "b", buffer, pcurrMessage->message_total_size))
    {
//        fprintf(stderr, "Couldn't beat back </3\n");
    }
    else
    {
#ifndef DEBUG
        fprintf(stderr, "Beat back \u2665\n");
#endif
    }
    free(buffer);

    return return_flag;
}

#endif
int csl_RequestScan (monitor_comms_t *comms, csl_zmessage *pcurrMessage, device_record *device_entry)
{
    int return_flag = CS_SUCCESS;

//    byte buffer[CSL_MAX_MESSAGE_LENGTH] = {0x00};
    byte *buffer = calloc(CSL_MAX_MESSAGE_LENGTH, sizeof(byte));

//  TODO: Heartbeat needs it's own thread/socket. This solution gets overwhelmed, but is functional.  Comment from Madi
#ifndef NDEBUG
    fprintf(stderr, "Received heartbeat \u2665\n");
#endif

    device_entry->last_heartbeat = zclock_mono();
//    registerHeartbeat(&device->probeTable, pcurrMessage->probe_id);
    pcurrMessage->probe_event = PROBE_READY;

    csl_serializeMessageByte(pcurrMessage, buffer);

    if (zsock_send(comms->responder, "b", buffer, pcurrMessage->message_total_size))
    {
        fprintf(stderr, "Couldn't beat back </3\n");
    }
    else
    {
#ifndef DEBUG
        fprintf(stderr, "Beat back \u2665\n");
#endif
    }
    return return_flag;
}

#ifndef DEPRECATED
bool registerHeartbeat (csl_probe_table* ptable, uint32_t probeID)
{
    int lindx = csl_getProbeTableIndex(ptable, probeID);
    if (lindx != -1)
    {
        ptable->info[lindx].upd_date_ms = zclock_mono();
        return(true);
    }
    return(false);
}
#endif




/****************************************************************************************
 * CKN Functions
 * =============
 *
 * To work with and eventually replace all of the above functions
 *
 *
 ****************************************************************************************/


/************************************************
 * int csl_ConvertFromZMessage()
 *  @param
 *          csl_message *new_message
 *
 *  @brief  Function is to retrieve the next message in the messaging system's message queue
 *          It also calls upon csl_messageUnPack to decompose the message into its component parts
 *
 *  @author Kerry
 *
 * @return
 *          Success - pointer to the message structure csl_message
 *          Failure - NULL pointer
 ************************************************/
csl_complete_message *csl_ConvertFromZMessage(csl_complete_message *new_message, csl_zmessage *z_source_message)
{
    // **** Set up the new_message header info ****/
//    new_message->message_header = calloc(1, sizeof (csl_message_header));
    new_message->message_header.message_type   = z_source_message->probe_event;
    new_message->message_header.message_length = z_source_message->message_total_size;

    memset(new_message->message_header.from_address.device_ip_address,NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_message->message_header.from_address.device_ip_address,z_source_message->probe_ip, SIZE_IP4_ADDRESS);

    // **** Convert the short MAC address from the probe into the full MAC address for the monitor **** //
    byte *short_mac         = calloc(IFHWADDRLEN, sizeof(byte));
    memcpy(short_mac, z_source_message->probe_mac_address, IFHWADDRLEN);
    memset(new_message->message_header.from_address.device_identifier,NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    byte *long_mac          = csl_MacAddressExpand(short_mac);
    free(short_mac);
    memcpy(new_message->message_header.from_address.device_identifier, long_mac, SIZE_DEVICE_IDENTIFIER);

    memset(new_message->message_header.from_address.misc_info,NULL_BINARY, SIZE_HOST_NAME);
    memcpy(new_message->message_header.from_address.misc_info,z_source_message->hostname, SIZE_HOST_NAME);

    memset(new_message->message_header.to_address.device_ip_address,NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_message->message_header.to_address.device_ip_address,DEFAULT_IP4_ADDRESS, SIZE_IP4_ADDRESS);

    memset(new_message->message_header.to_address.device_identifier,NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    memcpy(new_message->message_header.to_address.device_identifier,DEFAULT_MAC_ADDRESS,SIZE_MAC_ADDRESS);

    strcpy(new_message->message_header.to_address.misc_info,CS_DATA_NOT_AVAILABLE);

    // **** create the scan record for the new_message->message_body **** //

    new_message->message_body.message_scan.scan_id             = (unsigned int)time(NULL);
    new_message->message_body.message_scan.probe_id            = z_source_message->probe_id;
    new_message->message_body.message_scan.element_type        = ELEMENT_EXEC_FILE;

    memset(new_message->message_body.message_scan.probe_uuid, NULL_BINARY, SIZE_ZUUID);
    memcpy(new_message->message_body.message_scan.probe_uuid, z_source_message->probe_uuid, SIZE_ZUUID);

    memset(new_message->message_body.message_scan.device_ip, NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_message->message_body.message_scan.device_ip, z_source_message->probe_ip,SIZE_IP4_ADDRESS);

    memset(new_message->message_body.message_scan.device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    memcpy(new_message->message_body.message_scan.device_mac_address, long_mac, SIZE_MAC_ADDRESS);
    free(long_mac);

    memset(new_message->message_body.message_scan.element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
 //   memcpy(new_message->message_body.message_scan.element_name, z_source_message->file_name, SIZE_ELEMENT_NAME);
    strcpy(new_message->message_body.message_scan.element_name, z_source_message->file_name);

    char hash_buffer[SIZE_HASH_NAME];
    MD5CharModule(new_message->message_body.message_scan.element_name, hash_buffer);
    memset(new_message->message_body.message_scan.element_name_hash, NULL_BINARY, SIZE_HASH_NAME);
    memcpy(new_message->message_body.message_scan.element_name_hash, hash_buffer, SIZE_HASH_NAME);

    memset(new_message->message_body.message_scan.scan_value, NULL_BINARY, SIZE_HASH_ELEMENT);
    memcpy(new_message->message_body.message_scan.scan_value, z_source_message->hash, SIZE_HASH_ELEMENT);

    new_message->message_body.message_scan.element_attributes  = z_source_message->file_attribute;
    new_message->message_body.message_scan.scan_date           = time(NULL);

    // **** We MUST add the additional message body fields, beyond just the scan record ****
    new_message->message_body.message_total_size                = z_source_message->message_total_size;
    memcpy(new_message->message_body.license_key, z_source_message->license_key, SIZE_LICENSE_KEY);
    memcpy(new_message->message_body.hostname, z_source_message->hostname, HOST_NAME_MAX);
    new_message->message_body.probe_event                       = z_source_message->probe_event;
//    print_csl_scan_record(new_record, "Debug", 1);

    return new_message;
}

#ifndef DEPRECATED
csl_complete_message *csl_ConvertFromZMessage(csl_complete_message *new_message, csl_zmessage *z_source_message)
{
    // **** Set up the new_message header info ****/
//    new_message->message_header = calloc(1, sizeof (csl_message_header));
    new_message->message_header.message_type   = z_source_message->probe_event;
    new_message->message_header.message_length = z_source_message->message_total_size;

    memset(new_message->message_header.from_address.device_ip_address,NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_message->message_header.from_address.device_ip_address,z_source_message->probe_ip, SIZE_IP4_ADDRESS);

    // **** Convert the short MAC address from the probe into the full MAC address for the monitor **** //
    byte *short_mac         = calloc(IFHWADDRLEN, sizeof(byte));
    memcpy(short_mac, z_source_message->probe_mac_address, IFHWADDRLEN);
 //   byte *tmp_identifier    = calloc(SIZE_DEVICE_IDENTIFIER, sizeof(byte));
 //   tmp_identifier          = csl_MacAddressExpand(short_mac);
    memset(new_message->message_header.from_address.device_identifier,NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    memcpy(new_message->message_header.from_address.device_identifier, csl_MacAddressExpand(short_mac), SIZE_DEVICE_IDENTIFIER);

    memset(new_message->message_header.from_address.misc_info,NULL_BINARY, SIZE_HOST_NAME);
    memcpy(new_message->message_header.from_address.misc_info,z_source_message->hostname, SIZE_HOST_NAME);

    memset(new_message->message_header.to_address.device_ip_address,NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_message->message_header.to_address.device_ip_address,DEFAULT_IP4_ADDRESS, SIZE_IP4_ADDRESS);

    memset(new_message->message_header.to_address.device_identifier,NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    memcpy(new_message->message_header.to_address.device_identifier,DEFAULT_MAC_ADDRESS,SIZE_MAC_ADDRESS);

    strcpy(new_message->message_header.to_address.misc_info,CS_DATA_NOT_AVAILABLE);

    // **** create the scan record for the new_message->message_body **** //
    csl_scan_record new_record;
    new_record.scan_id             = (unsigned int)time(NULL);
    new_record.probe_id            = z_source_message->probe_id;

    memset(new_record.probe_uuid, NULL_BINARY, SIZE_ZUUID);
    memcpy(new_record.probe_uuid, z_source_message->probe_uuid, SIZE_ZUUID);

    memset(new_record.device_ip, NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(new_record.device_ip, z_source_message->probe_ip,SIZE_IP4_ADDRESS);

    memset(new_record.device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    memcpy(new_record.device_mac_address, csl_MacAddressExpand(short_mac), SIZE_MAC_ADDRESS);

    memset(new_record.element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
    memcpy(new_record.element_name, z_source_message->file_name, SIZE_ELEMENT_NAME);

    char hash_buffer[SIZE_HASH_NAME];
    MD5CharModule(new_record.element_name, hash_buffer);
    memset(new_record.element_name_hash, NULL_BINARY, SIZE_HASH_NAME);
    memcpy(new_record.element_name_hash, hash_buffer, SIZE_HASH_NAME);

    memset(new_record.scan_value, NULL_BINARY, SIZE_HASH_ELEMENT);
    memcpy(new_record.scan_value, z_source_message->hash, SIZE_HASH_ELEMENT);

    new_record.element_attributes  = z_source_message->file_attribute;
    new_record.scan_date           = time(NULL);

    // **** add the scan record to the new_message->message_body **** //
    memset(&new_message->message_body, NULL_BINARY, sizeof( csl_message_body));
    memcpy(&new_message->message_body.message_scan, &new_record, sizeof(csl_scan_record));

    // **** ToDo - We MUST add the additional message body fields, beyond just the scan record
//    print_csl_scan_record(new_record, "Debug", 1);

    return new_message;
}

#endif
void print_csl_scan_record(csl_scan_record *my_record, char* heading, int rev)
{
    printf("\nPrinting %s csl_scan_record %d\n", heading, rev);
    printf("\tScan ID:             %u\n",  my_record->scan_id);
    printf("\tProbe ID:            %f\n",  my_record->probe_id);
    printf("\tProbe UUID:          %s\n",  csl_Byte2String(my_record->probe_uuid, SIZE_ZUUID));
    printf("\tDevice IP:           %s\n",  csl_Byte2String(my_record->device_ip, SIZE_IP4_ADDRESS));
    printf("\tDevice MAC Address:  %s\n",  csl_Byte2String(my_record->device_mac_address, SIZE_MAC_ADDRESS));
    printf("\tElement Name:        %s\n",  my_record->element_name);
    printf("\tElement Name Hash:   %s\n",  csl_Hash2String(my_record->element_name_hash, SIZE_HASH_NAME));
    printf("\tElement Scan Value:  %s\n",  csl_Hash2String(my_record->scan_value, SIZE_HASH_ELEMENT));
    printf("\tElement Attribs:     %u\n",  my_record->element_attributes);
    char       buf1[80];
    struct tm *ts;
    ts = localtime(&my_record->scan_date);
    strftime(buf1, sizeof(buf1), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    printf("\tScan Time:           %s\n\n",  buf1);

}

/************************************************
 * Message Specific Struct Conversion Functions
 * ============================================
 * CS_scan to/from 2D byte array functions
 ************************************************/

csl_message_body *csl_ScanRecordToMessage(csl_message_body *message_out, csl_scan_record *in_record)
{
    memcpy(&message_out->message_scan, in_record, sizeof(csl_scan_record));
    return message_out;
}

csl_scan_record *csl_MessageToScanRecord(csl_message_body *in_message)
{
    csl_scan_record *out_record = calloc(sizeof(csl_scan_record), sizeof(byte));
    memcpy(out_record, &in_message->message_scan, sizeof(csl_scan_record));
    return out_record;
}
#ifndef DEPRECATED
csl_scan_record    *csl_MessageToScanRecord(csl_scan_record *out_record, CS_2d_byte_array *in_array)
{
    /******* Debugging Commands *****************
    unsigned int test_scan_id  = csl_ByteArray2UInt(in_array->array_row[SCAN_ORDINAL_SCAN_ID]);
    unsigned int test_probe_id  = csl_ByteArray2UInt(in_array->array_row[SCAN_ORDINAL_PROBE_ID]);
    char *test_probe_uuid_id  = csl_ByteArray2String(in_array->array_row[SCAN_ORDINAL_PROBE_UUID]);
    char *test_device_ip  = csl_ByteArray2String(in_array->array_row[SCAN_ORDINAL_DEVICE_IP]);
    char *test_mac_address  = csl_ByteArray2String(in_array->array_row[SCAN_ORDINAL_DEVICE_MAC_ADDRESS]);
    char *test_name  = csl_ByteArray2String(in_array->array_row[SCAN_ORDINAL_ELEMENT_NAME]);
    unsigned int test_name_hash  = csl_ByteArray2UInt(in_array->array_row[SCAN_ORDINAL_ELEMENT_NAME_HASH]);
    unsigned int test_scan_value  = csl_ByteArray2UInt(in_array->array_row[SCAN_ORDINAL_SCAN_VALUE]);
    unsigned short test_attribs  = csl_ByteArray2UShort(in_array->array_row[SCAN_ORDINAL_ELEMENT_ATTRIBS]);
    ********************************************/

    if (in_array->number_of_rows != SCAN_RECORD_NUMBER_OF_FIELDS)
    {
        // TODO - Issue Error Warning
        return out_record;
    }

    CS_1d_byte_array *scan_id           = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_SCAN_ID);
    out_record->scan_id                 = csl_ByteArray2UInt(scan_id);

    CS_1d_byte_array *probe_id          = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_PROBE_ID);
    out_record->probe_id                = csl_ByteArray2UInt(probe_id);

    CS_1d_byte_array *probe_uuid        = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_PROBE_UUID);
    out_record->probe_uuid              = csl_ByteArray2UChar(probe_uuid);

    CS_1d_byte_array *device_ip         = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_DEVICE_IP);
    out_record->device_ip               = csl_ByteArray2UChar(device_ip);

    CS_1d_byte_array *mac_address       = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_DEVICE_MAC_ADDRESS);
    out_record->device_mac_address      = csl_ByteArray2UChar(mac_address);

    CS_1d_byte_array *element_name      = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_ELEMENT_NAME);
    out_record->element_name            = csl_ByteArray2String(element_name);

    CS_1d_byte_array *element_name_hash = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_ELEMENT_NAME_HASH);
    out_record->element_name_hash       = csl_ByteArray2UChar(element_name_hash);

    CS_1d_byte_array *scan_value        = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_SCAN_VALUE);
    out_record->scan_value              = csl_ByteArray2UChar(scan_value);

    CS_1d_byte_array *element_attribs   = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_ELEMENT_ATTRIBS);
    out_record->element_attributes      = csl_ByteArray2UShort(element_attribs);

    CS_1d_byte_array *scan_date         = csl_2dByteArrayRetrieveRow(in_array,SCAN_ORDINAL_SCAN_DATE);
    out_record->scan_date               = csl_ByteArray2Time(scan_date);

    /******* Debugging Commands *****************
    unsigned int test_scan_id1  = out_record->scan_id;
    unsigned int test_probe_id1  = out_record->probe_id;
    byte *test_probe_uuid_id1  = out_record->probe_uuid;
    byte *test_device_ip1  = out_record->device_ip;
    byte *test_mac_address1  = out_record->device_mac_address;
    char *test_name1  = out_record->element_name;
    unsigned int test_name_hash1  = out_record->element_name_hash;
    unsigned int test_scan_value1  = out_record->scan_value;
    unsigned short test_attribs1  = out_record->element_attributes;
    *********************************************/

    return out_record;
}
#endif

/***** Removed: Perhaps to come back when messages have more than one scan in them *****
CS_3d_byte_array *csl_MessageBody2ByteArray(CS_3d_byte_array *out_array, CS_2d_byte_array *in_message_body)
{
    out_array = calloc(1, sizeof(CS_3d_byte_array)),
    out_array->number_of_tables = in_message_body->number_of_rows;
    for (unsigned short i = 0; i < out_array->number_of_tables; i++)
    {
        out_array->array_table[i] = csl_2dByteArrayCopy2dArray(out_array->array_table[i],
                                                in_message_body->array_row[i]);
    }
    return out_array;
}
**********************************************************/

/************************************************
 * DEBUG and other utilities - todo - REPLACE!!!!
 ************************************************/

static void csl_print_message(csl_zmessage *message)
{
    if( message -> probe_event == PROBE_END_GOLD_STANDARD ||
        message -> probe_event == PROBE_END_RECURRING_SCAN )
    {

        printf("\n-----------csl_message--------------\n");

        printf("Message total byte size: %lu\n" , message->message_total_size);
        printf("File name size: %u\n", message->file_name_size);
        printf("File_name: %s\n", message->file_name);

        printf("Hash: ");
        for(int i = 0; i < 32; i ++)
        {
            printf("%c", (char) message->hash[i]);
        }
        printf("\n");

        printf("Probe_id (DB): %u - %s\n",
               message->probe_id, (message->probe_id == 0) ? "UNREGISTERED" : "");
        printf("Probe_uuid: %s\n", message->probe_uuid);
        printf("Probe_hostname: %s\n", message->hostname);
        printf("Probe_ip: %s\n", message->probe_ip);
        printf("Probe_mac_address: %02x:%02x:%02x:%02x:%02x:%02x\n",
               message->probe_mac_address[0],
               message->probe_mac_address[1],
               message->probe_mac_address[2],
               message->probe_mac_address[3],
               message->probe_mac_address[4],
               message->probe_mac_address[5]
        );
        printf("Probe_event: %i (%s)\n", message->probe_event, csl_get_probe_event_name(message->probe_event));
        printf("-----------end_csl_message----------\n");

    }
}

static void csl_print_message_raw_bin_as_hex(byte *buffer)
{
    int i;

    for (i = 0; i < (int) CSL_MAX_MESSAGE_LENGTH; i++)
    {
        if (i > 0) printf(":");
        if (i % 32 == 0) {
            printf("\n");
        }
        printf("%02X", buffer[i]);
    }
    printf("\n");
}

char *csl_get_probe_event_name(int probe_event_index)
{
    char *return_string = calloc(SIZE_PROBE_EVENT_NAME, sizeof(char));
    switch(probe_event_index)
    {
        case PROBE_READY:
            strcpy(return_string,"Ready");
            break;

        case PROBE_HANDSHAKE:
            strcpy(return_string, "Handshake");
            break;

        case PROBE_REGISTERED:
            strcpy(return_string, "Registered");
            break;

        case PROBE_GOLD_STANDARD:
            strcpy(return_string, "Crytica Standard");
            break;

        case PROBE_END_GOLD_STANDARD:
            strcpy(return_string, "End Crytica Standard");
            break;

        case PROBE_RECURRING_SCAN:
            strcpy(return_string, "Recurring Scan");
            break;

        case PROBE_END_RECURRING_SCAN:
            strcpy(return_string, "End Recurring Scan");
            break;

        case PROBE_END_SCAN:
            strcpy(return_string, "End Scan");
            break;

        case PROBE_START_SCAN:
            strcpy(return_string, "Start Scan");
            break;

        case PROBE_TERMINATE:
            strcpy(return_string, "Terminate");
            break;

        case PROBE_HEARTBEAT:
            strcpy(return_string, "Heartbeat");
            break;

        case PROBE_CONNECTION_DENIED:
            strcpy(return_string, "Probe Connection Denied");
            break;

        default:
            sprintf(return_string, "Unknown Probe Event Code [%d]", probe_event_index);
            break;

    }
    return return_string;
}

int csl_AcknowledgeHandshake(csl_zmessage *current_zmessage, monitor_comms_t *comms)
{
    int return_flag                     = CS_SUCCESS;
    byte buffer[CSL_MAX_MESSAGE_LENGTH] = {0x00};

    csl_zmessage response_msg;
#ifndef DEPRECATED
    csl_createNewMessage(&response_msg, current_zmessage->license_key,
                         NULL, 0, NULL,
                         NULL, NULL, NULL,
                         PROBE_REGISTERED, current_zmessage->probe_id);
#endif
    csl_createNewMessage(&response_msg, current_zmessage->license_key,
                         NULL, 0,                                        // filename and file attributes
                         current_zmessage->hostname, current_zmessage->probe_uuid,
                         current_zmessage->probe_ip, current_zmessage->probe_mac_address,
                         PROBE_REGISTERED, current_zmessage->probe_id);
    csl_serializeMessageByte(&response_msg, buffer);
    zsock_send(comms->responder, "b", &buffer, response_msg.message_total_size);

    return return_flag;
}

#ifndef DEPRECATED
CS_2d_byte_array   *csl_ScanRecordTo2dByteArray(CS_2d_byte_array *out_array, csl_scan_record *in_record)
{
//    CS_2d_byte_array *out_array             = calloc(1, sizeof(CS_2d_byte_array));
    out_array->number_of_rows               = 0;

    CS_1d_byte_array *scan_id               = csl_UInt2ByteArray(in_record->scan_id);
    out_array                               = csl_2dByteArrayAddRow(out_array, scan_id);
//    unsigned int test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *probe_id              = csl_UInt2ByteArray(in_record->probe_id);
    out_array                               = csl_2dByteArrayAddRow(out_array, probe_id);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *probe_uuid            = csl_UChar2ByteArray(in_record->probe_uuid, SIZE_ZUUID);
    out_array                               = csl_2dByteArrayAddRow(out_array, probe_uuid);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *device_ip             = csl_UChar2ByteArray(in_record->device_ip, SIZE_IP4_ADDRESS);
    out_array                               = csl_2dByteArrayAddRow(out_array, device_ip);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *device_mac_address    = csl_UChar2ByteArray(in_record->device_mac_address, SIZE_MAC_ADDRESS);
    out_array                               = csl_2dByteArrayAddRow(out_array, device_mac_address);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

//    CS_1d_byte_array *element_name          = calloc(1, sizeof(CS_1d_byte_array));
    CS_1d_byte_array *element_name          = csl_String2ByteArray(in_record->element_name);
    out_array                               = csl_2dByteArrayAddRow(out_array, element_name);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *element_name_hash     = csl_UChar2ByteArray(in_record->element_name_hash, SIZE_HASH_NAME);
    out_array                               = csl_2dByteArrayAddRow(out_array, element_name_hash);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *scan_value            = csl_UChar2ByteArray(in_record->scan_value, SIZE_HASH_ELEMENT);
    out_array                               = csl_2dByteArrayAddRow(out_array, scan_value);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *element_attributes    = csl_UShort2ByteArray(in_record->element_attributes);
    out_array                               = csl_2dByteArrayAddRow(out_array, element_attributes);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    CS_1d_byte_array *scan_date             = csl_Time2ByteArray(in_record->scan_date);
    out_array                               = csl_2dByteArrayAddRow(out_array, scan_date);
//    test_scan_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);

    //out_array1   = out_array;

/******* Debugging Commands *********************
    unsigned int test_scan_id1  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_ID]);
    unsigned int test_probe_id  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_PROBE_ID]);
    char *test_probe_uuid_id  = csl_ByteArray2String(out_array->array_row[SCAN_ORDINAL_PROBE_UUID]);
    char *test_device_ip  = csl_ByteArray2String(out_array->array_row[SCAN_ORDINAL_DEVICE_IP]);
    char *test_mac_address  = csl_ByteArray2String(out_array->array_row[SCAN_ORDINAL_DEVICE_MAC_ADDRESS]);
    char *test_name  = csl_ByteArray2String(out_array->array_row[SCAN_ORDINAL_ELEMENT_NAME]);
    unsigned int test_name_hash  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_ELEMENT_NAME_HASH]);
    unsigned int test_scan_value  = csl_ByteArray2UInt(out_array->array_row[SCAN_ORDINAL_SCAN_VALUE]);
*************************************************/
    return out_array;
}
#endif

/************************************************
 * Specialized Functions
 * =====================
 ************************************************/


//  4/23/20, GGS, replace CSL_MESSAGE_LENGTH with various message sizes
//                chg memcpy (several places) with dynamic sizes
//          CKN emended this code

int csl_serializeMessageByte (csl_zmessage* message, byte* buffer)
{
    int buffer_size = message->message_total_size;       //CSL_MESSAGE_LENGTH;
    size_t index = 0;

    memcpy(buffer+index, &message->message_total_size, sizeof(size_t));
    index += sizeof(size_t);

    memcpy(buffer+index, &message->file_name_size, sizeof(uint32_t));
    index += sizeof(uint32_t);

    //"\n" not serialized - add when deserializing
    memcpy(buffer + index, message->file_name, message->file_name_size);
    index += message->file_name_size;

    memcpy(buffer + index, message->hash, SIZE_HASH_ELEMENT_Z);
    index += SIZE_HASH_ELEMENT_Z;

    memcpy(buffer + index, message->probe_ip, SIZE_IP4_ADDRESS);
    index += SIZE_IP4_ADDRESS;

    memcpy(buffer + index, message->probe_mac_address, IFHWADDRLEN);
    index += IFHWADDRLEN;

    memcpy(buffer + index, message->probe_uuid, SIZE_ZUUID);
    index += SIZE_ZUUID;

    memcpy(buffer + index, &message->probe_event, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(buffer + index, message->hostname, SIZE_HOST_NAME);
    index += SIZE_HOST_NAME;

    memcpy (buffer + index, &message->probe_id, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(buffer + index, message->license_key, SIZE_LICENSE_KEY);
    index += SIZE_LICENSE_KEY;

    memcpy(buffer + index, &message->file_attribute, sizeof(message->file_attribute));
    index += sizeof(message->file_attribute);

    if (index != buffer_size)
    {
        message->message_total_size = index;
        memcpy(buffer+0, &message->message_total_size, sizeof(size_t));
        printf("WARNING! serialization index size [%lu] does not equal buffer size [%d]!\n",
               index, buffer_size);
//        fprintf(stderr,"WARNING! serialization index size does not equal buffer size! Data is being lost!\n");
    }

    return CS_SUCCESS;
}

int csl_deserializeMessageByte (csl_zmessage* message, byte* buffer)
{
//    int buffer_size = CSL_MESSAGE_LENGTH;
    size_t index = 0;

    memcpy(&message->message_total_size, buffer+index, sizeof(size_t));
//    printf("message_total_size [%lu]\n", message->message_total_size);
    index += sizeof(size_t);

    uint32_t buffer_size = message->message_total_size;

    memcpy(&message->file_name_size, buffer+index, sizeof(uint32_t));
    index += sizeof(uint32_t);
//    message->file_name  = calloc(message->file_name_size + 1, sizeof (char));
    memset( message->file_name, NULL_BINARY, SIZE_ELEMENT_NAME);
    memcpy( message->file_name, buffer + index, message->file_name_size);
    message->file_name[message->file_name_size] = END_OF_STRING; //nullterm is not sent in msg, so ensure safely terminated strings
    index += message->file_name_size;

    memcpy( message->hash, buffer+index, SIZE_HASH_ELEMENT_Z);
    index += SIZE_HASH_ELEMENT_Z;

    memcpy(message->probe_ip, buffer + index, SIZE_IP4_ADDRESS);
    index += SIZE_IP4_ADDRESS;

/**** This is future code for use of our MAC address ****
    memcpy(message->probe_mac_address, buffer + index, SIZE_MAC_ADDRESS);
    index += SIZE_MAC_ADDRESS;
 ********************************************************/
    memcpy(message->probe_mac_address, buffer + index, IFHWADDRLEN);
    index += IFHWADDRLEN;

    memcpy(message->probe_uuid, buffer + index, SIZE_ZUUID);
    index += SIZE_ZUUID;

    memcpy(&message->probe_event, buffer + index, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(message->hostname, buffer + index, SIZE_HOST_NAME);
    index += SIZE_HOST_NAME;

    memcpy (&message->probe_id, buffer + index, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(message->license_key, buffer + index, SIZE_LICENSE_KEY);
    index += SIZE_LICENSE_KEY;

    memcpy(&message->file_attribute, buffer + index, sizeof(message->file_attribute));
    index += sizeof(message->file_attribute);

    if (index != buffer_size)
    {
        fprintf(stderr, "WARNING! de-serialization index size does not equal buffer size! Data is being lost!\n");
    }

    return CS_SUCCESS;
}

#ifndef DEPRECATED

//  4/23/20, GGS, replace CSL_MESSAGE_LENGTH with various message sizes
//                chg memcpy (several places) with dynamic sizes

int csl_serializeMessageByte (csl_zmessage* message, byte* buffer)
{
    int buffer_size = message->message_total_size;       //CSL_MESSAGE_LENGTH;
    int index = 0;

    memcpy(buffer+index, &message->message_total_size, sizeof(message->message_total_size));
    index += sizeof(message->message_total_size);


    memcpy(buffer+index, &message->file_name_size, sizeof(message->file_name_size));
    index += sizeof(message->file_name_size);

    //"\n" not serialized - add when deserializing
    memcpy(buffer + index, message->file_name, message->file_name_size);
    index += message->file_name_size;

    memcpy(buffer + index, message->hash, sizeof(message->hash));
    index += sizeof(message->hash);

    memcpy(buffer + index, message->probe_ip, sizeof(message->probe_ip));
    index += sizeof(message->probe_ip);

    memcpy(buffer + index, message->probe_mac_address, sizeof(message->probe_mac_address));
    index += sizeof(message->probe_mac_address);

    memcpy(buffer + index, message->probe_uuid, sizeof(message->probe_uuid));
    index += sizeof(message->probe_uuid);

    memcpy(buffer + index, &message->probe_event, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(buffer + index, message->hostname, sizeof(message->hostname));
    index += sizeof(message->hostname);

    memcpy (buffer + index, &message->probe_id, sizeof(uint32_t));
    index += sizeof(uint32_t);


    memcpy(buffer + index, message->license_key, sizeof(message->license_key));
    index += sizeof(message->license_key);


    if (index != buffer_size) {
        fprintf(stderr,"ERROR! serialization index size does not equal buffer size! Data is being lost!\n");
    }


    return 0;
}

int csl_deserializeMessageByte (csl_zmessage* message, byte* buffer)
{
//    int buffer_size = CSL_MESSAGE_LENGTH;
    int index = 0;

    memcpy(&message->message_total_size, buffer+index, sizeof(message->message_total_size));
    index += sizeof(message->message_total_size);

    uint32_t buffer_size = message->message_total_size;

    memcpy(&message->file_name_size, buffer+index, sizeof(message->file_name_size));
    index += sizeof(message->file_name_size);

    memcpy( message->file_name, buffer + index, message->file_name_size);
    message->file_name[message->file_name_size] = '\0'; //nullterm is not sent in msg, so ensure safely terminated strings
    index += message->file_name_size;


    memcpy( message->hash, buffer+index, sizeof(message->hash));
    index += sizeof(message->hash);

    memcpy(message->probe_ip, buffer + index, sizeof(message->probe_ip));
    index += sizeof(message->probe_ip);

    memcpy(message->probe_mac_address, buffer + index, sizeof(message->probe_mac_address));
    index += sizeof(message->probe_mac_address);

    memcpy(message->probe_uuid, buffer + index, sizeof(message->probe_uuid));
    index += sizeof(message->probe_uuid);

    memcpy(&message->probe_event, buffer + index, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(message->hostname, buffer + index, sizeof(message->hostname));
    index += sizeof(message->hostname);

    memcpy ( &message->probe_id, buffer + index, sizeof(uint32_t));
    index += sizeof(uint32_t);

    memcpy(message->license_key, buffer + index, sizeof(message->license_key));
    index += sizeof(message->license_key);

    if (index != buffer_size)
    {
        fprintf(stderr, "ERROR! de-serialization index size does not equal buffer size! Data is being lost!\n")
    }


    return 0;
}

#endif

