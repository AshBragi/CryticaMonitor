//
// Created by kerry on 7/12/20.
//

#include "csl_mysql.h"


//MYSQL_RES       *query_result   = NULL;

/************************************************
 * **********************************************
 * Generic MySQL Functions
 * =======================
 *
 * **********************************************
 ************************************************/

/************************************************
 * MYSQL* csl_ConnectToDB()
 *  @param
 *          MYSQL                           *db_connection
 *          csl_mysql_connection_params*    connection_params
 *
 *  @brief  Establish Connection to a MySQL database
 *
 *  @author Kerry
 *
 *  @note   The connection_params is pointer to a struct of parameters for connection
 *
 *  @return Pointer to a MYSQL connection
 ************************************************/
MYSQL* csl_ConnectToDB(csl_mysql_connection_params* connection_params)
{
    MYSQL *db_connection = NULL;        // The NULL here is essential, as mysql_init looks for, and requires, a NULL value
    db_connection = mysql_init(db_connection);
    mysql_options(db_connection, MYSQL_READ_DEFAULT_GROUP, CS_SQL_DEFAULT_GROUP);
    db_connection = mysql_real_connect(
            db_connection,
            connection_params->dbHost,
            connection_params->dbUser,
            connection_params->dbUserPassword,
            connection_params->dbName,
            0,NULL,0);

/**** The mysql_real_connect parameters *********
 *  MYSQL *mysql_real_connect(MYSQL *mysql, const char *host,
 *                              const char *user, const char *passwd,
 *                              const char *db, unsigned int port,
 *                              const char *unix_socket, unsigned long client_flag)
 ************************************************/

    if (db_connection == NULL)
    {
        fprintf(stderr, "Failed to connect to database: Error: %s\n",
                mysql_error(db_connection));
        // TODO - Elaborate on this error
    }

    return db_connection;
}

/************************************************
 * MYSQL* csl_DisconnectFromDB()
 *  @param
 *          MYSQL   *db_connection
 *
 *  @brief  Disconnect from a MySQL database
 *
 *  @author Kerry
 *
 *  @return void - This is yet again one of the few exceptions to the policy against void functions
 ************************************************/
void csl_DisconnectFromDB(MYSQL *db_connection)
{
    mysql_close(db_connection);
    return;             // This is redundant, but we prefer to be consistent and explicit
}

/************************************************
 * bool csl_LicenseKeyVerify() - Checks to see if license key is registered in the DB
 *  @param
 *          MYSQL* db_connection                    - Pointer to the DB
 *          char licenseKey[SIZE_DB_LICENSE_KEY]    - licenseKey (license key received by monitor)
 *
 *  @return
 *          true on valid license
 *          false otherwise
 ************************************************/
bool csl_LicenseKeyVerify (MYSQL* db_connection, char licenseKey[SIZE_DB_LICENSE_KEY])
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/************************************************
 * void csl_mysql_free_result()
 *  @param  MYSQL_RES    result
 *
 *  @brief  It is important that the MYSQL_RES field be freed after use. It is preferable
 *          not to call a MySQL function directly from outside the csl_mysql.c/h module. This function,
 *          which is callable from any Crytica module, provides the entire project access to this functionality.
 *
 *  @author Kerry
 *
 *  @return void - NOTE: this is one of the few exceptions to our policy of not using void functions
 ************************************************/
void csl_mysql_free_result(MYSQL_RES *result)
{
    mysql_free_result(result);
    return;         // although this return is "redundant" we want it here for "consistency"
}


/************************************************
 * MYSQL_RES csl_QueryDB()
 *  @param
 *          MYSQL       *db_connection   - Pointer to the DB
 *          const char  *query           - Text of the query
 *
 *  @brief  Send a query to the DB - One from which rows may be returned
 *
 *  @author Kerry
 *
 *  @note   Although in MySQL the term "query" is generic and can mean a true query or a database update,
 *          we have chosen to be more explicit. Queries, that is requests for information from the database
 *          are handled in this function: csl_QueryDB().
 *          Updates and insertions, are handled in a different function: csl_UpdateDB().
 *
 *  @return
 *          On Success - Pointer to the query result
 *          On Failure - NULL
 ************************************************/
