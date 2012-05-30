/* -------------------------------------------------------------------------- */
/* Copyright 2002-2012, OpenNebula Project Leads (OpenNebula.org)             */
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

const char * Backup::table = "backup";

const char * Backup::db_names = "oid,vid,bk_dir,state,stime,etime";

const char * Backup::extended_db_names =
    "backup.oid, backup.vid, backup.bk_dir, backup.state, backup.stime, backup.etime";
    
const char * Backup::db_bootstrap = "CREATE TABLE IF NOT EXISTS "
    "backup (oid INTEGER,"
    "vid INTEGER,bk_dir TEXT,state INTEGER,stime INTEGER,etime INTEGER,"
    "PRIMARY KEY(oid))";

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


Backup::Backup(
    int _oid,
    int _vid):
        oid(_oid),
        vid(_vid),
        bk_dir(""),
        state(INIT),
        stime(0),
        etime(0){};

/* -------------------------------------------------------------------------- */

Backup::Backup(
    int      _oid,
    int      _vid,
    string&    _bk_dir):
        oid(_oid),
        vid(_vid),
        bk_dir(_bk_dir),
        state(INIT),
        stime(0),
        etime(0){};


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::insert(SqlDB * db, string& error_str)
{
    int             rc;

    rc = insert_replace(db, false);

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::update(SqlDB * db)
{
    int             rc;

    rc = insert_replace(db, true);

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::insert_replace(SqlDB *db, bool replace)
{
    ostringstream   oss;

    int    rc;

    
    char * sql_bk_dir;
    
    

    if (vid == -1)
    {
        return 0;
    }

   

    sql_bk_dir = db->escape_str(bk_dir.c_str());

    if ( sql_bk_dir == 0 )
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
        "'" << sql_bk_dir << "'," <<
        state << "," <<
        stime << "," <<
        etime << ")";

    rc = db->exec(oss);

  
    db->free_str(sql_bk_dir);
    
    return rc;


error_vmm:
    db->free_str(sql_bk_dir);

}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::select_cb(void *nil, int num, char **values, char **names)
{
    if ((!values[OID]) ||
        (!values[VID]) ||
        (!values[BK_DIR]) ||
        (!values[STATE]) ||
        (!values[STIME]) ||
        (!values[ETIME]))        
    {
        return -1;
    }

    oid      = atoi(values[OID]);
    vid      = atoi(values[VID]);

   
    bk_dir   = values[BK_DIR];

   state = static_cast<BkState>(atoi(values[STATE]));

   
    stime = static_cast<time_t>(atoi(values[STIME]));
    etime = static_cast<time_t>(atoi(values[ETIME]));

    

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::dump(ostringstream& oss, int num, char **values, char **names)
{
    if ((!values[OID])||
        (!values[VID])||
        (num != STATE)||
        (!values[STIME])||
        (!values[ETIME]))
    {
        return -1;
    }

    oss <<
        "<BACKUP>" <<
          "<VID>"     << values[VID]           << "</VID>"     <<
          "<STATE>" << values[STATE]      << "</STATE>"<<
          "<STIME>"   << values[STIME]         << "</STIME>"   <<
          "<ETIME>"   << values[ETIME]         << "</ETIME>"   <<
        "</BACKUP>";

    return 0;
}

/* -------------------------------------------------------------------------- */

int Backup::select(SqlDB * db)
{
    ostringstream   oss;
    int             rc;

    if (oid == -1)
    {
        return -1;
    }

    if ( vid == -1)
    {
        oss << "SELECT " << db_names << " FROM backup WHERE oid = "<< oid <<
            " AND vid=(SELECT MAX(vid) FROM backup WHERE oid = " << oid << ")";
    }
    else
    {
        oss << "SELECT " << db_names << " FROM backup WHERE oid = " << oid
            << " AND vid = " << vid;
    }

    set_callback(static_cast<Callbackable::Callback>(&Backup::select_cb));

    rc = db->exec(oss,this);

    unset_callback();

    
    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int Backup::drop(SqlDB * db)
{
    ostringstream   oss;

    oss << "DELETE FROM " << table << " WHERE oid= "<< oid;

    return db->exec(oss);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ostream& operator<<(ostream& os, const Backup& backup)
{
    string backup_str;

    os << backup.to_xml(backup_str);

    return os;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


string& Backup::to_str(string& str) const
{
    ostringstream oss;

    oss<< "\tVID      = " <<vid          << endl
       << "\tSTATE = " << state      << endl
       << "\tSTIME    = " << stime         << endl
       << "\tETIME    = " << etime;

   str = oss.str();

   return str;

}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& Backup::to_xml(string& xml) const
{
    ostringstream oss;

    oss <<
        "<BACKUP>" <<
          "<VID>"     << vid           << "</VID>"   <<
          "<STATE>"<<state     << "</STATE>"<<
          "<STIME>"   << stime         << "</STIME>" <<
          "<ETIME>"   << etime         << "</ETIME>" <<         
        "</BACKUP>";

   xml = oss.str();

   return xml;
}
