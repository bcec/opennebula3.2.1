/* -------------------------------------------------------------------------- */
/* Copyright 2002-2011, OpenNebula Project Leads (OpenNebula.org)             */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may    */
/* not use this file except in compliance with the License. You may obtain    */
/* a copy of the License at                                                   */
/*                                                                            */
/* http://www.apache.org/licenses/LICENSE-2.0                                 */
/*                                                                            */
/* Unless required by applicable law or agreed to in writing, software        */
/* distributed under the License is distributed on an "AS IS" BASIS,          */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and        */
/* limitations under the License.                                             */
/* -------------------------------------------------------------------------- */

#ifndef BACKUPDISK_H_
#define BACKUPDISK_H_

#include "ObjectSQL.h"

using namespace std;

/**
 *  The BackupDisk class, it represents an execution record of a Virtual Machine.
 */

class BackupDisk:public ObjectSQL
{
public:
    enum DkState
    {
          INIT                       = 0,
          START_BACKUPDISK      = 1,
          BACKUPDISK_SUCESS    = 2,        
          BACKUPDISK_FAIL        = 3         
    };

   BackupDisk(int oid, int vid = -1);

    BackupDisk(
        int             oid,
        int             vid,
        string&      dk_name,
        string&      dk_dir);

    ~BackupDisk(){};

    /**
     *  Function to write the Backup Record in an output stream
     */
    friend ostream& operator<<(ostream& os, const BackupDisk& backupdisk);

    /**
     * Function to print the Backup object into a string in
     * plain text
     *  @param str the resulting string
     *  @return a reference to the generated string
     */
    string& to_str(string& str) const;

    /**
     * Function to print the Backup object into a string in
     * XML format
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml(string& xml) const;

private:
    friend class VirtualMachine;
    friend class VirtualMachinePool;
   
    // ----------------------------------------
    // DataBase implementation variables
    // ----------------------------------------
    enum ColNames
    {
        OID             = 0,
        VID             = 1,
        DK_NAME    = 2,
        DK_DIR       = 3,
        STATE         = 4
    };

    static const char * table;

    static const char * db_names;

    static const char * extended_db_names;

    static const char * db_bootstrap;

   

    static string column_name(const ColNames column)
    {
        switch (column)
        {
        case VID:
            return "vid";        
        default:
            return "";
        }
    }

    // ----------------------------------------
    // BackupDisk fields
    // ----------------------------------------
    int     oid;
    int     vid;

    
    string dk_dir;

    DkState     state;

    
    string dk_name;



   
    
    /**
     *  Writes the backup record in the DB
     *    @param db pointer to the database.
     *    @return 0 on success.
     */
    int insert(SqlDB * db, string& error_str);

    /**
     *  Reads the backup record from the DB
     *    @param db pointer to the database.
     *    @return 0 on success.
     */
    int select(SqlDB * db);

    /**
     *  Updates the backup record
     *    @param db pointer to the database.
     *    @return 0 on success.
     */
     int update(SqlDB * db);

    /**
     *  Removes the all backup records from the DB
     *    @param db pointer to the database.
     *    @return 0 on success.
     */
    int drop(SqlDB * db);

    /**
     *  Execute an INSERT or REPLACE Sql query.
     *    @param db The SQL DB
     *    @param replace Execute an INSERT or a REPLACE
     *    @return 0 on success
     */
    int insert_replace(SqlDB *db, bool replace);

    /**
     *  Callback function to unmarshall a history object (History::select)
     *    @param num the number of columns read from the DB
     *    @para names the column names
     *    @para vaues the column values
     *    @return 0 on success
     */
    int select_cb(void *nil, int num, char **values, char **names);

    /**
     *  Function to unmarshall a backup object into an output stream with XML
     *  format.
     *    @param oss the output stream
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    static int dump(ostringstream& oss, int  num, char **names, char **values);
};

#endif /*BACKUPDISK_H_*/