MYSQL_RES *csl_QueryDB (MYSQL* db_connection, const char* query)
{
    //int return_value = CS_SUCCESS;

    MYSQL_RES *result;

    int query_value = mysql_query(db_connection, query);
    if (query_value != ZERO)
    {
        result = NULL;
        printf("\t<%s> **** ERROR: Bad Database Query ****\n", __PRETTY_FUNCTION__ );
        printf("     Query: %s\n", query);
        // todo issue an appropriate MySQL error here
    }
    else // query succeeded, process any data returned by it
    {
        result = mysql_store_result(db_connection);
        if (result == NULL)
        {
            printf("\t<%s> **** ERROR: result == NULL ****\n", __PRETTY_FUNCTION__ );
            printf("     Query: %s\n", query);
        }
    }
    return result;
}

#ifndef DEPRECATED
/************************************************
 * CS_3d_byte_array *csl_ReturnQueryRow()
 *  @param  MYSQL        *db_connection
 *  @param  MYSQL_RES    *result
 *
 *  @brief  To convert the result of a MYSQL query (i.e., MYSQL_RES) into a csl 3D array of bytes
 *
 *  @author Kerry
 *
 *  @note   While the MYSQL_RES is a very useful construct, it is MySQL dependent. Therefore, for Crytica
 *          we use a three dimensional construct to store and access database query results. This creates
 *          for Crytica a modicum of code independence from the underlying database engine.
 *
 *          The correspondence between the MySQL query results and the Crytica 3D construct is a follows:
 *              MySQL column (or field)         - CS_1d_byte_array: [byte array length][byte array]
 *              MySQL row (table or view)       - CS_2d_byte_array: [number of 1D arrays][1D arrays]
 *              MySQL result (multiple rows)    - CS_3d_byte_array: [number of 2D arrays][2D arrays]
 *          See the csl_utilities library for more information about the CS_Nd-byte_arrays
 *
 *  @return Success - pointer to the CS_3d_byte_array
 *          Failure - NULL pointer
 ************************************************/
CS_3d_byte_array *csl_ReturnQueryRow(MYSQL *db_connection, MYSQL_RES *result)
{
    CS_3d_byte_array *return_array  = calloc(sizeof(CS_3d_byte_array), sizeof(byte));
    return_array->number_of_tables  = ZERO;
    unsigned int num_columns = mysql_num_fields(result);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        CS_2d_byte_array *stage_row  = calloc(sizeof(CS_2d_byte_array), sizeof(byte));
        for (int i = 0; i < num_columns; i++)
        {
            CS_1d_byte_array *stage_field   = calloc(sizeof(CS_1d_byte_array), sizeof(byte));
//            stage_field->number_of_element_bytes    = strlen(row[i]);
            stage_field = csl_String2ByteArray(row[i]);
            stage_row                       = csl_2dByteArrayAddRow(stage_row, stage_field);
        }
        return_array = csl_3dByteArrayAddTable(return_array, stage_row);
    }
    if (return_array->number_of_tables  == ZERO)
    {
        return_array = NULL;
    }
    return return_array;
}
#endif

/************************************************
 * int csl_UpdateDB ()
 *  @param
 *          MYSQL       *db_connection   - Pointer to the DB
 *          const char  *query           - Text of the query (update or insert, not a request for data)
 *
 *  @brief  Send an Update ("query") to the DB - One from which no return results are expected
 *
 *  @author Kerry
 *
 *  @note   Although in MySQL the term "query" is generic and can mean a true query or a database update,
 *          we have chosen to be more explicit. Queries, that update information content in the database,
 *          for example, INSERT and UPDATE, are handled in this function: csl_UpdateDB().
 *          Queries requesting information are handled in a different function: csl_QueryDB().
 *
 *  @return
 *          On Success  - CS_SUCCESS    // todo - change this to the number of rows updated/inserted
 *          On Failure  - Error Code
 ************************************************/
