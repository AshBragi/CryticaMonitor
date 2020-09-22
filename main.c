/****************************************
 *	 Crytica Monitor
 *   =========================
 *
 *   Author: Kerry aka CKN
 *
 *   Original Create Date(Format: YYYY-MM-DD):
 *      2020-06-17
 *
 ****************************************/


/**** CS headers ****/
#include "CryticaMonitor.h"
#include "csl_message.h"
#include "csl_mysql.h"


/**** STUB HEADERS ****/
//#include "Stub_Comm.h"

/************************************************
 *  Local "Global Static" Variables
 *  ======================
 ************************************************/

/************************************************
 * Monitor Information Table
 * =========================
 * The monitor_info_table is where the monitor specific data are stored
 ************************************************/
static monitor_info_table G_monitor_table;

/************************************************
 * Status Quo Table
 * ================
 * The G_status_quo_table is where the most recent scan for each device is stored.
 *      There is one column (status_quo_column), an array of rows, per device
 *      Each column contains:
 *
 *      There is one row (status_quo_record) in each column for the data of a scanned element
 *
 ************************************************/
static status_quo_record G_status_quo_table[MAX_DEVICES][MAX_ELEMENTS];

/************************************************
 * Device Table
 * ============
 * The G_device_table has one record for each device being monitored
 * **********************************************/
static device_record G_device_table[MAX_DEVICES];

/************************************************
 * Zero MQ  & Message Globals
 * ===============
 * We need the ZMQ variables until we can uncouple Zero MQ completely from our main module
 ************************************************/
static csl_zmessage            G_current_zmessage;
static monitor_comms_t         *G_zmq_comms_t;
static csl_complete_message    G_current_cs_message;

/************************************************
 * Scan Table
 * ==========
 * Stores all the records of the "current" scan.
 * This table consists of:
 *      unsigned short      scan_element_ctr                - The number or scan table rows
 *      short               device_index                    - The device from which the scan was taken
 *      csl_scan_record     scan_elements[MAX_ELEMENTS]     - An array (rows) for scans, one per element
 ************************************************/
static scan_structure          G_scan_table;

/************************************************
 * MySQL Globals
 * =============
 * We need to maintain the context of the current MySQL database here in the main module - perhaps in the future
 * that will change
 ************************************************/
MYSQL                          *G_db_connection;

/************************************************
 * Global Variables for Status, Error, & Indices
 * =============================================
 *
 ************************************************/
static int                     G_error_status;
static short                   G_current_scan_device_index;

/************************************************
 * Interrupt Stop to "gracefully" exit
 * Not sure we need it ... // todo - research this
 ************************************************
volatile sig_atomic_t stop;

void sig_handler(int signum)
{
    stop = 1;
}
 **** End of Graceful Interrupt Code *************/

/************************************************
 * **********************************************
 *
 *		===========
 *		MAIN Module
 *		===========
 *
 * **********************************************
 ************************************************/
int main()
{

/**************************************
 *	One-time set up processes
 **************************************/

    printf("\n\t===================================================");
    printf("\n\tHello, World! A Crytica Monitor is watching you 00!");
    printf("\n\t===================================================\n\n");
    printf("\t%s\n", CRYTICA_MONITOR);
    printf("\t%s\n\n", CRYTICA_COPYRIGHT);

    // Initialize CZMQ zsys layer ... Must be called from "main"
    zsys_init();

    if (monitorInitialize() != true)
    {
        printf("\t<%s> **** ERROR: Monitor Failed to Initialize ****\n", __PRETTY_FUNCTION__);
        printf("\t\t=> Run Aborted! <=\n\n");
        return (CS_FATAL_ERROR);
    }


    if (configFileCreate(&G_monitor_table.comm_params) != true)
    {
        printf("\t<%s> **** ERROR: Monitor Failed to Create Config File ****\n", __PRETTY_FUNCTION__);
        printf("\t\t=> Run Aborted! <=\n\n");
        return (CS_FATAL_ERROR);
    }

/************************************************
  *      Main Loop
  *      =========
  *
  * 		The Main Loop is a message driven loop, i.e.,
  * 		    Wait to receive a message
  * 		    Process the message
  * 		    If all is fine, loop back to wait for the next message in the queue
  *
  *			In Version 1.0, the number of Probes/Device monitored will be fixed at 1
  *			(although we need to be prepared for situations where there may be more than 1).
  ***********************************************/

    // Initialize the status variables //
    bool good_run               = true;
    G_error_status              = CS_SUCCESS;
    short next_scan_index       = -1;
    short message_device_index  = -1;

    /********************************************
     * This is the start of the main "while" loop
     ********************************************/
    while (good_run == true)
    {
        // **** Check for next message from the message queue and return its type **** //
        short message_type  = csl_zmessage_get(&G_current_zmessage,
                                               G_zmq_comms_t,
                                               &G_current_cs_message);

        if (message_type == CS_FATAL_ERROR)
        {
            monitorShutDown();
            return (CS_FATAL_ERROR);
        }
        // Find the device_index for the device transmitting the message
        message_device_index = deviceFindByDeviceIdentifier(
              G_current_cs_message.message_header.from_address.device_identifier);
        if (message_device_index < 0)
        {
           // If the device is not known, ignore it. This error is dealt with elsewhere
//         good_run == false;      // Todo - THIS IS A KLUGE ... we MUST revisit this!
          continue;
        }

        // Provide the zmessage with the probe id
        G_current_zmessage.probe_id   = G_device_table[message_device_index].probe_id;
        
        // **** Primary Loop Switch - Switch statement is based upon the message type ****
        switch (message_type)
        {
            case PROBE_WAITING:
                break;

            case PROBE_HANDSHAKE:
                if (messageHandshakeProccess(&G_current_zmessage, G_zmq_comms_t) != CS_SUCCESS)
                {
                    printf("\t<%s> WARNING: Failed Handshake for device_index [%d]\n",
                           __PRETTY_FUNCTION__, message_device_index);
                }
                break;

            case PROBE_HEARTBEAT:
                good_run = messageHeartBeatProcess(message_device_index);
                if (good_run != true)
                {
                    printf("\t<%s> WARNING: Failed Heart Beat for device_index [%d]\n",
                           __PRETTY_FUNCTION__, message_device_index);
                }
                break;

            case PROBE_TIMEOUT:
                good_run = messageTimeoutProcess();
                break;

            case SCAN_RECEIVED: // Note: After a scan is received from a probe, a new scan is requested
                // Check for new Crytica standard
                if (cStandardNeeded() < 0)
                {
                    // todo throw error message and process the error
                    printf("\t<%s> **** ERROR: Failed DB Query for cStandardNeeded Function for device_index[%d]\n",
                           __PRETTY_FUNCTION__, message_device_index);
                    break;
                }
//                G_current_scan_device_index    = deviceFindByDeviceIdentifier(
//                        G_current_cs_message.message_header.from_address.device_identifier);
                G_current_scan_device_index = message_device_index;
                printf("\t<%s> Scan of [%d] records received from device [%d]\n",
                       __PRETTY_FUNCTION__, G_scan_table.scan_element_ctr, G_current_scan_device_index);
                good_run = scanEvaluate(G_current_scan_device_index);
                if (good_run)
                {
                    G_device_table[G_current_scan_device_index].currently_scanning = false;

 //                   next_scan_index = scan_get_next_index((short) (G_current_scan_device_index + 1));
 //                   messageHeartBeatProcess(next_scan_index);
                    messageHeartBeatProcess(G_current_scan_device_index);
                }
                break;

            case CS_END_OF_RUN:
                good_run    = false;
                break;

            default:
                messageUnknownProcess(message_type, message_device_index);
                good_run    = false;
                break;

        }    // End of switch(messageType())

        // Check for need to abort the run
        if (G_error_status == CS_FATAL_ERROR)
        {
            good_run  = false;
        }

    }	// End of main run (while) loop

/**************************************
 *	One-time shutdown processes
 **************************************/
// Perform all types of shutdown one-time, tasks ... handshakes with the DBs  ...
// notify Audit Ledger of Shutdown ...Flush DBs ... et cetera
    monitorShutDown();

    return EXIT_SUCCESS;
}
/************************************************
 *	End of Main Module
 ************************************************/



