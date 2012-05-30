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

#ifndef VIRTUAL_MACHINE_H_
#define VIRTUAL_MACHINE_H_

#include "VirtualMachineTemplate.h"
#include "PoolSQL.h"
#include "History.h"
#include "Backup.h"
#include "BackupDisk.h"
#include "Log.h"
#include "NebulaLog.h"

#include <time.h>
#include <sstream>

using namespace std;

class AuthRequest;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/**
 *  The Virtual Machine class. It represents a VM...
 */
class VirtualMachine : public PoolObjectSQL
{
public:
    // -------------------------------------------------------------------------
    // VM States
    // -------------------------------------------------------------------------

    /**
     *  Global Virtual Machine state
     */
    enum VmState
    {
        INIT      = 0,
        PENDING   = 1,
        HOLD      = 2,
        ACTIVE    = 3,
        STOPPED   = 4,
        SUSPENDED = 5,
        DONE      = 6,
        FAILED    = 7
    };

    /**
     *  Virtual Machine state associated to the Life-cycle Manager
     */
    enum LcmState
    {
        LCM_INIT       = 0,
        PROLOG         = 1,
        BOOT           = 2,
        RUNNING        = 3,
        MIGRATE        = 4,
        SAVE_STOP      = 5,
        SAVE_SUSPEND   = 6,
        SAVE_MIGRATE   = 7,
        PROLOG_MIGRATE = 8,
        PROLOG_RESUME  = 9,
        EPILOG_STOP    = 10,
        EPILOG         = 11,
        SHUTDOWN       = 12,
        CANCEL         = 13,
        FAILURE        = 14,
        CLEANUP        = 15,
        UNKNOWN        = 16,
        OFF                 = 17,
        OFF_BACKUP  =18,
        PROLOG_BACKUP = 19,
        OFF_RECOVER             =  20,     //added by shenxy
        PROLOG_RECOVER = 21,
        DELETEBAK = 22,
        DELETE_BACKUP_FAIL = 23,
        DELETE_RECOVER_FAIL = 24,
        OFF_BACKUPDISK      =25,
        PROLOG_BACKUPDISK   =26,
        ON  = 28,
        DELETE_BACKUP_COMPRESS = 29,
        PROLOG_RECOVER_DECOMPRESS =30,
        RECOVER_DECOMPRESS_FAIL =31
    };

    // -------------------------------------------------------------------------
    // Log & Print
    // -------------------------------------------------------------------------

    /**
     *  writes a log message in vm.log. The class lock should be locked and
     *  the VM MUST BE obtained through the VirtualMachinePool get() method.
     */
    void log(
        const char *            module,
        const Log::MessageType  type,
        const ostringstream&    message) const
    {
        if (_log != 0)
        {
            _log->log(module,type,message.str().c_str());
        }
    };

    /**
     *  writes a log message in vm.log. The class lock should be locked and
     *  the VM MUST BE obtained through the VirtualMachinePool get() method.
     */
    void log(
        const char *            module,
        const Log::MessageType  type,
        const char *            message) const
    {
        if (_log != 0)
        {
            _log->log(module,type,message);
        }
    };

    /**
     * Function to print the VirtualMachine object into a string in
     * XML format
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml(string& xml) const;

    /**
     * Function to print the VirtualMachine object into a string in
     * XML format, with extended information (full history records)
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml_extended(string& xml) const;

    /**
     *  Rebuilds the object from an xml formatted string
     *    @param xml_str The xml-formatted string
     *
     *    @return 0 on success, -1 otherwise
     */
    int from_xml(const string &xml_str);

    // ------------------------------------------------------------------------
    // Dynamic Info
    // ------------------------------------------------------------------------

    /**
     *  Updates VM dynamic information (id).
     *   @param _deploy_id the VMM driver specific id
     */
    void update_info(
        const string& _deploy_id)
    {
        deploy_id = _deploy_id;
    };

    /**
     *  Updates VM dynamic information (usage counters).
     *   @param _memory used by the VM (total)
     *   @param _cpu used by the VM (rate)
     *   @param _net_tx transmitted bytes (total)
     *   @param _net_tx received bytes (total)
     */
    void update_info(
        const int _memory,
        const int _cpu,
        const int _net_tx,
        const int _net_rx)
    {
        if (_memory != -1)
        {
            memory = _memory;
        }

        if (_cpu != -1)
        {
            cpu    = _cpu;
        }

        if (_net_tx != -1)
        {
            net_tx = _net_tx;
        }

        if (_net_rx != -1)
        {
            net_rx = _net_rx;
        }
    };