int csl_UpdateDB (MYSQL* db_connection, const char* query)
{
    int return_value = CS_SUCCESS;

    if (mysql_query(db_connection, query) != ZERO)
    {
        printf("\t<%s> **** Warning: Failed DB Update [%s] ****\n", __PRETTY_FUNCTION__, query);
        return_value = CS_ERROR_DB_QUERY;
        // todo issue an error ... We could return the result of the query, but before we do that we need to
        // todo sync up our error message numbers with MySQL's error numbers
    }

    return return_value;
}

/************************************************
 * bool csl_QueryStoredProcedure () - Query the database using stored procedures
 *  @param
 *      MYSQL*      db_connection       - Pointer to the DB
 *      int         stored_proc_id      - Stored procedure identifier
 *      cs_header*  header              - final format of csl_generic_message_record
 *
 *  @return
 *      true    - on success
 *      false   - on failure
 *************************************************/
bool csl_QueryStoredProcedure (MYSQL* db_connection, int stored_proc_id, cs_header* hdr)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/************************************************
 * **********************************************
 * Monitor Specific MySQL Functions
 * =======================
 *
 * **********************************************
 ************************************************/

csl_monitor_record  *csl_ReturnMonitorRows(MYSQL *db_connection, MYSQL_RES *result)
{
    unsigned int num_rows   = mysql_num_rows(result);
    unsigned int row_ctr    = 0;
    csl_monitor_record *return_monitor = calloc(num_rows, sizeof(csl_monitor_record));
//    csl_monitor_record return_monitor[num_rows];

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        return_monitor[row_ctr].monitor_id = strtoll(row[0], NULL, BASE_TEN);
        row_ctr++;
    }
    if (num_rows != row_ctr)
    {
        printf("\t<%s> num_rows and row_ctr do not match [%d] [%d]\n",__PRETTY_FUNCTION__, num_rows, row_ctr);
    }
    return return_monitor;
}

csl_monitor_device_record *csl_ReturnMonitorDeviceRows(MYSQL *db_connection, MYSQL_RES *result)
{
    unsigned int num_rows   = mysql_num_rows(result);
    unsigned int row_ctr    = 0;
    char         tmp_identifier[SIZE_DEVICE_IDENTIFIER + 1];
    csl_monitor_device_record *return_monitor_devices = calloc(num_rows, sizeof(csl_monitor_device_record));

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)))
    {
        return_monitor_devices[row_ctr].device_id = strtoll(row[0], NULL, BASE_TEN);
        strcpy(tmp_identifier, row[1]);
        memset(return_monitor_devices[row_ctr].device_identifier, NULL_BINARY, SIZE_DEVICE_IDENTIFIER);
        memcpy(return_monitor_devices[row_ctr].device_identifier, tmp_identifier, strlen(tmp_identifier));
        row_ctr++;
    }
    if (num_rows != row_ctr)
    {
        printf("\t<%s> num_rows and row_ctr do not match [%d] [%d]\n",__PRETTY_FUNCTION__, num_rows, row_ctr);
    }
    return return_monitor_devices;
}

cs_standard_record *csl_ReturnStandardRow(MYSQL *db_connection, MYSQL_RES *result)
{
    cs_standard_record *return_standard = calloc(1, sizeof(cs_standard_record));
    unsigned int num_rows   = mysql_num_rows(result);
    if (num_rows != 1)
    {
        printf("\t<%s> Bad return rows [%d] from Standard DB Query\n",__PRETTY_FUNCTION__, num_rows);
        return_standard = NULL;
    }
    else
    {
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        return_standard->cs_element_type = (short) strtol(row[0], NULL, BASE_TEN);
        return_standard->cs_element_name = calloc(strlen(row[1]) + 1, sizeof(char));
        memset(return_standard->cs_element_name, NULL_BINARY, strlen(row[1]));
        strcpy(return_standard->cs_element_name, row[1]);
    }
    return return_standard;
}

