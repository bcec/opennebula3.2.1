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

#ifndef POOL_SQL_H_
#define POOL_SQL_H_

#include <map>
#include <string>
#include <queue>

#include "SqlDB.h"
#include "PoolObjectSQL.h"
#include "Log.h"
#include "Hook.h"
#include "Backup.h"
#include "BackupDisk.h"


using namespace std;

/**
 * PoolSQL class. Provides a base class to implement persistent generic pools.
 * The PoolSQL provides a synchronization mechanism (mutex) to operate in
 * multithreaded applications. Any modification or access function to the pool
 * SHOULD block the mutex.
 */
class PoolSQL: public Callbackable, public Hookable
{
public:

    /**
     * Initializes the oid counter. This function sets lastOID to
     * the last used Object identifier by querying the corresponding database
     * table. This function SHOULD be called before any pool related function.
     *   @param _db a pointer to the database
     *   @param table the name of the table supporting the pool (to set the oid
     *   counter). If null the OID counter is not updated.
     *   @param with_uid the Pool objects have an owner id (uid)
     */
    PoolSQL(SqlDB * _db, const char * _table);

    virtual ~PoolSQL();

    /**
     *  Allocates a new object, writting it in the pool database. No memory is
     *  allocated for the object.
     *   @param objsql an initialized ObjectSQL
     *   @return the oid assigned to the object or -1 in case of failure
     */
    virtual int allocate(
        PoolObjectSQL   *objsql,
        string&          error_str);

    //----------------------------------------------------added by shenxy 20120227
    /* Function to  get VM backup recover info*/
    virtual void bkstate(int oid);
    /* Function to init  get VM backup recover info*/
    int  init_bkstate(void *nil, int num, char **values, char **names);


     //----------------------------------------------------added by shenxy 20120228
     /* Function to get VM backupdisk state info*/
     virtual void dkstate(int oid);
     /* Function to init  get VM backupdiskr info*/
     int  init_dkstate(void *nil, int num, char **values, char **names);
     

     //----------------------------------------------------added by shenxy 20120229
     /* Function to allocate a new VM backup object id,writting it in the pool database.*/
     virtual int bkoid();

    /* Callback to set the backup lastOID */
     virtual void init_bkoid();
    /* Callback to set the backup lastOID */
    int  init_bk(void *nil, int num, char **values, char **names);

    /* Function to update VM backup state*/
    virtual void updatebkstate(int oid,Backup::BkState state);
    

    //-----------------------------------------------------added by shenxy 20120301
    /* Function to get VM backup id*/
    virtual void getbid(int bid);
    /* Function to init get VM backup id*/
    int  init_getbid(void *nil, int num, char **values, char **names);

    /* Function to  get VM backup url*/
    virtual void bkdir(int oid);
     /* Function to init  get VM backup url*/
    int  init_bkdir(void *nil, int num, char **values, char **names);
     

     //----------------------------------------------------added by shenxy 20120302
     /* Function to init get VM backupdisk id*/
     virtual void init_dkoid();
      /* Function to init get VM backupdisk id*/
     int  init_dk(void *nil, int num, char **values, char **names);

      /* Function to get VM backupdisk id*/
      virtual int dkoid();

      /* Function to update VM backupdisk state*/
      virtual void updatedkstate(int oid,BackupDisk::DkState state);
   //------------------------------------------------------added by shenxy

   

    /**
     *  Gets an object from the pool (if needed the object is loaded from the
     *  database).
     *   @param oid the object unique identifier
     *   @param lock locks the object if true
     *
     *   @return a pointer to the object, 0 in case of failure
     */
    PoolObjectSQL * get(int oid, bool lock);

    /**
     *  Gets an object from the pool (if needed the object is loaded from the
     *  database).
     *   @param name of the object
     *   @param uid id of owner
     *   @param lock locks the object if true
     *
     *   @return a pointer to the object, 0 in case of failure
     */
    PoolObjectSQL * get(const string& name, int uid, bool lock);

    /**
     * Updates the cache name index. Must be called when the owner of an object
     * is changed
     *
     * @param old_name Object's name before the change
     * @param old_uid Object's owner ID before the change
     * @param new_name Object's name after the change
     * @param new_uid Object's owner ID after the change
     */
    void update_cache_index(string& old_name,
                            int     old_uid,
                            string& new_name,
                            int     new_uid);

    /**
     *  Finds a set objects that satisfies a given condition
     *   @param oids a vector with the oids of the objects.
     *   @param the name of the DB table.
     *   @param where condition in SQL format.
     *
     *   @return 0 on success
     */
    virtual int search(
        vector<int>&    oids,
        const char *    table,
        const string&   where);

    /**
     *  Updates the object's data in the data base. The object mutex SHOULD be
     *  locked.
     *    @param objsql a pointer to the object
     *
     *    @return 0 on success.
     */
    virtual int update(
        PoolObjectSQL * objsql)
    {
        int rc;

        rc = objsql->update(db);

        if ( rc == 0 )
        {
            do_hooks(objsql, Hook::UPDATE);
        }

        return rc;
    };