    /**
     *  Returns the deployment ID
     *    @return the VMM driver specific ID
     */
    const string& get_deploy_id() const
    {
        return deploy_id;
    };

    /**
     *  Sets the VM exit time
     *    @param _et VM exit time (when it arraived DONE/FAILED states)
     */
    void set_exit_time(time_t et)
    {
        etime = et;
    };

    // ------------------------------------------------------------------------
    // History
    // ------------------------------------------------------------------------
    /**
     *  Adds a new history record an writes it in the database.
     */
    void add_history(
        int     hid,
        const string& hostname,
        const string& vm_dir,
        const string& vmm_mad,
        const string& vnm_mad,
        const string& tm_mad);

     //-----------------------------------------------------added by shenxy
     /**
     *  Adds a new backup record an writes it in the database.
     */
    void add_backup(
        int                        oid,
        int                        vid,
       string&                   bk_dir,
       int *                        bid);

     /**
     *  Sets the backup state that originated the VM backup state the previous host
     *    @param _state backup state to leave this host
     */
    void set_bkstate(Backup::BkState _state)
    {
        backup->state =_state;
    }; 

     /**
     *  Sets backup  start time of a VM.
     *    @param _stime time when the VM started
     */
    void set_bkstime(time_t _stime)
    {
        backup->stime=_stime;
    };

     /**
     *  Sets backup end time of a VM.
     *    @param _etime time when the VM finished
     */
    void set_bketime(time_t _etime)
    {
        backup->etime=_etime;
    };

      /**
     *  Updates the VM backup record
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert_backup(SqlDB * db)
    {
       string error_str = "insert backup";   
        if ( backup != 0 )
        {
            return backup->insert(db,error_str);
        }
        else
            return -1;
    };

     /**
     *  Updates the VM backupdisk record
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert_backupdisk(SqlDB * db)                 //added by shenxy 20120302
    {
       string error_str = "insert backupdisk";   
        if ( backupdisk != 0 )
        {
            return backupdisk->insert(db,error_str);
        }
        else
            return -1;
    };


      /**
     *  Returns the VM backup url
     *  function MUST be called before this one.
     *    @return the VM backup   url
     */
    const string & get_bk_dir() const
    {
        return backup->bk_dir;
    };

    /* Updates VM dynamic bakup information */
    int update_backup(SqlDB * db)
    {
      
        if ( backup != 0 )
        {
            return backup->update(db);
        }
        else
            return -1;
    };


     //-------------------------------------------------------------------
     /**
     *  Adds a new backupdisk record an writes it in the database.
     */
     void add_backupdisk(
        int                        oid,
        int                        vid,
       string&                   dk_name,
       string&                   dk_dir);

      /**
     *  Sets the backupdisk state that originated the VM backup state the previous host
     *    @param _state backupdisk state to leave this host
     */
     void set_dkstate(BackupDisk::DkState _state)
    {
        backupdisk->state =_state;
    };

    /* Updates VM dynamic bakupdisk information */
    int update_backupdisk(SqlDB * db)
    {
        if ( backupdisk != 0 )
        {
            return backupdisk->update(db);
        }
        else
            return -1;
    };

     /**
     *  Returns the VM backupdisk url
     *  function MUST be called before this one.
     *    @return the VM backupdisk   url
     */
    const string & get_dk_dir() const
    {
        return backupdisk->dk_dir;
    };

    /**
     *  Returns the VM backupdisk name
     *  function MUST be called before this one.
     *    @return the VM backupdisk  name
     */
    const string & get_dk_name() const
    {
        return backupdisk->dk_name;
    };

    

    //--------------------------------------------------------------------

    /**
     *  Duplicates the last history record. Only the host related fields are
     *  affected (i.e. no counter is copied nor initialized).
     *    @param reason explaining the new addition.
     */
    void cp_history();

    /**
     *  Duplicates the previous history record. Only the host related fields are
     *  affected (i.e. no counter is copied nor initialized).
     *    @param reason explaining the new addition.
     */
    void cp_previous_history();

    /**
     *  Checks if the VM has a valid history record. This function
     *  MUST be called before using any history related function.
     *    @return true if the VM has a record
     */
    bool hasHistory() const
    {
        return (history!=0);
    };

    /**
     *  Checks if the VM has a valid previous history record. This function
     *  MUST be called before using any previous_history related function.
     *    @return true if the VM has a previous record
     */
    bool hasPreviousHistory() const
    {
        return (previous_history!=0);
    };

