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

#include "TransferManager.h"
#include "NebulaLog.h"

#include "Nebula.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

extern "C" void * tm_action_loop(void *arg)
{
    TransferManager *  tm;

    if ( arg == 0 )
    {
        return 0;
    }

    tm = static_cast<TransferManager *>(arg);

    NebulaLog::log("TrM",Log::INFO,"Transfer Manager started.");

    tm->am.loop(0,0);

    NebulaLog::log("TrM",Log::INFO,"Transfer Manager stopped.");

    return 0;
}

/* -------------------------------------------------------------------------- */

int TransferManager::start()
{
    int               rc;
    pthread_attr_t    pattr;

    rc = MadManager::start();

    if ( rc != 0 )
    {
        return -1;
    }

    NebulaLog::log("TrM",Log::INFO,"Starting Transfer Manager...");

    pthread_attr_init (&pattr);
    pthread_attr_setdetachstate (&pattr, PTHREAD_CREATE_JOINABLE);

    rc = pthread_create(&tm_thread,&pattr,tm_action_loop,(void *) this);

    return rc;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::trigger(Actions action, int _vid)
{
    int *   vid;
    string  aname;

    vid = new int(_vid);

    switch (action)
    {
    case PROLOG:
        aname = "PROLOG";
        break;

    case PROLOG_MIGR:
        aname = "PROLOG_MIGR";
        break;

    case PROLOG_RESUME:
        aname = "PROLOG_RESUME";
        break;

    case PROLOG_BACKUP:                       //added by shenxy
        aname = "PROLOG_BACKUP";
        break;

   case PROLOG_RECOVER:                       //added by shenxy
        aname = "PROLOG_RECOVER";           
        break;     

   case PROLOG_BACKUPDISK:                      //added by shenxy
        aname = "PROLOG_BACKUPDISK";
        break;

    case EPILOG:
        aname = "EPILOG";
        break;

    case EPILOG_STOP:
        aname = "EPILOG_STOP";
        break;

    case EPILOG_DELETE:
        aname = "EPILOG_DELETE";
        break;

    case EPILOG_DELETE_PREVIOUS:
        aname = "EPILOG_DELETE_PREVIOUS";
        break;

    case CHECKPOINT:
        aname = "CHECKPOINT";
        break;

    case DRIVER_CANCEL:
        aname = "DRIVER_CANCEL";
        break;

    case DELETE_RECOVER_FAIL:                       //added by shenxy       
        aname = "DELETE_RECOVER_FAIL";
        break; 

    case RECOVER_DECOMPRESS_FAIL:                       //added by shenxy       
        aname = "RECOVER_DECOMPRESS_FAIL";
        break; 

    case DELETE_BACKUP_FAIL:                       //added by shenxy       
        aname = "DELETE_BACKUP_FAIL";
        break; 

    case DELETEBAK:                                        //added by shenxy
        aname = "DELETEBAK";
        break; 
    
    case ON:                                                     //added by shenxy
        aname = "ON";
        break;       

    case DELETE_BACKUP_COMPRESS:              //added by shenxy
        aname = "DELETE_BACKUP_COMPRESS";
        break;       

   case PROLOG_RECOVER_DECOMPRESS:              //added by shenxy
        aname = "PROLOG_RECOVER_DECOMPRESS";
        break;   
        
    case FINALIZE:
        aname = ACTION_FINALIZE;
        break;

    default:
        delete vid;
        return;
    }

    am.trigger(aname,vid);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::do_action(const string &action, void * arg)
{
    int vid;

    if (arg == 0)
    {
        return;
    }

    vid  = *(static_cast<int *>(arg));

    delete static_cast<int *>(arg);

    if (action == "PROLOG")
    {
        prolog_action(vid);
    }
    else if (action == "PROLOG_MIGR")
    {
        prolog_migr_action(vid);
    }
    else if (action == "PROLOG_RESUME")
    {
        prolog_resume_action(vid);
    }
    else if (action == "PROLOG_BACKUP")
    {
        prolog_backup_action(vid);
    }
    else if (action == "PROLOG_RECOVER")
    {
        prolog_recover_action(vid);
    }
    else if (action == "PROLOG_BACKUPDISK")
    {
        prolog_backupdisk_action(vid);
    } 
    else if (action == "EPILOG")
    {
        epilog_action(vid);
    }
    else if (action == "EPILOG_STOP")
    {
        epilog_stop_action(vid);
    }
    else if (action == "EPILOG_DELETE")
    {
        epilog_delete_action(vid);
    }
    else if (action == "EPILOG_DELETE_PREVIOUS")
    {
        epilog_delete_previous_action(vid);
    }
    else if (action == "CHECKPOINT")
    {
        checkpoint_action(vid);
    }
    else if (action == "DRIVER_CANCEL")
    {
        driver_cancel_action(vid);
    }
    else if (action == "DELETE_RECOVER_FAIL")
    {
        delete_recover_fail_action(vid);
    }
    else if (action == "RECOVER_DECOMPRESS_FAIL")
    {
        recover_decompress_fail_action(vid);
    }
    else if (action == "DELETE_BACKUP_FAIL")
    {
        delete_backup_fail_action(vid);
    }
    else if (action == "DELETEBAK")
    {
        epilog_deletebak_action(vid);
    }
    else if (action == "DELETE_BACKUP_COMPRESS")
    {
         epilog_deletecps_action(vid);
    }
    else if (action == "PROLOG_RECOVER_DECOMPRESS")
    {
         epilog_decompress_action(vid);
    }
    else if (action == "ON")
    {
        prolog_on_action(vid);
    }
    else if (action == ACTION_FINALIZE)
    {
        NebulaLog::log("TrM",Log::INFO,"Stopping Transfer Manager...");

        MadManager::stop();
    }
    else
    {
        ostringstream oss;
        oss << "Unknown action name: " << action;

        NebulaLog::log("TrM", Log::ERROR, oss);
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::prolog_action(int vid)
{
    ofstream      xfr;
    ostringstream os;
    string        xfr_name;

    const VectorAttribute * disk;
    string source;
    string type;
    string clon;
    string files;
    string size;
    string format;

    VirtualMachine * vm;
    Nebula&          nd = Nebula::instance();

    const TransferManagerDriver * tm_md;

    vector<const Attribute *> attrs;
    int                       num;

    int  context_result;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".prolog";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Swap and image Commands
    // ------------------------------------------------------------------------

    num = vm->get_template_attribute("DISK",attrs);

    for (int i=0; i < num ;i++,source="",type="",clon="")
    {
        disk = dynamic_cast<const VectorAttribute *>(attrs[i]);

        if ( disk == 0 )
        {
            continue;
        }

        type = disk->vector_value("TYPE");

        if ( type.empty() == false)
        {
            transform(type.begin(),type.end(),type.begin(),
                (int(*)(int))toupper);
        }

        if ( type == "SWAP" )
        {
            // -----------------------------------------------------------------
            // Generate a swap disk image
            // -----------------------------------------------------------------
            size = disk->vector_value("SIZE");

            if (size.empty()==true)
            {
                vm->log("TM",Log::WARNING,"No size in swap image, skipping");
                continue;
            }

            xfr << "MKSWAP " << size << " " << vm->get_hostname() << ":"
                << vm->get_remote_dir() << "/disk." << i << endl;
        }
        else if ( type == "FS" )
        {
            // -----------------------------------------------------------------
            // Create a clean file system disk image
            // -----------------------------------------------------------------
            size   = disk->vector_value("SIZE");
            format = disk->vector_value("FORMAT");

            if ( size.empty() || format.empty())
            {
                vm->log("TM",Log::WARNING,"No size or format in plain FS image,"
                        " skipping");
                continue;
            }

            xfr << "MKIMAGE " << size << " " << format << " "
                << vm->get_hostname() << ":" << vm->get_remote_dir()
                << "/disk." << i << endl;
        }
        else
        {
            // -----------------------------------------------------------------
            // CLONE or LINK disk images
            // -----------------------------------------------------------------
            clon = disk->vector_value("CLONE");
            size = disk->vector_value("SIZE");

            if ( clon.empty() == true )
            {
                clon = "YES"; //Clone by default
            }
            else
            {
                transform(clon.begin(),clon.end(),clon.begin(),
                    (int(*)(int))toupper);
            }

            if (clon == "YES")
            {
                xfr << "CLONE ";
            }
            else
            {
                xfr << "LN ";
            }

            // -----------------------------------------------------------------
            // Get the disk image, and set source URL
            // -----------------------------------------------------------------
            source = disk->vector_value("SOURCE");

            if ( source.empty() )
            {
                goto error_empty_disk;
            }

            if ( source.find(":") == string::npos ) //Regular file
            {
                xfr << nd.get_nebula_hostname() << ":" << source << " ";
            }
            else //TM Plugin specific protocol
            {
                xfr << source << " ";
            }

            xfr << vm->get_hostname() << ":" << vm->get_remote_dir()
                << "/disk." << i;

            if (!size.empty()) //Add size for dev based disks
            {
                xfr << " " << size;
            }

            xfr << endl;
        }
    }

    // ------------------------------------------------------------------------
    // Generate context file (There are 0...num-1 disks, constext is disk.num)
    // ------------------------------------------------------------------------

    context_result = vm->generate_context(files);

    if ( context_result == -1 )
    {
        goto error_context;
    }

    if ( context_result )
    {
        xfr << "CONTEXT " << vm->get_context_file() << " ";

        if (!files.empty())
        {
            xfr << files << " ";
        }

        xfr <<  vm->get_hostname() << ":" << vm->get_remote_dir()
            << "/disk." << num << endl;
    }

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_context:
    os.str("");
    os << "prolog, could not write context file for VM " << vid;
    goto error_common;

error_history:
    os.str("");
    os << "prolog, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog, error getting driver " << vm->get_tm_mad();
    goto error_common;

error_empty_disk:
    os.str("");
    os << "prolog, undefined source disk image in VM template";
    xfr.close();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vm->unlock();
    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::prolog_migr_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;


    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".migrate";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Move image directory
    // ------------------------------------------------------------------------

    xfr << "MV ";
    xfr << vm->get_previous_hostname() << ":" << vm->get_remote_dir() << " ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_migr, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_migr, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_migr, error getting driver " << vm->get_tm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vm->unlock();
    return;
}

//added by shenxy
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

void TransferManager::prolog_backup_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;
    string          bkdir;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();    

    const TransferManagerDriver * tm_md;


    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".backup";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Backup image directory
    // ------------------------------------------------------------------------

    xfr << "BK_CPS ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << " ";
    xfr << nd.get_nebula_hostname() << ":" << vm->get_bk_dir()  <<"/images"<<
endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_backup, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_backup, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_backup, error getting driver " << vm->get_tm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL );
    vm->unlock();
    return;
}


/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::prolog_recover_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    
    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;


    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".recover";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Move image directory
    // ------------------------------------------------------------------------
   if (vm->get_state() == VirtualMachine::FAILED )
    {
        vm->set_state(VirtualMachine::ACTIVE);
        vmpool->update(vm);
    }
   else
    {
        xfr << "RENAME ";
        xfr << vm->get_hostname() << ":" << vm->get_remote_dir()<< " ";
        xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << "_bak" << 
endl;
    }
    
    xfr << "BK ";
    xfr << nd.get_nebula_hostname() << ":" << vm->recover_dir<<"/images"<< " ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() <<"_compress"<< endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_recover, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_recover, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_recover, error getting driver " << vm->get_tm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);
    vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL );

    vm->unlock();
    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::prolog_backupdisk_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();

    const TransferManagerDriver * tm_md;
    

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {        
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".backupdisk";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Move image directory
    // ------------------------------------------------------------------------

    xfr << "MKVMIMAGE ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() <<"/disk.0"<< " ";
    xfr << nd.get_nebula_hostname() << ":" << vm->get_dk_dir() 
         << "/" << vm->get_dk_name() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_backup, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_backup, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_backup, error getting driver " << vm->get_tm_mad();
    goto error_common;

error_common:    
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);    
    
    vm->unlock();
    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::prolog_resume_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;


    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".resume";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Move image directory
    // ------------------------------------------------------------------------

    xfr << "MV ";
    xfr << nd.get_nebula_hostname() << ":" << vm->get_local_dir() << "/images ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_resume, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_resume, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_resume, error getting driver " << vm->get_tm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vm->unlock();
    return;
}

