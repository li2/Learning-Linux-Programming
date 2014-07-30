/* A simplistic structure to represent the current CD, excluding the track
   information */
struct current_cd_st {
    int artist_id;
    int cd_id;
    char artist_name[100];
    char title[100];
    char catalogue[100];
};

/* A simplistic track details structure */
struct current_tracks_st {
    int cd_id;
    char track[20][100];
};

#define MAX_CD_RESULT 10
struct cd_search_st {
    int cd_id[MAX_CD_RESULT];
};

/* Database backend functions */
int database_start(char *name, char *password);
void database_end();

/* Functions for adding a CD */
int add_cd(char *artist, char *title, char *catalogue, int *cd_id);
int add_tracks(struct current_tracks_st *tracks);

/* Functions for finding and retrieving a CD */
int find_cds(char *search_str, struct cd_search_st *results);
int get_cd(int cd_id, struct current_cd_st *dest);
int get_cd_tracks(int cd_id, struct current_tracks_st *dest);

/* Function for deleting items */
int delete_cd(int cd_id);


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "app_mysql.h"

int main()
{
    struct current_cd_st cd;
    struct cd_search_st cd_res;
    struct current_tracks_st ct;
    int cd_id;
    int res, i;

    /* The first thing your app must always do is initialize a database connection,
       providing a valid user name and password */
    database_start("root", "weiyi");
    
    /* Then you test adding a CD: */
    res = add_cd("Bu Yi", "He Bu Wan De Jiu", "201407301321", &cd_id);
    printf("Result of adding a cd was %d, cd_id is %d\n", res, cd_id);

    memset(&ct, 0, sizeof(ct));
    ct.cd_id = cd_id;
    strcpy(ct.track[0], "San Feng");
    strcpy(ct.track[1], "Yang Rou Mian");
    strcpy(ct.track[2], "Na Me Jiu");
    add_tracks(&ct);

    /* Now search for the CD and retrieve information from the first CD found */
    res = find_cds("Jiu", &cd_res);
    printf("Found %d cds, first has ID %d\n", res, cd_res.cd_id[0]);

    res = get_cd(cd_res.cd_id[0], &cd);
    printf("get_cd returned %d\n", res);

    memset(&ct, 0, sizeof(ct));
    res = get_cd_tracks(cd_res.cd_id[0], &ct);
    printf("get_cd_tracks returned %d\n", res);
    printf("Title: %s\n", cd.title);
    i = 0;
    while (i < res) {
        printf("\ttrack %d is %s\n", i, ct,track[i]);
        i++;
    }

    /* Finally, delete the CD */
    res = delete_cd(cd_res.cd_id[0]);
    printf("delete_cd returned %d\n", res);

    /* Then disconnect and exit */
    database_end();
    return EXIT_SUCCESS;
}


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
                            NUll, 0)) {
        fprintf(stderr, "Database connection failure: %d, %s\n",
                mysql_errno(&my_connection), mysql_error(&my_connection));
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
//todo
        /* Last, but no least, set the ID of the newly added row */
        *cd_id = new_cd_id;
        if (new_cd_id != -1) {
            return(1);
        }
        return(0);
    }
}
