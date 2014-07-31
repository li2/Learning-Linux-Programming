#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mysql.h"
#include "app_mysql.h"

static MYSQL my_connection;
static int dbconnected = 0;

static int get_artist_id(char *artist);

int database_start(char *name, char *pwd)
{
    if (dbconnected) {
        return(1);
    }

    mysql_init(&my_connection);
    if (!mysql_real_connect(&my_connection, "localhost", name, pwd, "blpcd", 0,
                            NULL, 0)) {
        fprintf(stderr, "Database connection failure: %d, %s\n",
                mysql_errno(&my_connection), mysql_error&my_connection));
        return(0);
    }
    dbconnected = 1;
    return(1);
}

void database_end()
{
    if (dbconnected) {
        mysql_close(&my_connection);
    }
    dbconnected = 0;
}

/* You need a sanity check to ensure that you're connected to the database.
   The code would take care of artist names automatically. */
int add_cd(char *artist, char *title, char *catalogue, int *cd_id)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char is[250];
    char es[250];
    int artist_id = -1;
    int new_cd_id = -1;

    if (!dbconnected) {
        return(0);
    }

    /* The next thing is to check if the artist already exists; if not, create one.
       This is all taken care of in the function get_artist_id*/
    artist_id = get_artist_id(artist);

    /* Now having got an artist-id, you can insert the main CD record. Notice the use
       of mysql_escape_string to protect any special characters in the title of the CD*/
    mysql_escape_string(es, title, strlen(title));
    sprintf(is, "INSERT INTO cd(title, artist_id, catalogue) VALUES('%s', %d, '%s')",
            es, artist_id, catalogue);
    res = mysql_query(&my_connection, is);
    if (res) {
        fprintf(stderr, "Insert error %d: %s\n",
                mysql_errno(&my_connection), mysql_error(&my_connection));
        return(0);
    }

    /* When you come to add the tracks for this CD, you will need to know the 
       ID that was used when the CD record was inserted. You made the field an 
       auto-increment field, so the database has automatically assigned an ID,
       but you need to explicitly retrieve the value, with LAST_INSERT_ID. */
    res = mysql_query(&my_connection, "SELECT LAST_INSERT_ID()");
    if (res) {
        printf("SELECT error: %s\n", mysql_error(&my_connection));
        return(0);
    } else {
        res_ptr = mysql_use_result(&my_connection);
        if (res_ptr) {
            if ((mysqlrow = mysql_fetch_row(res_ptr))) {
                sscanf(mysqlrow[0], "%d", &new_cd_id);
            }
            mysql_free_result(res_ptr);
        }
        /* Last, but no least, set the ID of the newly added row */
        *cd_id = new_cd_id;
        if (new_cd_id != -1) {
            return(1);
        }
        return(0);
    }
}

/* Find or create an artist_id for the given string */
static int get_artist_id(char *artist)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char qs[250];
    char is[250];
    char es[250];
    int artist_id = -1;

    /* Does it already exist? */
    mysql_escape_string(es, artist, strlen(artist));
    sprintf(qs, "SELECT id FROM artist WHERE name='%s'", es);

    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "SELECT error %s\n", mysql_error(&my_connection));
        return(0);
    } else {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr) {
            if (mysql_num_rows(res_ptr) > 0) {
                if ((mysqlrow = mysql_fetch_row(res_ptr))) {
                    sscanf(mysqlrow[0], "%d", &artist_id);
                }
            }
            mysql_free_result(res_ptr);
        }
    }

    if (artist_id != -1) {
        return(artist_id);
    }

    sprintf(is, "INSERT INTO artist(name) VALUES('%s')", es);
    res = mysql_query(&my_connection, is);
    if (res) {
        fprintf(stderr, "Insert error %d: %s\n",
                mysql_errno(&my_connection), mysql_error(&my_connection));
        return(0);
    }

    res = mysql_query(&my_connection, "SELECT LAST_INSERT_ID()");
    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&my_connection));
        return(0);    
    } else {
        res_ptr = mysql_use_result(&my_connection);
        if (res_ptr) {
            if ((mysqlrow = mysql_fetch_row(res_ptr))) {
                sscanf(mysqlrow[0], "%d", &artist_id);
            }
            mysql_free_result(res_ptr);
        }
    }
    return(artist_id);
}

int add_tracks(struct current_tracks_st *tracks)
{
    int res;
    char is[250];
    char es[250];
    int i;

    if (!dbconnected) {
        return(0);
    }

    i = 0;
    while (tracks->track[i][0]) {
        mysql_escape_string(es, tracks->track[i], strlen(tracks->track[i]));
        sprintf(is, "INSERT INTO track(cd_id, track_id, title) VALUES(%d, %d, '%s')",
                tracks->cd_id, i + 1, es);
        res = mysql_query(&my_connection, is);
        if (res) {
            fprintf(stderr, "Insert error %d: %s\n",
                    mysql_errno(&my_connection), mysql_error(&my_connection));
            return(0);
        }
        i++;
    }
    return(1);
}

/* Based on given a CD ID value, to retrieve CD information. Databases are good at
   knowing how to perfrom complex queries efficiently, so never write appliaction code
   to do work that you could simply have asked database to do by passing it SQL. */
