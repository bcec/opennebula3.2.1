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

#include "LifeCycleManager.h"
#include "Nebula.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::save_success_action(int vid)
{
    VirtualMachine *    vm;
    ostringstream       os;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    if ( vm->get_lcm_state() == VirtualMachine::SAVE_MIGRATE )
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
        int                 cpu,mem,disk;
        time_t              the_time = time(0);

        //----------------------------------------------------
        //                PROLOG_MIGRATE STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_MIGRATE);

        vmpool->update(vm);

        vm->set_previous_etime(the_time);

        vm->set_previous_running_etime(the_time);

        vm->set_previous_reason(History::USER);

        vmpool->update_previous_history(vm);

        vm->set_prolog_stime(the_time);

        vmpool->update_history(vm);

        vm->get_requirements(cpu,mem,disk);

        hpool->del_capacity(vm->get_previous_hid(),cpu,mem,disk);

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_MIGRATE");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_MIGR,vid);
    }
    else if (vm->get_lcm_state() == VirtualMachine::SAVE_SUSPEND)
    {
        Nebula&             nd = Nebula::instance();
        DispatchManager *   dm = nd.get_dm();
        int                 cpu,mem,disk;
        time_t              the_time = time(0);

        //----------------------------------------------------
        //                SUSPENDED STATE
        //----------------------------------------------------

        vm->set_running_etime(the_time);

        vm->set_etime(the_time);

        vm->set_reason(History::STOP_RESUME);

        vmpool->update_history(vm);

        vm->get_requirements(cpu,mem,disk);

        hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

        //----------------------------------------------------

        dm->trigger(DispatchManager::SUSPEND_SUCCESS,vid);
    }
    else if ( vm->get_lcm_state() == VirtualMachine::SAVE_STOP)
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
        time_t              the_time = time(0);

        //----------------------------------------------------
        //                 EPILOG_STOP STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::EPILOG_STOP);

        vmpool->update(vm);

        vm->set_epilog_stime(the_time);

        vm->set_running_etime(the_time);

        vm->set_reason(History::STOP_RESUME);

        vmpool->update_history(vm);

        vm->log("LCM", Log::INFO, "New VM state is EPILOG_STOP");

        //----------------------------------------------------

        tm->trigger(TransferManager::EPILOG_STOP,vid);
    }
    /*
    //added by shenxy 
    else if ( vm->get_lcm_state() == VirtualMachine::SAVE_BACKUP)   
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
        //time_t              the_time = time(0);

        //----------------------------------------------------
        //                 PROLOG_BACKUP STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_BACKUP);

        vmpool->update(vm);        

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_BACKUP ");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_BACKUP,vid);
    }*/
    /*
    else if ( vm->get_lcm_state() == VirtualMachine::SAVE_RECOVER )
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();       

        //----------------------------------------------------
        //                 PROLOG_RECOVER STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_RECOVER);

        vmpool->update(vm);        

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_RECOVER ");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_RECOVER,vid);
    }*/
    /*
    else if ( vm->get_lcm_state() == VirtualMachine::SAVE_BACKUPDISK)
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();

        //----------------------------------------------------
        //                 PROLOG_BACKUPDISK STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_BACKUPDISK);

        vmpool->update(vm);        

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_BACKUPDISK ");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_BACKUPDISK,vid);
    }*/
    else
    {
        vm->log("LCM",Log::ERROR,"save_success_action, VM in a wrong state");
    }

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::save_failure_action(int vid)
{
    VirtualMachine *    vm;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    if ( vm->get_lcm_state() == VirtualMachine::SAVE_MIGRATE )
    {
        int                     cpu,mem,disk;
        time_t                  the_time = time(0);

        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();

        //----------------------------------------------------
        //           RUNNING STATE FROM SAVE_MIGRATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::RUNNING);

        vm->set_etime(the_time);

        vm->set_reason(History::ERROR);

        vmpool->update_history(vm);

        vm->get_requirements(cpu,mem,disk);

        hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

        vm->set_previous_etime(the_time);

        vm->set_previous_running_etime(the_time);

        vm->set_previous_reason(History::USER);

        vmpool->update_previous_history(vm);

        // --- Add new record by copying the previous one

        vm->cp_previous_history();

        vmpool->update(vm); //update last_seq & state

        vm->set_stime(the_time);

        vm->set_running_stime(the_time);

        vmpool->update_history(vm);

        vm->log("LCM", Log::INFO, "Fail to save VM state while migrating."
                " Assuming that the VM is still RUNNING (will poll VM).");

        //----------------------------------------------------

        vmm->trigger(VirtualMachineManager::POLL,vid);
    }
    else if ( vm->get_lcm_state() == VirtualMachine::SAVE_SUSPEND ||
                  vm->get_lcm_state() == VirtualMachine::SAVE_STOP
                  //vm->get_lcm_state() == VirtualMachine::SAVE_BACKUP ||
                  //vm->get_lcm_state() == VirtualMachine::SAVE_RECOVER ||
                 // vm->get_lcm_state() == VirtualMachine::SAVE_BACKUPDISK
                 )
    {
        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();

        //-----------------------------------------------------------------
        //    RUNNING STATE FROM SAVE_SUSPEND OR SAVE_STOP 
        //-----------------------------------------------------------------
         /*
         if ( vm->get_lcm_state() == VirtualMachine::SAVE_RECOVER)
        {    
            vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL );
         }*/
         /*
         if ( vm->get_lcm_state() == VirtualMachine::SAVE_BACKUP)
        {    
            vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL );
         }   
         */
         /*
        if ( vm->get_lcm_state() == VirtualMachine::SAVE_BACKUPDISK)
        {    
            vmpool->updatedkstate(vm->dkbid,BackupDisk::BACKUPDISK_FAIL );
         }   */

        vm->set_state(VirtualMachine::RUNNING);

        vmpool->update(vm);

        vm->log("LCM", Log::INFO, "Fail to save VM state."
                " Assuming that the VM is still RUNNING (will poll VM).");

          //----------------------------------------------------

          vmm->trigger(VirtualMachineManager::POLL,vid);
    }

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::deploy_success_action(int vid)
{
    VirtualMachine *    vm;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //                 RUNNING STATE
    //----------------------------------------------------

    if ( vm->get_lcm_state() == VirtualMachine::MIGRATE )
    {
        int     cpu,mem,disk;
        time_t  the_time = time(0);

        vm->set_running_stime(the_time);

        vmpool->update_history(vm);

        vm->set_previous_etime(the_time);

        vm->set_previous_running_etime(the_time);

        vm->set_previous_reason(History::USER);

        vmpool->update_previous_history(vm);

        vm->get_requirements(cpu,mem,disk);

        hpool->del_capacity(vm->get_previous_hid(),cpu,mem,disk);
    }

    if (vm->lcmstate == VirtualMachine::PROLOG_RECOVER_DECOMPRESS)        //added by shenxy
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
                     
       vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_SUCESS);

       vm->set_state(VirtualMachine::DELETEBAK);

       vmpool->update(vm);

       vm->lcmstate = VirtualMachine::RUNNING ;

        vm->log("LCM", Log::INFO, "New VM state is DELETEBAK");

        //----------------------------------------------------

        tm->trigger(TransferManager::DELETEBAK,vid);
             
     }
    else if (vm->lcmstate == VirtualMachine::PROLOG_BACKUP)        //added by shenxy
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
                     
        vm->set_state(VirtualMachine::DELETE_BACKUP_COMPRESS);

        vmpool->update(vm);

        vm->lcmstate = VirtualMachine::RUNNING ;

        vm->log("LCM", Log::INFO, "New VM state is DELETE_BACKUP_COMPRESS");

        //----------------------------------------------------

        tm->trigger(TransferManager::DELETE_BACKUP_COMPRESS,vid);
             
     }

    vm->set_state(VirtualMachine::RUNNING);

    vmpool->update(vm);

    vm->log("LCM", Log::INFO, "New VM state is RUNNING");

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::deploy_failure_action(int vid)
{

    VirtualMachine *    vm;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    if ( vm->get_lcm_state() == VirtualMachine::MIGRATE )
    {
        int     cpu,mem,disk;
        time_t  the_time = time(0);

        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();

        //----------------------------------------------------
        //           RUNNING STATE FROM MIGRATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::RUNNING);

        vmpool->update(vm);

        vm->set_etime(the_time);

        vm->set_reason(History::ERROR);

        vm->set_previous_etime(the_time);

        vm->set_previous_running_etime(the_time);

        vm->set_previous_reason(History::USER);

        vmpool->update_previous_history(vm);

        vm->get_requirements(cpu,mem,disk);

        hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

        // --- Add new record by copying the previous one

        vm->cp_previous_history();

        vmpool->update(vm); //update last_seq & state

        vm->set_stime(the_time);

        vm->set_running_stime(the_time);

        vmpool->update_history(vm);

        vm->log("LCM", Log::INFO, "Fail to life migrate VM."
                " Assuming that the VM is still RUNNING (will poll VM).");

        //----------------------------------------------------

        vmm->trigger(VirtualMachineManager::POLL,vid);
    }
    else if (vm->get_lcm_state() == VirtualMachine::BOOT)
    {
        time_t  the_time = time(0);

        vm->set_running_etime(the_time);

        failure_action(vm);
    }

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::shutdown_success_action(int vid)
{
    Nebula&             nd = Nebula::instance();
    TransferManager *   tm = nd.get_tm();
    VirtualMachine *    vm;
    time_t              the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //                   EPILOG STATE
    //----------------------------------------------------

    vm->set_state(VirtualMachine::EPILOG);

    vmpool->update(vm);

    vm->set_epilog_stime(the_time);

    vm->set_running_etime(the_time);

    vmpool->update_history(vm);

    vm->log("LCM", Log::INFO, "New VM state is EPILOG");

    //----------------------------------------------------

    tm->trigger(TransferManager::EPILOG,vid);

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::shutdown_failure_action(int vid)
{
    VirtualMachine *        vm;

    Nebula&                 nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //    RUNNING STATE FROM SHUTDOWN
    //----------------------------------------------------

    vm->set_state(VirtualMachine::RUNNING);

    vmpool->update(vm);

    vm->log("LCM", Log::INFO, "Fail to shutdown VM."
            " Assuming that the VM is still RUNNING (will poll VM).");

    //----------------------------------------------------

    vmm->trigger(VirtualMachineManager::POLL,vid);

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::prolog_success_action(int vid)
{
    Nebula&                 nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();
    VirtualMachine *        vm;
    time_t                  the_time = time(0);
    ostringstream           os;

    TransferManager *   tm = nd.get_tm();

    VirtualMachineManager::Actions action;
    VirtualMachine::LcmState       lcm_state;

    vm = vmpool->get(vid, true);

    if ( vm == 0 )
    {
        return;
    }

    lcm_state = vm->get_lcm_state();
     
    if (lcm_state == VirtualMachine::PROLOG)
    {
        action = VirtualMachineManager::DEPLOY;
    }
    else if ( lcm_state == VirtualMachine::PROLOG_MIGRATE ||
                 lcm_state == VirtualMachine::PROLOG_RESUME 
                 //lcm_state == VirtualMachine::PROLOG_RECOVER || 
                 //lcm_state == VirtualMachine::DELETE_RECOVER_FAIL 
                 )

    {
        action = VirtualMachineManager::RESTORE;
    }
    else if ( lcm_state == VirtualMachine::PROLOG_BACKUP)
    {
         action = VirtualMachineManager::DEPLOY;

         vm->set_bketime(time(0));
         
         //vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_SUCESS );

         vm->set_bkstate(Backup::BACKUP_SUCESS);

         vmpool->update_backup(vm); 

         vm->lcmstate = VirtualMachine::PROLOG_BACKUP;
     }
    else if  ( lcm_state == VirtualMachine::PROLOG_RECOVER_DECOMPRESS ||
                  lcm_state == VirtualMachine::DELETE_RECOVER_FAIL||
                  lcm_state == VirtualMachine::RECOVER_DECOMPRESS_FAIL||
                  lcm_state == VirtualMachine::DELETE_BACKUP_FAIL)
    {
             action = VirtualMachineManager::DEPLOY;
    }
    else if ( lcm_state == VirtualMachine::PROLOG_BACKUPDISK)
    {
         action = VirtualMachineManager::DEPLOY;         

         vm->set_dkstate(BackupDisk::BACKUPDISK_SUCESS);
         
         vmpool->update_backupdisk(vm); 
     }    
    else
    {
        vm->log("LCM",Log::ERROR,"prolog_success_action, VM in a wrong state");
        vm->unlock();

        return;
    }

     //added by shenxy
    if (vm->get_lcm_state() == VirtualMachine::PROLOG_RECOVER_DECOMPRESS)
    {             
              //lcmstate ,not lcm_state   
             vm->lcmstate = VirtualMachine::PROLOG_RECOVER_DECOMPRESS;                       
     }

      //added by shenxy
     string xmlist;
     nd.get_configuration_attribute("XM_LIST",xmlist);
     if((lcm_state == VirtualMachine::PROLOG_BACKUP || 
          lcm_state == VirtualMachine::PROLOG_RECOVER_DECOMPRESS ||
          lcm_state == VirtualMachine::DELETE_RECOVER_FAIL||
          lcm_state == VirtualMachine::RECOVER_DECOMPRESS_FAIL||
          lcm_state == VirtualMachine::DELETE_BACKUP_FAIL||
          lcm_state == VirtualMachine::PROLOG_BACKUPDISK)
          &&  xmlist == "xen")
     {  
              vm->set_state(VirtualMachine::OFF);

              vmpool->update(vm);

              tm->trigger(TransferManager::ON,vid);
     }
     else
     {
                 //----------------------------------------------------
                 //                     BOOT STATE
                 //----------------------------------------------------

                 vm->set_state(VirtualMachine::BOOT);

                 vmpool->update(vm);

                 vm->set_prolog_etime(the_time);

                 vm->set_running_stime(the_time);

                 vmpool->update_history(vm);

                 vm->log("LCM", Log::INFO, "New VM state is BOOT");

                 //----------------------------------------------------
   
                 vmm->trigger(action,vid);
    }

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::prolog_failure_action(int vid)
{
    VirtualMachine *    vm;
    time_t  the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    if ( vm->get_lcm_state() == VirtualMachine::PROLOG_BACKUP )
    {
        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();

        TransferManager *   tm = nd.get_tm(); 
        //----------------------------------------------------
        //           PROLOG_BACKUP STATE 
        //----------------------------------------------------            

         vm->log("LCM", Log::INFO, "PROLOG_BACKUP state is error");
        
         //vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL );

        //----------------------------------------------------
        //                LCM FAILURE STATE
        //----------------------------------------------------

         vm->set_state(VirtualMachine::DELETE_BACKUP_FAIL);

         vmpool->update(vm);   

         vm->log("LCM", Log::INFO, "New VM state is DELETE_BACKUP_FAIL");

         vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL  );

         vm->lcmstate = VirtualMachine::RUNNING;
       //------------- Clean up  files ----------------

        tm->trigger(TransferManager::DELETE_BACKUP_FAIL,vm->get_oid());
    }
    else if ( vm->get_lcm_state() == VirtualMachine::PROLOG_RECOVER )
    {
        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();       
 
        TransferManager *   tm = nd.get_tm(); 

        //----------------------------------------------------
        //           PROLOG_RECOVER STATE 
        //----------------------------------------------------            

         vm->log("LCM", Log::INFO, "PROLOG_RECOVER state is error"); 

        //----------------------------------------------------
        //                LCM FAILURE STATE
        //----------------------------------------------------

         vm->set_state(VirtualMachine::DELETE_RECOVER_FAIL);

         vmpool->update(vm);   

         vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL  );

         vm->lcmstate = VirtualMachine::RUNNING;
       //------------- Clean up remote files ----------------

         tm->trigger(TransferManager::DELETE_RECOVER_FAIL,vm->get_oid());
      
    }
    else if ( vm->get_lcm_state() == VirtualMachine::PROLOG_RECOVER_DECOMPRESS )
    {
        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();       
 
        TransferManager *   tm = nd.get_tm(); 

        //----------------------------------------------------
        //           PROLOG_RECOVER_DECOMPRESS STATE 
        //----------------------------------------------------            

         vm->log("LCM", Log::INFO, "PROLOG_RECOVER_DECOMPRESS state is error"); 

        //----------------------------------------------------
        //                LCM FAILURE STATE
        //----------------------------------------------------

         vm->set_state(VirtualMachine::RECOVER_DECOMPRESS_FAIL);

         vmpool->update(vm);   

         vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL  );

         vm->lcmstate = VirtualMachine::RUNNING;
       //------------- Clean up remote files ----------------

        tm->trigger(TransferManager::RECOVER_DECOMPRESS_FAIL,vm->get_oid());
      
    }
    else if ( vm->get_lcm_state() == VirtualMachine::PROLOG_BACKUPDISK )
    {
        Nebula&                 nd = Nebula::instance();
        VirtualMachineManager * vmm = nd.get_vmm();

        //----------------------------------------------------
        //           PROLOG_BACKUPDISK   STATE 
        //----------------------------------------------------            

         vm->log("LCM", Log::INFO, "PROLOG_BACKUPDISK state is error");

         vmpool->updatedkstate(vm->dkbid,BackupDisk::BACKUPDISK_FAIL  );
                 
        //----------------------------------------------------

         //added by shenxy
         TransferManager *   tm = nd.get_tm();
         string xmlist;
         nd.get_configuration_attribute("XM_LIST",xmlist);
         if(xmlist == "xen")
         {  
              vm->set_state(VirtualMachine::OFF);

              vmpool->update(vm);

              tm->trigger(TransferManager::ON,vid);
          }
          else
          {
                 vmm->trigger(VirtualMachineManager::DEPLOY,vid);
          }
    }
    else
    {
        vm->set_prolog_etime(the_time);

        failure_action(vm);
    }

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::epilog_success_action(int vid)
{
    Nebula&             nd = Nebula::instance();
    DispatchManager *   dm = nd.get_dm();

    VirtualMachine *    vm;
    time_t              the_time = time(0);
    int                 cpu,mem,disk;

    DispatchManager::Actions action;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    if (vm->get_lcm_state() == VirtualMachine::EPILOG_STOP)
    {
        action = DispatchManager::STOP_SUCCESS;
    }
    else if (vm->get_lcm_state() == VirtualMachine::EPILOG)
    {
        action = DispatchManager::DONE;
    }
    else
    {
        vm->log("LCM",Log::ERROR,"epilog_success_action, VM in a wrong state");
        vm->unlock();

        return;
    }

    vm->set_epilog_etime(the_time);

    vm->set_etime(the_time);

    vmpool->update_history(vm);

    vm->get_requirements(cpu,mem,disk);

    hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

    //----------------------------------------------------

    dm->trigger(action,vid);

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::epilog_failure_action(int vid)
{
    VirtualMachine * vm;
    time_t           the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    vm->set_epilog_etime(the_time);

    failure_action(vm);

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::cancel_success_action(int vid)
{
    Nebula&             nd = Nebula::instance();
    TransferManager *   tm = nd.get_tm();
    VirtualMachine *    vm;
    time_t              the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //                   EPILOG STATE
    //----------------------------------------------------

    vm->set_state(VirtualMachine::EPILOG);

    vmpool->update(vm);

    vm->set_reason(History::CANCEL);

    vm->set_epilog_stime(the_time);

    vm->set_running_etime(the_time);

    vmpool->update_history(vm);

    vm->log("LCM", Log::INFO, "New VM state is EPILOG");

    //----------------------------------------------------

    tm->trigger(TransferManager::EPILOG,vid);

    vm->unlock();
}


/* --------------------------------------------------------------------------*/
/* --------------------------------------------------------------------------*/

void  LifeCycleManager::off_success_action(int vid)
{
    Nebula&             nd = Nebula::instance();
    TransferManager *   tm = nd.get_tm();
    VirtualMachine *    vm;
    time_t              the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

     //added by shenxy
     if ( vm->get_lcm_state() == VirtualMachine::OFF_BACKUP)   
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();
        //time_t              the_time = time(0);

        //----------------------------------------------------
        //                 PROLOG_BACKUP STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_BACKUP);

        vmpool->update(vm);        

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_BACKUP ");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_BACKUP,vid);
    }
    else if(vm->get_lcm_state() == VirtualMachine::OFF_RECOVER)
    {
           Nebula&             nd = Nebula::instance();
           TransferManager *   tm = nd.get_tm();       

            //----------------------------------------------------
            //                 PROLOG_RECOVER STATE
            //----------------------------------------------------

            vm->set_state(VirtualMachine::PROLOG_RECOVER);

            vmpool->update(vm);        

            vm->log("LCM", Log::INFO, "New VM state is PROLOG_RECOVER ");

            //----------------------------------------------------

            tm->trigger(TransferManager::PROLOG_RECOVER,vid);
    }
    else if ( vm->get_lcm_state() == VirtualMachine::OFF_BACKUPDISK)   
    {
        Nebula&             nd = Nebula::instance();
        TransferManager *   tm = nd.get_tm();

        //----------------------------------------------------
        //                 PROLOG_BACKUPDISK STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::PROLOG_BACKUPDISK);

        vmpool->update(vm);        

        vm->log("LCM", Log::INFO, "New VM state is PROLOG_BACKUPDISK ");

        //----------------------------------------------------

        tm->trigger(TransferManager::PROLOG_BACKUPDISK,vid);
    }
    else
    {
        //----------------------------------------------------
        //                   OFF  STATE
        //----------------------------------------------------

        vm->set_state(VirtualMachine::OFF);

        vmpool->update(vm);
    

        vm->log("LCM", Log::INFO, "New VM state is OFF");    
     }

    vm->unlock();
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void  LifeCycleManager::xmlist_success_action(int vid)
{
    Nebula&                 nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();
    TransferManager *   tm = nd.get_tm();
    VirtualMachine *        vm;
    time_t                  the_time = time(0);
    ostringstream           os;

    VirtualMachineManager::Actions action;
    VirtualMachine::LcmState       lcm_state;

    vm = vmpool->get(vid, true);

    if ( vm == 0 )
    {
        vm->log("VM",Log::ERROR,"xmlist_success_action, VM in a wrong state");
        return;
    }

    lcm_state = vm->get_lcm_state();

    if (lcm_state == VirtualMachine::OFF)
    {
        action = VirtualMachineManager::DEPLOY;
    }
    
    else
    {
        vm->log("LCM",Log::ERROR,"xmlist_success_action, VM in a wrong state");
        vm->unlock();

        return;
    }

    //----------------------------------------------------
    //                     BOOT STATE
    //----------------------------------------------------

               
    vm->set_state(VirtualMachine::BOOT);

    vmpool->update(vm);
   

    vm->log("LCM", Log::INFO, "New VM state is BOOT");

    //----------------------------------------------------

    vmm->trigger(action,vid);

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void  LifeCycleManager::xmlist_failure_action(int vid)
{
    VirtualMachine *    vm;
    time_t  the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    vm->log("LCM", Log::INFO, "xmlist_failure");
    

    vm->unlock();

    return;
}

/* -------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------*/

void  LifeCycleManager::prolog_recover_decompress_action(int vid)
{
      Nebula&             nd = Nebula::instance();
      TransferManager *   tm = nd.get_tm();
      VirtualMachine *    vm;

      vm = vmpool->get(vid,true);

      if ( vm == 0 )
     {
             return;
      }

      vm->set_state(VirtualMachine::PROLOG_RECOVER_DECOMPRESS);
 
      vmpool->update(vm);        

      vm->log("LCM", Log::INFO, "New VM state is PROLOG_RECOVER_DECOMPRESS ");

      tm->trigger(TransferManager::PROLOG_RECOVER_DECOMPRESS,vid);

      vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::cancel_failure_action(int vid)
{
    VirtualMachine *    vm;

    Nebula&                 nd = Nebula::instance();
    VirtualMachineManager * vmm = nd.get_vmm();

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //added by shenxy
     if ( vm->get_lcm_state() == VirtualMachine::OFF_BACKUP)   
    {
          vmpool->updatebkstate(vm->bakbid,Backup::BACKUP_FAIL );
    }
    else if(vm->get_lcm_state() == VirtualMachine::OFF_RECOVER)
    {
          vmpool->updatebkstate(vm->updatebkstateoid,Backup::RESUME_FAIL );
    }

    //----------------------------------------------------
    //    RUNNING STATE FROM CANCEL
    //----------------------------------------------------

    vm->set_state(VirtualMachine::RUNNING);

    vmpool->update(vm);

    vm->log("LCM", Log::INFO, "Fail to cancel VM."
            " Assuming that the VM is still RUNNING (will poll VM).");

    //----------------------------------------------------

    vmm->trigger(VirtualMachineManager::POLL,vid);

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::monitor_failure_action(int vid)
{
    VirtualMachine * vm;

    time_t  the_time = time(0);

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    vm->set_running_etime(the_time);

    failure_action(vm);

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::monitor_suspend_action(int vid)
{
    VirtualMachine *    vm;

    int     cpu,mem,disk;
    time_t  the_time = time(0);

    Nebula&             nd = Nebula::instance();
    DispatchManager *   dm = nd.get_dm();

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //                  SAVE_SUSPEND STATE
    //----------------------------------------------------

    vm->set_state(VirtualMachine::SAVE_SUSPEND);

    vmpool->update(vm);

    vm->set_running_etime(the_time);

    vm->set_etime(the_time);

    vm->set_reason(History::STOP_RESUME);

    vmpool->update_history(vm);

    vm->get_requirements(cpu,mem,disk);

    hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

    vm->log("LCM", Log::INFO, "VM is suspended.");

    //----------------------------------------------------

    dm->trigger(DispatchManager::SUSPEND_SUCCESS,vid);

    vm->unlock();
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::monitor_done_action(int vid)
{
    VirtualMachine *    vm;

    vm = vmpool->get(vid,true);

    if ( vm == 0 )
    {
        return;
    }

    //----------------------------------------------------
    //                   UNKNWON STATE
    //----------------------------------------------------

    vm->set_state(VirtualMachine::UNKNOWN);

    vmpool->update(vm);

    vm->log("LCM", Log::INFO, "New VM state is UNKNOWN");

    //----------------------------------------------------

    vm->unlock();
}


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void  LifeCycleManager::failure_action(VirtualMachine * vm)
{
    Nebula&             nd = Nebula::instance();
    DispatchManager *   dm = nd.get_dm();

    time_t  the_time = time(0);
    int     cpu,mem,disk;

    //----------------------------------------------------
    //                LCM FAILURE STATE
    //----------------------------------------------------

    vm->set_state(VirtualMachine::FAILURE);

    vmpool->update(vm);

    vm->set_etime(the_time);

    vm->set_reason(History::ERROR);

    vmpool->update_history(vm);

    vm->get_requirements(cpu,mem,disk);

    hpool->del_capacity(vm->get_hid(),cpu,mem,disk);

    //--- VM to FAILED. Remote host cleanup upon VM deletion ---

    dm->trigger(DispatchManager::FAILED,vm->get_oid());
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
