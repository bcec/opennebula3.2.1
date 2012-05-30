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

SRC=$1
DST=$2

if [ -z "${ONE_LOCATION}" ]; then
    TMCOMMON=/usr/lib/one/mads/tm_common.sh
else
    TMCOMMON=$ONE_LOCATION/lib/mads/tm_common.sh
fi

. $TMCOMMON

SRC_PATH=`arg_path $SRC`
SRC_HOST=`arg_host $SRC`

####################################################add by gaozh for oNest begin###################################################################
ONED_CONF_PATH="/opt/nebula/ONE/etc/oned.conf"

STORE_TYPE=`grep ^STORE_TYPE $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BUCKET_NAME=`grep ^BUCKET_NAME $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BK_DIR=`grep ^BK_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`
ONEST_TOOL_DIR=`grep ^ONEST_TOOL_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`
MATCH_RESULT=`echo $SRC_PATH |grep $BK_DIR`

if [ "oNest" = $STORE_TYPE ] && [ -n "${MATCH_RESULT}" ]
then
    VMID_BKID=`echo $SRC_PATH | awk -F"/" '{print $5}'`
    VMID=`echo $SRC_PATH | awk -F"/" '{print $4}'`
    log "Deleting $SRC_PATH"
    if [ -z "${VMID_BKID}" ]
    then
	exec_and_log "$SSH $SRC_HOST $ONEST_TOOL_DIR/batch_op.py --op del --config $ONEST_TOOL_DIR/config.ini --dest-bucket $BUCKET_NAME --prefix $VMID"
    else
	exec_and_log "$SSH $SRC_HOST $ONEST_TOOL_DIR/batch_op.py --op del --config $ONEST_TOOL_DIR/config.ini --dest-bucket $BUCKET_NAME --prefix $VMID_BKID/"
    fi
else
####################################################add by gaozh for oNest end#####################################################################

    log "Deleting $SRC_PATH"
    exec_and_log "$SSH $SRC_HOST rm -rf $SRC_PATH" \
    "Error deleting $SRC_PATH"

####################################################add by gaozh for oNest begin###################################################################
fi
####################################################add by gaozh for oNest end#####################################################################
