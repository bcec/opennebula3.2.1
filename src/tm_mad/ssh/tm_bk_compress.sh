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

ONED_CONF_PATH="/opt/nebula/ONE/etc/oned.conf"

STORE_TYPE=`grep ^STORE_TYPE $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BUCKET_NAME=`grep ^BUCKET_NAME $ONED_CONF_PATH | awk -F"=" '{print $2}'`
ONEST_TOOL_DIR=`grep ^ONEST_TOOL_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`

#----------------------------------
log "Compressing and moving $SRC_PATH"
exec_and_log "$SSH $SRC_HOST mkdir -p ${SRC_PATH}_compress"
for file in `$SSH $SRC_HOST ls $SRC_PATH`
do
     $SSH $SRC_HOST "cd $SRC_PATH;tar -Sczvf ${SRC_PATH}_compress/${file}.tgz ${file}"
done
#----------------------------------

if [ "oNest" = $STORE_TYPE ]
then

    log "Moving $SRC_PATH to oNest."
    
    BK_VMID=`echo $DST_PATH | awk -F"/" '{print $5}'`
    SRC_DIR=`dirname $SRC_PATH`
    exec_and_log "$SSH $SRC_HOST mv ${SRC_PATH}_compress $SRC_DIR/$BK_VMID"
    code1=`exec_and_log_unexit "$SSH $SRC_HOST $ONEST_TOOL_DIR/batch_op.py --op put --config $ONEST_TOOL_DIR/config.ini --location $SRC_DIR/$BK_VMID --dest-bucket $BUCKET_NAME"`
    exec_and_log "$SSH $SRC_HOST mv $SRC_DIR/$BK_VMID ${SRC_PATH}_compress"
    
    if [ "x$code1" != "x0" ]; then
         exit $code1
    fi

else
    exec_and_log "$SSH $DST_HOST mkdir -p $DST_PATH"
    log "Moving $SRC_PATH"
    exec_and_log "$SCP -r ${SRC}_compress/* $DST"
fi

