#define _XOPEN_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <ndbm.h>  /* may need to be changed to gdbm-ndbm.h on some distributions */

#include "cd_data.h"

#define CDC_FILE_BASE "cdc_data"
#define CDT_FILE_BASE "cdt_data"
#define CDC_FILE_DIR  "cdc_data.dir"
#define CDC_FILE_PAG  "cdc_data.pag"
#define CDT_FILE_DIR  "cdt_data.dir"
#define CDT_FILE_PAG  "cdt_data.pag"

/* use these two file scope variables to keep track of the current database */
static DBM *cdc_dbm_ptr = NULL;
static DBM *cdt_dbm_ptr = NULL;

/* By default, the function opens an existing database, but by passing a 
   nonzero parameter, you can force it to create a new empty database,
   effectively removing any exiting database. If the database is successfully
   initialized, the two database pointers are also initialized, indicating that
   a database is open.*/
int database_initialize(const int new_database)
{
    int open_mode = O_CREAT | O_RDWR;

    /* If any existing database is open then close it */
    if (cdc_dbm_ptr) {
        dbm_close(cdc_dbm_ptr);
    }
    if (cdt_dbm_ptr) {
        dbm_close(cdt_dbm_ptr);
    }

    if (new_database) {
        /* delete old files */
        (void) unlink(CDC_FILE_PAG);
        (void) unlink(CDC_FILE_DIR);
        (void) unlink(CDT_FILE_PAG);
        (void) unlink(CDT_FILE_DIR);
    }

    /* Open some new files, creating them if required */
    cdc_dbm_ptr = dbm_open(CDC_FILE_BASE, open_mode, 0644);
    cdt_dbm_ptr = dbm_open(CDT_FILE_BASE, open_mode, 0644);
    if (!cdc_dbm_ptr || !cdt_dbm_ptr) {
        fprintf(stderr, "Unable to create database\n");
        cdc_dbm_ptr = cdt_dbm_ptr = NULL;
        return(0);
    }
    return(1);
}

/* Close the database if it was open and sets the two database pointers to null
   to indicate that no database is currently open. */
void database_close(void)
{
    if (cdc_dbm_ptr) {
        dbm_close(cdc_dbm_ptr);
    }
    if (cdt_dbm_ptr) {
        dbm_close(cdt_dbm_ptr);
    }
    cdc_dbm_ptr = cdt_dbm_ptr = NULL;
}

/* Retrieve a single catalog entry when passed a pointer pointing to a catalog text string. If the entry isn't found, the returned data has an empty catalog field. */
cdc_entry get_cdc_entry(const char *cd_catalog_ptr)
{
    cdc_entry entry_to_return;
    char entry_to_find[CAT_CAT_LEN + 1];
    datum local_data_datum;
    datum local_key_datum;

    memset(&entry_to_return, '\0', sizeof(entry_to_return));

    /* start with some sanity checks, to ensure that a database is open and that
       you were passed reasonable parameters - that is, the search key contains
       only the valid string and nulls */
    if (!cdc_dbm_ptr || !cdt_dbm_ptr) {
        return(entry_to_return);
    }
    if (!cd_catalog_ptr) {
        return(entry_to_return);
    }
    if (strlen(cd_catalog_ptr) >= CAT_CAT_LEN) {
        return(entry_to_return);
    }

    memset(&entry_to_find, '\0', sizeof(entry_to_find));
    strcpy(entry_to_find, cd_catalog_ptr);

    /* set up the datum structure the dbm functions require, and then
       use dbm_fetch to retrieve the data. If no data was retrieved,
       return the empty entry_to_return structure */
    local_key_datum.dptr = (void *)entry_to_find;
    local_key_datum.dsize = sizeof(entry_to_find);

    memset(&local_data_datum, '\0', sizeof(local_data_datum));
    local_data_datum = dbm_fetch(cdc_dbm_ptr, local_key_datum);
    if (local_data_datum.dptr) {
        memcpy(&entry_to_return, (char *)local_data_datum.dptr, local_data_datum.dsize);
    }
    return(entry_to_return);
}

/* Retrieve a single track entry, a pointer pointing to a catalog text string and
   a track number as parameters. */
cdt_entry get_cdt_entry(const char *cd_catalog_ptr, const int track_no)
{
    cdt_entry entry_to_return;
    char entry_to_find[CAT_CAT_LEN + 10];
    datum local_data_datum;
    datum local_key_datum;

    memset(&entry_to_return, '\0', sizeof(entry_to_return));

    if (!cdc_dbm_ptr || !cdt_dbm_ptr) {
        return(entry_to_return);
    }
    if (!cd_catalog_ptr) {
        return(entry_to_return);
    }
    if (strlen(cd_catalog_ptr) >= CAT_CAT_LEN) {
        return(entry_to_return);
    }

    /* set up the search key, which is a composite key of catalog entry
       and track number. only difference with get_cdc_entry function */
    memset(&entry_to_find, '\0', sizeof(entry_to_find));
    sprintf(entry_to_find, "%s %d", cd_catalog_ptr, track_no);

    local_key_datum.dptr = (void *)entry_to_find;
    local_key_datum.dsize = sizeof(entry_to_find);

    memset(&local_data_datum, '\0', sizeof(local_data_datum));
    local_data_datum = dbm_fetch(cdc_dbm_ptr, local_key_datum);
    if (local_data_datum.dptr) {
        memcpy(&entry_to_return, (char *)local_data_datum.dptr, local_data_datum.dsize);
    }
    return(entry_to_return);
}

/* Add a new catalog entry */
int add_cdc_entry(const cdc_entry entry_to_add)
{
    char key_to_add[CAT_CAT_LEN + 1];
    datum local_data_datum;
    datum local_key_datum;
    int result;

    if (!cdc_dbm_ptr || !cdt_dbm_ptr) {
        return(0);
    }
    if (strlen(entry_to_add.catalog) >= CAT_CAT_LEN) {
        return(0);
    }

    memset(&key_to_find, '\0', sizeof(key_to_find));
    strcpy(key_to_find, entry_to_add.catalog);

    local_key_datum.dptr = (void *)key_to_add;
    local_key_datum.dsize = sizeof(key_to_add);
    local_data_datum.dptr = (void *)&entry_to_add;
    local_data_datum.dsize = sizeof(entry_to_add);

    result = dbm_store(cdc_dbm_ptr, local_key_datum, local_data_datum, DBM_REPLACE); 
    
    /*dbm_store() uses 0 for success */
    if (result == 0) {
        return(1);
    }

    return(0);
}

//to 10.
int add_cdt_entry()