//added by shenxy 20120228
/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::delete_backup_action(string bkdir,int vid)
{
    Nebula&     nd = Nebula::instance();

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    string delete_backup_cmd;
    delete_backup_cmd = "/opt/nebula/ONE/lib/tm_commands/ssh/delete_backup.sh " + nd.get_nebula_hostname() +" "+ bkdir ;
	char const *cmd = delete_backup_cmd.c_str();
	system(cmd);


    return;
}



/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::epilog_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    const VectorAttribute * disk;
    string          save;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;

    vector<const Attribute *>   attrs;
    int                         num;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".epilog";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // copy back VM image (DISK with SAVE="yes")
    // ------------------------------------------------------------------------

    num = vm->get_template_attribute("DISK",attrs);

    for (int i=0; i < num ;i++,save="")
    {
        disk = dynamic_cast<const VectorAttribute *>(attrs[i]);

        if ( disk == 0 )
        {
            continue;
        }

        save = disk->vector_value("SAVE");

        if ( save.empty() == true)
        {
            continue;
        }

        transform(save.begin(),save.end(),save.begin(),(int(*)(int))toupper);

        if ( save == "YES" )
        {
            xfr << "MV " << vm->get_hostname() << ":" << vm->get_remote_dir()
                << "/disk." << i << " "
                << nd.get_nebula_hostname() << ":" << vm->get_local_dir()
                << "/disk." << i << endl;
        }
    }

    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "epilog, error getting driver " << vm->get_vmm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::EPILOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vm->unlock();
    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::epilog_stop_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;


    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".stop";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Move image directory
    // ------------------------------------------------------------------------

    xfr << "MV ";
    xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << " ";
    xfr << nd.get_nebula_hostname() << ":" << vm->get_local_dir() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_stop, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_stop, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_stop, error getting driver " << vm->get_tm_mad();

