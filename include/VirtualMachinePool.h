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

#ifndef VIRTUAL_MACHINE_POOL_H_
#define VIRTUAL_MACHINE_POOL_H_

#include "PoolSQL.h"
#include "VirtualMachine.h"

#include <time.h>

using namespace std;


/**
 *  The Virtual Machine Pool class. ...
 */
class VirtualMachinePool : public PoolSQL
{
public:

    VirtualMachinePool(SqlDB * db,
                       vector<const Attribute *> hook_mads,
                       const string& hook_location,
                       const string& remotes_location);

    ~VirtualMachinePool(){};

    /**
     *  Function to allocate a new VM object
     *    @param uid user id (the owner of the VM)
     *    @param gid the id of the group this object is assigned to
     *    @param vm_template a VM Template object describing the VM
     *    @param oid the id assigned to the VM (output)
     *    @param error_str Returns the error reason, if any
     *    @param on_hold flag to submit on hold
     *    @return oid on success, -1 error inserting in DB or -2 error parsing
     *  the template
     */
    int allocate (
        int                      uid,
        int                      gid,
        const string&            uname,
        const string&            gname,
        VirtualMachineTemplate * vm_template,
        int *                    oid,
        string&                  error_str,
        bool                     on_hold = false);

     //added by shenxy 20120227
     /* Function to get  backup recover info*/
     void  stateinfo(int oid ,int * state);

     //added by shenxy 20120228
    /* Function to get  backupdisk info*/
    void diskstateinfo(int oid,int* state);

     //-----------------------------------------added by shenxy 20120229
     /* Function to allocate a new VM backup object id*/
     int bkoid();

      /* Function to update VM backup state*/
     void  updatebkstate(int oid,Backup::BkState  state);

     /* Function to update  backup VM*/
    int update_backup(
        VirtualMachine * vm)
    {
        return vm->update_backup(db);
    }

     /* Function to check  backup id*/
     int checkbid(int bid);

    /* Function to get  backup url*/ 
     void  bkdir(int oid,string* bkrecover);


    /* Function to update  backupdisk VM*/
    int update_backupdisk(
        VirtualMachine * vm)
    {
        return vm->update_backupdisk(db);
    }

    /* Function to allocate a new VM backupdisk object id*/
    int dkoid();

     
    //--------------------------------------------------added by shenxy 
  
    /**
     *  Function to get a VM from the pool, if the object is not in memory
     *  it is loade from the DB
     *    @param oid VM unique id
     *    @param lock locks the VM mutex
     *    @return a pointer to the VM, 0 if the VM could not be loaded
     */
    VirtualMachine * get(
        int     oid,
        bool    lock)
    {
        return static_cast<VirtualMachine *>(PoolSQL::get(oid,lock));
    };

    /**
     *  Function to get the IDs of running VMs
     *   @param oids a vector that contains the IDs
     *   @param vm_limit Max. number of VMs returned
     *   @param last_poll Return only VMs which last_poll is less than or equal
     *          to this value.
     *   @return 0 on success
     */
    int get_running(
        vector<int>&    oids,
        int             vm_limit,
        time_t          last_poll);

    /**
     *  Function to get the IDs of pending VMs
     *   @param oids a vector that contains the IDs
     *   @return 0 on success
     */
    int get_pending(
        vector<int>&    oids);

    //--------------------------------------------------------------------------
    // Virtual Machine DB access functions
    //--------------------------------------------------------------------------

    /**
     *  Updates the history record of a VM, the vm's mutex SHOULD be locked
     *    @param vm pointer to the virtual machine object
     *    @return 0 on success
     */
    int update_history(
        VirtualMachine * vm)
    {
        return vm->update_history(db);
    }
     
     /**
     *  Updates the backup record of a VM, the vm's mutex SHOULD be locked
     *    @param vm pointer to the virtual machine object
     *    @return 0 on success
     */
    int insert_backup(                             //added by shenxy 20120229
        VirtualMachine * vm)
    {
        return vm->insert_backup(db);
    }

     /**
     *  Updates the backupdisk record of a VM, the vm's mutex SHOULD be locked
     *    @param vm pointer to the virtual machine object
     *    @return 0 on success
     */
    int insert_backupdisk(                          //added by shenxy 20120302
        VirtualMachine * vm)
    {
        return vm->insert_backupdisk(db);
    }

    /**
     *  Updates the previous history record, the vm's mutex SHOULD be locked
     *    @param vm pointer to the virtual machine object
     *    @return 0 on success
     */
    int update_previous_history(
        VirtualMachine * vm)
    {
        return vm->update_previous_history(db);
    }

    /**
     *  Bootstraps the database table(s) associated to the VirtualMachine pool
     *    @return 0 on success
     */
    static int bootstrap(SqlDB * _db)
    {
        return VirtualMachine::bootstrap(_db);
    };

    /**
     *  Dumps the VM pool in XML format. A filter can be also added to the query
     *  Also the hostname where the VirtualMachine is running is added to the
     *  pool
     *  @param oss the output stream to dump the pool contents
     *  @param where filter for the objects, defaults to all
     *
     *  @return 0 on success
     */
    int dump(ostringstream& oss, const string& where)
    {
        return PoolSQL::dump(oss, "VM_POOL", VirtualMachine::table, where);
    }

private:
    /**
     *  Factory method to produce VM objects
     *    @return a pointer to the new VM
     */
    PoolObjectSQL * create()
    {
        return new VirtualMachine(-1,-1,-1,"","",0);
    };
};

#endif /*VIRTUAL_MACHINE_POOL_H_*/
