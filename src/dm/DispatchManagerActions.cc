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

#include "DispatchManager.h"
#include "NebulaLog.h"

#include "Nebula.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::deploy (
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
        return -1;
    }

    vid = vm->get_oid();

    oss << "Deploying VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if ( vm->get_state() == VirtualMachine::PENDING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        vm->set_state(VirtualMachine::ACTIVE);

        vmpool->update(vm);

        vm->log("DiM", Log::INFO, "New VM state is ACTIVE.");

        lcm->trigger(LifeCycleManager::DEPLOY,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:

    oss.str("");
    oss << "Could not deploy VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -1;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::migrate(
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
        return -1;
    }

    vid = vm->get_oid();

    oss << "Migrating VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::MIGRATE,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not migrate VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -1;
}


//added by shenxy
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

int DispatchManager::backup(
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
        return -1;
    }

    vid = vm->get_oid();

    oss << "Backuping VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::BACKUP,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not backup VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);     

    vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL );
    vm->unlock();
    return -1;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

int DispatchManager::recover(
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
        return -1;
    }

    vid = vm->get_oid();

    oss << "recover VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);
  
    Nebula&             nd  = Nebula::instance();
    LifeCycleManager *  lcm = nd.get_lcm();

    lcm->trigger(LifeCycleManager::RECOVER,vid);    
    
    vm->unlock();

    return 0;

}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

int DispatchManager::backupdisk(
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
          return -1;
    }

    vid = vm->get_oid();

    oss << "BackupDisk  VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::BACKUPDISK,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not backupdisk VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);
    
    vm->unlock();
    return -1;
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::live_migrate(
    VirtualMachine *    vm)
{
    ostringstream oss;
    int           vid;

    if ( vm == 0 )
    {
        return -1;
    }

    vid = vm->get_oid();

    oss << "Live-migrating VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::LIVE_MIGRATE,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not live-migrate VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -1;
}

/* ************************************************************************** */
/* ************************************************************************** */

