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
#include <limits.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>

#include "VirtualMachine.h"
#include "VirtualNetworkPool.h"
#include "ImagePool.h"
#include "NebulaLog.h"

#include "Nebula.h"


#include "vm_var_syntax.h"

/* ************************************************************************** */
/* Virtual Machine :: Constructor/Destructor                                  */
/* ************************************************************************** */

VirtualMachine::VirtualMachine(int           id,
                               int           _uid,
                               int           _gid,
                               const string& _uname,
                               const string& _gname,
                               VirtualMachineTemplate * _vm_template):
        PoolObjectSQL(id,VM,"",_uid,_gid,_uname,_gname,table),
        last_poll(0),
        state(INIT),
        lcm_state(LCM_INIT),
        stime(time(0)),
        etime(0),
        deploy_id(""),
        memory(0),
        cpu(0),
        net_tx(0),
        net_rx(0),
        history(0),
        previous_history(0),
        _log(0)
{
    if (_vm_template != 0)
    {
        obj_template = _vm_template;
    }
    else
    {
        obj_template = new VirtualMachineTemplate;
    }
}

VirtualMachine::~VirtualMachine()
{
    for (unsigned int i=0 ; i < history_records.size() ; i++)
    {
            delete history_records[i];
    }

    if ( _log != 0 )
    {
        delete _log;
    }

    if ( obj_template != 0 )
    {
        delete obj_template;
    }
}

/* ************************************************************************** */
/* Virtual Machine :: Database Access Functions                               */
/* ************************************************************************** */

const char * VirtualMachine::table = "vm_pool";

const char * VirtualMachine::db_names =
    "oid, name, body, uid, gid, last_poll, state, lcm_state, "
    "owner_u, group_u, other_u";

