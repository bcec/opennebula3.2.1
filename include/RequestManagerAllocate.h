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

#ifndef REQUEST_MANAGER_ALLOCATE_H_
#define REQUEST_MANAGER_ALLOCATE_H_

#include "Request.h"
#include "Nebula.h"

#include "VirtualNetworkTemplate.h"
#include "ImageTemplate.h"
#include "VirtualMachineTemplate.h"

using namespace std;

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class RequestManagerAllocate: public Request
{
protected:
    RequestManagerAllocate(const string& method_name,
                           const string& help,
                           const string& xml_args,
                           bool  dt)
        :Request(method_name,xml_args,help), do_template(dt)
    {
        auth_op = AuthRequest::CREATE;
    };

    ~RequestManagerAllocate(){};

    /* -------------------------------------------------------------------- */

    void request_execute(xmlrpc_c::paramList const& _paramList,
                         RequestAttributes& att);

    virtual bool allocate_authorization(Template * obj_template,
                                        RequestAttributes& att);

    /* -------------------------------------------------------------------- */

    virtual Template * get_object_template() { return 0; };

    virtual int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                              Template * tmpl,
                              int& id, 
                              string& error_str,
                              RequestAttributes& att) = 0;
private:

    bool do_template;
};


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class VirtualMachineAllocate: public RequestManagerAllocate
{
public:
    VirtualMachineAllocate():
        RequestManagerAllocate("VirtualMachineAllocate",
                               "Allocates a new virtual machine",
                               "A:ss",
                               true)
    {    
        Nebula& nd = Nebula::instance();
        pool       = nd.get_vmpool();
        auth_object = PoolObjectSQL::VM;
    };

    ~VirtualMachineAllocate(){};
    /* --------------------------------------------------------------------- */

    Template * get_object_template() 
    { 
        return new VirtualMachineTemplate; 
    };

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);

    bool allocate_authorization(Template * obj_template,
                                RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class VirtualNetworkAllocate: public RequestManagerAllocate
{
public:
    VirtualNetworkAllocate():
        RequestManagerAllocate("VirtualNetworkAllocate",
                               "Allocates a new virtual network",
                               "A:ss",
                               true)
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_vnpool();
        auth_object = PoolObjectSQL::NET;
    };

    ~VirtualNetworkAllocate(){};

    /* --------------------------------------------------------------------- */

    Template * get_object_template() 
    { 
        return new VirtualNetworkTemplate; 
    };

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class ImageAllocate: public RequestManagerAllocate
{
public:
    ImageAllocate():
        RequestManagerAllocate("ImageAllocate",
                               "Allocates a new image",
                               "A:ss",
                               true)
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_ipool();
        auth_object = PoolObjectSQL::IMAGE;
    };

    ~ImageAllocate(){};

    /* --------------------------------------------------------------------- */

    Template * get_object_template() 
    { 
        return new ImageTemplate; 
    };

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class TemplateAllocate : public RequestManagerAllocate
{
public:
    TemplateAllocate():
        RequestManagerAllocate("TemplateAllocate",
                               "Allocates a new virtual machine template",
                               "A:ss",
                               true)
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_tpool();
        auth_object = PoolObjectSQL::TEMPLATE;
    };

    ~TemplateAllocate(){};

    /* --------------------------------------------------------------------- */

    Template * get_object_template() 
    { 
        return new VirtualMachineTemplate; 
    };

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class HostAllocate : public RequestManagerAllocate
{
public:
    HostAllocate():
        RequestManagerAllocate("HostAllocate",
                               "Allocates a new host",
                               "A:ssssss",
                               false)
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_hpool();
        auth_object = PoolObjectSQL::HOST;
    };

    ~HostAllocate(){};

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class UserAllocate: public RequestManagerAllocate
{
public:
    UserAllocate():
        RequestManagerAllocate("UserAllocate",
                               "Returns user information",
                               "A:ssss",
                               false)
    {    
        Nebula& nd  = Nebula::instance();
        pool        = nd.get_upool();
        auth_object = PoolObjectSQL::USER;
    };

    ~UserAllocate(){};

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

class GroupAllocate: public RequestManagerAllocate
{
public:
    GroupAllocate():
        RequestManagerAllocate("GroupAllocate",
                               "Allocates a new group",
                               "A:ss",
                               false)
    {    
        Nebula& nd = Nebula::instance();
        pool       = nd.get_gpool();
        auth_object = PoolObjectSQL::GROUP;
    };

    ~GroupAllocate(){};

    int pool_allocate(xmlrpc_c::paramList const& _paramList, 
                      Template * tmpl,
                      int& id, 
                      string& error_str,
                      RequestAttributes& att);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#endif