/************************************************
 * **********************************************
 *
 *		===========
 *		Support Functions
 *		===========
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
int alertDeviceWriteRecord(device_alert_record *alert_record)
{
    int         return_value    = CS_SUCCESS;
    char        mysql_insert[SIZE_CS_SQL_COMMAND];
    char        alert_data[SIZE_ALERT_DEVICE_DATA];
    char        *time_string    = csl_Time2String(alert_record->alert_date);

    sprintf(alert_data,"Sent From: [%s] [%s] - Sent To: [%s] [%s]",
            alert_record->device_from_ip_address, alert_record->device_from_info,
            alert_record->device_to_ip_address, alert_record->device_to_info);

    //   sprintf(mysql_insert, "Insert into cs_monitor.v_alert_log "
    sprintf(mysql_insert, "Insert into %s.%s "
                          "(monitor_id, device_identifier, probe_id, alert_type, alert_data, alert_process_date)"
                          "values (%llu, '%s', %f, %d, '%s', '%s')",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_ALERT_LOG_VIEW,
            alert_record->monitor_id,
            alert_record->device_identifier,
            csl_AssignProbeID(alert_record->monitor_id, 0),
            alert_record->alert_type,
            alert_data,
            time_string);
    return_value = csl_UpdateDB(G_db_connection, mysql_insert);

    free (time_string);
    return return_value;
}

/************************************************
 * bool alertOnDevice()
 *  @param
 *          byte    *device_identifier
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
bool alertOnDevice (byte *device_identifier, short alert_type)
{
    bool return_flag        = true;
    char *identifier_string = csl_Byte2String(device_identifier, SIZE_DEVICE_IDENTIFIER);

    // Build Basic Alert Message
    device_alert_record alert_record;
    alert_record.alert_type             = alert_type;
    alert_record.alert_date             = time(NULL);
    alert_record.monitor_id             = G_monitor_table.monitor_id;
    strcpy(alert_record.device_identifier, identifier_string);
    free(identifier_string);

    // Get device data from current_message
    if (memcmp(
            G_current_cs_message.message_header.from_address.device_identifier,
            device_identifier, SIZE_DEVICE_IDENTIFIER) != 0)
    {
        // Build "Bad Message" Alert
        strcpy(alert_record.device_from_ip_address, "**** Unknown: From IP Address ****");
        strcpy(alert_record.device_from_info, "**** Unknown: From Address Info ****");
        strcpy(alert_record.device_to_ip_address, "**** Unknown: To IP Address ****");
        strcpy(alert_record.device_to_info, "**** Unknown: To Address Info ****");
    } else
    {
        strcpy(alert_record.device_from_ip_address,
               G_current_cs_message.message_header.from_address.device_ip_address);
        strcpy(alert_record.device_from_info, G_current_cs_message.message_header.from_address.misc_info);
        strcpy(alert_record.device_to_ip_address,
               G_current_cs_message.message_header.to_address.device_ip_address);
        strcpy(alert_record.device_to_info, G_current_cs_message.message_header.to_address.misc_info);
    }

    if (alertDeviceWriteRecord(&alert_record) != CS_SUCCESS)
    {
        return_flag = false;
        // todo - Throw Error Message
    }
    return return_flag;
}

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
bool alertOnElementAddition(unsigned short device_index, csl_scan_record scan_element)
{
    /********************************************
     * Currently, this is essentially the same code as alertOnElementModification
     * Hence we can call it from here.
     * In the future, that might not be the case, so we have commented out the code, but
     * left a place to expand on it in the future
     ********************************************/

    return alertOnElementModification(device_index, scan_element, ALERT_ADD_ELEMENT);
#ifdef FUTURE_CODE // When we might want to have a different type of message for add versus modify
    bool return_flag = true;

    // Build the alert record and write it to the database
    scan_alert_record alert_record;
    alert_record.monitor_id        = G_monitor_table.monitor_id;
    alert_record.alert_type        = ALERT_ADD_ELEMENT;
    alert_record.alert_date        = time(NULL);       //i.e., time(Null) is "now"
    alert_record.element_type      = scan_element.element_type;
    alert_record.element_name      = calloc(strlen(scan_element.element_name), sizeof(char));
    strcpy(alert_record.element_name, scan_element.element_name);
    memcpy(alert_record.scan_value, scan_element.scan_value, SIZE_HASH_ELEMENT);
    alert_record.scan_date         = scan_element.scan_date;
    alert_record.device_id         = G_device_table[device_index].device_id;
    memset(alert_record.device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER+1);
    memcpy(alert_record.device_identifier, G_device_table[device_index].device_identifier, SIZE_DEVICE_IDENTIFIER);
    alert_record.scan_id           = scan_element.scan_id;
    alert_record.probe_id          = G_device_table->probe_id;
    if (alertScanWriteRecord(&alert_record) != CS_SUCCESS)
    {
        return_flag = false;
      // todo - Throw Error Message
    }
    free (alert_record.element_name);

    return return_flag;