const char * VirtualMachine::db_bootstrap = "CREATE TABLE IF NOT EXISTS "
        "vm_pool (oid INTEGER PRIMARY KEY, name VARCHAR(128), body TEXT, uid INTEGER, "
        "gid INTEGER, last_poll INTEGER, state INTEGER, lcm_state INTEGER, "
        "owner_u INTEGER, group_u INTEGER, other_u INTEGER)";

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::select(SqlDB * db)
{
    ostringstream   oss;
    ostringstream   ose;

    int             rc;
    int             last_seq;

    Nebula&         nd = Nebula::instance();

    // Rebuild the VirtualMachine object
    rc = PoolObjectSQL::select(db);

    if( rc != 0 )
    {
        return rc;
    }

    //Get History Records. Current history is built in from_xml() (if any).
    if( hasHistory() )
    {
        last_seq = history->seq - 1;

        for (int i = last_seq; i >= 0; i--)
        {
            History * hp;

            hp = new History(oid, i);
            rc = hp->select(db);

            if ( rc != 0)
            {
                goto error_previous_history;
            }

            history_records[i] = hp;

            if ( i == last_seq )
            {
                previous_history = hp;
            }
        }
    }

    //Create support directory for this VM
    oss.str("");
    oss << nd.get_var_location() << oid;

    mkdir(oss.str().c_str(), 0777);
    chmod(oss.str().c_str(), 0777);

    //Create Log support for this VM
    try
    {
        _log = new FileLog(nd.get_vm_log_filename(oid),Log::DEBUG);
    }
    catch(exception &e)
    {
        ose << "Error creating log: " << e.what();
        NebulaLog::log("ONE",Log::ERROR, ose);

        _log = 0;
    }

    return 0;

error_previous_history:
    ose << "Can not get previous history record (seq:" << history->seq
        << ") for VM id: " << oid;

    log("ONE", Log::ERROR, ose);
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::insert(SqlDB * db, string& error_str)
{
    int    rc;
    string name;

    SingleAttribute *   attr;
    string              aname;
    string              value;
    ostringstream       oss;


    // ------------------------------------------------------------------------
    // Check template for restricted attributes
    // ------------------------------------------------------------------------

    if ( uid != 0 && gid != GroupPool::ONEADMIN_ID )
    {
        VirtualMachineTemplate *vt = 
            static_cast<VirtualMachineTemplate *>(obj_template);
        
        if (vt->check(aname))
        {
            goto error_restricted;            
        }
    }

    // ------------------------------------------------------------------------
    // Set a name if the VM has not got one and VM_ID
    // ------------------------------------------------------------------------

    oss << oid;
    value = oss.str();

    attr = new SingleAttribute("VMID",value);

    obj_template->set(attr);

    get_template_attribute("NAME",name);

    if ( name.empty() == true )
    {
        oss.str("");
        oss << "one-" << oid;
        name = oss.str();

        replace_template_attribute("NAME", name);
    }
    else if ( name.length() > 128 )
    {
        goto error_name_length;
    }

    this->name = name;

    // ------------------------------------------------------------------------
    // Get network leases
    // ------------------------------------------------------------------------

    rc = get_network_leases(error_str);

    if ( rc != 0 )
    {
        goto error_leases_rollback;
    }

    // ------------------------------------------------------------------------
    // Get disk images
    // ------------------------------------------------------------------------

    rc = get_disk_images(error_str);

    if ( rc != 0 )
    {
        // The get_disk_images method has an internal rollback for
        // the acquired images, release_disk_images() would release all disks
        goto error_leases_rollback;
    }

    // -------------------------------------------------------------------------
    // Parse the context & requirements
    // -------------------------------------------------------------------------

    rc = parse_context(error_str);

    if ( rc != 0 )
    {
        goto error_context;
    }

    rc = parse_requirements(error_str);

    if ( rc != 0 )
    {
        goto error_requirements;
    }

    parse_graphics();

    // ------------------------------------------------------------------------
    // Insert the VM
    // ------------------------------------------------------------------------

    rc = insert_replace(db, false, error_str);

    if ( rc != 0 )
    {
        goto error_update;
    }

    return 0;

error_update:
    goto error_rollback;

error_context:
    goto error_rollback;

error_requirements:
    goto error_rollback;

error_rollback:
    release_disk_images();

error_leases_rollback:
    release_network_leases();
    goto error_common;

error_restricted:
    oss << "VM Template includes a restricted attribute " << aname << "."; 
    error_str = oss.str(); 
    goto error_common;

error_name_length:
    oss << "NAME is too long; max length is 128 chars.";
    error_str = oss.str(); 
    goto error_common;

error_common:
    NebulaLog::log("ONE",Log::ERROR, error_str);

    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::parse_context(string& error_str)
{
    int rc, num;

    vector<Attribute *> array_context;
    VectorAttribute *   context;

    string *            str;
    string              parsed;

    num = obj_template->remove("CONTEXT", array_context);

    if ( num == 0 )
    {
        return 0;
    }
    else if ( num > 1 )
    {
        error_str = "Only one CONTEXT attribute can be defined.";
        return -1;
    }

    context = dynamic_cast<VectorAttribute *>(array_context[0]);

    if ( context == 0 )
    {
        error_str = "Wrong format for CONTEXT attribute.";
        return -1;
    }

    str = context->marshall(" @^_^@ ");

    if (str == 0)
    {
        NebulaLog::log("ONE",Log::ERROR, "Can not marshall CONTEXT");
        return -1;
    }

    rc = parse_template_attribute(*str,parsed);

    if ( rc == 0 )
    {
        VectorAttribute * context_parsed;

        context_parsed = new VectorAttribute("CONTEXT");
        context_parsed->unmarshall(parsed," @^_^@ ");


        string target = context_parsed->vector_value("TARGET");

        if ( target.empty() )
        {
            Nebula&       nd = Nebula::instance();
            string        dev_prefix;

            nd.get_configuration_attribute("DEFAULT_DEVICE_PREFIX",dev_prefix);
            dev_prefix += "b";

            context_parsed->replace("TARGET", dev_prefix);
        }

        obj_template->set(context_parsed);
    }

    /* --- Delete old context attributes --- */

    for (int i = 0; i < num ; i++)
    {
        if (array_context[i] != 0)
        {
            delete array_context[i];
        }
    }

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::parse_graphics()
{
    int num;

    vector<Attribute *> array_graphics;
    VectorAttribute *   graphics;

    num = obj_template->get("GRAPHICS", array_graphics);

    if ( num == 0 )
    {
        return;
    }

    graphics = dynamic_cast<VectorAttribute * >(array_graphics[0]);

    if ( graphics == 0 )
    {
        return;
    }

    string port = graphics->vector_value("PORT");

    if ( port.empty() )
    {
        Nebula&       nd = Nebula::instance();

        ostringstream oss;
        istringstream iss;

        int           base_port;
        string        base_port_s;

        nd.get_configuration_attribute("VNC_BASE_PORT",base_port_s);
        iss.str(base_port_s);
        iss >> base_port;

        oss << ( base_port + oid );
        graphics->replace("PORT", oss.str());
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::parse_requirements(string& error_str)
{
    int rc, num;

    vector<Attribute *> array_reqs;
    SingleAttribute *   reqs;

    string              parsed;

    num = obj_template->remove("REQUIREMENTS", array_reqs);

    if ( num == 0 )
    {
        return 0;
    }
    else if ( num > 1 )
    {
        error_str = "Only one REQUIREMENTS attribute can be defined.";
        return -1;
    }

    reqs = dynamic_cast<SingleAttribute *>(array_reqs[0]);

    if ( reqs == 0 )
    {
        error_str = "Wrong format for REQUIREMENTS attribute.";
        return -1;
    }

    rc = parse_template_attribute(reqs->value(),parsed);

    if ( rc == 0 )
    {
        SingleAttribute * reqs_parsed;

        reqs_parsed = new SingleAttribute("REQUIREMENTS",parsed);
        obj_template->set(reqs_parsed);
    }

    /* --- Delete old requirements attributes --- */

    for (int i = 0; i < num ; i++)
    {
        if (array_reqs[i] != 0)
        {
            delete array_reqs[i];
        }
    }

    return rc;
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

int VirtualMachine::insert_replace(SqlDB *db, bool replace, string& error_str)
{
    ostringstream   oss;
    int             rc;

    string xml_body;
    char * sql_deploy_id;
    char * sql_name;
    char * sql_xml;

    sql_deploy_id = db->escape_str(deploy_id.c_str());

    if ( sql_deploy_id == 0 )
    {
        goto error_generic;
    }

    sql_name =  db->escape_str(name.c_str());

    if ( sql_name == 0 )
    {
        goto error_name;
    }

    sql_xml = db->escape_str(to_xml(xml_body).c_str());

    if ( sql_xml == 0 )
    {
        goto error_body;
    }

    if ( validate_xml(sql_xml) != 0 )
    {
        goto error_xml;
    }

    if(replace)
    {
        oss << "REPLACE";
    }
    else
    {
        oss << "INSERT";
    }

    oss << " INTO " << table << " ("<< db_names <<") VALUES ("
        <<          oid             << ","
        << "'" <<   sql_name        << "',"
        << "'" <<   sql_xml         << "',"
        <<          uid             << ","
        <<          gid             << ","
        <<          last_poll       << ","
        <<          state           << ","
        <<          lcm_state       << ","
        <<          owner_u         << ","
        <<          group_u         << ","
        <<          other_u         << ")";

    db->free_str(sql_deploy_id);
    db->free_str(sql_name);
    db->free_str(sql_xml);

    rc = db->exec(oss);

    return rc;

error_xml:
    db->free_str(sql_deploy_id);
    db->free_str(sql_name);
    db->free_str(sql_xml);

    error_str = "Error transforming the VM to XML.";

    goto error_common;

error_body:
    db->free_str(sql_deploy_id);
    db->free_str(sql_name);
    goto error_generic;

error_name:
    db->free_str(sql_deploy_id);
    goto error_generic;

error_generic:
    error_str = "Error inserting VM in DB.";
error_common:
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::add_history(
    int   hid,
    const string& hostname,
    const string& vm_dir,
    const string& vmm_mad,
    const string& vnm_mad,
    const string& tm_mad)
{
    ostringstream os;
    int           seq;

    if (history == 0)
    {
        seq = 0;
    }
    else
    {
        seq = history->seq + 1;

        previous_history = history;
    }

    history = new History(oid,seq,hid,hostname,vm_dir,vmm_mad,vnm_mad,tm_mad);

    history_records.push_back(history);
};

//added by shenxy
/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void VirtualMachine::add_backup(
    int     oid,
    int     vid,
    string& bk_dir,
    int* bid)
{      
    ostringstream os;

    int bkoid = oid;
        
    char bvid[100],boid[100];
    sprintf(bvid,"%d",vid);
    sprintf(boid,"%d",bkoid);
    string v(bvid);
    string o(boid);
    
    bk_dir += "/";
    bk_dir +=v;
    bk_dir +="/";
    bk_dir += v;
    bk_dir += "_";
    bk_dir += o;
    
   *bid = bkoid;

    NebulaLog::log("ONE",Log::INFO,bk_dir);
      
    backup = new Backup(bkoid,vid,bk_dir);
    
   
};

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void VirtualMachine::add_backupdisk(
    int     oid,
    int     vid,
    string& dk_name,
    string& dk_dir)
{      
    ostringstream os;   
      
    backupdisk = new BackupDisk(oid,vid,dk_name,dk_dir);   
   
};


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::cp_history()
{
    History * htmp;

    if (history == 0)
    {
        return;
    }

    htmp = new History(oid,
                       history->seq + 1,
                       history->hid,
                       history->hostname,
                       history->vm_dir,
                       history->vmm_mad_name,
                       history->vnm_mad_name,
                       history->tm_mad_name);


    previous_history = history;
    history          = htmp;

    history_records.push_back(history);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::cp_previous_history()
{
    History * htmp;

    if ( previous_history == 0 || history == 0)
    {
        return;
    }

    htmp = new History(oid,
                       history->seq + 1,
                       previous_history->hid,
                       previous_history->hostname,
                       previous_history->vm_dir,
                       previous_history->vmm_mad_name,
                       previous_history->vnm_mad_name,
                       previous_history->tm_mad_name);

    previous_history = history;
    history          = htmp;

    history_records.push_back(history);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::get_requirements (int& cpu, int& memory, int& disk)
{
    string          scpu;
    istringstream   iss;
    float           fcpu;

    get_template_attribute("MEMORY",memory);
    get_template_attribute("CPU",scpu);

    if ((memory == 0) || (scpu==""))
    {
        cpu    = 0;
        memory = 0;
        disk   = 0;

        return;
    }

    iss.str(scpu);
    iss >> fcpu;

    cpu    = (int) (fcpu * 100);//now in 100%
    memory = memory * 1024;     //now in bytes
    disk   = 0;

    return;
}
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::get_disk_images(string& error_str)
{
    int                   num_disks, rc;
    vector<Attribute  * > disks;
    ImagePool *           ipool;
    VectorAttribute *     disk;
    vector<int>           acquired_images;

    int     n_os = 0; // Number of OS images
    int     n_cd = 0; // Number of CDROMS
    int     n_db = 0; // Number of DATABLOCKS
    string  type;
    int     image_id;

    ostringstream    oss;
    Image::ImageType img_type;

    Nebula& nd = Nebula::instance();
    ipool      = nd.get_ipool();

    num_disks  = obj_template->get("DISK",disks);

    for(int i=0, index=0; i<num_disks; i++)
    {
        disk = dynamic_cast<VectorAttribute * >(disks[i]);

        if ( disk == 0 )
        {
            continue;
        }

        rc = ipool->disk_attribute(disk, i, &index, &img_type, uid, image_id);

        if (rc == 0 )
        {
            acquired_images.push_back(image_id);

            switch(img_type)
            {
                case Image::OS:
                    n_os++;
                    break;
                case Image::CDROM:
                    n_cd++;
                    break;
                case Image::DATABLOCK:
                    n_db++;
                    break;
                default:
                    break;
            }

            if( n_os > 1 )  // Max. number of OS images is 1
            {
                goto error_max_os;
            }

            if( n_cd > 1 )  // Max. number of CDROM images is 1
            {
                goto error_max_cd;
            }

            if( n_db > 10 )  // Max. number of DATABLOCK images is 10
            {
                goto error_max_db;
            }
        }
        else if ( rc == -1 )
        {
            goto error_image;
        }
    }

    return 0;

error_max_os:
    error_str = "VM can not use more than one OS image.";
    goto error_common;

error_max_cd:
    error_str = "VM can not use more than one CDROM image.";
    goto error_common;

error_max_db:
    error_str = "VM can not use more than 10 DATABLOCK images.";
    goto error_common;

error_image:
    error_str = "Could not get disk image for VM.";
    goto error_common;

error_common:
    ImageManager *  imagem  = nd.get_imagem();

    vector<int>::iterator it;

    for ( it=acquired_images.begin() ; it < acquired_images.end(); it++ )
    {
        // Set disk_path and save_id to empty string, this way the image manager
        // won't try to move any files
        imagem->release_image(*it,"",-1,"");
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::release_disk_images()
{
    string  iid;
    string  saveas;
    int     num_disks;

    vector<Attribute const  * > disks;
    ImageManager *              imagem;

    string  disk_base_path = "";

    Nebula& nd = Nebula::instance();
    imagem     = nd.get_imagem();

    num_disks   = get_template_attribute("DISK",disks);

    if (hasHistory() != 0)
    {
        disk_base_path = get_local_dir();
    }

    for(int i=0; i<num_disks; i++)
    {
        VectorAttribute const *  disk =
            dynamic_cast<VectorAttribute const * >(disks[i]);

        if ( disk == 0 )
        {
            continue;
        }

        iid    = disk->vector_value("IMAGE_ID");
        saveas = disk->vector_value("SAVE_AS");

        if ( iid.empty() )
        {
            if (!saveas.empty())
            {
                imagem->disk_to_image(disk_base_path,i,saveas);
            }
        }
        else
        {
            imagem->release_image(iid,disk_base_path,i,saveas);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::get_network_leases(string& estr)
{
    int                   num_nics, rc;
    vector<Attribute  * > nics;
    VirtualNetworkPool *  vnpool;
    VectorAttribute *     nic;

    Nebula& nd = Nebula::instance();
    vnpool     = nd.get_vnpool();

    num_nics   = obj_template->get("NIC",nics);

    for(int i=0; i<num_nics; i++)
    {
        nic = dynamic_cast<VectorAttribute * >(nics[i]);

        if ( nic == 0 )
        {
            continue;
        }

        rc = vnpool->nic_attribute(nic, uid, oid);

        if (rc == -1)
        {
            goto error_vnet; 
        }
    }

    return 0;

error_vnet:
    estr = "Could not get virtual network for VM.";
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::release_network_leases()
{
    Nebula& nd = Nebula::instance();

    VirtualNetworkPool * vnpool = nd.get_vnpool();

    string                        vnid;
    string                        ip;
    int                           num_nics;

    vector<Attribute const  * >   nics;
    VirtualNetwork          *     vn;

    num_nics   = get_template_attribute("NIC",nics);

    for(int i=0; i<num_nics; i++)
    {
        VectorAttribute const *  nic =
            dynamic_cast<VectorAttribute const * >(nics[i]);

        if ( nic == 0 )
        {
            continue;
        }

        vnid = nic->vector_value("NETWORK_ID");

        if ( vnid.empty() )
        {
            continue;
        }

        ip   = nic->vector_value("IP");

        if ( ip.empty() )
        {
            continue;
        }

        vn = vnpool->get(atoi(vnid.c_str()),true);

        if ( vn == 0 )
        {
            continue;
        }

        vn->release_lease(ip);
        vnpool->update(vn);

        vn->unlock();
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::generate_context(string &files)
{
    ofstream file;

    vector<const Attribute*> attrs;
    const VectorAttribute *  context;

    map<string, string>::const_iterator it;

    files = "";

    if ( history == 0 )
        return -1;

    if ( get_template_attribute("CONTEXT",attrs) != 1 )
    {
        log("VM", Log::INFO, "Virtual Machine has no context");
        return 0;
    }

    file.open(history->context_file.c_str(),ios::out);

    if (file.fail() == true)
    {
        ostringstream oss;

        oss << "Could not open context file: " << history->context_file;
        log("VM", Log::ERROR, oss);
        return -1;
    }

    context = dynamic_cast<const VectorAttribute *>(attrs[0]);

    if (context == 0)
    {
        file.close();
        return -1;
    }

    files = context->vector_value("FILES");

    const map<string, string> values = context->value();

    file << "# Context variables generated by OpenNebula\n";

    for (it=values.begin(); it != values.end(); it++ )
    {
        file << it->first <<"=\""<< it->second << "\"" << endl;
    }

    file.close();

    return 1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::save_disk(int disk_id, int img_id, string& error_str)
{
    int                   num_disks;
    vector<Attribute  * > disks;
    VectorAttribute *     disk;

    string                disk_id_str;
    int                   tmp_disk_id;

    ostringstream oss;
    istringstream iss;

    if ( state == DONE || state == FAILED )
    {
        goto error_state;
    }

    num_disks  = obj_template->get("DISK",disks);

    for(int i=0; i<num_disks; i++, iss.clear())
    {
        disk = dynamic_cast<VectorAttribute * >(disks[i]);

        if ( disk == 0 )
        {
            continue;
        }

        disk_id_str = disk->vector_value("DISK_ID");

        iss.str(disk_id_str);
        iss >> tmp_disk_id;

        if ( tmp_disk_id == disk_id )
        {
            if(!((disk->vector_value("SAVE_AS")).empty()))
            {
                goto error_saved;
            }

            if(!((disk->vector_value("PERSISTENT")).empty()))
            {
                goto error_persistent;
            }

            disk->replace("SAVE", "YES");

            oss << (img_id);
            disk->replace("SAVE_AS", oss.str());

            return 0;
        }
    }

    goto error_not_found;

error_state:
    oss << "VM cannot be in DONE or FAILED state.";
    goto error_common;

error_persistent:
    oss << "Source image for DISK " << disk_id << " is persistent.";
    goto error_common;

error_saved:
    oss << "The DISK " << disk_id << " is already going to be saved.";
    goto error_common;

error_not_found:
    oss << "The DISK " << disk_id << " does not exist for VM " << oid << ".";

error_common:
    error_str = oss.str();

    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VirtualMachine::set_auth_request(int uid,
                                      AuthRequest& ar,
                                      VirtualMachineTemplate *tmpl) 
{
    int                   num;
    vector<Attribute  * > vectors;
    VectorAttribute *     vector;

    Nebula& nd = Nebula::instance();

    ImagePool *           ipool  = nd.get_ipool();
    VirtualNetworkPool *  vnpool = nd.get_vnpool();

    num = tmpl->get("DISK",vectors);

    for(int i=0; i<num; i++)
    {

        vector = dynamic_cast<VectorAttribute * >(vectors[i]);

        if ( vector == 0 )
        {
            continue;
        }

        ipool->authorize_disk(vector,uid,&ar);
    }

    vectors.clear();

    num = tmpl->get("NIC",vectors);

    for(int i=0; i<num; i++)
    {
        vector = dynamic_cast<VectorAttribute * >(vectors[i]);

        if ( vector == 0 )
        {
            continue;
        }

        vnpool->authorize_nic(vector,uid,&ar);
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

pthread_mutex_t VirtualMachine::lex_mutex = PTHREAD_MUTEX_INITIALIZER;

extern "C"
{
    typedef struct yy_buffer_state * YY_BUFFER_STATE;

    int vm_var_parse (VirtualMachine * vm,
                      ostringstream *  parsed,
                      char **          errmsg);

    int vm_var_lex_destroy();

    YY_BUFFER_STATE vm_var__scan_string(const char * str);

    void vm_var__delete_buffer(YY_BUFFER_STATE);
}

/* -------------------------------------------------------------------------- */

int VirtualMachine::parse_template_attribute(const string& attribute,
                                             string&       parsed)
{
    YY_BUFFER_STATE  str_buffer = 0;
    const char *     str;
    int              rc;
    ostringstream    oss_parsed;
    char *           error_msg = 0;

    pthread_mutex_lock(&lex_mutex);

    str        = attribute.c_str();
    str_buffer = vm_var__scan_string(str);

    if (str_buffer == 0)
    {
        goto error_yy;
    }

    rc = vm_var_parse(this,&oss_parsed,&error_msg);

    vm_var__delete_buffer(str_buffer);

    vm_var_lex_destroy();

    pthread_mutex_unlock(&lex_mutex);

    if ( rc != 0 && error_msg != 0 )
    {
        ostringstream oss;

        oss << "Error parsing: " << attribute << ". " << error_msg;
        log("VM",Log::ERROR,oss);

        free(error_msg);
    }

    parsed = oss_parsed.str();

    return rc;

error_yy:
    log("VM",Log::ERROR,"Error setting scan buffer");
    pthread_mutex_unlock(&lex_mutex);
    return -1;
}
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& VirtualMachine::to_xml(string& xml) const
{
    return to_xml_extended(xml,false);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& VirtualMachine::to_xml_extended(string& xml) const
{
    return to_xml_extended(xml,true);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

string& VirtualMachine::to_xml_extended(string& xml, bool extended) const
{
    string template_xml;
    string history_xml;
    string perm_xml;
    ostringstream	oss;

    oss << "<VM>"
        << "<ID>"        << oid       << "</ID>"
        << "<UID>"       << uid       << "</UID>"
        << "<GID>"       << gid       << "</GID>"
        << "<UNAME>"     << uname     << "</UNAME>" 
        << "<GNAME>"     << gname     << "</GNAME>" 
        << "<NAME>"      << name      << "</NAME>"
        << perms_to_xml(perm_xml)
        << "<LAST_POLL>" << last_poll << "</LAST_POLL>"
        << "<STATE>"     << state     << "</STATE>"
        << "<LCM_STATE>" << lcm_state << "</LCM_STATE>"
        << "<STIME>"     << stime     << "</STIME>"
        << "<ETIME>"     << etime     << "</ETIME>"
        << "<DEPLOY_ID>" << deploy_id << "</DEPLOY_ID>"
        << "<MEMORY>"    << memory    << "</MEMORY>"
        << "<CPU>"       << cpu       << "</CPU>"
        << "<NET_TX>"    << net_tx    << "</NET_TX>"
        << "<NET_RX>"    << net_rx    << "</NET_RX>"
        << obj_template->to_xml(template_xml);

    if ( hasHistory() )
    {
        oss << "<HISTORY_RECORDS>";

        if ( extended )
        {
            for (unsigned int i=0; i < history_records.size(); i++)
            {
                oss << history_records[i]->to_xml(history_xml);
            }
        }
        else
        {
            oss << history->to_xml(history_xml);
        }

        oss << "</HISTORY_RECORDS>";
    }
    else
    {
        oss << "<HISTORY_RECORDS/>";
    }

    oss << "</VM>";

    xml = oss.str();

    return xml;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int VirtualMachine::from_xml(const string &xml_str)
{
    vector<xmlNodePtr> content;

    int istate;
    int ilcmstate;
    int rc = 0;

    // Initialize the internal XML object
    update_from_str(xml_str);

    // Get class base attributes
    rc += xpath(oid,       "/VM/ID",    -1);

    rc += xpath(uid,       "/VM/UID",   -1);
    rc += xpath(gid,       "/VM/GID",   -1);

    rc += xpath(uname,     "/VM/UNAME", "not_found");
    rc += xpath(gname,     "/VM/GNAME", "not_found");
    rc += xpath(name,      "/VM/NAME",  "not_found");

    rc += xpath(last_poll, "/VM/LAST_POLL", 0);
    rc += xpath(istate,    "/VM/STATE",     0);
    rc += xpath(ilcmstate, "/VM/LCM_STATE", 0);

    rc += xpath(stime,     "/VM/STIME",    0);
    rc += xpath(etime,     "/VM/ETIME",    0);
    rc += xpath(deploy_id, "/VM/DEPLOY_ID","");

    rc += xpath(memory,    "/VM/MEMORY",   0);
    rc += xpath(cpu,       "/VM/CPU",      0);
    rc += xpath(net_tx,    "/VM/NET_TX",   0);
    rc += xpath(net_rx,    "/VM/NET_RX",   0);

    // Permissions
    rc += perms_from_xml();

    state     = static_cast<VmState>(istate);
    lcm_state = static_cast<LcmState>(ilcmstate);

    // Get associated classes
    ObjectXML::get_nodes("/VM/TEMPLATE", content);

    if (content.empty())
    {
        return -1;
    }

    // Virtual Machine template
    rc += obj_template->from_xml_node(content[0]);

    // Last history entry
    ObjectXML::free_nodes(content);
    content.clear();

    ObjectXML::get_nodes("/VM/HISTORY_RECORDS/HISTORY", content);

    if (!content.empty())
    {
        history = new History(oid);
        rc += history->from_xml_node(content[0]);

        history_records.resize(history->seq + 1);
        history_records[history->seq] = history;

        ObjectXML::free_nodes(content);
    }

    if (rc != 0)
    {
        return -1;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