    /**
     *  Returns the VMM driver name for the current host. The hasHistory()
     *  function MUST be called before this one.
     *    @return the VMM mad name
     */
    const string & get_vmm_mad() const
    {
        return history->vmm_mad_name;
    };

    /**
     *  Returns the VMM driver name for the previous host. The hasPreviousHistory()
     *  function MUST be called before this one.
     *    @return the VMM mad name
     */
    const string & get_previous_vmm_mad() const
    {
        return previous_history->vmm_mad_name;
    };

    /**
     *  Returns the VNM driver name for the current host. The hasHistory()
     *  function MUST be called before this one.
     *    @return the VNM mad name
     */
    const string & get_vnm_mad() const
    {
        return history->vnm_mad_name;
    };

    /**
     *  Returns the VNM driver name for the previous host. The hasPreviousHistory()
     *  function MUST be called before this one.
     *    @return the VNM mad name
     */
    const string & get_previous_vnm_mad() const
    {
        return previous_history->vnm_mad_name;
    };

    /**
     *  Returns the TM driver name for the current host. The hasHistory()
     *  function MUST be called before this one.
     *    @return the TM mad name
     */
    const string & get_tm_mad() const
    {
        return history->tm_mad_name;
    };

    /**
     *  Returns the TM driver name for the previous host. The
     *  hasPreviousHistory() function MUST be called before this one.
     *    @return the TM mad name
     */
    const string & get_previous_tm_mad() const
    {
        return previous_history->tm_mad_name;
    };

    /**
     *  Returns the transfer filename. The transfer file is in the form:
     *          $ONE_LOCATION/var/$VM_ID/transfer.$SEQ
     *  or, in case that OpenNebula is installed in root
     *          /var/lib/one/$VM_ID/transfer.$SEQ
     *  The hasHistory() function MUST be called before this one.
     *    @return the transfer filename
     */
    const string & get_transfer_file() const
    {
        return history->transfer_file;
    };

    /**
     *  Returns the deployment filename. The deployment file is in the form:
     *          $ONE_LOCATION/var/$VM_ID/deployment.$SEQ
     *  or, in case that OpenNebula is installed in root
     *          /var/lib/one/$VM_ID/deployment.$SEQ
     *  The hasHistory() function MUST be called before this one.
     *    @return the deployment filename
     */
    const string & get_deployment_file() const
    {
        return history->deployment_file;
    };

    /**
     *  Returns the context filename. The context file is in the form:
     *          $ONE_LOCATION/var/$VM_ID/context.sh
     *  or, in case that OpenNebula is installed in root
     *          /var/lib/one/$VM_ID/context.sh
     *  The hasHistory() function MUST be called before this one.
     *    @return the deployment filename
     */
    const string & get_context_file() const
    {
        return history->context_file;
    }

    /**
     *  Returns the remote deployment filename. The file is in the form:
     *          $VM_DIR/$VM_ID/images/deployment.$SEQ
     *  The hasHistory() function MUST be called before this one.
     *    @return the deployment filename
     */
    const string & get_remote_deployment_file() const
    {
        return history->rdeployment_file;
    };

    /**
     *  Returns the checkpoint filename for the current host. The checkpoint file
     *  is in the form:
     *          $VM_DIR/$VM_ID/images/checkpoint
     *  The hasHistory() function MUST be called before this one.
     *    @return the checkpoint filename
     */
    const string & get_checkpoint_file() const
    {
        return history->checkpoint_file;
    };

    /**
     *  Returns the remote VM directory. The VM remote dir is in the form:
     *          $VM_DIR/$VM_ID/
     *  or, in case that OpenNebula is installed in root
     *          /var/lib/one/$VM_ID/
     *  The hasHistory() function MUST be called before this one.
     *    @return the remote directory
     */
    const string & get_remote_dir() const
    {
        return history->vm_rhome;
    };

    /**
     *  Returns the local VM directory. The VM local dir is in the form:
     *          $ONE_LOCATION/var/$VM_ID/
     *  The hasHistory() function MUST be called before this one.
     *    @return the remote directory
     */
    const string & get_local_dir() const
    {
        return history->vm_lhome;
    };

    /**
     *  Returns the hostname for the current host. The hasHistory()
     *  function MUST be called before this one.
     *    @return the hostname
     */
    const string & get_hostname() const
    {
        return history->hostname;
    };