#endif
}

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
bool alertOnElementDeletion (status_quo_record deleted_record, unsigned long long device_id, time_t scan_date)
{
    bool return_flag = true;
    // get the element name from the CryticaStandard DB record
    char *element_name = elementNameRetrieve(G_monitor_table.monitor_id,
                                                              device_id,
                                                              deleted_record.element_identifier);
    if (strcmp(element_name, ELEMENT_NAME_NOT_FOUND) == 0)
    {
        printf("\t<%s> Could not read element name for:\n", __PRETTY_FUNCTION__);
        printf("\t\t Monitor [%llu], Device [%llu], element_identifier[%s]\n\n",
               G_monitor_table.monitor_id, device_id, deleted_record.element_identifier);
    }
    else
    {   // Build the alert record and write it to the database
        scan_alert_record alert_record;
        alert_record.monitor_id        = G_monitor_table.monitor_id;
        alert_record.alert_type        = ALERT_DEL_ELEMENT;
        alert_record.alert_date        = time(NULL);       //i.e., time(NULL) is "now"
        alert_record.element_type      = deleted_record.element_type;
        memset(alert_record.element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
        strcpy(alert_record.element_name, element_name);
        memcpy(alert_record.scan_value, deleted_record.scan_value, SIZE_HASH_ELEMENT);
        alert_record.scan_date         = scan_date;
        alert_record.device_id         = G_device_table[G_current_scan_device_index].device_id;
        alert_record.probe_id          = csl_AssignProbeID(G_monitor_table.monitor_id, G_current_scan_device_index);
        memset(alert_record.device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER+1);
        memcpy(alert_record.device_identifier, G_device_table[G_current_scan_device_index].device_identifier, SIZE_DEVICE_IDENTIFIER);
        if (alertScanWriteRecord(&alert_record) != CS_SUCCESS)
        {
            return_flag = false;
            // todo - Throw Error Message
        }
    }
    free (element_name);
    return return_flag;
}

/************************************************
 * bool alertOnElementModification()
 *  @param
 *          unsigned short  device_index
 *          csl_scan_record scan_elements
 *          short           alert_type
 *
 *  @brief
 *          Populates the alert record fields when a scan anomaly is detected
 *          Sends the alert record to the database to be written
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool alertOnElementModification(unsigned short device_index, csl_scan_record scan_element, short alert_type)
{
    bool return_flag = true;

    // Build the alert record and write it to the database
    scan_alert_record alert_record;
    alert_record.monitor_id        = G_monitor_table.monitor_id;
    alert_record.alert_type        = alert_type;
    alert_record.element_type      = scan_element.element_type;
    memset(alert_record.element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
    strcpy(alert_record.element_name, scan_element.element_name);
    memcpy(alert_record.scan_value, scan_element.scan_value, SIZE_HASH_ELEMENT);
    alert_record.scan_date         = scan_element.scan_date;
    alert_record.alert_date        = time(NULL);
    alert_record.device_id         = G_device_table[device_index].device_id;
    memset(alert_record.device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER+1);
    memcpy(alert_record.device_identifier, G_device_table[device_index].device_identifier, SIZE_DEVICE_IDENTIFIER);
    alert_record.scan_id           = scan_element.scan_id;
    alert_record.probe_id          = csl_AssignProbeID(G_monitor_table.monitor_id, G_current_scan_device_index);
    if (alertScanWriteRecord(&alert_record) != CS_SUCCESS)
    {
        return_flag = false;
        // todo - Throw Error Message
    }
//    free (alert_record.element_name);

    return return_flag;
}

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
int alertScanWriteRecord (scan_alert_record *alert_record)
{
    int  return_value   = CS_SUCCESS;
    char mysql_insert[SIZE_CS_SQL_COMMAND];
    memset(mysql_insert, NULL_BINARY, SIZE_CS_SQL_COMMAND);
    char *time_string   = csl_Time2String(alert_record->alert_date);

    sprintf(mysql_insert, "Insert into %s.%s "
            "(monitor_id, device_identifier, device_id, probe_id, alert_type, element_type, element_name, alert_process_date)"
            " values "
            "(%llu, '%s', %llu, %f, %d, '%s', '%s', '%s')",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_ALERT_LOG_VIEW,
            alert_record->monitor_id,
            alert_record->device_identifier,
            alert_record->device_id,
            alert_record->probe_id,
            alert_record->alert_type,
            element_types[alert_record->element_type],
            alert_record->element_name,
            time_string);

    printf("\t<%s> Alert Scan Write:\n\t%s\n", __PRETTY_FUNCTION__, mysql_insert);

    return_value = csl_UpdateDB(G_db_connection, mysql_insert);
    if (return_value != CS_SUCCESS)
    {
        printf("\t<%s> **** ERROR: Failed to Insert in %s.%s with command:\n\t%s\n",
               __PRETTY_FUNCTION__, CS_SQL_MONITOR_SCHEMA, CS_SQL_ALERT_LOG_VIEW, mysql_insert);
    }

    free (time_string);
    return return_value;
}


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
 * @return CS_SUCCESS or an error code (yet to be determined)
 ************************************************/
bool configFileCreate(monitor_comm_params *comms)
{
    bool return_flag = true;

    // Greet the user
    printf("\n");
    printf("My name is: [%s] ... What's yours?\n\n",comms->monitor_hostname);
    printf("Hi, my IP addresses are:\n"
           "\tExternal [%s]\n"
           "\tInternal [%s]\n\n",comms->monitor_external_ip_address, comms->monitor_internal_ip_address);

    printf("Registration Key:     [%s]\n", comms->monitor_public_key);
    printf("Broadcast Address:    [%s]\n",comms->broadcasting_address);
    printf("Responder Address:    [%s]\n", comms->responder_address);
    printf("Scan Address:         [%s]\n\n", comms->scan_address);

    // Write the Internal IP config file
    FILE *monitorInConfig;
    char configFileName[strlen(comms->monitor_hostname) + strlen(FILE_MONITOR_INTERNAL_CONFIG) + 1];
    sprintf(configFileName,"%s_%s", comms->monitor_hostname, FILE_MONITOR_INTERNAL_CONFIG);
    monitorInConfig = fopen(configFileName, "w+");
    fprintf(monitorInConfig,"%s\n", comms->monitor_internal_ip_address);
    fprintf(monitorInConfig, "%s\n", comms->monitor_public_key);
    fprintf(monitorInConfig, "%02d\n", csl_get_last_octet(comms->monitor_internal_ip_address));
//    fprintf(monitorInConfig, "%02d\n", DEFAULT_PORT_PREFIX);
    fclose(monitorInConfig);

    // Write the External IP config file - if needed
    if (strcmp(comms->monitor_external_ip_address, DEFAULT_IP4_ADDRESS) != 0)
    {
        FILE *monitorExConfig;
//        char configFileName[strlen(comms->monitor_hostname) + strlen(FILE_MONITOR_EXTERNAL_CONFIG) + 1];
        sprintf(configFileName,"%s_%s", comms->monitor_hostname, FILE_MONITOR_EXTERNAL_CONFIG);
        monitorExConfig = fopen(configFileName, "w+");
        fprintf(monitorExConfig,"%s\n", comms->monitor_external_ip_address);
        fprintf(monitorExConfig, "%s\n", comms->monitor_public_key);
        fprintf(monitorExConfig, "%02d\n", csl_get_last_octet(comms->monitor_internal_ip_address));
        fclose(monitorExConfig);
    }

    return return_flag;
}

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
 *              G_device_table->cs_standard_flag = true   // the device needs a new Crytica Standard
 *
 *  @return Success - number of devices found
 *          Failure - ERROR Code
 ************************************************/
int    cStandardNeeded()
{
    int return_count = 0;

    // prepare the query statement and send the query to the database
    char mysql_query[SIZE_CS_SQL_COMMAND];
    // look for all devices assigned to this monitor, in which the c_standard_date is the default date
    sprintf(mysql_query,
            "Select distinct device_id, device_identifier from %s.%s "
            "where monitor_id = '%llu' and crytica_standard_date = '%s'",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW,
            G_monitor_table.monitor_id, DEFAULT_DATE);

    MYSQL_RES *result    = NULL;
    result = csl_QueryDB (G_db_connection, mysql_query);
    if (result == NULL)                       // query returned an error flag
    {
        // todo - issue error message on bad query result
        printf("\t<%s> **** ERROR: Failed DB Query:\n", __PRETTY_FUNCTION__ );
        printf("\t\t[%s]\n\n", mysql_query);
        return_count = CS_ERROR_DB_QUERY;
    }
    else
    {
        unsigned int row_count    = mysql_num_rows(result);
        if (row_count != 0)  // i.e., There ARE devices needing a new Crytica Standard
        {
            csl_monitor_device_record *monitor_device_row = csl_ReturnMonitorDeviceRows(G_db_connection, result);

            short device_index;
            // **** cycle through the found rows - should be one row per device needing new standard (for this monitor)
            for (unsigned int i = 0; i < row_count; i++)
            {
                device_index = deviceFindByDeviceID(monitor_device_row[i].device_id);
                if (device_index == CS_DEVICE_NOT_FOUND)
                {
                    // todo issue a fatal error and exit
                    printf("\t<%s> Could not find device_id [%llu] in G_device_table\n",
                    __PRETTY_FUNCTION__, monitor_device_row[i].device_id);
                }
                // If we have found the device, set the flag in the G_device_table
                G_device_table[device_index].cs_standard_flag = true;
                return_count++;
            }
            free(monitor_device_row);
        }
    }

    csl_mysql_free_result(result);
    return return_count;
}

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
        byte                *element_identifier)
{
    MYSQL_RES *result    = NULL;
    char mysql_query[SIZE_CS_SQL_COMMAND];
    char *identifier_string = csl_Hash2String(element_identifier, SIZE_HASH_NAME);

    sprintf(mysql_query,
            "Select element_type, element_name from %s.%s where "
            "Device_ID = %llu and Monitor_ID = %llu and element_identifier = '%s'",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_STANDARD_VIEW,
            device_id, monitor_id, identifier_string);
    result = csl_QueryDB (G_db_connection, mysql_query);
    cs_standard_record *standard_record = csl_ReturnStandardRow(G_db_connection, result);
    if (standard_record == NULL)
    {
        printf("\t<%s> **** ERROR: Could execute query [%s]\n", __PRETTY_FUNCTION__, mysql_query);
    }
    free(identifier_string);
    csl_mysql_free_result(result);
    return standard_record;
}

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
bool    cStandardWriteToDB(unsigned short device_index)
{
    bool return_flag    = true;

    // **** First Clean out the previous CS Standard table **** //
    char sql_command [SIZE_CS_SQL_COMMAND];
    memset (sql_command, NULL_BINARY, SIZE_CS_SQL_COMMAND);
    sprintf(sql_command, "Delete from %s.%s "
                         "where standard_id > 0 and monitor_id = %llu and device_id = %llu",
                         CS_SQL_MONITOR_SCHEMA, CS_SQL_STANDARD_VIEW,
                         G_monitor_table.monitor_id, G_device_table[device_index].device_id);
    int return_value = csl_UpdateDB (G_db_connection, sql_command);
    if (return_value != CS_SUCCESS)
    {
        printf("\t<%s> **** ERROR ****\n", __PRETTY_FUNCTION__);
        printf("     Failed to delete previous %s - Error Code[%d]\n", CS_SQL_STANDARD_VIEW, return_value);
        printf("     Insert String [%s]\n\n", sql_command);
        // todo issue error message
    }

    // **** And Clean out the previous Element Names View **** //
    memset (sql_command, NULL_BINARY, SIZE_CS_SQL_COMMAND);
    sprintf(sql_command, "Delete from %s.%s "
                         "where monitor_id = %llu and device_id = %llu",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_ELEMENT_ADDED_NAMES_VIEW,
            G_monitor_table.monitor_id, G_device_table[device_index].device_id);
    return_value = csl_UpdateDB (G_db_connection, sql_command);
    if (return_value != CS_SUCCESS)
    {
        printf("\t<%s> **** ERROR ****\n", __PRETTY_FUNCTION__);
        printf("     Failed to delete previous [%s] - Error Code[%d]\n", CS_SQL_ELEMENT_ADDED_NAMES_VIEW, return_value);
        printf("     Insert String [%s]\n\n", sql_command);
        // todo issue error message
    }

    // **** Write New CS Standard ****
    cs_standard_record  new_record;

    // initialize the static fields, those common to this entire Crytica Standard
    new_record.cs_monitor_id     = G_monitor_table.monitor_id;
    new_record.cs_device_id      = G_device_table[device_index].device_id;
    new_record.cs_date           = time(NULL);       //i.e., time(Null) is "now"
    new_record.cs_standard_type  = CS_CODE_SELF_DEFINED;

    // **** Cycle through the device table for this device ****
    for (int i = 0; i < G_scan_table.scan_element_ctr; i++)
    {
        new_record.cs_element_type         = G_scan_table.scan_elements[i].element_type;
        memcpy(new_record.cs_element_identifier, G_scan_table.scan_elements[i].element_name_hash, SIZE_HASH_NAME);
        new_record.cs_element_name         = calloc(strlen(G_scan_table.scan_elements[i].element_name) + 1,sizeof(char));
        strcpy(new_record.cs_element_name, G_scan_table.scan_elements[i].element_name);
        memcpy(new_record.cs_scan_value, G_scan_table.scan_elements[i].scan_value, SIZE_HASH_ELEMENT);

        if (cStandardWriteRecord(&new_record) != CS_SUCCESS)
        {
            // todo - issue error failed to write new Standard
            return_flag = false;
        }
        free (new_record.cs_element_name);
    }

    // **** Reset the flag to ****
    char *current_time   = csl_Time2String(time(NULL));
    memset (sql_command, NULL_BINARY, SIZE_CS_SQL_COMMAND);
    sprintf(sql_command, "Update %s.%s set crytica_standard_date = '%s' "
                         "where monitor_id = %llu and device_id = %llu",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW,
            current_time,
            G_monitor_table.monitor_id, G_device_table[device_index].device_id);
    free (current_time);
    if (csl_UpdateDB(G_db_connection, sql_command) != CS_SUCCESS)
    {
        // todo - issue error message failed to update monitor-device date
        return_flag = false;
    }

    return return_flag;
}


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
int cStandardWriteRecord(cs_standard_record *new_record)
{
    char mysql_insert[SIZE_CS_SQL_COMMAND];
    char *time_string               = csl_Time2String(new_record->cs_date);
    char *element_identifier_string = csl_Hash2String(new_record->cs_element_identifier, SIZE_HASH_NAME);
    char *scan_value_string         = csl_Hash2String(new_record->cs_scan_value, SIZE_HASH_ELEMENT);

    // **** Prepare the "Query" ****
    sprintf(mysql_insert, "Insert into %s.%s "
                          "(standard_type, standard_date, monitor_id, device_id,"
                          " element_type, element_identifier, element_name, scan_value)"
                          " values "
                          "(%d, '%s', %llu, %llu,"
                          " %d, '%s', '%s', '%s')",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_STANDARD_TABLE,
            new_record->cs_standard_type,
            time_string,
            new_record->cs_monitor_id,
            new_record->cs_device_id,

            new_record->cs_element_type,
            element_identifier_string,
            new_record->cs_element_name,

            scan_value_string);

    // **** Submit the Query ****
    int return_value = csl_UpdateDB (G_db_connection, mysql_insert);

    // **** Check the results ****
    if (return_value != CS_SUCCESS)
    {
        printf("**** ERROR ****\n");
        printf("     Failed to write Crytica Standard to DB - Error Code[%d]\n", return_value);
        printf("     Insert String [%s]\n\n", mysql_insert);
        // todo issue error message
    }

    free(time_string);
    free(element_identifier_string);
    free(scan_value_string);

    return return_value;
}


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
long long deviceAssignedToMonitor(byte *device_identifier, byte *device_mac_address)
{
    long long device_id     = (long long) CS_DEVICE_NOT_FOUND;
    MYSQL_RES *result       = NULL;
    char mysql_query[SIZE_CS_SQL_COMMAND];
    char *identifier_string = csl_Byte2String(device_identifier, SIZE_DEVICE_IDENTIFIER);

    sprintf(mysql_query,
            "Select Device_ID, device_identifier from %s.%s "
            "where Device_Identifier = '%s' and Monitor_ID = %llu",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW,
            identifier_string, G_monitor_table.monitor_id);
    free (identifier_string);

    result = csl_QueryDB (G_db_connection, mysql_query);
    if (result != NULL)
    {
        int return_row_ctr = mysql_num_rows(result);
        if (return_row_ctr != 1)
        {
            printf("\t<%s> ERROR, too many rows returned for query on [%s] [%llu]\n",
                   __PRETTY_FUNCTION__, device_identifier, G_monitor_table.monitor_id);
        }
        else
        {
            csl_monitor_device_record *monitor_device_row = csl_ReturnMonitorDeviceRows(G_db_connection, result);
            if (monitor_device_row != NULL)
            {
                device_id = (long long) monitor_device_row[0].device_id;
            }
            free(monitor_device_row);
        }
    }

    csl_mysql_free_result(result);
    return device_id;
}

