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

/* ************************************************************************** */
/* Image Pool                                                                 */
/* ************************************************************************** */

#include "ImagePool.h"
#include "AuthManager.h"
#include "Nebula.h"
#include "PoolObjectAuth.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
string ImagePool::_default_type;
string ImagePool::_default_dev_prefix;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

ImagePool::ImagePool(SqlDB *       db,
                     const string& __default_type,
                     const string& __default_dev_prefix):
                        PoolSQL(db,Image::table)
{
    ostringstream sql;

    // Init static defaults
    _default_type       = __default_type;
    _default_dev_prefix = __default_dev_prefix;

    // Set default type
    if (_default_type != "OS"       &&
        _default_type != "CDROM"    &&
        _default_type != "DATABLOCK" )
    {
        NebulaLog::log("IMG", Log::ERROR, "Bad default for type, setting OS");
        _default_type = "OS";
    }
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int ImagePool::allocate (
        int            uid,
        int            gid,
        const string&  uname,
        const string&  gname,
        ImageTemplate* img_template,
        int *          oid,
        string&        error_str)
{
    Image *         img;
    Image *         img_aux = 0;
    string          name;
    ostringstream   oss;

    img = new Image(uid, gid, uname, gname, img_template);

    // Check name
    img->get_template_attribute("NAME", name);

    if ( name.empty() )
    {
        goto error_name;
    }

    if ( name.length() > 128 )
    {
        goto error_name_length;
    }

    // Check for duplicates
    img_aux = get(name,uid,false);

    if( img_aux != 0 )
    {
        goto error_duplicated;
    }

    // ---------------------------------------------------------------------
    // Insert the Object in the pool & Register the image in the repository
    // ---------------------------------------------------------------------
    *oid = PoolSQL::allocate(img, error_str);
    
    if ( *oid != -1 )
    {
        Nebula&        nd     = Nebula::instance();
        ImageManager * imagem = nd.get_imagem();

        if ( imagem->register_image(*oid) == -1 )
        {
            error_str = "Failed to copy image to repository. "
                        "Image left in ERROR state.";
            return -1;
        }
    }

    return *oid;

error_name:
    oss << "NAME cannot be empty.";

    goto error_common;

error_name_length:
    oss << "NAME is too long; max length is 128 chars.";
    goto error_common;

error_duplicated:
    oss << "NAME is already taken by IMAGE "
        << img_aux->get_oid() << ".";

error_common:
    delete img;

    *oid = -1;
    error_str = oss.str();

    return *oid;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static int get_disk_uid(VectorAttribute *  disk, int _uid)
{
    istringstream  is;

    string uid_s ;
    string uname;
    int    uid;

    if (!(uid_s = disk->vector_value("IMAGE_UID")).empty())
    {
        is.str(uid_s);
        is >> uid;

        if( is.fail() )
        {
            return -1;
        }
    }
    else if (!(uname = disk->vector_value("IMAGE_UNAME")).empty())
    {
        User *     user;
        Nebula&    nd    = Nebula::instance();
        UserPool * upool = nd.get_upool();
        
        user = upool->get(uname,true);
        
        if ( user == 0 )
        {
            return -1;
        }

        uid = user->get_oid();

        user->unlock();
    }
    else
    {
        uid = _uid;        
    }

    return uid;
}
        
/* -------------------------------------------------------------------------- */

static int get_disk_id(const string& id_s)
{
    istringstream  is;
    int            id;

    is.str(id_s);
    is >> id;

    if( is.fail() )
    {
        return -1;
    }

    return id;
}

/* -------------------------------------------------------------------------- */

int ImagePool::disk_attribute(VectorAttribute *  disk,
                              int                disk_id,
                              int *              index,
                              Image::ImageType * img_type,
                              int                uid,
                              int&               image_id)
{
    string  source;
    Image * img = 0;
    int     rc  = 0;

    ostringstream oss;

    Nebula&        nd     = Nebula::instance();
    ImageManager * imagem = nd.get_imagem();

    if (!(source = disk->vector_value("IMAGE")).empty())
    {
        int uiid = get_disk_uid(disk,uid);
       
        if ( uiid == -1)
        {
            return -1; 
        }

        img = imagem->acquire_image(source, uiid);

        if ( img == 0 )
        {
            return -1;
        }
    }
    else if (!(source = disk->vector_value("IMAGE_ID")).empty())
    {
        int iid = get_disk_id(source);

        if ( iid == -1)
        {
            return -1; 
        }

        img = imagem->acquire_image(iid);

        if ( img == 0 )
        {
            return -1;
        }
    }
    else //Not using the image repository
    {
        string type;

        rc   = -2;
        type = disk->vector_value("TYPE");

        transform(type.begin(),type.end(),type.begin(),(int(*)(int))toupper);

        if( type == "SWAP" )
        {
            string target = disk->vector_value("TARGET");

            if ( target.empty() )
            {
                string  dev_prefix = _default_dev_prefix;

                dev_prefix += "d";

                disk->replace("TARGET", dev_prefix);
            }
        }
    }

    if ( img != 0 )
    {
        img->disk_attribute(disk, index, img_type);

        image_id = img->get_oid();
        
        update(img);

        img->unlock();
    }

    oss << disk_id;
    disk->replace("DISK_ID",oss.str());

    return rc;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void ImagePool::authorize_disk(VectorAttribute * disk,int uid, AuthRequest * ar)
{
    string          source;
    Image *         img = 0;
    
    PoolObjectAuth  perm;

    if (!(source = disk->vector_value("IMAGE")).empty())
    {
        int uiid = get_disk_uid(disk,uid);
       
        if ( uiid == -1)
        {
            return; 
        }

        img = get(source , uiid, true);
    }
    else if (!(source = disk->vector_value("IMAGE_ID")).empty())
    {
        int iid = get_disk_id(source);

        if ( iid == -1)
        {
            return; 
        }

        img = get(iid, true);
    }

    if (img == 0)
    {
        return;
    }

    img->get_permissions(perm);

    img->unlock();

    ar->add_auth(AuthRequest::USE, perm);
}