    /**
     *  Returns the hostname for the previous host. The hasPreviousHistory()
     *  function MUST be called before this one.
     *    @return the hostname
     */
    const string & get_previous_hostname() const
    {
        return previous_history->hostname;
    };

    /**
     *  Returns the reason that originated the VM migration in the previous host
     *    @return the migration reason to leave this host
     */
    const History::MigrationReason get_previous_reason() const
    {
        return previous_history->reason;
    };

    /**
     *  Get host id where the VM is or is going to execute. The hasHistory()
     *  function MUST be called before this one.
     */
    int get_hid()
    {
        return history->hid;
    }

    /**
     *  Get host id where the VM was executing. The hasPreviousHistory()
     *  function MUST be called before this one.
     */
    int get_previous_hid()
    {
        return previous_history->hid;
    }

    /**
     *  Sets start time of a VM.
     *    @param _stime time when the VM started
     */
    void set_stime(time_t _stime)
    {
        history->stime=_stime;
    };

    /**
     *  Sets end time of a VM.
     *    @param _etime time when the VM finished
     */
    void set_etime(time_t _etime)
    {
        history->etime=_etime;
    };

    /**
     *  Sets end time of a VM in the previous Host.
     *    @param _etime time when the VM finished
     */
    void set_previous_etime(time_t _etime)
    {
        previous_history->etime=_etime;
    };

    /**
     *  Sets start time of VM prolog.
     *    @param _stime time when the prolog started
     */
    void set_prolog_stime(time_t _stime)
    {
        history->prolog_stime=_stime;
    };

    /**
     *  Sets end time of VM prolog.
     *    @param _etime time when the prolog finished
     */
    void set_prolog_etime(time_t _etime)
    {
        history->prolog_etime=_etime;
    };

    /**
     *  Sets start time of VM running state.
     *    @param _stime time when the running state started
     */
    void set_running_stime(time_t _stime)
    {
        history->running_stime=_stime;
    };

    /**
     *  Sets end time of VM running state.
     *    @param _etime time when the running state finished
     */
    void set_running_etime(time_t _etime)
    {
        history->running_etime=_etime;
    };

    /**
     *  Sets end time of VM running state in the previous host.
     *    @param _etime time when the running state finished
     */
    void set_previous_running_etime(time_t _etime)
    {
        previous_history->running_etime=_etime;
    };

    /**
     *  Sets start time of VM epilog.
     *    @param _stime time when the epilog started
     */
    void set_epilog_stime(time_t _stime)
    {
        history->epilog_stime=_stime;
    };

    /**
     *  Sets end time of VM epilog.
     *    @param _etime time when the epilog finished
     */
    void set_epilog_etime(time_t _etime)
    {
        history->epilog_etime=_etime;
    };

    /**
     *  Sets the reason that originated the VM migration
     *    @param _reason migration reason to leave this host
     */
    void set_reason(History::MigrationReason _reason)
    {
        history->reason=_reason;
    };

    /**
     *  Sets the reason that originated the VM migration in the previous host
     *    @param _reason migration reason to leave this host
     */
    void set_previous_reason(History::MigrationReason _reason)
    {
        previous_history->reason=_reason;
    };

    // ------------------------------------------------------------------------
    // Template
    // ------------------------------------------------------------------------
    /**
     *  Parse a string and substitute variables (e.g. $NAME) using the VM
     *  template values:
     *    @param attribute, the string to be parsed
     *    @param parsed, the resulting parsed string
     *    @return 0 on success.
     */
    int  parse_template_attribute(const string& attribute, string& parsed);
    
    /**
     *  Factory method for virtual machine templates
     */
    Template * get_new_template()
    {
        return new VirtualMachineTemplate;
    }

    // ------------------------------------------------------------------------
    // States
    // ------------------------------------------------------------------------
    /**
     *  Returns the VM state (Dispatch Manager)
     *    @return the VM state
     */
    VmState get_state() const
    {
        return state;
    };

    /**
     *  Returns the VM state (life-cycle Manager)
     *    @return the VM state
     */
    LcmState get_lcm_state() const
    {
        return lcm_state;
    };

    /**
     *  Sets VM state
     *    @param s state
     */
    void set_state(VmState s)
    {
        state = s;
    };

    /**
     *  Sets VM LCM state
     *    @param s state
     */
    void set_state(LcmState s)
    {
        lcm_state = s;
    };