/************************************************
 * short deviceFindByDeviceID()
 *  @param
 *          unsigned long long device_id
 *
 *  @brief  Function to find a device in the G_device_table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a G_device_table in which each entry
 *          contains information about a device the monitor is monitoring.
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique device_id
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short deviceFindByDeviceID(unsigned long long device_id)
{
    short device_index = CS_DEVICE_NOT_FOUND;
    for (short i = 0; i < G_monitor_table.device_ctr; i++)
    {
        if (device_id == G_device_table[i].device_id)
        {
            device_index  = i;
            return device_index;
        }
    }
    //todo Throw Error Message Here
    printf("\t<%s> **** ERROR: Could Not Find device_id[%llu] in the device_table\n",
           __PRETTY_FUNCTION__, device_id);

    return CS_DEVICE_NOT_FOUND;
}


/************************************************
 * short deviceFindByDeviceIdentifier()
 *  @param
 *          char *device_identifier
 *
 *  @brief  Function to find a device in the G_device_table
 *
 *  @author Kerry
 *
 *  @note   The monitor maintains a G_device_table in which each entry
 *          contains information about a device the monitor is monitoring.
 *          This function returns the row number corresponding to a
 *          specific device, identified by the device's unique device identifier
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short deviceFindByDeviceIdentifier(byte device_identifier[])
{
    short device_index = CS_DEVICE_NOT_FOUND;

    for (short i = 0; i < G_monitor_table.device_ctr; i++)
    {
 //       int cmp_result = memcmp(tmp_compare, G_device_table[i].device_identifier, SIZE_DEVICE_IDENTIFIER);
        if (memcmp(device_identifier, G_device_table[i].device_identifier, SIZE_DEVICE_IDENTIFIER) == 0)
        {
            device_index  = i;
            return device_index;
        }
    }
    //todo Throw Error Message Here
    char display_field[SIZE_DEVICE_IDENTIFIER + 1];
    memset(display_field, NULL_BINARY, SIZE_DEVICE_IDENTIFIER+1);
    memcpy(display_field, device_identifier, SIZE_DEVICE_IDENTIFIER);
    printf("\t<%s> **** ERROR: Could Not Find device_identifier[%s] in the device_table\n",
           __PRETTY_FUNCTION__, display_field);

    return CS_DEVICE_NOT_FOUND;
}

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
 *          The probe_id = monitor_id + ((device_index + 1) / MAX_DEVICES)
 *
 * @return the row number (the device_index) of the found device or an error code: CS_DEVICE_NOT_FOUND
 ************************************************/
short deviceFindByProbeID(double probe_id)
{
    short device_index = CS_DEVICE_NOT_FOUND;
    for (short i = 0; i < G_monitor_table.device_ctr; i++)
    {
        if (probe_id == G_device_table[i].probe_id)
        {
            device_index  = i;
            return device_index;
        }
    }
    //todo Throw Error Message Here
    printf("\t<%s> **** ERROR: Could Not Find probe_id[%10.4f] in the device_table\n",
           __PRETTY_FUNCTION__, probe_id);
    return CS_DEVICE_NOT_FOUND;
}


/************************************************
 * short    deviceRegisterNew()
 *  @param  byte                *device_identifier  - The identifier of the new device being added to the device table
 *  @param  unsigned long long  device_id           - The device_id of the new device being added to the device table
 *  @param  char                *device_mac_address - The MAC address of the new device being added
 *
 *  @brief  To add a new device to the monitor's internal G_device_table. It is assumed at this point that the device
 *          identifier and id were checked with the config database, just to be sure. The new device is added to the
 *          table and the device counter is incremented.
 *
 *  @author Kerry
 *
 *  @return the largest index value, i.e., how many devices (minus one) are in the table
 ************************************************/
