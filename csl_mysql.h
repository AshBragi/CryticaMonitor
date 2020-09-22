//
// Created by C. Kerry Nemovicher, Ph.D. on 6/19/20.
//

#ifndef CRYTICAMONITOR_CSL_MYSQL_H
#define CRYTICAMONITOR_CSL_MYSQL_H

//Includes
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include </home/kerry/CLionProjects/mysql/include/mysql.h>
#include "csl_constants.h"
#include "CryticaMonitor.h"
#include "csl_message.h"

//#include "mysql/mysql.h"
//#include "csl_monitorSQ.h"
/************************************************
#define DB_PORT                 3306
#define DB_HOST                 "127.0.0.1"
#define DB_SERVER               "127.0.0.1"
*************************************************/
#define DB_PORT                 3306
//#define DB_HOST                 "75.140.40.244"
//#define DB_SERVER               "75.140.40.244"
#define DB_USER                 "crytica"
#define DB_USER_PWD             "crytica123!"
//#define DB_NAME                 "cs_monitor"

#define ACT_AUDIT_LOG           0

#define ACT_INSERT_CONFIG       1
#define ACT_UPDATE_CONFIG       2
#define ACT_SELECT_CONFIG       3

#define ACT_INSERT_DEVICE       11
#define ACT_UPDATE_DEVICE       12
#define ACT_SELECT_DEVICE       13

#define ACT_INSERT_ELEMENT      21
#define ACT_UPDATE_ELEMENT      22
#define ACT_SELECT_ELEMENT      23

#define ACT_INSERT_LOCATION     31
#define ACT_UPDATE_LOCATION     32
#define ACT_SELECT_LOCATION     33

#define ACT_INSERT_MONITOR      41
#define ACT_UPDATE_MONITOR      42
#define ACT_SELECT_MONITOR      43

#define ACT_INSERT_PROBE        51
#define ACT_UPDATE_PROBE        52
#define ACT_SELECT_PROBE        53

#define ACT_INSERT_SIGNATURE    61
#define ACT_UPDATE_SIGNATURE    62
#define ACT_SELECT_SIGNATURE    63
#define ACT_SELECT_LAST_HASH    64


/**** typedefs from mysql.h ***************************/

