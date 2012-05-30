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

#ifndef IMAGE_MANAGER_DRIVER_H_
#define IMAGE_MANAGER_DRIVER_H_

#include <map>
#include <string>
#include <sstream>

#include "Mad.h"

using namespace std;

//Forward definition of related classes
class ImagePool;
class ImageManager;

/**
 *  ImageManagerDriver represents the basic abstraction for Image Repository 
 *  drivers. It implements the protocol and recover functions from the Mad 
 *  interface.
 */
class ImageManagerDriver : public Mad
{
public:

    ImageManagerDriver(int        userid, 
                       const      map<string,string>& attrs, 
                       bool       sudo,
                       ImagePool* _ipool):
            Mad(userid,attrs,sudo),ipool(_ipool){};

    virtual ~ImageManagerDriver(){};

    /**
     *  Implements the Image Manager driver protocol.
     *    @param message the string read from the driver
     */
    void protocol(string& message);

    /**
     *  TODO: What do we need here? Check on-going xfr?
     */
    void recover();

private:
    friend class ImageManager;

    /**
     *  Reference to the ImagePool
     */
    ImagePool * ipool;

	/**	
	 *  Configuration file for the driver
	 */
	//Template	driver_conf;

    void cp(int oid, const string& source) const;

    /**
     *  Sends a move request to the MAD: "MV IMAGE_ID SRC_PATH DST_PATH"
     *    @param oid the image id.
     *    @param destination is a driver specific location or "-" if not 
     *           initialized
     *    @param size_mb of the image to be created
     */
    void mv(int oid, const string& source, const string& destination) const;

    /**
     *  Sends a make filesystem request to the MAD: "MKFS IMAGE_ID PATH SIZE_MB"
     *    @param oid the image id.
     *    @param fs type
     *    @param size_mb of the image to be created
     */
    void mkfs(int           oid, 
              const string& fs, 
              int           size_mb) const;
    /**
     *  Sends a delete request to the MAD: "DELETE IMAGE_ID PATH"
     *    @param oid the image id.
     *    @param destination is the path to the image to be removed
     */
    void rm(int oid, const string& destination) const;
};

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#endif /*IMAGE_MANAGER_DRIVER_H_*/