short     deviceRegisterNew(byte *device_identifier, unsigned long long device_id, byte *device_mac_address)
{
    short return_value                                      = G_monitor_table.device_ctr;
    G_device_table[G_monitor_table.device_ctr].device_id    = device_id;

    // **** NOTE: For the time-being, we are using the mac address also as the device identifier **** //
    memset(G_device_table[G_monitor_table.device_ctr].device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    memcpy(G_device_table[G_monitor_table.device_ctr].device_identifier, device_mac_address, SIZE_DEVICE_IDENTIFIER);
    memset(G_device_table[G_monitor_table.device_ctr].device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    memcpy(G_device_table[G_monitor_table.device_ctr].device_mac_address, device_mac_address, SIZE_MAC_ADDRESS);
    G_device_table[G_monitor_table.device_ctr].device_index         = G_monitor_table.device_ctr;
    G_device_table[G_monitor_table.device_ctr].status_element_ctr   = 0;
    G_device_table[G_monitor_table.device_ctr].probe_id             =
            csl_AssignProbeID(G_monitor_table.monitor_id, G_monitor_table.device_ctr);
    G_device_table[G_monitor_table.device_ctr].cs_standard_flag     = true;
    G_monitor_table.device_ctr++;
    if (G_monitor_table.device_ctr >= MAX_DEVICES)
    {
        printf("\t<%s> ERROR: Device Table Overflow! Device Counter = [%d]\n",
               __PRETTY_FUNCTION__, G_monitor_table.device_ctr);
    }
    printf("\t<%s> DEBUG: Just added device [%d], with MAC [%s]\n",
           __PRETTY_FUNCTION__, return_value, device_mac_address);
    return return_value;
}

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
        byte                *element_identifier)
{
    MYSQL_RES *result    = NULL;
    char mysql_query[SIZE_CS_SQL_COMMAND];
    char *identifier_string = csl_Hash2String(element_identifier, SIZE_HASH_NAME);

    sprintf(mysql_query,
                "Select distinct (element_name) "
                "from %s.%s where "
                "Device_ID = %llu and Monitor_ID = %llu and element_identifier = '%s'",
                CS_SQL_MONITOR_SCHEMA, CS_SQL_ELEMENT_NAMES_VIEW,
                device_id, monitor_id, identifier_string);
    result = csl_QueryDB (G_db_connection, mysql_query);
    char *element_name = csl_ReturnElementName(G_db_connection, result);
    if (element_name == NULL)
    {
        printf("\t<%s> **** ERROR: Could not execute query [%s]\n", __PRETTY_FUNCTION__, mysql_query);
        strcpy(element_name, ELEMENT_NAME_NOT_FOUND);
    }

    csl_mysql_free_result(result);
    free(identifier_string);
    return element_name;
}
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
 *              - the global variable G_current_scan_device_index is set, and CS_SUCCESS is returned
 *          An authorized device the monitor has not yet registered
 *              - the new device is registered, G_current_scan_device_index is set and CS_SUCCESS is returned
 *          An unauthorized device
 *              - an alert is issued (CS_ALERT_DEVICE_UNKNOWN) and CS_DEVICE_UNKNOWN is returned
 *
 *  @author Kerry
 *
 *  @return
 *      Success - Index of device in the device table
 *      Failure - CS_DEVICE_UNKNOWN
 ************************************************/
int     messageCheckOrigin()
{
    int return_value    = CS_DEVICE_UNKNOWN;
    // Get the message's device_identifier
    byte device_identifier[SIZE_DEVICE_IDENTIFIER];
    memset(device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
    memcpy(device_identifier, G_current_cs_message.message_header.from_address.device_identifier, SIZE_DEVICE_IDENTIFIER);

    byte device_mac_address[SIZE_MAC_ADDRESS];
    memset(device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    memcpy(device_mac_address, G_current_cs_message.message_header.from_address.device_identifier, SIZE_MAC_ADDRESS);

    // Find the appropriate device index for the Status Quo Table Lookup
    return_value = deviceFindByDeviceIdentifier(device_identifier);
    if (return_value < 0)        // i.e., the device was not found in the G_device_table
    {
        // Determine whether this is a new, legitimate device for this monitor, previously not registered
        long long device_id = deviceAssignedToMonitor(device_identifier, device_mac_address);
//
        if (device_id != (long long) CS_DEVICE_NOT_FOUND)
        {
            // Authorized Device - Register it
            return_value                = deviceRegisterNew(device_identifier, device_id, device_mac_address);
            G_current_zmessage.probe_id   = return_value;
//            G_current_zmessage.probe_id   = csl_AssignProbeID(G_monitor_table.monitor_id, return_value);
//            G_current_zmessage.probe_id   = G_monitor_table.monitor_id + device_id; // todo - this is also in deviceRegisterNew?
            // todo - Add an error check here
        }
        else
        {
            // Unauthorized Device - Issue an Alert
            // todo - Issue An Alert
            alertOnDevice (device_identifier, ALERT_DEVICE_UNKNOWN);
            return_value = CS_DEVICE_UNKNOWN;
        }
    }
//    G_current_scan_device_index    = return_value; // We should not need this here.

    return return_value;
}

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
int     messageHandshakeProccess(csl_zmessage *curr_zmessage, monitor_comms_t *comms)
{
    int return_flag = CS_SUCCESS;

    // Acknowledge Handshake
    return_flag = csl_AcknowledgeHandshake(curr_zmessage, comms);
    return return_flag;
}

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
bool    messageHeartBeatProcess(short device_index)
{
    int success_flag    = CS_SUCCESS;
    success_flag        = csl_ProcessHeartbeat (G_zmq_comms_t, &G_current_zmessage, &G_device_table[device_index]);

    if (success_flag != CS_SUCCESS)
    {
//        printf("\t<%s> **** ERROR: Heartbeat Failed\n", __PRETTY_FUNCTION__);     // todo we need more verbosity here
        return false;
    }
    return true;
}

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
int     messageTimeoutProcess()
{
    printf("STUB function messageTimeoutProcess()\n");    // todo remove Stub process
    return CS_SUCCESS;
}

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
int     messageUnknownProcess(int message_type, short device_id)
{
    printf("\t<%s> Received Message of Unknown Type [%d] while processing device_index[%d]\n",
           __PRETTY_FUNCTION__, message_type, device_id);
    return CS_SUCCESS;
}


/************************************************
 *      Monitor Start-up and Shutdown Functions
 *      =======================================
 ************************************************/

/************************************************
 * bool monitorInitialize()
 *  @param  None
 *
 *  @brief  When the monitor first launches, there are many start-up tasks
 *          that need to be performed. Rather than clog up the main() routine
 *          with them, they are collected into this support function.
 *
 *  @author Kerry
 *
 *  @note   The logic is:
 *              Setup the G_monitor_table with information about the monitor and its environment, e.g.,
 *                  - IP Address(s)
 *                  - MAC Address
 *                  - Hostname
 *              Setup the Monitor's Communications Protocols
 *              Connect to the local Crytica MySQL database to verify the Monitor
 *              Initialize the Monitor's Important Tables
 *                  - Device Table
 *                  - Status Quo Table
 *
 *  @return true on success, false on failure
 ************************************************/
bool monitorInitialize()
{
    bool    return_flag = true;

    /********************************************
     * Set up the monitor identity and communication fields
     * These are stored in the G_monitor_table in the comms structure
     ********************************************/

    // Initialize the G_monitor_table with default values
    G_monitor_table.monitor_id   = DEFAULT_MONITOR_ID;
    G_monitor_table.device_ctr   = 0;
    G_monitor_table.max_devices  = MAX_DEVICES;      // todo: Future releases this needs to be more flexible, based on "device class

    if (comm_params_initialize (&G_monitor_table.comm_params, BROADCASTER_PORT, DATA_PIPELINE_PORT, SCAN_PORT) != CS_SUCCESS)
    {
        printf("\t<%s> Failed to initialize the comm_params\n", __PRETTY_FUNCTION__);
    }

    /********************************************
    * Setup the Communication Fields
    ********************************************/
    // **** Obtain the Monitor's External IP Address ****
    char *tmpCommand    = calloc(SIZE_LINUX_COMMAND, sizeof(byte));
    char ip_address_external[SIZE_IP4_ADDRESS] = {END_OF_STRING};
    sprintf(tmpCommand,"dig +short myip.opendns.com @resolver1.opendns.com > %s", FILE_IP_ADDRESS);
    int system_return = system(tmpCommand);
    if (system_return == 0)
    {
        FILE *IPfile;
        IPfile = fopen(FILE_IP_ADDRESS, "r");
        fscanf(IPfile, "%s", ip_address_external);
        fclose(IPfile);
        sprintf(tmpCommand, "rm %s", FILE_IP_ADDRESS);
        system(tmpCommand);
    }
    else
    {
        sprintf(ip_address_external, DEFAULT_IP4_ADDRESS);
    }

    // **** Obtain the Monitor's HostName ****
    char my_host_name[SIZE_HOST_NAME];
    memset(my_host_name, NULL_BINARY, SIZE_HOST_NAME);
    int return_name = gethostname(my_host_name, SIZE_HOST_NAME);

    // **** Obtain Internal IP Address (copied code) *****
    struct ifaddrs *ifaddr, *ifa;
    int family, return_int;
    char ip_address_internal[SIZE_IP4_ADDRESS+1];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        // todo - expand on this error
        printf("\t<%s> ERROR: Could not obtain the internal IP Address. getifaddrs() Failed\n",
               __PRETTY_FUNCTION__ );
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* For an AF_INET* interface address, test the address */

        if (family == AF_INET || family == AF_INET6)
        {
            return_int = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            ip_address_internal, (SIZE_IP4_ADDRESS+1),
                            NULL, 0, NI_NUMERICHOST);
            if (return_int != 0)
            {
                printf("\t<%s> ERROR: getnameinfo() failed: %s\n", __PRETTY_FUNCTION__, gai_strerror(return_int));
                // todo - expand on this error or not ... not sure that we need to flag it
            }

//            if (strncmp(ip_address_internal, IP_PREFIX, SIZE_IP_PREFIX) == 0)
            if (strncmp(ip_address_internal, LOCAL_HOST_IP, SIZE_LOCAL_HOST_IP) != 0)
            {
                printf("\t<%s> DEBUG: %s\'s Internal IP address found: <%s>\n\n",
                       __PRETTY_FUNCTION__, my_host_name, ip_address_internal);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);

    // **** Obtain the Monitor's MAC Address ****
    sprintf(tmpCommand,"ip addr show | grep link/ether > %s", FILE_MAC_ADDRESS);
    system(tmpCommand);
    char mac_address[SIZE_MAC_ADDRESS+1] = {END_OF_STRING};
    char tmp_field[SIZE_LINUX_COMMAND]   = {END_OF_STRING};
    FILE *MACaddressFile;
    MACaddressFile      = fopen (FILE_MAC_ADDRESS, "r");
    fscanf(MACaddressFile, "%s%s", tmp_field, mac_address);
    fclose(MACaddressFile);
    sprintf(tmpCommand,"rm %s", FILE_MAC_ADDRESS);
    system(tmpCommand);
    free(tmpCommand);

    // **** Populate the G_current_zmessage Communication Parameters **** //
    int last_octet      = csl_get_last_octet(ip_address_internal) * 1000;
    int broadcast_port  = last_octet + BROADCASTER_PORT;
    int responder_port  = last_octet + RESPONDER_PORT;
    int scan_port       = last_octet + SCAN_PORT;
    G_zmq_comms_t = monitor_comms_new(broadcast_port, responder_port, scan_port, my_host_name);

    // **** Populate the G_monitor_table's Communication Parameters **** //
    strcpy(G_monitor_table.comm_params.monitor_external_ip_address, ip_address_external);
 //   memset(G_monitor_table.comm_params.monitor_external_ip_address, NULL_BINARY, SIZE_IP4_ADDRESS);
    strcpy(G_monitor_table.comm_params.monitor_internal_ip_address, ip_address_internal);
    strcpy(G_monitor_table.comm_params.monitor_hostname, my_host_name);
    strcpy(G_monitor_table.comm_params.monitor_mac_address, mac_address);
    strcpy(G_monitor_table.comm_params.monitor_hostname, my_host_name);
    strcpy(G_monitor_table.comm_params.monitor_public_key, G_zmq_comms_t->monitor_public_key);
    sprintf(G_monitor_table.comm_params.broadcasting_address,"%d", broadcast_port);
    sprintf(G_monitor_table.comm_params.responder_address, "%d", responder_port);
    sprintf(G_monitor_table.comm_params.data_pipeline_address,"%d", responder_port);
    sprintf(G_monitor_table.comm_params.scan_address,"%d", scan_port);
    strcpy(G_monitor_table.comm_params.public_cert_file_name, G_zmq_comms_t->public_cert_file_name);

//    FILE* testLogFile = fopen ("Exceptions.txt" , "w");

    /********************************************
     * Initialize the Monitor's important tables
     *      The Status Quo Table    - Keeps track of the latest scan data for each device
     *      The Device Table        - Keeps track of the devices the monitor is monitoring
     ********************************************/
    // **** Initialize G_status_quo_table to all 0s - indicating no current devices and scans ****
    for (int i = 0; i < MAX_DEVICES; i++)
    {
        for (int j = 0; j < MAX_ELEMENTS; j++)
        {
            memset(G_status_quo_table[i][j].element_identifier, NULL_BINARY, SIZE_ELEMENT_IDENTIFIER);
            G_status_quo_table[i][j].element_attributes   = 0;
//            memset(G_status_quo_table[i][j].element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
//            strcpy(G_status_quo_table[i][j].element_name,DEFAULT_ELEMENT_NAME);
            memset(G_status_quo_table[i][j].scan_value, NULL_BINARY, SIZE_HASH_ELEMENT);
            G_status_quo_table[i][j].alert_code           = 0;
        }
    }

//    printf("\t<%s>  we are true up to here ... and then?\n",__PRETTY_FUNCTION__);
    // **** Initialize G_device_table to all 0s - indicating no active devices ****
    for (short i = 0; i < MAX_DEVICES; i++)
    {
        G_device_table[i].device_id             = 0L;
        memset(G_device_table[i].device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
        memcpy(G_device_table[i].device_mac_address, DEFAULT_MAC_ADDRESS, strlen(DEFAULT_MAC_ADDRESS));
        memset(G_device_table[i].device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER);   // 0 out the byte array
        memcpy(G_device_table[i].device_identifier, DEFAULT_MAC_ADDRESS, strlen(DEFAULT_MAC_ADDRESS));
        G_device_table[i].status_element_ctr    = 0;
        G_device_table[i].cs_standard_flag      = true;
        G_device_table[i].last_heartbeat        = 0L;
        G_device_table[i].probe_id              = csl_AssignProbeID(DEFAULT_MONITOR_ID, 1);
        G_device_table[i].device_index          = i;
    }
//    printf("\t<%s> we are true up to here ... and then?\n",__PRETTY_FUNCTION__);

    /********************************************
     * Connect to the MySQL Database
     ********************************************/
    // **** Setup the MySQL parameters and declarations ****
    csl_mysql_connection_params connect_db_params;
    connect_db_params.dbPort           = DB_PORT;
    connect_db_params.dbHost           = "localhost";
    connect_db_params.dbServer         = NULL;
    connect_db_params.dbName           = CS_SQL_MONITOR_SCHEMA;
    connect_db_params.dbUser           = DB_USER;
    connect_db_params.dbUserPassword   = DB_USER_PWD;

    // **** Connect to the DB Server ****
    G_db_connection = csl_ConnectToDB(&connect_db_params);
    if ( !G_db_connection )
    {
        // todo - issue error message
        printf("\t<%s>**** ERROR: Monitor Failed to Connect to its MySQL Database ****\n",__PRETTY_FUNCTION__ );
        return false;
    }

    // **** Connect to the Monitor Schema ****
    char database_query[SIZE_CS_SQL_COMMAND];
    sprintf(database_query,"USE %s", CS_SQL_MONITOR_SCHEMA);

    if (csl_UpdateDB(G_db_connection, database_query) != CS_SUCCESS)
    {
        // todo - issue error message
        return false;
    }

    // **** Obtain Monitor Data from the Database ****
    MYSQL_RES *result      = NULL;
    char mysql_query[SIZE_CS_SQL_COMMAND];

    memset(mysql_query, NULL_BINARY, SIZE_CS_SQL_COMMAND);
    sprintf(mysql_query,
            "Select Monitor_ID from %s.%s "
            "where Monitor_Identifier = '%s'",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_VIEW,
            G_monitor_table.comm_params.monitor_mac_address);
    printf("\t<%s> Monitor MAC Address is: %s\n", __PRETTY_FUNCTION__, G_monitor_table.comm_params.monitor_mac_address);
    result = csl_QueryDB (G_db_connection, mysql_query);
    if (result != NULL)
    {
        unsigned int returned_rows = mysql_num_rows(result);
        switch (returned_rows)
        {
            case 0:
                printf("\t<%s> Error: no rows found in the database for monitor_identifier [%s]\n",
                       __PRETTY_FUNCTION__, G_monitor_table.comm_params.monitor_mac_address);
                break;

            case 1:
                break;

            default:
                printf("\t<%s> Error: too many rows [%d] in the database for monitor_identifier [%s]\n",
                   __PRETTY_FUNCTION__, returned_rows, G_monitor_table.comm_params.monitor_mac_address);
                break;
        }
        csl_monitor_record *monitor_row = csl_ReturnMonitorRows(G_db_connection, result);
        G_monitor_table.monitor_id = monitor_row[0].monitor_id;
        free(monitor_row);
    }
    else
    {
        return_flag = false;
        if (result == NULL)
        {
            printf("**** ERROR ****\n");
            printf("     Monitor: [%s] [%s] is not configured in the database\n\n",
                   G_monitor_table.comm_params.monitor_mac_address,
                   G_monitor_table.comm_params.monitor_hostname);
            // todo - issue error message monitor not in the database
        }
        else
        {
            printf("**** ERROR ****\n");
            printf("     Database Error [%d] Looking up Monitor: [%s] [%s]\n\n",
                   return_flag,
                   G_monitor_table.comm_params.monitor_mac_address,
                   G_monitor_table.comm_params.monitor_hostname);
            // todo - issue error message mysql error
        }
    }
    csl_mysql_free_result(result);

    // **** Initialize the crytica_standard date field in the database ****
    result      = NULL;
    sprintf(mysql_query,
            "Update %s.%s "
            "set crytica_standard_date = '%s' "
            "where Monitor_id = %llu",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW,
            DEFAULT_DATE, G_monitor_table.monitor_id);
    if (csl_UpdateDB(G_db_connection, mysql_query) != CS_SUCCESS)
    {
        printf("\t<%s>, Failed to initialize the Crytica Standard Date in the %s.%s\n",
               __PRETTY_FUNCTION__ , CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW);
    }

    // **** Obtain Monitor's Assigned Devices Data from the Database ****
    result      = NULL;
    sprintf(mysql_query,
            "Select Distinct Device_ID, Device_Identifier from %s.%s "
            "where Monitor_id = %llu order by Device_ID asc",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_MONITOR_DEVICE_VIEW,
            G_monitor_table.monitor_id);
    result = csl_QueryDB (G_db_connection, mysql_query);
    if (result != NULL)
    {
        int return_rows = mysql_num_rows(result);
        csl_monitor_device_record *monitor_device_row = csl_ReturnMonitorDeviceRows(G_db_connection, result);
        for (unsigned int i = 0; i < return_rows; i++)
        {
            G_device_table[i].device_id   = monitor_device_row[i].device_id;
            G_device_table[i].probe_id    = csl_AssignProbeID(G_monitor_table.monitor_id, i);
                    memset(G_device_table[i].device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
            memcpy(G_device_table[i].device_identifier, monitor_device_row[i].device_identifier, SIZE_DEVICE_IDENTIFIER);
            printf("\t<%s> Supported device_id [%llu] has device_identifier [%s] & probe_id [%0.4f]\n",
                   __PRETTY_FUNCTION__ , G_device_table[i].device_id,
                   G_device_table[i].device_identifier, G_device_table[i].probe_id);
//            printf("Device [%d] device_id [%llu] device_identifier [%s] is %s\n", i , G_device_table[i].device_id,
//                   G_device_table[i].cs_standard_flag ? "true" : "false", G_device_table[i].device_identifier);
            G_monitor_table.device_ctr++;
        }
        free(monitor_device_row);
    }

    // **** Initialize the Crytica Standard Table and Element Added Names Table ****
    result      = NULL;
    sprintf(mysql_query,
            "Truncate Table %s.%s",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_STANDARD_TABLE);
    if (csl_UpdateDB (G_db_connection, mysql_query) != CS_SUCCESS)
    {
        printf("\t<%s> **** WARNING: Failed to truncate %s.%s\n",
                   __PRETTY_FUNCTION__ , CS_SQL_MONITOR_SCHEMA, CS_SQL_STANDARD_TABLE);
        return_flag = false;
    }

    result      = NULL;
    sprintf(mysql_query,
            "Truncate Table %s.%s",
            CS_SQL_MONITOR_SCHEMA, CS_SQL_ELEMENT_ADDED_NAMES_TABLE);
    if (csl_UpdateDB (G_db_connection, mysql_query) != CS_SUCCESS)
    {
        printf("\t<%s> **** WARNING: Failed to truncate %s.%s\n",
               __PRETTY_FUNCTION__ , CS_SQL_MONITOR_SCHEMA, CS_SQL_ELEMENT_ADDED_NAMES_TABLE);
        return_flag = false;
    }

    /********************************************
     * Wrap up the initialization
     ********************************************/
    csl_mysql_free_result(result);
//    if (return_flag == false)
//        return return_flag;

    return return_flag;
}

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
bool    monitorShutDown()
{
    bool return_flag = true;

    // **** Disconnect from the database ****
    csl_DisconnectFromDB(G_db_connection);    // Note: This is a void function, so indicator of success or failure

    // **** Take Down the Communication Ports ****
    if (monitor_comms_destroy(G_zmq_comms_t) != CS_SUCCESS)
    {
        return_flag = false;
    }

    printf("\n=> Say: 'Good Night Gracie'\n");
    printf("\n\t ============================");
    printf("\n\t **** Good Night Gracie! ****");
    printf("\n\t ============================\n\n");

    return return_flag;
}

/************************************************
 *      Scan Functions
 *      ==============
 ************************************************/

short scan_get_next_index(short next_scan_index)
{
    for (short i = next_scan_index; MAX_DEVICES; i++)
    {
        if (G_device_table[i].last_heartbeat != 0L)
        {
            return i;
        }
    }

    for (short i = 0; i <= next_scan_index; i++)
    {
        if (G_device_table[i].last_heartbeat != 0L)
            return i;
    }

    return (short) CS_ERROR;
}


/************************************************
 * bool scanEvaluate()
 *  @param  short device_index
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
bool scanEvaluate(short device_index)
{
    bool return_flag = true;

    // Determine if this device needs a new Crytica Standard
    if (G_device_table[device_index].cs_standard_flag == true)
    {
        // Generate a new Crytica Standard
        return_flag = statusQuoTableBuild(device_index);
        if (return_flag == true)
        {
            return_flag = cStandardWriteToDB(device_index);
            G_device_table[device_index].cs_standard_flag = false;
        }
    }
    else
    {
        // Read Through the scan results table to look for "alerts"
        if (statusQuoTableSearch(device_index) < 0)
        {
            // todo - Throw Error Message Here
            return_flag = false;
        }
    }

    return return_flag;
}

#ifndef DEPRECATED
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
 *  @note   Although it is true that this function could use the global variable "G_current_scan_device_index", if we
 *          ever move to a more asynchronous environment, being explicit about from which device we are
 *          requesting the scan cannnot hurt us.
 *
 *  @return true on success, false on failure
 ************************************************/
bool    scanRequest(short device_index)
{
    int return_value = csl_RequestScan (G_zmq_comms_t,  &G_current_zmessage, &G_device_table[device_index]);
    if (return_value != CS_SUCCESS)
    {
        printf("**** ERROR [%d] in function scanRequest()\n", return_value);
        return false;
    }

    return true;
}
#endif

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
int scanTableAddRow(csl_scan_record *new_scan, short device_index)
{
    if (device_index != G_scan_table.device_index)
    {
        printf("\t<%s> ERROR: Attempting to store scan from device [%d] during scan of device [%d\n",
               __PRETTY_FUNCTION__, device_index, G_scan_table.device_index);
    }
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_id             = new_scan->scan_id;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_id            = new_scan->probe_id;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_date           = new_scan->scan_date;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_type        = new_scan->element_type;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_attributes  = new_scan->element_attributes;

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_value, NULL_BINARY, SIZE_HASH_ELEMENT);
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_value, new_scan->scan_value, SIZE_HASH_ELEMENT);

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name_hash, NULL_BINARY, SIZE_HASH_NAME+1);
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name_hash, new_scan->element_name_hash,
           SIZE_HASH_NAME);

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_uuid, NULL_BINARY, SIZE_ZUUID);
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_uuid, new_scan->probe_uuid, SIZE_ZUUID);

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_ip, NULL_BINARY, SIZE_IP4_ADDRESS);
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_ip, new_scan->device_ip, SIZE_IP4_ADDRESS);

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_mac_address, NULL_BINARY, SIZE_MAC_ADDRESS);
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_mac_address, new_scan->device_mac_address,
           SIZE_MAC_ADDRESS);

    memset(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name, NULL_BINARY, SIZE_ELEMENT_NAME);
    strcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name, new_scan->element_name);

    G_scan_table.scan_element_ctr++;
    if (G_scan_table.scan_element_ctr >= MAX_ELEMENTS)
    {
        printf("\t<%s> ***** ERROR: Scan Table Overflow [%d] ****\n", __PRETTY_FUNCTION__, G_scan_table.scan_element_ctr);
        return CS_TABLE_OVERFLOW;
    }

    return CS_SUCCESS;
}