char *csl_ReturnElementName(MYSQL *db_connection, MYSQL_RES *result)
{
    char *element_name = calloc(SIZE_ELEMENT_NAME + 1, sizeof(char));
    unsigned int num_rows   = mysql_num_rows(result);
    if (num_rows != 1)
    {
        printf("\t<%s> Bad return rows [%d] from Element Name DB Query\n",__PRETTY_FUNCTION__, num_rows);
        strcpy(element_name, ELEMENT_NAME_NOT_FOUND);;
    }
    else
    {
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        strcpy(element_name, row[0]);
    }
    return element_name;
}

#ifndef DEPRECATED
/************************************************
 * int csl_AlertDeviceWriteRecord()
 *  @param
 *          MYSQL                   *db_connection
 *          csl_device_alert_record *alert_record
 *
 *  @brief  Write a device alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int csl_AlertDeviceWriteRecord(MYSQL *db_connection, csl_device_alert_record *alert_record)
{
    int         return_value    = CS_SUCCESS;
    char        mysql_insert[SIZE_MYSQL_COMMAND];
    char        alert_data[SIZE_ALERT_DEVICE_DATA];

    sprintf(alert_data,"Sent From: [%s] [%s] - Sent To: [%s] [%s]",
           alert_record->device_from_ip_address, alert_record->device_from_info,
           alert_record->device_to_ip_address, alert_record->device_to_info);

    sprintf(mysql_insert, "Insert into cs_monitor.v_alert_log "
                                "(monitor_id, device_id, probe_id, alert_type, alert_data, alert_process_date)"
                                "values (%llu, '%s', %llu, %d, '%s', '%s');",
                                alert_record->monitor_id,
                                alert_record->device_identifier,
                                (long long) ZERO,
                                alert_record->alert_type,
                                alert_data,
                                csl_Time2String(alert_record->alert_date));
    return_value = csl_UpdateDB(db_connection, mysql_insert);

    return return_value;
}

/************************************************
 * int csl_AlertScanWriteRecord()
 *  @param
 *          MYSQL                   *db_connection
 *          csl_device_alert_record *alert_record
 *
 *  @brief  Write a scan alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int csl_AlertScanWriteRecord (MYSQL *db_connection, csl_scan_alert_record *alert_record)
{
    int         return_value    = CS_SUCCESS;
    MYSQL_RES   *query_result   = NULL;
    char        mysql_insert[SIZE_MYSQL_COMMAND];
 //   char        alert_data[SIZE_ALERT_DEVICE_DATA];

    sprintf(mysql_insert, "Insert into cs_monitor.v_alert_log "
                          "(monitor_id, device_id, probe_id, alert_type, element_type, element_name, alert_process_date)"
                          "values (%llu, %llu, %llu, %d, '%s', '%s', '%s');",
            alert_record->monitor_id,
            alert_record->device_id,
            (long long) ZERO,
            alert_record->alert_type,
            element_types[alert_record->element_type],
            alert_record->element_name,
            csl_Time2String(alert_record->alert_date));

    return_value = csl_UpdateDB(db_connection, mysql_insert);

    return return_value;
}

/************************************************
 * int csl_StandardReadRecord()
 *  @param
 *          csl_monitor_scan_record     *scan_record,
 *          MYSQL                       *db_connection
 *          unsigned long long          monitor_id
 *          unsigned long long          device_id
 *          unsigned short              element_identifier
 *
 *  @brief  Queries the cs_monitor.v_cs_standard View to find the current record of a CS-Standard for
 *          a specific element on a specific device.
 *
 *  @note   This is not necessary for releases prior to 0.5.0, since we are storing the element name in those
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS or error code
 ************************************************/