int get_cd(int cd_id, struct current_cd_st *dest)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char qs[250];

    if (!dbconnected) {
        return(0);
    }
    memset(dest, 0, sizeof(*dest));
    dest->artist_id = -1;

    sprintf(qs, "SELECT artist.id, cd.id, artist.name, cd.title, cd.catalogue \
            FROM artist, cd WHERE artist.id = cd.artist_id and cd.id = %d", cd_id);

    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&my_connection));
    } else {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr) {
            if (mysql_num_rows(res_ptr)) {
                if ((mysqlrow = mysql_fetch_row(res_ptr))) {
                    sscanf(mysqlrow[0], "%d", &dest->artist_id);
                    sscanf(mysqlrow[1], "%d", &dest->cd_id);
                    strcpy(dest->artist_name, mysqlrow[2]);
                    strcpy(dest->title, mysqlrow[3]);
                    strcpy(dest->catalogue, mysqlrow[4]);
                }
            }
            mysql_free_result(res_ptr);
        }
    }
    if (dest->artist_id != -1) {
        return(1);
    }
    return(0);
}

int get_cd_tracks(int cd_id, struct current_tracks_st *dest)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char qs[250];
    int i = 0, num_tracks = 0;

    if (!dbconnected) {
        return(0);
    }
    memset(dest, 0, sizeof(*dest));
    dest->cd_id = -1;

    sprintf(qs, "SELECT track_id, title FROM track WHERE track.cd_id = %d \
            ORDER BY track_id", cd_id);

    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&my_connection));
    } else {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr) {
            if ((num_tracks = mysql_num_rows(res_ptr)) > 0) {
                while ((mysqlrow = mysql_fetch_row(res_ptr))) {
                    strcpy(dest->track[i], mysqlrow[1]);
                    i++;
                }
                dest->cd_id = cd_id;
            }
            mysql_free_result(res_ptr);
        }
    }
    return(num_tracks);
}

/* Search for CDs. Kept the interface simple by limiting the number of results
   that could be returned. */
int find_cds(char *search_str, struct cd_search_st *results)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char qs[250];
    int i = 0;
    char ss[250];
    int num_rows = 0;

    if (!dbconnected) {
        return(0);
    }
    memset(dest, -1, sizeof(*dest));
    mysql_escape_string(ss, search_str, strlen(search_str));

    /* % is the character you need to insert in the SQL to match any string. */
    sprintf(ss, "SELECT DISTINCT artist.id, cd.id FROM artist, cd WHERE \ 
            artist.id = cd.artist_id and ( \ 
            artist.name LIKE '%%%s%%' OR \ 
            cd.title LIKE '%%%s%%' OR \ 
            cd.catalogue LIKE '%%%s%%')", ss, ss, ss);

    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "SELECT error: %s\n", mysql_error(&my_connection));
    } else {
        res_ptr = mysql_store_result(&my_connection);
        if (res_ptr) {
            if ((num_rows = mysql_num_rows(res_ptr)) > 0) {
                while ((mysqlrow = mysql_fetch_row(res_ptr)) && (i < MAX_CD_RESULT)) {
                    sscanf(mysqlrow[1], "%d", &dest->cd_id[i]);
                    i++;
                }
            }
            mysql_free_result(res_ptr);
        }
    }
    return(num_rows);
}

/* Delete CDs. In line with the policy of managing artist entries silently, you will
   delete the artist for the CD if no other CDs exist that have the same artist string.
   But SQL has no way of expressing deletes from multiple tables, so you do it in turn. */
int delete_cd(int cd_id)
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW mysqlrow;

    int res;
    char qs[250];
    int artist_id = -1;
    int num_rows = -1;

    if (!dbconnected) {
        return(0);
    }

    sprintf(qs, "SELECT artist_id FROM cd WHERE artist_id = \ 
            (SELECT artist_id FROM cd WHERE id = '%d')", cd_id);
    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "SELECT erro: %s\n", mysql_error(&my_connection));
    } else {
        res_ptr = mysql_use_result(&my_connection);
        if (res_ptr) {
            if ((num_rows = mysql_num_rows(res_ptr)) == 1) {
                /* artist not used by any other CDs */
                mysqlrow = mysql_fetch_row(res_ptr);
                sscanf(mysqlrow[0], "%d", &artist_id);
            }
            mysql_free_result(res_ptr);
        }
    }

    sprintf(qs, "DELETE FROM track WHERE cd_id = '%d'", cd_id);
    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "DELETE erro (track) %d: %s\n",
                mysql_errno(&my_connection), mysql_error(&my_connection));
        return(0);
    }

    sprintf(qs, "DELETE FROM cd WHERE id = '%d'", cd_id);
    res = mysql_query(&my_connection, qs);
    if (res) {
        fprintf(stderr, "DELETE erro (cd) %d: %s\n",
                mysql_errno(&my_connection), mysql_error(&my_connection));
        return(0);
    }

    if (artist_id != -1) {
        /* artist entry is now unrelated to any CDs, delete it*/
        sprintf(qs, "DELETE FROM artist WHERE id = '%d'", artist_id);
        res = mysql_query(&my_connection, qs);
        if (res) {
            fprintf(stderr, "DELETE erro (artist) %d: %s\n",
                    mysql_errno(&my_connection), mysql_error(&my_connection));
        }
    }

    return(1);
}