error_common:
    (nd.get_lcm())->trigger(LifeCycleManager::EPILOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);

    vm->unlock();
    return;
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::epilog_delete_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".delete";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_delete, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_delete, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_delete, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::epilog_delete_previous_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory() || !vm->hasPreviousHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_previous_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".delete_prev";
    xfr.open(xfr_name.c_str(),ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
    xfr << "DELETE " << vm->get_previous_hostname() <<":"<< vm->get_remote_dir()
        << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_delete_previous, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_delete, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_previous_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_delete, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_previous_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::driver_cancel_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Get the Driver for this host
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    // ------------------------------------------------------------------------
    // Cancel the current operation
    // ------------------------------------------------------------------------
    
    tm_md->driver_cancel(vid);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "driver_cancel, VM " << vid << " has no history";
    goto error_common;

error_driver:
    os.str("");
    os << "driver_cancel, error getting driver " << vm->get_vmm_mad();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void TransferManager::checkpoint_action(int vid)
{

}

/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

void TransferManager::prolog_on_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;
    Nebula&             nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();

    const TransferManagerDriver * tm_md;
    

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
             return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }

    xfr_name = vm->get_transfer_file() + ".xm_list";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // xm list host
    // ------------------------------------------------------------------------

    xfr << "XMLIST ";
    xfr << vm->get_hostname() << endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "prolog_on_xm_list, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "prolog_on_xm_list, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "prolog_on_xm_list, error getting driver " << vm->get_tm_mad();
    goto error_common;

