/* ------------------------------------------------------------------------ */
/* Copyright 2002-2012, OpenNebula Project Leads (OpenNebula.org)           */
/*                                                                          */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may  */
/* not use this file except in compliance with the License. You may obtain  */
/* a copy of the License at                                                 */
/*                                                                          */
/* http://www.apache.org/licenses/LICENSE-2.0                               */
/*                                                                          */
/* Unless required by applicable law or agreed to in writing, software      */
/* distributed under the License is distributed on an "AS IS" BASIS,        */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. */
/* See the License for the specific language governing permissions and      */
/* limitations under the License.                                           */
/* -------------------------------------------------------------------------*/

#ifndef IMAGE_H_
#define IMAGE_H_

#include "PoolSQL.h"
#include "ImageTemplate.h"
#include "NebulaLog.h"

using namespace std;

/**
 *  The Image class.
 */
class Image : public PoolObjectSQL
{
public:
    /**
     *  Type of Images
     */
    enum ImageType
    {
        OS        = 0, /** < Base OS image */
        CDROM     = 1, /** < An ISO9660 image */
        DATABLOCK = 2  /** < User persistent data device */
    };

    /**
     *  Image State
     */
    enum ImageState
    {
        INIT      = 0, /** < Initialization state */
        READY     = 1, /** < Image ready to use */
        USED      = 2, /** < Image in use */
        DISABLED  = 3, /** < Image can not be instantiated by a VM */
        LOCKED    = 4, /** < FS operation for the Image in process */
        ERROR     = 5  /** < Error state the operation FAILED*/
    };

    // *************************************************************************
    // Image Public Methods
    // *************************************************************************

    /**
     * Function to print the Image object into a string in XML format
     *  @param xml the resulting XML string
     *  @return a reference to the generated string
     */
    string& to_xml(string& xml) const;

    /**
     *  Rebuilds the object from an xml formatted string
     *    @param xml_str The xml-formatted string
     *
     *    @return 0 on success, -1 otherwise
     */
    int from_xml(const string &xml_str);

    /**
     *  Returns true if the image is persistent
     *     @return true if the image is persistent
     */
    bool isPersistent() const
    {
        return (persistent_img == 1);
    };

    /**
     *  Returns the source path of the image
     *     @return source of image
     */
    const string& get_source() const
    {
        return source;
    }

    /**
     *  Returns the original path of the image
     *     @return path of image
     */
    const string& get_path() const
    {
        return path;
    }

    /**
     *  Returns the fs_type for the image (defined for datablocks)
     *     @return fs_type
     */
    const string& get_fstype() const
    {
        return fs_type;
    }

    /**
     *  Returns the size of the image 
     *     @return size in mb
     */
    int get_size() const
    {
        return size_mb;
    }

    /**
     *  Sets the source path of the image
     */
    void set_source(const string& _source)
    {
        source = _source;
    }

    /**
     *  Sets the size for the image
     */
    void set_size(unsigned int _size_mb)
    {
        size_mb = _size_mb;
    }

    /**
     *  Returns the type of the image
     *     @return type
     */
    ImageType get_type()
    {
        return type;
    }
    /**
     *  Returns the image state
     *     @return state of image
     */
    ImageState get_state()
    {
        return state;
    }

    /**
     *  Sets the image state
     *     @param state of image
     */
    void set_state(ImageState _state)
    {
        state = _state;
    }

    /**
     *
     */
    int dec_running ()
    {
        return --running_vms;
    }

    /**
     *
     */
    int inc_running()
    {
        return ++running_vms;
    }

    /**
     *
     */
    int get_running()
    {
        return running_vms;
    }

    /**
     * Sets the Image type.
     *
     * @param _type the new type. It will be transformed to upper case
     * @return 0 on success, -1 otherwise
     */
    int set_type(string& _type);

    /**
     *  Check if the image can be used by other users
     *  @return true if group or others can access the image
     */
    bool isPublic()
    {
       return (group_u == 1 || other_u == 1); 
    }

    /**
     *  Set permissions for the Image. Extends the PoolSQLObject method
     *  by checking the persistent state of the image.
     */
    int set_permissions(int _owner_u,
                        int _owner_m,
                        int _owner_a,
                        int _group_u,
                        int _group_m,
                        int _group_a,
                        int _other_u,
                        int _other_m,
                        int _other_a,
                        string& error_str)
    {
        if ( isPersistent() && (_group_u == 1 || _other_u == 1) )
        {
            error_str = "Image cannot be public and persistent.";

            return -1;
        } 

        return PoolObjectSQL::set_permissions(_owner_u, _owner_m, _owner_a,
                                              _group_u, _group_m, _group_a,
                                              _other_u, _other_m, _other_a,
                                              error_str);
    };

