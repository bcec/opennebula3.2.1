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

#ifndef REQUEST_H_
#define REQUEST_H_

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>

#include "RequestManager.h"
#include "AuthManager.h"
#include "PoolObjectSQL.h"

using namespace std;

/**
 *  The Request Class represents the basic abstraction for the OpenNebula
 *  XML-RPC API. This interface must be implemented by any XML-RPC API call
 */
class Request: public xmlrpc_c::method
{
public:
    /**
     *  Wraps the actual execution function by authorizing the user
     *  and calling the request_execute virtual function
     *    @param _paramlist list of XML parameters
     *    @param _retval value to be returned to the client
     */
    virtual void execute(
        xmlrpc_c::paramList const& _paramList,
        xmlrpc_c::value *   const  _retval);

    /**
     *  Error codes for the XML-RPC API
     */
    enum ErrorCode {
        SUCCESS        = 0x0000,
        AUTHENTICATION = 0x0100,
        AUTHORIZATION  = 0x0200,
        NO_EXISTS      = 0x0400,
        ACTION         = 0x0800,
        XML_RPC_API    = 0x1000,
        INTERNAL       = 0x2000,
    };

protected:

    /* ---------------------------------------------------------------------*/
    /*                     Attributes of the Request                        */
    /* ---------------------------------------------------------------------*/

    /* -------- Dynamic (specific for a request of the same method) -------- */

    struct RequestAttributes
    {
        int uid;                  /**< id of the user */
        int gid;                  /**< id of the user's group */

        string uname;             /**< name of the user */
        string gname;             /**< name of the user's group */

        string session;           /**< Session from ONE XML-RPC API */

        xmlrpc_c::value * retval; /**< Return value from libxmlrpc-c */
    };

    /* -------- Static (shared among request of the same method) -------- */

    PoolSQL * pool;           /**< Pool of objects */
    string    method_name;    /**< The name of the XML-RPC method */

    PoolObjectSQL::ObjectType auth_object;/**< Auth object for the request */
    AuthRequest::Operation    auth_op;    /**< Auth operation for the request */

    /* -------------------- Constructors ---------------------------------- */

    Request(const string& mn, 
            const string& signature, 
            const string& help): pool(0),method_name(mn)
    {
        _signature = signature;
        _help      = help;
    };

    virtual ~Request(){};

    /* -------------------------------------------------------------------- */
    /* -------------------------------------------------------------------- */

    /**
     *  Performs a basic authorization for this request using the uid/gid
     *  from the request. The function gets the object from the pool to get 
     *  the public attribute and its owner. The authorization is based on 
     *  object and type of operation for the request.
     *    @param oid of the object, can be -1 for objects to be created, or
     *    pools.
     *    @param att the specific request attributes
     *
     *    @return true if the user is authorized.
     */
    bool basic_authorization(int oid, RequestAttributes& att)
    {
        return basic_authorization(oid, auth_op, att);
    };

    /**
     *  Performs a basic authorization for this request using the uid/gid
     *  from the request. The function gets the object from the pool to get
     *  the public attribute and its owner. The authorization is based on
     *  object and type of operation for the request.
     *    @param oid of the object, can be -1 for objects to be created, or
     *    pools.
     *    @param op operation of the request.
     *    @param att the specific request attributes
     *
     *    @return true if the user is authorized.
     */
    bool basic_authorization(int oid, AuthRequest::Operation op,
                             RequestAttributes& att);
            
    /**
     *  Actual Execution method for the request. Must be implemented by the
     *  XML-RPC requests
     *    @param _paramlist of the XML-RPC call (complete list)
     *    @param att the specific request attributes
     */
    virtual void request_execute(xmlrpc_c::paramList const& _paramList,
                                 RequestAttributes& att) = 0;

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param val to be returned to the client
     *    @param att the specific request attributes
     */
    void success_response(int val, RequestAttributes& att);

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param val string to be returned to the client
     *    @param att the specific request attributes
     */
    void success_response(const string& val, RequestAttributes& att);

    //added by shenxy 20120227
    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param val string to be returned to the client
     *    @param oids stands for the number of VM on a specific status
     *    @param att the specific request attributes
     */
    void success_response(
                                   const string& val, 
                                   vector<int>& oids,
                                   RequestAttributes& att);

    /**
     *  Builds an XML-RPC response updating retval. After calling this function
     *  the xml-rpc excute method should return
     *    @param ec error code for this call
     *    @param val string representation of the error
     *    @param att the specific request attributes
     */
    void failure_response(ErrorCode ec,
                          const string& val,
                          RequestAttributes& att);

    /**
     *  Gets a string representation for the Auth object in the
     *  request.
     *    @param ob object for the auth operation
     *    @return string equivalent of the object
     */
    static string object_name(PoolObjectSQL::ObjectType ob);

    /**
     *  Logs authorization errors
     *    @param message with the authorization error details
     *    @return string for logging
     *    @param att the specific request attributes
     */
    string authorization_error (const string &message, RequestAttributes& att);

    /**
     *  Logs authenticate errors
     *    @return string for logging
     */
    string authenticate_error ();

    /**
     *  Logs get object errors
     *    @param object over which the get failed
     *    @param id of the object over which the get failed
     *    @return string for logging
     */
    string get_error (const string &object, int id);

    /**
     *  Logs action errors
     *    @param err_desc brief description of the error
     *    @param err_detail additional error details from Managers & Pools
     *    @return string for logging
     */
    string request_error (const string &err_desc, const string &err_detail);

    /**
     *  Logs allocate errors
     *    @param message with the allocate error details
     *    @return string for logging
     */
    string allocate_error (const string& error);

    /**
     *  Logs allocate errors for a given resource
     *    @param obj the resource
     *    @param message with the allocate error details
     *    @return string for logging
     */
    string allocate_error (PoolObjectSQL::ObjectType obj, const string& error);

    /**
     *  Logs allocate errors
     *    @param message with the allocate error details (parsing)
     *    @return string for logging
     */
    string allocate_error (char *error);
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#endif //REQUEST_H_