#ifndef DEPRECATED
int scanTableAddRow(csl_scan_record *new_scan)
{
    G_scan_table.scan_element_ctr++;
    if (G_scan_table.scan_element_ctr >= MAX_ELEMENTS)
    {
        printf("***** ERROR: Scan Table Overflow [%d] ****\n", G_scan_table.scan_element_ctr);
        return CS_TABLE_OVERFLOW;
    }

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_id             = new_scan->scan_id;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_id            = new_scan->probe_id;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_date           = new_scan->scan_date;
    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_attributes  = new_scan->element_attributes;

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_value            = calloc(SIZE_HASH_ELEMENT, sizeof(byte));
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].scan_value, new_scan->scan_value, SIZE_HASH_ELEMENT);

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name_hash    = calloc(SIZE_HASH_NAME, sizeof(byte));
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name_hash, new_scan->element_name_hash,
           SIZE_HASH_NAME);

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_uuid            = calloc(SIZE_ZUUID, sizeof(byte));
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].probe_uuid, new_scan->probe_uuid, SIZE_ZUUID);

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_ip             = calloc(SIZE_IP4_ADDRESS, sizeof(byte));
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_ip, new_scan->device_ip, SIZE_IP4_ADDRESS);

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_mac_address   = calloc(SIZE_MAC_ADDRESS, sizeof(byte));
    memcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].device_mac_address, new_scan->device_mac_address,
           SIZE_MAC_ADDRESS);

    G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name          = calloc(strlen(new_scan->element_name) + 1,
                                                                                         sizeof(char));
    strcpy(G_scan_table.scan_elements[G_scan_table.scan_element_ctr].element_name, new_scan->element_name);

    print_csl_scan_record(new_scan, "Debug", G_scan_table.scan_element_ctr);

    return CS_SUCCESS;
}
#endif
/************************************************
 * int scanTableInitialize()
 *  @param  - None
 *
 *  @brief  Initializes all of the values in the G_scan_table
 *
 *  @author Kerry
 *
 *  @note   This is called each time a new scan is received from a probe. It is possible that
 *          we can get away with setting:
 *              G_scan_table.scan_element_ctr = 0;
 *          or perhaps setting
 *              G_scan_table.scan_elements[i].scan_id = 0L;
 *          but we shall see
 *
 * @return scan_element_ctr
 ************************************************/