    // ------------------------------------------------------------------------
    // Timers
    // ------------------------------------------------------------------------
    /**
     *  Gets time from last information polling.
     *    @return time of last poll (epoch) or 0 if never polled
     */
    time_t get_last_poll() const
    {
        return last_poll;
    };

    /**
     *  Sets time of last information polling.
     *    @param poll time in epoch, normally time(0)
     */
    void set_last_poll(time_t poll)
    {
        last_poll = poll;
    };

    /**
     *  Get the VM physical requirements for the host.
     *    @param cpu
     *    @param memory
     *    @param disk
     */
    void get_requirements (int& cpu, int& memory, int& disk);

    // ------------------------------------------------------------------------
    // Network Leases & Disk Images
    // ------------------------------------------------------------------------
    /**
     *  Get all network leases for this Virtual Machine
     *  @return 0 if success
     */
    int get_network_leases(string &error_str);

    /**
     *  Releases all network leases taken by this Virtual Machine
     */
    void release_network_leases();

    /**
     *  Get all disk images for this Virtual Machine
     *  @param error_str Returns the error reason, if any
     *  @return 0 if success
     */
    int get_disk_images(string &error_str);

    /**
     *  Releases all disk images taken by this Virtual Machine
     */
    void release_disk_images();

    // ------------------------------------------------------------------------
    // Context related functions
    // ------------------------------------------------------------------------
    /**
     *  Writes the context file for this VM, and gets the paths to be included
     *  in the context block device (CBD)
     *    @param  files space separated list of paths to be included in the CBD
     *    @return 0 if success
     */
    int  generate_context(string &files);

    // ------------------------------------------------------------------------
    // Image repository related functions
    // ------------------------------------------------------------------------
    /**
     *  Set the SAVE_AS attribute for the "disk_id"th disk.
     *    @param  disk_id Index of the disk to save
     *    @param  img_id ID of the image this disk will be saved to.
     *    @return 0 if success
     */
    int  save_disk(int disk_id, int img_id, string& error_str);

    // ------------------------------------------------------------------------
    // Authorization related functions
    // ------------------------------------------------------------------------
    /**
     *  Sets an authorization request for a VirtualMachine template based on
     *  the images and networks used
     *    @param  uid for template owner
     *    @param  ar the AuthRequest object
     *    @param  tmpl the virtual machine template
     */
    static void set_auth_request(int uid, 
                                 AuthRequest& ar, 
                                 VirtualMachineTemplate *tmpl);
    
    /**
     *  Last  VM backup id
     */ 
   int                     bakbid;          //added by shenxy

   /* VM recover url*/
   string                  recover_dir;

   /* VM update backup state id*/
   int                      updatebkstateoid;

    /* VM lcm state*/
    LcmState            lcmstate;

     /**
     *  Last  VM backupdisk id
     */ 
     int                     dkbid;           //added by shenxy


private:

    // -------------------------------------------------------------------------
    // Friends
    // -------------------------------------------------------------------------
    friend class VirtualMachinePool;

    // *************************************************************************
    // Virtual Machine Attributes
    // *************************************************************************

    // -------------------------------------------------------------------------
    // VM Scheduling & Managing Information
    // -------------------------------------------------------------------------
    /**
     *  Last time (in epoch) that the VM was polled to get its status
     */
    time_t      last_poll;

    // -------------------------------------------------------------------------
    // Virtual Machine Description
    // -------------------------------------------------------------------------
    /**
     *  The state of the virtual machine.
     */
    VmState     state;

    /**
     *  The state of the virtual machine (in the Life-cycle Manager).
     */
    LcmState    lcm_state;

    /**
     *  Start time, the VM enter the nebula system (in epoch)
     */
    time_t      stime;

    /**
     *  Exit time, the VM leave the nebula system (in epoch)
     */
    time_t      etime;

    /**
     *  Deployment specific identification string, as returned by the VM driver
     */
    string      deploy_id;

    /**
     *  Memory in Megabytes used by the VM
     */
    int         memory;

    /**
     *  CPU usage (percent)
     */
    int         cpu;

    /**
     *  Network usage, transmitted Kilobytes
     */
    int         net_tx;

    /**
     *  Network usage, received Kilobytes
     */
    int         net_rx;

    /**
     *  History record, for the current host
     */
    History *   history;

     //added by shenxy
     /**
     *  Backup record, for the current host
     */
    Backup *   backup;

    /**
     *  Backupdisk record, for the current host
     */
    BackupDisk * backupdisk;

    /**
     *  History record, for the previous host
     */
    History *   previous_history;


