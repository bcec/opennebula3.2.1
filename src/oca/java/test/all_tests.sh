#!/bin/bash

# -------------------------------------------------------------------------- #
# Copyright 2002-2012, OpenNebula Project Leads (OpenNebula.org)             #
#                                                                            #
# Licensed under the Apache License, Version 2.0 (the "License"); you may    #
# not use this file except in compliance with the License. You may obtain    #
# a copy of the License at                                                   #
#                                                                            #
# http://www.apache.org/licenses/LICENSE-2.0                                 #
#                                                                            #
# Unless required by applicable law or agreed to in writing, software        #
# distributed under the License is distributed on an "AS IS" BASIS,          #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
# See the License for the specific language governing permissions and        #
# limitations under the License.                                             #
#--------------------------------------------------------------------------- #

if [ -z $ONE_LOCATION ]; then
    echo "ONE_LOCATION not defined."
    exit -1
fi

ONEDCONF_LOCATION="$ONE_LOCATION/etc/oned.conf"

if [ -f $ONEDCONF_LOCATION ]; then
    echo "$ONEDCONF_LOCATION has to be overwritten, move it to a safe place."
    exit -1
fi

cp oned.conf $ONEDCONF_LOCATION

export ONE_XMLRPC=http://localhost:2666/RPC2

RC=0

./test.sh HostTest
let RC=RC+$?

./test.sh ImageTest
let RC=RC+$?

./test.sh SessionTest
let RC=RC+$?

./test.sh UserTest
let RC=RC+$?

./test.sh VirtualMachineTest
let RC=RC+$?

./test.sh VirtualNetworkTest
let RC=RC+$?

./test.sh TemplateTest
let RC=RC+$?

./test.sh GroupTest
let RC=RC+$?

./test.sh AclTest
let RC=RC+$?

exit $RC