#ifndef COMMENT_OUT_DB
typedef struct MYSQL
{
// temp comment     NET net;                     // Communication parameters //
    unsigned char *connector_fd; // ConnectorFd for SSL //
    char *host, *user, *passwd, *unix_socket, *server_version, *host_info;
    char *info, *db;
    struct CHARSET_INFO *charset;
// temp comment     MYSQL_FIELD *fields;
    struct MEM_ROOT *field_alloc;
    uint64_t affected_rows;
    uint64_t insert_id;      // id if insert on table with NEXTNR //
    uint64_t extra_info;     // Not used //
    unsigned long thread_id; // Id for connection in server //
    unsigned long packet_length;
    unsigned int port;
    unsigned long client_flag, server_capabilities;
    unsigned int protocol_version;
    unsigned int field_count;
    unsigned int server_status;
    unsigned int server_language;
    unsigned int warning_count;
// temp comment     struct st_mysql_options options;
// temp comment     enum mysql_status status;
// temp comment     enum enum_resultset_metadata resultset_metadata;
    bool free_me;   // If free in mysql_close //
    bool reconnect; // set to 1 if automatic reconnect //

    // session-wide random string //
// temp comment     char scramble[SCRAMBLE_LENGTH + 1];

// temp comment    LIST *stmts; // list of all statements //
    const struct MYSQL_METHODS *methods;
    void *thd;
    /*
      Points to boolean flag in MYSQL_RES  or MYSQL_STMT. We set this flag
      from mysql_stmt_close if close had to cancel result set of this object.
    //
    bool *unbuffered_fetch_owner;
    void *extension;
} MYSQL;
*************************************************/


typedef struct MYSQL_RES
{
    uint64_t row_count;
// temp comment     MYSQL_FIELD *fields;
    struct MYSQL_DATA *data;
// temp comment     MYSQL_ROWS *data_cursor;
    unsigned long *lengths; /* column lengths of current row */
    MYSQL *handle;          /* for unbuffered reads */
    const struct MYSQL_METHODS *methods;
// temp comment    MYSQL_ROW row;         /* If unbuffered read */
// temp comment     MYSQL_ROW current_row; /* buffer to current row */
    struct MEM_ROOT *field_alloc;
    unsigned int field_count, current_field;
    bool eof; /* Used by mysql_fetch_row */
    /* mysql_stmt_close() had to cancel this result */
    bool unbuffered_fetch_cancelled;
// temp comment    enum enum_resultset_metadata metadata;
    void *extension;
} MYSQL_RES;

typedef struct
{
    char message[20];
}csl_generic_message_record;

#endif
/*************** End of typedefs from mysql.h & csl_message.h ***********************************/




/**** Not sure why we need this struct - CKN ****/
typedef struct
{
    int probe_id;                       //32
    int element_id;                     //32
    int device_id;                      //32
    int location_id;                    //32
    int monitor_id;                     //32
    int max_probes;                     //32
    int signature_id;                   //32
    char description[65];               //65
    char signature[65];                 //65
    char signature_previous[65];        //65
    char file_folder[4096];           //4096
    char object_name[256];             //256
    char signature_description[256];   //256
}cs_header;

/**** mysql connection parameters ****/
typedef struct
{
    unsigned int dbPort;
    char* dbHost;
    char* dbServer;
    char* dbUser;
    char* dbUserPassword;
    char* dbName;
} csl_mysql_connection_params;





typedef struct
{
    short           message_type;
    time_t          message_date;
    unsigned int    monitor_identifier;
    char            device_identifier[SIZE_DEVICE_IDENTIFIER+1];
    char*           generic_message;
} csl_generic_message_record;

//Function Prototypes



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
MYSQL* csl_ConnectToDB(csl_mysql_connection_params* connection_params);


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
void csl_DisconnectFromDB(MYSQL *db_connection);


/************************************************
 * bool csl_LicenseKeyVerify() - Checks to see if license key is registered in the DB
 *
 *  @param
 *      MYSQL* db_connection                    - Pointer to the DB
 *      char licenseKey[SIZE_DB_LICENSE_KEY]    - licenseKey (license key received by monitor)
 *
 * @return
 *      true on valid license
 *      false otherwise
 ************************************************/
bool csl_LicenseKeyVerify (MYSQL* db_connection, char licenseKey[SIZE_DB_LICENSE_KEY]);

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
void csl_mysql_free_result(MYSQL_RES *result);


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
MYSQL_RES *csl_QueryDB (MYSQL* db_connection, const char* query);

/************************************************
 * bool csl_QueryStoredProcedure () - Query the database using stored procedures
 *  @param
 *      MYSQL*      db_connection       - Pointer to the DB
 *      int         stored_proc_id      - Stored procedure identifier
 *      cs_header*  header              - final format of csl_generic_message_record
 *
 *  @return
 *      result->number
 *      false   - on failure
 *************************************************/
bool csl_QueryStoredProcedure (MYSQL* db_connection, int stored_proc_id, cs_header* hdr);

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
CS_3d_byte_array *csl_ReturnQueryRow(MYSQL *db_connection, MYSQL_RES *result);
#endif

/************************************************
 * int csl_UpdateDB ()
 *
 *  @param
 *          MYSQL       *db_connection   - Pointer to the DB
 *          const char  *query           - Text of the query
 *
 *  @brief  Send an Update ("query") to the DB - One from which no return results are expected
 *
 *  @return
 *          On Success  - CS_SUCCESS    // todo - change this to the number of rows updated/inserted
 *          On Failure  - Error Code
 ************************************************/
int csl_UpdateDB (MYSQL* db_connection, const char* query);

/************************************************
 * **********************************************
 * Monitor Specific MySQL Functions
 * =======================
 *
 * **********************************************
 ************************************************/

csl_monitor_record  *csl_ReturnMonitorRows(MYSQL *db_connection, MYSQL_RES *result);

csl_monitor_device_record *csl_ReturnMonitorDeviceRows(MYSQL *db_connection, MYSQL_RES *result);

cs_standard_record *csl_ReturnStandardRow(MYSQL *db_connection, MYSQL_RES *result);

char *csl_ReturnElementName(MYSQL *db_connection, MYSQL_RES *result);

/************************************************
 * **********************************************
 * Deprecated MySQL Functions
 * =======================
 *
 * **********************************************
 ************************************************/

#ifndef DEPRECATED
/************************************************
 * **********************************************
 * Monitor Specific MySQL Functions
 * =======================
 *
 * **********************************************
 ************************************************/

/************************************************
 * int csl_AlertDeviceWriteRecord()
 *  @param
 *          csl_device_alert_record *alert_record
 *
 *  @brief  Write an alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int csl_AlertDeviceWriteRecord(device_alert_record *alert_record);

/************************************************
 * int csl_AlertScanWriteRecord()
 *  @param
 *          MYSQL                   *db_connection
 *          csl_device_alert_record *alert_record
 *
 *  @brief  Write an alert message to the cs_monitor.v_alert_log view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS  - on successful write
 *          error-code  - on failure
 ************************************************/
int csl_AlertScanWriteRecord (MYSQL *db_connection, csl_scan_alert_record *alert_record);


bool    csl_CStandardGenerate();

/************************************************
 * bool csl_CStandardNeeded()
 *  @param
 *          MYSQL               *db_connection,
 *          monitor_info_table  *monitor_table,
 *          device_record       **device_table
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
 *  @return true on success, false on failure
 ************************************************/
bool    csl_CStandardNeeded(MYSQL *db_connection, monitor_info_table *monitor_table, device_record *device_table);

/************************************************
 * bool csl_DeviceAssignedToMonitor()
 *  @param
 *          byte *device_identifier
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
 *          true - the device is assigned to this monitor
 *          false - the device is not assigned to this monitor
 ************************************************/
bool    csl_DeviceAssignedToMonitor(
                MYSQL *db_connection,
                unsigned long long monitor_id,
                byte *device_identifier,
                unsigned long long *device_id,
                char *device_mac_address);

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
*  @return CS_SUCCESS
*          error code
************************************************/
int csl_StandardReadRecord(
        csl_monitor_scan_record *scan_record,
        MYSQL *db_connection,
        unsigned long long monitor_id,
        unsigned long long device_id,
        unsigned short element_identifier);

/************************************************
 * int csl_StandardWriteRecord()
 *  @param
 *          MYSQL                   *db_connection
 *          csl_monitor_scan_record new_record
 *
 *  @brief  Writes a new Crytica Standard record to the cs_monitor.v_cs_standard view
 *
 *  @author Kerry
 *
 *  @return CS_SUCCESS
 *          error code
 ************************************************/
int csl_StandardWriteRecord(MYSQL *db_connection, csl_monitor_scan_record new_record);






bool csl_write_msg_misc_to_db (csl_generic_message_record* message, MYSQL* db_connection);

/**
 * @brief Prepares the db to write a gold standard
 * @param dbConnection (connection to mysql pointer)
 * @param message (message object pointer to read from)
 * @return true on success
 */
bool csl_gold_standard_prepare (csl_generic_message_record* message, MYSQL* dbConnection);

/**
 * @brief Writes the gold standard to the db
 * @param message (message object to read from)
 * @param db_connection (connection to mysql db)
 * @return true on success
 */
bool csl_gold_standard_to_db(csl_generic_message_record* message, MYSQL* db_connection);

/**
 * @brief Inserts (registers) a probe into the database uas part of successful probe/mon handshake
 * @param message (message object to read from containing probe information)
 * @param db_connection (connection to mysql db)
 * @param monitorID (DB id for monitor)
 * @param deviceID (DB id for device)
 * @return probe_id on insert success or -1 on failure
 */
int csl_insert_probe(csl_generic_message_record* message, MYSQL* db_connection, int monitorID, int deviceID);

/**
 *
 *
 */
int csl_insert_monitor(MYSQL* db_connection);

/**
 * @brief Searches the db for an exception
 * @param message (message object to read from)
 * @param db_connection (connection to mysql db)
 * @return true on success
 */
bool csl_search_db_for_match(csl_generic_message_record* message, MYSQL* db_connection);

bool csl_alert_exists(csl_generic_message_record *message, MYSQL* db_connection);

// temp comment     bool csl_insert_alert(csl_generic_message_record* message, MYSQL* db_connection, int monitor_id, char* toPeek, enum ALERT_ID alert_info);
/**
 * @brief frees the result object
 * @param result (address to pointer of result)
 */
void csl_free_result (MYSQL_RES** result);

/**
 * @brief frees the connection object
 * @param connection (pointer to mysql connection)
 */
void csl_free_db_connection (MYSQL* connection);

/**
 * @brief get filename from database
 * @param name_ptr (pointer to the name of the file being looked for)
 * @param db_connection (pointer to mysql connection)
 * @param phash (hash of the file that was deleted)
 * @param pmach_addr (probe mac address for searching)
 */

void csl_get_file_name_from_db (char* name_ptr, MYSQL* db_connection, char* phash, unsigned char* pmac_addr);


/**
 * @brief check to see if Gold Standard exists in DB
 * @param db_connection (pointer to mysql connection)
 * @param message (message from probe with device_identifier to check with)
 * @return true if GS exists for the device checked, false if it doesn't
 */
bool csl_does_GS_exist (MYSQL* db_connection, csl_generic_message_record* message);


#endif

#endif //CRYTICAMONITOR_CSL_MYSQL_H