int DispatchManager::shutdown (
    int vid)
{
    ostringstream       oss;
    VirtualMachine *    vm;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Shutting down VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::SHUTDOWN,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:

    oss.str("");
    oss << "Could not shutdown VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::hold(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Holding VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state() == VirtualMachine::PENDING)
    {
        vm->set_state(VirtualMachine::HOLD);

        vmpool->update(vm);

        vm->log("DiM", Log::INFO, "New VM state is HOLD.");
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:

    oss.str("");
    oss << "Could not hold VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::release(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Releasing VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state() == VirtualMachine::HOLD)
    {
        vm->set_state(VirtualMachine::PENDING);

        vmpool->update(vm);

        vm->log("DiM", Log::INFO, "New VM state is PENDING.");
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not release VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::stop(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Stopping VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::STOP,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not stop VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::cancel(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Cancelling VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        vm->unlock();

        lcm->trigger(LifeCycleManager::CANCEL,vid);
    }
    else
    {
        goto error;
    }

    return 0;

error:
    oss.str("");
    oss << "Could not cancel VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

//addec by shenxy 20120228
/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

int DispatchManager::off(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Offing VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        vm->unlock();

        lcm->trigger(LifeCycleManager::OFF,vid);
    }
    else
    {
        goto error;
    }

    return 0;

error:
    oss.str("");
    oss << "Could not off VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

int DispatchManager::on(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Oning VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::OFF )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        vm->unlock();

        lcm->trigger(LifeCycleManager::ON,vid);
    }
    else
    {
        goto error;
    }

    return 0;

error:
    oss.str("");
    oss << "Could not on VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::suspend(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Suspending VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::SUSPEND,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not suspend VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::resume(
    int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Resuming VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state() == VirtualMachine::STOPPED )
    {
        vm->set_state(VirtualMachine::PENDING);

        vmpool->update(vm);

        vm->log("DiM", Log::INFO, "New VM state is PENDING.");
    }
    else if (vm->get_state() == VirtualMachine::SUSPENDED)
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        vm->set_state(VirtualMachine::ACTIVE);

        vmpool->update(vm);

        vm->log("DiM", Log::INFO, "New VM state is ACTIVE.");

        lcm->trigger(LifeCycleManager::RESTORE,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not resume VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();
    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::restart(int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Restarting VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state() == VirtualMachine::ACTIVE &&
        (vm->get_lcm_state() == VirtualMachine::UNKNOWN ||
         vm->get_lcm_state() == VirtualMachine::BOOT))
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::RESTART,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not restart VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();

    return -2;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::reboot(int vid)
{
    VirtualMachine *    vm;
    ostringstream       oss;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    oss << "Rebooting VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    if (vm->get_state()     == VirtualMachine::ACTIVE &&
        vm->get_lcm_state() == VirtualMachine::RUNNING )
    {
        Nebula&             nd  = Nebula::instance();
        LifeCycleManager *  lcm = nd.get_lcm();

        lcm->trigger(LifeCycleManager::REBOOT,vid);
    }
    else
    {
        goto error;
    }

    vm->unlock();

    return 0;

error:
    oss.str("");
    oss << "Could not reboot VM " << vid << ", wrong state.";
    NebulaLog::log("DiM",Log::ERROR,oss);

    vm->unlock();

    return -2;
}
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::finalize(
    int vid)
{
    VirtualMachine * vm;
    ostringstream    oss;
    VirtualMachine::VmState state;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    state = vm->get_state();

    oss << "Finalizing VM " << vid;
    NebulaLog::log("DiM",Log::DEBUG,oss);

    Nebula&            nd  = Nebula::instance();
    TransferManager *  tm  = nd.get_tm();
    LifeCycleManager * lcm = nd.get_lcm();

    switch (state)
    {
        case VirtualMachine::SUSPENDED:
        case VirtualMachine::FAILED:
            tm->trigger(TransferManager::EPILOG_DELETE,vid);

        case VirtualMachine::INIT:
        case VirtualMachine::PENDING:
        case VirtualMachine::HOLD:
        case VirtualMachine::STOPPED:
            vm->set_exit_time(time(0));

            vm->set_state(VirtualMachine::LCM_INIT);
            vm->set_state(VirtualMachine::DONE);
            vmpool->update(vm);

            vm->release_network_leases();

            vm->release_disk_images();

            vm->log("DiM", Log::INFO, "New VM state is DONE.");
        break;

        case VirtualMachine::ACTIVE:
            lcm->trigger(LifeCycleManager::DELETE,vid);
        break;
        case VirtualMachine::DONE:
        break;
    }

    vm->unlock();

    return 0;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int DispatchManager::resubmit(int vid)
{
    VirtualMachine * vm;
    ostringstream    oss;
    int              rc = 0;

    Nebula&             nd  = Nebula::instance();
    LifeCycleManager *  lcm = nd.get_lcm();
    TransferManager *   tm  = nd.get_tm();

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return -1;
    }

    switch (vm->get_state())
    {
        case VirtualMachine::SUSPENDED:
            NebulaLog::log("DiM",Log::ERROR,
                "Can not resubmit a suspended VM. Resume it first");
            rc = -2;
        break;

        case VirtualMachine::INIT: // No need to do nothing here
        case VirtualMachine::PENDING:
        break;

        case VirtualMachine::FAILED: //Cleanup VM host files
            tm->trigger(TransferManager::EPILOG_DELETE,vid);
        case VirtualMachine::HOLD: // Move the VM to PENDING in any of these
        case VirtualMachine::STOPPED:
            vm->set_state(VirtualMachine::LCM_INIT);
            vm->set_state(VirtualMachine::PENDING);
            vmpool->update(vm);

            vm->log("DiM", Log::INFO, "New VM state is PENDING.");
        break;

        case VirtualMachine::ACTIVE: //Cleanup VM resources before PENDING
            lcm->trigger(LifeCycleManager::CLEAN,vid);
        break;
        case VirtualMachine::DONE:
            NebulaLog::log("DiM",Log::ERROR,
                "Can not resubmit a VM already in DONE state");
            rc = -2;
        break;
    }

    vm->unlock();

    return rc;
}
