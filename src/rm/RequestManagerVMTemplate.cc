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

#include "RequestManagerVMTemplate.h"
#include "PoolObjectAuth.h"
#include "Nebula.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void VMTemplateInstantiate::request_execute(xmlrpc_c::paramList const& paramList,
                                            RequestAttributes& att)
{
    int    id   = xmlrpc_c::value_int(paramList.getInt(1));
    string name = xmlrpc_c::value_string(paramList.getString(2));

    int rc, vid;

    PoolObjectAuth perms;

    Nebula& nd = Nebula::instance();
    VirtualMachinePool* vmpool = nd.get_vmpool();
    VMTemplatePool * tpool     = static_cast<VMTemplatePool *>(pool);

    VirtualMachineTemplate * tmpl;
    VMTemplate *             rtmpl;

    string error_str;

    rtmpl = tpool->get(id,true);

    if ( rtmpl == 0 )
    {
        failure_response(NO_EXISTS,
                get_error(object_name(auth_object),id),
                att);

        return;
    }

    tmpl  = rtmpl->clone_template();

    rtmpl->get_permissions(perms);

    rtmpl->unlock();

    tmpl->erase("NAME");
    tmpl->set(new SingleAttribute("NAME",name));

    if ( att.uid != 0 )
    {
        AuthRequest ar(att.uid, att.gid);
        string      tmpl_txt;

        ar.add_auth(auth_op, perms); //USE TEMPLATE

        VirtualMachine::set_auth_request(att.uid, ar, tmpl);

        if (UserPool::authorize(ar) == -1)
        {
            failure_response(AUTHORIZATION,
                    authorization_error(ar.message, att),
                    att);

            delete tmpl;
            return;
        }
    }

    rc = vmpool->allocate(att.uid, att.gid, att.uname, att.gname, tmpl, &vid,
            error_str, false);

    if ( rc < 0 )
    {
        failure_response(INTERNAL,
                allocate_error(PoolObjectSQL::VM,error_str),
                att);

        return;
    }
    
    success_response(vid, att);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