int scanTableInitialize(short device_index)
{
    G_scan_table.scan_element_ctr = 0;
    G_scan_table.device_index     = device_index;
    for (unsigned short i = 0; i < MAX_ELEMENTS; i++)
    {
        memset(&G_scan_table.scan_elements[i], NULL_BINARY, sizeof(csl_scan_record));
    }
    return G_scan_table.scan_element_ctr;
}


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
bool statusQuoTableAddRow (unsigned short device_index, unsigned short sq_table_row, csl_scan_record scanRecord)
{
    bool return_flag = true;

    if (G_device_table[device_index].status_element_ctr >= MAX_ELEMENTS)
    {
        // todo - Throw an error flag here
        printf("\t<%s> ERROR: Status Quo Table Overflow for Device [%d]\n", __PRETTY_FUNCTION__, device_index);
        return_flag = false;
        return return_flag;
    }
    G_device_table[device_index].status_element_ctr++;
    memcpy(G_status_quo_table[device_index][sq_table_row].element_identifier,scanRecord.element_name_hash, SIZE_HASH_NAME);
    G_status_quo_table[device_index][sq_table_row].element_type         = scanRecord.element_type;
    G_status_quo_table[device_index][sq_table_row].element_attributes   = scanRecord.element_attributes;
    memcpy(G_status_quo_table[device_index][sq_table_row].scan_value, scanRecord.scan_value, SIZE_HASH_ELEMENT);
    G_status_quo_table[device_index][sq_table_row].alert_code           = 0;

//    print_csl_scan_record(&scanRecord, "Debug", sq_table_row);
    return return_flag;
}