int csl_StandardReadRecord(
        csl_monitor_scan_record *scan_record,
        MYSQL *db_connection,
        unsigned long long monitor_id,
        unsigned long long device_id,
        unsigned short element_identifier)
{
    MYSQL_RES *result    = NULL;
    char mysql_query[SIZE_MYSQL_COMMAND];
    char *select_statement      = "Select * from cs_monitor.v_cs_standard ";
    sprintf(mysql_query,
            "%s where Device_Id = '%llu' and Monitor_ID = %llu and cs_element_identifier = %du;",
            select_statement, device_id, monitor_id, element_identifier);
    result = csl_QueryDB (db_connection, mysql_query);
    /********************************************
     * Todo - This function is unfinished. We need to still pull in the values from the returned result row(s)
     ********************************************/
    csl_mysql_free_result(result);
    return CS_SUCCESS;


}


/**
 * @brief Writes all misc info to the db
 * @param message (message object to read from)
 * @param db_connection (connection to mysql pointer)
 * @return true on success
 */
bool csl_write_msg_misc_to_db (csl_generic_message_record* message, MYSQL* db_connection)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief Prepares the db to write a Crytica Standard
 * @param dbConnection (connection to mysql pointer)
 * @param message (message object pointer to read from)
 * @return true on success
 */
bool csl_gold_standard_prepare (csl_generic_message_record* message, MYSQL* dbConnection)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief Writes the Crytica Standard to the db
 * @param message (message object to read from)
 * @param db_connection (connection to mysql db)
 * @return true on success
 */
bool csl_gold_standard_to_db(csl_generic_message_record* message, MYSQL* db_connection)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief Inserts (registers) a probe into the database uas part of successful probe/mon handshake
 * @param message (message object to read from containing probe information)
 * @param db_connection (connection to mysql db)
 * @param monitorID (DB id for monitor)
 * @param deviceID (DB id for device)
 * @return probe_id on insert success or -1 on failure
 */
int csl_insert_probe(csl_generic_message_record* message, MYSQL* db_connection, int monitorID, int deviceID)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 *
 *
 */
int csl_insert_monitor(MYSQL* db_connection)
{
    return CS_SUCCESS;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief Searches the db for an exception
 * @param message (message object to read from)
 * @param db_connection (connection to mysql db)
 * @return true on success
 */
bool csl_search_db_for_match(csl_generic_message_record* message, MYSQL* db_connection)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

bool csl_alert_exists(csl_generic_message_record *message, MYSQL* db_connection)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

// temp comment     bool csl_insert_alert(csl_generic_message_record* message, MYSQL* db_connection, int monitor_id, char* toPeek, enum ALERT_ID alert_info);
/**
 * @brief frees the result object
 * @param result (address to pointer of result)
 */
void csl_free_result (MYSQL_RES** result)
{
    return;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief frees the connection object
 * @param connection (pointer to mysql connection)
 */
void csl_free_db_connection (MYSQL* connection)
{
    return;    // todo - this is STUB ... we need to fix this SOON!
}

/**
 * @brief get filename from database
 * @param name_ptr (pointer to the name of the file being looked for)
 * @param db_connection (pointer to mysql connection)
 * @param phash (hash of the file that was deleted)
 * @param pmach_addr (probe mac address for searching)
 */

void csl_get_file_name_from_db (char* name_ptr, MYSQL* db_connection, char* phash, unsigned char* pmac_addr)
{
    return;    // todo - this is STUB ... we need to fix this SOON!
}


/**
 * @brief check to see if Crytica Standard exists in DB
 * @param db_connection (pointer to mysql connection)
 * @param message (message from probe with device_identifier to check with)
 * @return true if GS exists for the device checked, false if it doesn't
 */
bool csl_does_GS_exist (MYSQL* db_connection, csl_generic_message_record* message)
{
    return true;    // todo - this is STUB ... we need to fix this SOON!
}

#endif