error_common:
    vm->log("TM", Log::ERROR, "xm list ");    
    vm->unlock();
    
    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::delete_recover_fail_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;
    Nebula&             nd = Nebula::instance();
    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".recover_fail";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
     xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir()
          <<"_compress" << endl;

     xfr << "RENAME ";
     xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << "_bak" << " ";
     xfr << vm->get_hostname() << ":" << vm->get_remote_dir()<<endl;
      
    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_delete, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_delete, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_delete, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::recover_decompress_fail_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;
    Nebula&             nd = Nebula::instance();
    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".decompress_fail";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
     xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir()
          <<"_compress" << endl;

    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir()
          << endl;

     xfr << "RENAME ";
     xfr << vm->get_hostname() << ":" << vm->get_remote_dir() << "_bak" << " ";
     xfr << vm->get_hostname() << ":" << vm->get_remote_dir()<<endl;
      
    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_delete, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_delete, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_delete, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}


/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::delete_backup_fail_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;
    Nebula&             nd = Nebula::instance();
    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".delete_backup";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
     xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() << 
      "_compress"<<endl;
  
    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "delete_backup_fail, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "delete_backup_fail, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "delete_backup_fail, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}


/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::epilog_deletebak_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".deletebak";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() 
         << "_bak" <<endl;

    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() 
         << "_compress" <<endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_delete, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_delete, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_delete, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::epilog_deletecps_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".deletecps";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Directory
    // ------------------------------------------------------------------------
    
    xfr << "DELETE " << vm->get_hostname() <<":"<< vm->get_remote_dir() 
         << "_compress" <<endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_deletecps, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_deletecps, could not open file: " << xfr_name;
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_deletecps, error getting driver " << vm->get_vmm_mad();
    os << ". You may need to manually clean " << vm->get_hostname() 
       << ":" << vm->get_remote_dir();

