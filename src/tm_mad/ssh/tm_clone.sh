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


log_debug "$1 $2"
log_debug "DST: $DST_PATH"

######################################################add by gaozh for oNest begin####################################################################
DST_DIR=`dirname $DST_PATH`
DST_DIR_DIR=`dirname $DST_DIR`

ONED_CONF_PATH="/opt/nebula/ONE/etc/oned.conf"

log "Creating directory $DST_DIR"
exec_and_log "$SSH $DST_HOST mkdir -p $DST_DIR" \
    "Error creating directory $DST_DIR"

STORE_TYPE=`grep ^STORE_TYPE $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BUCKET_NAME=`grep ^BUCKET_NAME $ONED_CONF_PATH | awk -F"=" '{print $2}'`
BK_DIR=`grep ^BK_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`
ONEST_TOOL_DIR=`grep ^ONEST_TOOL_DIR $ONED_CONF_PATH | awk -F"=" '{print $2}'`
MATCH_RESULT=`echo $SRC_PATH |grep $BK_DIR`

if [ "oNest" = $STORE_TYPE ] && [ -n "${MATCH_RESULT}" ]
then
    VMID_BKID=`echo $SRC_PATH | awk -F"/" '{print $5}'`
    code1=`exec_and_log_unexit "$SSH $DST_HOST $ONEST_TOOL_DIR/batch_op.py --op get --config $ONEST_TOOL_DIR/config.ini --location $DST_DIR_DIR --objectid $BUCKET_NAME/$VMID_BKID/disk.0.tgz"`
    exec_and_log "$SSH $DST_HOST mv $DST_DIR_DIR/$VMID_BKID ${DST_DIR}_compress"
    exec_and_log "$SSH $DST_HOST tar -xzvf ${DST_DIR}_compress/disk.0.tgz \
                          -C ${DST_DIR}  "
    exec_and_log "$SSH $DST_HOST rm -rf ${DST_DIR}_compress" 
    if [ "x$code1" != "x0" ]; then
            exit $code1
    fi
else
#####################################################add by gaozh for oNest end###################################################################
    case $SRC in
    http://*)
        log "Downloading $SRC"
        exec_and_log "$SSH $DST_HOST $WGET -O $DST_PATH $SRC" \
            "Error downloading $SRC"
        ;;

    *)
        if [ -n "${MATCH_RESULT}" ] 
	then
	    log "Cloning $SRC"
	    exec_and_log "$SSH $DST_HOST mkdir -p ${DST_DIR}_compress" \
		"Error creating directory ${DST_DIR}_compress"
	    exec_and_log "$SCP ${SRC}.tgz ${DST_HOST}:${DST_DIR}_compress" \
	        "Error copying ${SRC}.tgz to ${DST_HOST}:${DST_DIR}_compress"
	    exec_and_log "$SSH $DST_HOST tar -xzvf ${DST_DIR}_compress/disk.0.tgz \
                          -C ${DST_DIR}  "
	    exec_and_log "$SSH $DST_HOST rm -rf ${DST_DIR}_compress"
	else
	    log "Cloning $SRC"
	    exec_and_log "$SCP $SRC $DST" \
	        "Error copying $SRC to $DST"
	fi
	;;
    esac
####################################################add by gaozh for oNest begin###################################################################
fi
####################################################add by gaozh for oNest end#####################################################################

exec_and_log "$SSH $DST_HOST chmod a+rw $DST_PATH"

