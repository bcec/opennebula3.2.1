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

//Convert json into the XML that OCCI server can understand
function json2xml(element,root_key) {
    var xml = "";
    if (!root_key) root_key="ROOT";

    if (typeof element == "object") { //Its an object
        $.each(element, function(key,value){
            if (value.constructor == Array){
                for (var i = 0; i < value.length; i++){
                    xml += json2xml(value[i],key);
                };
                //do not wrap arrays in root_key
                return xml;

            } else
                xml += json2xml(value,key);
        });
    } else { //its a simple value. Base condition
        xml += element.toString();
    };
    return "<" + root_key.toUpperCase() + ">" + xml + "</" + root_key.toUpperCase() + ">";
};


$.ajaxSetup({
    converters: {
        "xml ONEjson": function(xml){
            return $.xml2json(xml);
        },
    }
});

var OCCI = {

    "Error": function(resp)
    {
        var error = {
            error : {
                message: resp.responseText,
                http_status : resp.status}
        };
        return error;
    },

    "is_error": function(obj)
    {
        return obj.error ? true : false;
    },

    "Helper": {
        "action": function(action, params)
        {
            obj = {
                "action": {
                    "perform": action
                }
            }
            if (params)
            {
                obj.action.params = params;
            }
            return obj;
        },

        "request": function(resource, method, data) {
            var r = {
                "request": {
                    "resource"  : resource,
                    "method"    : method
                }
            }
            if (data)
            {
                if (typeof(data) != "array")
                {
                    data = [data];
                }
                r.request.data = data;
            }
            return r;
        },

        "pool": function(resource, response)
        {
            var pool_name = resource + "_COLLECTION";
            var type = resource;
            var pool;

            if (typeof(pool_name) == "undefined")
            {
                return Error('Incorrect Pool');
            }

            var p_pool = [];

            if (response[pool_name]) {
                pool = response[pool_name][type];
            } else { pull = null };

            if (pool == null)
            {
                return p_pool;
            }
            else if (pool.length)
            {
                for (i=0;i<pool.length;i++)
                {
                    p_pool[i]={};
                    p_pool[i][type]=pool[i];
                }
                return(p_pool);
            }
            else
            {
                p_pool[0] = {};
                p_pool[0][type] = pool;
                return(p_pool);
            }
        }
    },

    "Action": {
        //server requests helper methods

        "create": function(params,resource){
            var callback = params.success;
            var callback_error = params.error;
            var data = json2xml(params.data,resource);
            var request = OCCI.Helper.request(resource,"create", data);

            $.ajax({
                url: resource.toLowerCase(),
                type: "POST",
                dataType: "xml ONEjson",
                data: data,
                success: function(response){
                    var res = {};
                    res[resource] = response;
                    return callback ? callback(request, res) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },

        "delete": function(params,resource){
            var callback = params.success;
            var callback_error = params.error;
            var id = params.data.id;
            var request = OCCI.Helper.request(resource,"delete", id);

            $.ajax({
                url: resource.toLowerCase() + "/" + id,
                type: "DELETE",
                success: function(){
                    return callback ? callback(request) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },

        "list": function(params,resource){
            var callback = params.success;
            var callback_error = params.error;
            var timeout = params.timeout || false;
            var request = OCCI.Helper.request(resource,"list");

            $.ajax({
                url: resource.toLowerCase(),
                type: "GET",
                data: {timeout: timeout},
                dataType: "xml ONEjson",
                success: function(response){
                    var res = {};
                    res[resource+"_COLLECTION"] = response;
                    return callback ?
                        callback(request, OCCI.Helper.pool(resource,res)) : null;
                },
                error: function(response)
                {
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },

        //Subresource examples: "fetch_template", "log"...
        "show": function(params,resource){
            var callback = params.success;
            var callback_error = params.error;
            var id = params.data.id;
            var request = OCCI.Helper.request(resource,"show", id);

            var url = resource.toLowerCase() + "/" + id;

            $.ajax({
                url: url,
                type: "GET",
                dataType: "xml ONEjson",
                success: function(response){
                    var res = {};
                    res[resource] = response;
                    return callback ? callback(request, res) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },

        //Example: Simple action: publish. Simple action with action obj: deploy
        //OCCI, rewrite
        "update": function(params,resource,method,action_obj){
            var callback = params.success;
            var callback_error = params.error;
            var id = params.data.id;
            var body = json2xml(params.data.body,resource);

            var request = OCCI.Helper.request(resource,method, id);

            $.ajax({
                url: resource.toLowerCase() + "/" + id,
                type: "PUT",
                data: body,
                dataType: "xml ONEjson",
                success: function(response){
                    var res = {};
                    res[resource] = response;
                    return callback ? callback(request,res) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },
/*
        "monitor": function(params,resource,all){
            var callback = params.success;
            var callback_error = params.error;
            var data = params.data;

            var method = "monitor";
            var action = OpenNebula.Helper.action(method);
            var request = OpenNebula.Helper.request(resource,method, data);

            var url = resource.toLowerCase();
            url = all ? url + "/monitor" : url + "/" + params.data.id + "/monitor";

            $.ajax({
                url: url,
                type: "GET",
                data: data['monitor'],
                dataType: "json",
                success: function(response){
                    return callback ? callback(request, response) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OpenNebula.Error(response)) : null;
                }
            });
        }
*/
    },

    "Auth": {
        "resource": "AUTH",

        "login": function(params)
        {
            var callback = params.success;
            var callback_error = params.error;
            var username = params.data.username;
            var password = params.data.password;
            var remember = params.remember;
            var lang = params.lang;

            var resource = OCCI.Auth.resource;
            var request = OCCI.Helper.request(resource,"login");

            $.ajax({
                url: "ui/login",
                type: "POST",
                data: {remember: remember, lang: lang},
                beforeSend : function(req) {
                    req.setRequestHeader( "Authorization",
                                        "Basic " + btoa(username + ":" + password)
                                        )
                },
                success: function(response){
                    return callback ? callback(request, response) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },

        "logout": function(params)
        {
            var callback = params.success;
            var callback_error = params.error;

            var resource = OCCI.Auth.resource;
            var request = OCCI.Helper.request(resource,"logout");

            $.ajax({
                url: "ui/logout",
                type: "POST",
                success: function(response){
                    return callback ? callback(request, response) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        }
    },

    "Network": {
        "resource": "NETWORK",

        "create": function(params){
            OCCI.Action.create(params,OCCI.Network.resource);
        },
        "delete": function(params){
            OCCI.Action.delete(params,OCCI.Network.resource);
        },
        "list": function(params){
            OCCI.Action.list(params,OCCI.Network.resource);
        },
        "show": function(params){
            OCCI.Action.show(params,OCCI.Network.resource);
        },
        "publish": function(params){
            params.data.body = { "PUBLIC": "YES" };
            OCCI.Action.update(params,OCCI.Network.resource,"publish");
        },
        "unpublish": function(params){
            params.data.body = { "PUBLIC": "NO" };
            OCCI.Action.update(params,OCCI.Network.resource,"unpublish");
        },
    },

    "VM": {
        "resource": "COMPUTE",

        "create": function(params){
            OCCI.Action.create(params,OCCI.VM.resource);
        },
        "delete": function(params){
            OCCI.Action.delete(params,OCCI.VM.resource);
        },
        "list": function(params){
            OCCI.Action.list(params,OCCI.VM.resource);
        },
        "show": function(params){
            OCCI.Action.show(params,OCCI.VM.resource);
        },
        "shutdown": function(params){
            params.data.body = { state : "SHUTDOWN" };
            OCCI.Action.update(params,OCCI.VM.resource,"shutdown");
        },
        "stop": function(params){
            params.data.body = { state : "STOPPED" };
            OCCI.Action.update(params,OCCI.VM.resource,"stop");
        },
        "cancel": function(params){
            params.data.body = { state : "CANCEL" };
            OCCI.Action.update(params,OCCI.VM.resource,"cancel");
        },
        "suspend": function(params){
            params.data.body = { state : "SUSPENDED" };
            OCCI.Action.update(params,OCCI.VM.resource,"suspend");
        },
        "resume": function(params){
            params.data.body = { state : "RESUME" };
            OCCI.Action.update(params,OCCI.VM.resource,"resume");
        },
        "done": function(params){
            params.data.body = { state : "DONE" };
            OCCI.Action.update(params,OCCI.VM.resource,"done");
        },
        "saveas" : function(params){
            var obj = params.data.extra_param;
            var disk_id = obj.disk_id;
            var im_name = obj.image_name;
            params.data.body = '<DISK id="'+disk_id+'"><SAVE_AS name="'+im_name+'" /></DISK>';
            OCCI.Action.update(params,OCCI.VM.resource,"saveas");
        },
/*        "vnc" : function(params,startstop){
            var callback = params.success;
            var callback_error = params.error;
            var id = params.data.id;
            var resource = OCCI.VM.resource;

            var method = startstop;
            var action = OCCI.Helper.action(method);
            var request = OCCI.Helper.request(resource,method, id);
            $.ajax({
                url: "vm/" + id + "/" + method,
                type: "POST",
                dataType: "json",
                success: function(response){
                    return callback ? callback(request, response) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },
        "startvnc" : function(params){
            OCCI.VM.vnc(params,"startvnc");
        },
        "stopvnc" : function(params){
            OCCI.VM.vnc(params,"stopvnc");
        },
        "monitor" : function(params){
            OCCI.Action.monitor(params,OCCI.VM.resource,false);
        },
        "monitor_all" : function(params){
            OCCI.Action.monitor(params,OCCI.VM.resource,true);
        }*/
    },

    "Image": {
        "resource": "STORAGE",

        "create": function(params){
            var callback = params.success;
            var callback_error = params.error;
            var data = {occixml : json2xml(params.data,OCCI.Image.resource)};
            var request = OCCI.Helper.request(OCCI.Image.resource,"create", data);

            $.ajax({
                type: 'POST',
                url: "storage",
                data: data,
                dataType: "xml ONEjson",
                success: function(response){
                    var res = {};
                    res["STORAGE"] = response;
                    return callback ? callback(request, res) : null;
                },
                error: function(response){
                    return callback_error ?
                        callback_error(request, OCCI.Error(response)) : null;
                }
            });
        },
        "delete": function(params){
            OCCI.Action.delete(params,OCCI.Image.resource);
        },
        "list": function(params){
            OCCI.Action.list(params,OCCI.Image.resource);
        },
        "show": function(params){
            OCCI.Action.show(params,OCCI.Image.resource);
        },
        "publish": function(params){
            params.data.body = { "PUBLIC":"YES" };
            OCCI.Action.update(params,OCCI.Image.resource,"publish");
        },
        "unpublish": function(params){
            params.data.body = { "PUBLIC":"NO" };
            OCCI.Action.update(params,OCCI.Image.resource,"unpublish");
        },
        "persistent": function(params){
            params.data.body = { "PERSISTENT":"YES" };
            OCCI.Action.update(params,OCCI.Image.resource,"persistent");
        },
        "nonpersistent": function(params){
            params.data.body = { "PERSISTENT":"NO" };
            OCCI.Action.update(params,OCCI.Image.resource,"nonpersistent");
        },
    },

    "Template" : {
        "resource" : "VMTEMPLATE",

        "create" : function(params){
            OCCI.Action.create(params,OCCI.Template.resource);
        },
        "delete" : function(params){
            OCCI.Action.delete(params,OCCI.Template.resource);
        },
        "list" : function(params){
            OCCI.Action.list(params,OCCI.Template.resource);
        },
        "show" : function(params){
            OCCI.Action.show(params,OCCI.Template.resource);
        },
        "chown" : function(params){
            OCCI.Action.chown(params,OCCI.Template.resource);
        },
        "chgrp" : function(params){
            OCCI.Action.chgrp(params,OCCI.Template.resource);
        },
        "update" : function(params){
            var action_obj = {"template_raw" : params.data.extra_param };
            OCCI.Action.simple_action(params,
                                     OCCI.Template.resource,
                                     "update",
                                     action_obj);
        },
        "fetch_template" : function(params){
            OCCI.Action.show(params,OCCI.Template.resource,"template");
        },
        "publish" : function(params){
            OCCI.Action.simple_action(params,OCCI.Template.resource,"publish");
        },
        "unpublish" : function(params){
            OCCI.Action.simple_action(params,OCCI.Template.resource,"unpublish");
        },

        "instantiate" : function(params) {
            var vm_name = params.data.extra_param ? params.data.extra_param : "";
            var action_obj = { "vm_name" : vm_name };
            OCCI.Action.simple_action(params,OCCI.Template.resource,
                                            "instantiate",action_obj);
        }
    },

    "Instance_type" : {
        "resource" : "INSTANCE_TYPE",
        "list" : function(params){
            OCCI.Action.list(params,OCCI.Instance_type.resource);
        },
    },

}