/************************************************
 * bool statusQuoTableBuild()
 *  @param
 *          unsigned short device_index
 *
 *  @brief  Builds a column, from 0, in the Status Quo Table. The column is associated with a specific device.
 *          It is used whenever a new Crytica Standard scan is taken for the specific device, and hence that is
 *          when that device's column needs to be rebuilt
 *
 *  @author Kerry
 *
 *  @return true on success, false on failure
 ************************************************/
bool statusQuoTableBuild(unsigned short device_index)
{
    bool return_flag = true;

    G_device_table[device_index].status_element_ctr = 0;
    for (unsigned short row_index = 0; row_index < G_scan_table.scan_element_ctr; row_index++)
    {
        if (statusQuoTableAddRow(device_index, row_index, G_scan_table.scan_elements[row_index]) != true)
        {
            // todo - Throw an error flag here
            return_flag = false;
            break;
        }
    }

    return return_flag;
}


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
bool statusQuoTableRemoveRow(unsigned short device_index, unsigned short row_number)
{
    bool return_flag = true;

    unsigned short next_row;
    unsigned short end_of_table = G_device_table[device_index].status_element_ctr;
    while (end_of_table > row_number)
    {
        next_row = row_number + 1;
        memcpy(&G_status_quo_table[device_index][row_number], &G_status_quo_table[device_index][next_row], sizeof(status_quo_record));
        row_number++;
    }
    G_device_table[device_index].status_element_ctr--;

    return return_flag;
}

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
 *          DEL_ELEMENT    0x8   - Not currently needed, as a deleted element will not be in the SQ Table
 *          MOD_ATTRIBS    0x10  - This row's file attribs been modified since the last search  0x0 = No, 0x10 = yes
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
short   statusQuoTableSearch(short device_index)
{
    short alert_ctr = 0;
    if (device_index != G_scan_table.device_index)
    {
        printf("\t<%s> ERROR: Searching SQ Table for device [%d] with scan from device [%d]\n",
               __PRETTY_FUNCTION__, device_index, G_scan_table.device_index);
        return CS_ERROR;
    }

    /********************************************
     * For each element in the scan, we search the sq table for a match ****
     ********************************************/

    // **** scan table loop ****
    for (unsigned short scan_index = 0; scan_index < G_scan_table.scan_element_ctr; scan_index++)
    {
        bool found_flag = false;

        // **** status quo table loop ****
        for (unsigned short sq_index = 0; sq_index < G_device_table[device_index].status_element_ctr; sq_index++)
        {
            // Check for the Row matching the element_name_hash
            if (memcmp(G_scan_table.scan_elements[scan_index].element_name_hash,
                G_status_quo_table[device_index][sq_index].element_identifier, SIZE_HASH_NAME) == 0)
            {
                // Found a match. Need to compare the specific values and note that we found this element
                found_flag = true;
                G_status_quo_table[device_index][sq_index].alert_code =
                        G_status_quo_table[device_index][sq_index].alert_code | MASK_COMPARED;

                // Check for Contents Modifications
                if (memcmp(G_scan_table.scan_elements[scan_index].scan_value,
                           G_status_quo_table[device_index][sq_index].scan_value, SIZE_HASH_ELEMENT) != 0)
                {
                    // **** Hash Modification Discovered ****

                    // Increment the alert_ctr and update the row to note that this change has been noted
                    alert_ctr++;
                    G_status_quo_table[device_index][sq_index].alert_code =
                            G_status_quo_table[device_index][sq_index].alert_code | MASK_MOD_CONTENTS;

                    // write to the alert log that element's contents have been modified
                    alertOnElementModification(device_index, G_scan_table.scan_elements[scan_index], ALERT_MOD_CONTENTS);

                    // modify the status quo table entry to contain the modified value
                    memcpy(G_status_quo_table[device_index][sq_index].scan_value,
                           G_scan_table.scan_elements[scan_index].scan_value, SIZE_HASH_ELEMENT);
                }

                // Check for Attributes Modifications
                if (G_scan_table.scan_elements[scan_index].element_attributes !=
                    G_status_quo_table[device_index][sq_index].element_attributes)
                {
                    // **** Attribute Modification Discovered ****

                    // Increment the alert_ctr and update the row to note that this change has been noted
                    alert_ctr++;
                    G_status_quo_table[device_index][sq_index].alert_code =
                            G_status_quo_table[device_index][sq_index].alert_code | MASK_MOD_ATTRIBS;

                    // write to the alert log that element's attributes have been modified
                    alertOnElementModification(device_index, G_scan_table.scan_elements[scan_index], ALERT_MOD_ATTRIBS);

                    // modify the status quo table entry to contain the modified value
                    G_status_quo_table[device_index][sq_index].element_attributes =
                            G_scan_table.scan_elements[scan_index].element_attributes;
                }
                break;   // Since we found it, we do not need to search for this scan entry anymore
            }
        }

        // **** Check for new, added element ****
        if (found_flag != true)     // The scan entry is not in the SQ table, so this must be an added element
        {
            // Add the entry to the SQ Table
            alert_ctr++;
            unsigned short sq_table_row    = G_device_table[device_index].status_element_ctr;
            statusQuoTableAddRow (device_index, sq_table_row, G_scan_table.scan_elements[scan_index]);

            // Flag that the comparison took place
            G_status_quo_table[device_index][sq_table_row].alert_code =
                    G_status_quo_table[device_index][sq_table_row].alert_code | MASK_COMPARED;
            G_status_quo_table[device_index][sq_table_row].alert_code =
                    G_status_quo_table[device_index][sq_table_row].alert_code | MASK_ADD_ELEMENT;

            // Send out an alert
            alertOnElementAddition(device_index, G_scan_table.scan_elements[scan_index]);

            // Add the entry to the element_added_names view
            int         return_value    = CS_SUCCESS;
            char        mysql_insert[SIZE_CS_SQL_COMMAND];

            sprintf(mysql_insert, "Insert into %s.%s "
                                  "(monitor_id, device_id, element_identifier, element_name)"
                                  "values (%llu, %llu, '%s', '%s')",
                    CS_SQL_MONITOR_SCHEMA, CS_SQL_ELEMENT_ADDED_NAMES_VIEW,
                    G_monitor_table.monitor_id,
                    G_device_table[device_index].device_id,
                    G_scan_table.scan_elements[scan_index].element_name_hash,
                    G_scan_table.scan_elements[scan_index].element_name);
            return_value = csl_UpdateDB(G_db_connection, mysql_insert);
        }
    }

    /********************************************
     * Finally, we check the sq table for entries for which there was no scan i.e., deleted elements
     * For each element in the sq table, we search the scan for a match ****
     ********************************************/

    // SQ Table loop
    for (unsigned short sq_index = 0; sq_index < G_device_table[device_index].status_element_ctr; sq_index++)
    {
        if ((G_status_quo_table[device_index][sq_index].alert_code & MASK_COMPARED) == 0)
        {   // if the entry in sq table was not flagged,it was not in the scan
            alert_ctr++;

            if (alertOnElementDeletion (G_status_quo_table[device_index][sq_index], G_device_table[device_index].device_id,
                                        G_scan_table.scan_elements[0].scan_date) == false)
            {
                // todo - Throw an error flag
            }
            if (statusQuoTableRemoveRow(device_index,sq_index) == false)
            {
                // todo - Throw an error flag
            }
        }
        else
        {   // the element was found, therefore zero out the Flagged bit in preparation for the next scan
            G_status_quo_table[device_index][sq_index].alert_code =
                    G_status_quo_table[device_index][sq_index].alert_code ^ MASK_COMPARED;
        }
    }
    printf("\t<%s> Finished scan of [%d] for device_index[%d]\n",
           __PRETTY_FUNCTION__, G_scan_table.scan_element_ctr, G_scan_table.device_index);
    if (scanTableInitialize(-1) != 0)
    {
        printf("\t<%s> Failed to initial scan table for device[%d]",
               __PRETTY_FUNCTION__ ,device_index);
    }
    return alert_ctr;
}