    /**
     *  Drops the object's data in the data base. The object mutex SHOULD be
     *  locked.
     *    @param objsql a pointer to the object
     *    @param error_msg Error reason, if any
     *    @return 0 on success, -1 DB error
     */
    virtual int drop(PoolObjectSQL * objsql, string& error_msg)
    {
        int rc = objsql->drop(db);

        if ( rc != 0 )
        {
            error_msg = "SQL DB error";
            return -1;
        }

        return 0;
    };

    /**
     *  Removes all the elements from the pool
     */
    void clean();

    /**
     *  Dumps the pool in XML format. A filter can be also added to the
     *  query
     *  @param oss the output stream to dump the pool contents
     *  @param where filter for the objects, defaults to all
     *
     *  @return 0 on success
     */
    virtual int dump(ostringstream& oss, const string& where) = 0;

protected:

    /**
     *  Pointer to the database.
     */
    SqlDB * db;

    /**
     *  Dumps the pool in XML format. A filter can be also added to the
     *  query
     *  @param oss the output stream to dump the pool contents
     *  @param elem_name Name of the root xml pool name
     *  @param table Pool table name
     *  @param where filter for the objects, defaults to all
     *
     *  @return 0 on success
     */
    int dump(ostringstream& oss, const string& elem_name,
             const char * table, const string& where);

    /* ---------------------------------------------------------------------- */
    /* Interface to access the lastOID assigned by the pool                   */
    /* ---------------------------------------------------------------------- */

    /**
     *  Gets the value of the last identifier assigned by the pool
     *   @return the lastOID of the pool
     */
    int get_lastOID()
    {
        return lastOID;
    };

    /**
     *  Sets the lastOID of the pool and updates the control database
     *    @param _lastOID for the pool
     */
    void set_update_lastOID(int _lastOID)
    {
        lastOID = _lastOID;

        update_lastOID();
    };

    //added by shenxy 20120227
   /* Function to get  VM backup recover info*/
    int         bkSTATE;

    /* Function to get  VM backupdisk info*/
    int         dkSTATE;

   /* Function to check VM backup id*/
    int      checkgetbid;

    /* Function to  VM backup url*/
    string          bkDIR;


private:

    pthread_mutex_t mutex;

    /**
     *  Max size for the pool, to control the memory footprint of the pool. This
     *  number MUST be greater than the max. number of objects that are
     *  accessed simultaneously.
     */
    static const unsigned int MAX_POOL_SIZE;

    /**
     *  Last object ID assigned to an object. It must be initialized by the
     *  target pool.
     */
    int lastOID;
   
     /**
     *  Last backup object ID
     */
    int  bkOID;                                //added by shenxy

    /**
     *  Last backupdisk object ID
     */
    int dkOID;                                  //added by shenxy

    /**
     *  Tablename for this pool
     */
    string table;

    /**
     *  The pool is implemented with a Map of SQL object pointers, using the
     *  OID as key.
     */
    map<int,PoolObjectSQL *> pool;

    /**
     *  This is a name index for the pool map. The key is the name of the object
     *  , that may be combained with the owner id.
     */
    map<string,PoolObjectSQL *> name_pool;

    /**
     *  Factory method, must return an ObjectSQL pointer to an allocated pool
     *  specific object.
     */
    virtual PoolObjectSQL * create() = 0;

    /**
     *  OID queue to implement a FIFO-like replacement policy for the pool
     *  cache.
     */
    queue<int> oid_queue;

    /**
     *  Function to lock the pool
     */
    void lock()
    {
        pthread_mutex_lock(&mutex);
    };

    /**
     *  Function to unlock the pool
     */
    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    };

    /**
     *  FIFO-like replacement policy function. Before removing an object (pop)
     *  from  the cache its lock is checked. The object is removed only if
     *  the associated mutex IS NOT blocked. Otherwise the oid is sent to the
     *  back of the queue.
     */
    void replace();

    /**
     *  Generate an index key for the object
     *    @param name of the object
     *    @param uid owner of the object, only used if needed
     *    
     *    @return the key, a string
     */
    virtual string key(const string& name, int uid)
    {
        ostringstream key;

        key << name << ':' << uid;

        return key.str();
    };

    /**
     *  Inserts the last oid into the pool_control table
     */
    void update_lastOID();

    /* ---------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------- */

    /**
     *  Callback to set the lastOID (PoolSQL::PoolSQL)
     */
    int  init_cb(void *nil, int num, char **values, char **names);

    /**
     *  Callback to store the IDs of pool objects (PoolSQL::search)
     */
    int  search_cb(void *_oids, int num, char **values, char **names);

    /**
     *  Callback function to get output in XML format
     *    @param num the number of columns read from the DB
     *    @param names the column names
     *    @param vaues the column values
     *    @return 0 on success
     */
    int dump_cb(void * _oss, int num, char **values, char **names);
};

#endif /*POOL_SQL_H_*/
