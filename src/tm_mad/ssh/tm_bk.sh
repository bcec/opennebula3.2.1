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
DST_PATH=`arg_path $DST`

SRC_HOST=`arg_host $SRC`
DST_HOST=`arg_host $DST`

DST_DIR=`dirname $DST_PATH`

######################################################add by gaozh for oNest begin####################################################################
ONED_CONF_PATH="/opt/nebula/ONE/etc/oned.conf"

STORE_TYPE=`grep ^STORE_TYPE $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BUCKET_NAME=`grep ^BUCKET_NAME $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BK_DIR=`grep ^BK_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`
ONEST_TOOL_DIR=`grep ^ONEST_TOOL_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`

IS_MKIMAGE=`echo $DST_PATH |grep "/opt/nebula/images"`

if [ "oNest" = $STORE_TYPE ] && [ -z "${IS_MKIMAGE}" ]
then

    log "Moving $SRC_PATH from oNest."
    
    MATCH_RESULT=`echo $SRC_PATH |grep $BK_DIR`

    if [ -n "${MATCH_RESULT}" ]
    then
        BK_VMID=`echo $SRC_PATH | awk -F"/" '{print $5}'`
	DST_DIR=`dirname $DST_PATH`
        exec_and_log "$SSH $DST_HOST mkdir -p $DST_DIR"
        code1=`exec_and_log_unexit "$SSH $DST_HOST $ONEST_TOOL_DIR/batch_op.py --op get --config $ONEST_TOOL_DIR/config.ini --location $DST_DIR --dest-bucket $BUCKET_NAME --prefix $BK_VMID/"`
        exec_and_log "$SSH $DST_HOST mv $DST_DIR/$BK_VMID $DST_PATH"
    #else
    #   BK_VMID=`echo $DST_PATH | awk -F"/" '{print $5}'`
    #	SRC_DIR=`dirname $SRC_PATH`
    #	exec_and_log "$SSH $SRC_HOST mv $SRC_PATH $SRC_DIR/$BK_VMID"
    #	code1=`exec_and_log_unexit "$SSH $SRC_HOST $ONEST_TOOL_DIR/batch_op.py --op put --config $ONEST_TOOL_DIR/config.ini --location $SRC_DIR/$BK_VMID --dest-bucket $BUCKET_NAME"`
    #	exec_and_log "$SSH $SRC_HOST mv $SRC_DIR/$BK_VMID $SRC_PATH"
    fi
    
    if [ "x$code1" != "x0" ]; then
         exit $code1
    fi

else
#####################################################add by gaozh for oNest end###################################################################

    log "Moving $SRC_PATH"
    exec_and_log "$SSH $DST_HOST mkdir -p $DST_DIR"
    exec_and_log "$SCP -r $SRC $DST"

####################################################add by gaozh for oNest begin###################################################################
fi
#####################################################add by gaozh for oNest end###################################################################
