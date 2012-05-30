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

#include "VirtualMachine.h"
#include "Nebula.h"

#include <iostream>
#include <sstream>

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

const char * BackupDisk::table = "backupdisk";

const char * BackupDisk::db_names = "oid,vid,dk_name,dk_dir,state";

const char * BackupDisk::extended_db_names =
    "backupdisk.oid, backupdisk.vid, backupdisk.dk_name,backupdisk.dk_dir," 
   " backupdisk.state";
    
const char * BackupDisk::db_bootstrap = "CREATE TABLE IF NOT EXISTS "
    "backupdisk (oid INTEGER,"
    "vid INTEGER,dk_name TEXT,dk_dir TEXT,state INTEGER,"
    "PRIMARY KEY(oid))";

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


BackupDisk::BackupDisk(
    int _oid,
    int _vid):
        oid(_oid),
        vid(_vid),
        dk_name(""),
        dk_dir(""),
        state(INIT){};

/* -------------------------------------------------------------------------- */

BackupDisk::BackupDisk(
    int     		_oid,
    int     		_vid,
    string& 		_dk_name,
    string& 		_dk_dir):
        oid(_oid),
        vid(_vid),
        dk_name(_dk_name),
        dk_dir(_dk_dir),
        state(INIT){};


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::insert(SqlDB * db, string& error_str)
{
    int             rc;

    rc = insert_replace(db, false);

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::update(SqlDB * db)
{
    int             rc;

    rc = insert_replace(db, true);

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::insert_replace(SqlDB *db, bool replace)
{
    ostringstream   oss;

    int    rc;

    
    char * sql_dk_dir;
    char * sql_dk_name;
    
    

    if (vid == -1)
    {
        return 0;
    }

    sql_dk_name = db->escape_str(dk_name.c_str());

    sql_dk_dir = db->escape_str(dk_dir.c_str());

    if ( sql_dk_dir == 0 )
    {
        goto error_vmm;
    }

   

   
    if(replace)
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    oss << " INTO " << table << " ("<< db_names <<") VALUES ("<<
        oid << "," <<
        vid << "," <<
        "'" << sql_dk_name << "'," <<
        "'" << sql_dk_dir << "'," <<
        state << ")";

    rc = db->exec(oss);

  
    db->free_str(sql_dk_name);

    db->free_str(sql_dk_dir);
    
    return rc;


error_vmm:
    db->free_str(sql_dk_dir);
     db->free_str(sql_dk_name);

}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::select_cb(void *nil, int num, char **values, char **names)
{
    if ((!values[OID]) ||
        (!values[VID]) ||
        (!values[DK_NAME]) ||
        (!values[DK_DIR]) ||
        (!values[STATE]))        
    {
        return -1;
    }

    oid      = atoi(values[OID]);
    vid      = atoi(values[VID]);

   
   dk_dir   = values[DK_DIR];

   state = static_cast<DkState>(atoi(values[STATE]));

   dk_name = values[DK_NAME];
   
    

    

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::dump(ostringstream& oss, int num, char **values, char **names)
{
    if ((!values[OID])||
        (!values[VID])||
        (num != STATE))
    {
        return -1;
    }

    oss <<
        "<BACKUPDISK>" <<
          "<VID>"     << values[VID]           << "</VID>"     <<
          "<STATE>" << values[STATE]      << "</STATE>"<<         
        "</BACKUPDISK>";

    return 0;
}

/* -------------------------------------------------------------------------- */

int BackupDisk::select(SqlDB * db)
{
    ostringstream   oss;
    int             rc;

    if (oid == -1)
    {
        return -1;
    }

    if ( vid == -1)
    {
        oss << "SELECT " << db_names << " FROM backupdisk WHERE oid = "<< oid <<
            " AND vid=(SELECT MAX(vid) FROM backupdisk WHERE oid = " << oid << ")";
    }
    else
    {
        oss << "SELECT " << db_names << " FROM backupdisk WHERE oid = " << oid
            << " AND vid = " << vid;
    }

    set_callback(static_cast<Callbackable::Callback>(&BackupDisk::select_cb));

    rc = db->exec(oss,this);

    unset_callback();

    
    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int BackupDisk::drop(SqlDB * db)
{
    ostringstream   oss;

    oss << "DELETE FROM " << table << " WHERE oid= "<< oid;

    return db->exec(oss);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ostream& operator<<(ostream& os, const BackupDisk& backupdisk)
{
    string backupdisk_str;

    os << backupdisk.to_xml(backupdisk_str);

    return os;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


string& BackupDisk::to_str(string& str) const
{
    ostringstream oss;

    oss<< "\tVID      = " <<vid          << endl
       << "\tSTATE = " << state      << endl;

   str = oss.str();

   return str;

}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& BackupDisk::to_xml(string& xml) const
{
    ostringstream oss;

    oss <<
        "<BACKUPDISK>" <<
          "<VID>"     << vid           << "</VID>"   <<
          "<STATE>"<<state     << "</STATE>"<<              
        "</BACKUPDISK>";

   xml = oss.str();

   return xml;
}