error_common:
    vm->log("TM", Log::ERROR, os);
    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void TransferManager::epilog_decompress_action(int vid)
{
    ofstream        xfr;
    ostringstream   os;
    string          xfr_name;

    VirtualMachine *    vm;

    Nebula&             nd = Nebula::instance();

    const TransferManagerDriver * tm_md;

    // ------------------------------------------------------------------------
    // Setup & Transfer script
    // ------------------------------------------------------------------------

    vm = vmpool->get(vid,true);

    if (vm == 0)
    {
        return;
    }

    if (!vm->hasHistory())
    {
        goto error_history;
    }

    tm_md = get(vm->get_tm_mad());

    if ( tm_md == 0 )
    {
        goto error_driver;
    }
    
    xfr_name = vm->get_transfer_file() + ".decompress";
    xfr.open(xfr_name.c_str(), ios::out | ios::trunc);

    if (xfr.fail() == true)
    {
        goto error_file;
    }

    // ------------------------------------------------------------------------
    // Delete the remote VM Compress Directory
    // ------------------------------------------------------------------------
    
    xfr << "DECOMPRESS " ;
    xfr << vm->get_hostname() <<":"<< vm->get_remote_dir()<<endl;

    xfr.close();

    tm_md->transfer(vid,xfr_name);

    vm->unlock();

    return;

error_history:
    os.str("");
    os << "epilog_decompress, VM " << vid << " has no history";
    goto error_common;

error_file:
    os.str("");
    os << "epilog_decompress, could not open file: " << xfr_name;
    goto error_common;

error_driver:
    os.str("");
    os << "epilog_decompress, error getting driver " << vm->get_tm_mad();

error_common:
    vm->set_state(VirtualMachine::PROLOG_RECOVER);
    vmpool->update(vm);   
    
    (nd.get_lcm())->trigger(LifeCycleManager::PROLOG_FAILURE,vid);
    vm->log("TM", Log::ERROR, os);
    vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL );

    vm->unlock();
    return;
}


/* ************************************************************************** */
/* MAD Loading                                                                                                                */ 
/* ************************************************************************** */

void TransferManager::load_mads(int uid)
{
    unsigned int                    i;
    ostringstream                   oss;
    const VectorAttribute *         vattr;
    int                             rc;
    string                          name;
    TransferManagerDriver *         tm_driver = 0;

    oss << "Loading Transfer Manager drivers.";

    NebulaLog::log("TM",Log::INFO,oss);

    for(i=0,oss.str("");i<mad_conf.size();i++,oss.str(""),tm_driver=0)
    {
        vattr = static_cast<const VectorAttribute *>(mad_conf[i]);

        name  = vattr->vector_value("NAME");

        oss << "\tLoading driver: " << name;
        NebulaLog::log("VMM", Log::INFO, oss);

        tm_driver = new TransferManagerDriver(
                uid,
                vattr->value(),
                (uid != 0),
                vmpool);

        if ( tm_driver == 0 )
            continue;

        rc = add(tm_driver);

        if ( rc == 0 )
        {
            oss.str("");
            oss << "\tDriver " << name << " loaded.";

            NebulaLog::log("TM",Log::INFO,oss);
        }
    }
}