    /**
     *  Complete set of history records for the VM
     */
    vector<History *> history_records;

    // -------------------------------------------------------------------------
    // Logging
    // -------------------------------------------------------------------------

    /**
     *  Log class for the virtual machine, it writes log messages in
     *          $ONE_LOCATION/var/$VID/vm.log
     *  or, in case that OpenNebula is installed in root
     *          /var/log/one/$VM_ID.log
     */
    FileLog *       _log;

    // *************************************************************************
    // DataBase implementation (Private)
    // *************************************************************************

    /**
     *  Bootstraps the database table(s) associated to the VirtualMachine
     *    @return 0 on success
     */
    static int bootstrap(SqlDB * db)
    {
        int rc;

        ostringstream oss_vm(VirtualMachine::db_bootstrap);
        ostringstream oss_hist(History::db_bootstrap);
        //added by shenxy
        ostringstream oss_bist(Backup::db_bootstrap);
        ostringstream oss_dist(BackupDisk::db_bootstrap);

        rc =  db->exec(oss_vm);
        rc += db->exec(oss_hist);
        //added by shenxy
        rc += db->exec(oss_bist);
        rc += db->exec(oss_dist);

        return rc;
    };

    /**
     *  Callback function to unmarshall a VirtualMachine object
     *  (VirtualMachine::select)
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int select_cb(void *nil, int num, char **names, char ** values);

    /**
     *  Execute an INSERT or REPLACE Sql query.
     *    @param db The SQL DB
     *    @param replace Execute an INSERT or a REPLACE
     *    @param error_str Returns the error reason, if any
     *    @return 0 one success
     */
    int insert_replace(SqlDB *db, bool replace, string& error_str);

    /**
     *  Updates the VM history record
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update_history(SqlDB * db)
    {
        if ( history != 0 )
        {
            return history->update(db);
        }
        else
            return -1;
    };

    /**
     *  Updates the previous history record
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update_previous_history(SqlDB * db)
    {
        if ( previous_history != 0 )
        {
            return previous_history->update(db);
        }
        else
            return -1;
    };

    // -------------------------------------------------------------------------
    // Attribute Parser
    // -------------------------------------------------------------------------

    /**
     * Mutex to perform just one attribute parse at a time
     */
    static pthread_mutex_t lex_mutex;

    /**
     *  Parse the "CONTEXT" attribute of the template by substituting
     *  $VARIABLE, $VARIABLE[ATTR] and $VARIABLE[ATTR, ATTR = VALUE]
     *    @param error_str Returns the error reason, if any
     *    @return 0 on success
     */
    int parse_context(string& error_str);

    /**
     *  Parse the "REQUIREMENTS" attribute of the template by substituting
     *  $VARIABLE, $VARIABLE[ATTR] and $VARIABLE[ATTR, ATTR = VALUE]
     *    @param error_str Returns the error reason, if any
     *    @return 0 on success
     */
    int parse_requirements(string& error_str);

    /**
     *  Parse the "GRAPHICS" attribute and generates a default PORT if not
     *  defined
     */
    void parse_graphics();

    /**
     *  Function that renders the VM in XML format optinally including
     *  extended information (all history records)
     *  @param xml the resulting XML string
     *  @param extended include additional info if true
     *  @return a reference to the generated string
     */
    string& to_xml_extended(string& xml, bool extended) const;

protected:

    //**************************************************************************
    // Constructor
    //**************************************************************************

    VirtualMachine(int id, 
                   int uid,
                   int gid, 
                   const string& uname,
                   const string& gname,
                   VirtualMachineTemplate * _vm_template);

    virtual ~VirtualMachine();

    // *************************************************************************
    // DataBase implementation
    // *************************************************************************

    static const char * table;

    static const char * db_names;

    static const char * db_bootstrap;

    /**
     *  Reads the Virtual Machine (identified with its OID) from the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int select(SqlDB * db);

    /**
     *  Writes the Virtual Machine and its associated template in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int insert(SqlDB * db, string& error_str);

    /**
     *  Writes/updates the Virtual Machine data fields in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    int update(SqlDB * db)
    {
        string error_str;
        return insert_replace(db, true, error_str);
    }

    /**
     * Deletes a VM from the database and all its associated information
     *   @param db pointer to the db
     *   @return -1
     */
    int drop(SqlDB * db)
    {
        NebulaLog::log("ONE",Log::ERROR, "VM Drop not implemented!");
        return -1;
    }
};

#endif /*VIRTUAL_MACHINE_H_*/