    /**
     *  Set/Unset an image as persistent
     *    @param persistent true to make an image persistent
     *    @param error_str Returns the error reason, if any
     *
     *    @return 0 on success
     */
    int persistent(bool persis, string& error_str)
    {
        if ( running_vms != 0 )
        {
            goto error_vms;
        }

        if (persis == true)
        {
            
            if ( isPublic() )
            {
                goto error_public;
            }
            
            persistent_img = 1;
        }
        else
        {
            persistent_img = 0;
        }

        return 0;

    error_vms:
        error_str = "Image cannot be in 'used' state.";
        goto error_common;

    error_public:
        error_str = "Image cannot be public and persistent.";
        goto error_common;

    error_common:
        return -1;

    }

    /**
     * Modifies the given disk attribute adding the following attributes:
     *  * SOURCE: the file-path.
     *  * BUS:    will only be set if the Image's definition includes it.
     *  * TARGET: the value set depends on:
     *    - OS images will be mounted at prefix + a:  hda, sda.
     *    - Prefix + b is reserved for the contex cdrom.
     *    - CDROM images will be at prefix + c:  hdc, sdc.
     *    - Several DATABLOCK images can be mounted, they will be set to
     *      prefix + (d + index) :   hdd, hde, hdf...
     * @param disk attribute for the VM template
     * @param index number of datablock images used by the same VM. Will be
     *              automatically increased.
     * @param img_type will be set to the used image's type
     */
    int disk_attribute(VectorAttribute * disk, int* index, ImageType* img_type);

    /**
     *  Factory method for image templates
     */
    Template * get_new_template()
    {
        return new ImageTemplate;
    }

private:

    // -------------------------------------------------------------------------
    // Friends
    // -------------------------------------------------------------------------

    friend class ImagePool;

    // -------------------------------------------------------------------------
    // Image Description
    // -------------------------------------------------------------------------

    /**
     *  Type of the Image
     */
    ImageType    type;

    /**
     *  Persistency of the Image
     */
    int          persistent_img;

    /**
     *  Registration time
     */
    time_t       regtime;

    /**
     *  Path to the image
     */
    string       source;

    /**
     *  Original Path to the image (optional if source is given or datablock)
     */
    string       path;

    /**
     *  File system type for the image (mandatory for datablocks)
     */
    string       fs_type;

    /**
     *  Size of the image in MB
     */
    unsigned int size_mb;

     /**
      *  Image state
      */
    ImageState   state;

    /**
     * Number of VMs using the image
     */
    int running_vms;

    // *************************************************************************
    // DataBase implementation (Private)
    // *************************************************************************

    /**
     *  Execute an INSERT or REPLACE Sql query.
     *    @param db The SQL DB
     *    @param replace Execute an INSERT or a REPLACE
     *    @param error_str Returns the error reason, if any
     *    @return 0 on success
     */
    int insert_replace(SqlDB *db, bool replace, string& error_str);

    /**
     *  Bootstraps the database table(s) associated to the Image
     *    @return 0 on success
     */
    static int bootstrap(SqlDB * db)
    {
        ostringstream oss_image(Image::db_bootstrap);

        return db->exec(oss_image);
    };

    /**
     *  "Encrypts" the password with SHA1 digest
     *  @param password
     *  @return sha1 encrypted password
     */
    static string sha1_digest(const string& pass);

protected:

    // *************************************************************************
    // Constructor
    // *************************************************************************

    Image(int            uid,
          int            gid,
          const string&  uname,
          const string&  gname,
          ImageTemplate* img_template);

    virtual ~Image();

    // *************************************************************************
    // DataBase implementation
    // *************************************************************************

    static const char * db_names;

    static const char * db_bootstrap;

    static const char * table;

    /**
     *  Writes the Image in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    virtual int insert(SqlDB *db, string& error_str);

    /**
     *  Writes/updates the Images data fields in the database.
     *    @param db pointer to the db
     *    @return 0 on success
     */
    virtual int update(SqlDB *db);
};

#endif /*IMAGE_H_*/
