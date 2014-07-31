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
        printf("\ttrack %d is %s\n", i, ct.track[i]);
        i++;
    }

    /* Finally, delete the CD */
    res = delete_cd(cd_res.cd_id[0]);
    printf("delete_cd returned %d\n", res);

    /* Then disconnect and exit */
    database_end();
    return EXIT_SUCCESS;
}
