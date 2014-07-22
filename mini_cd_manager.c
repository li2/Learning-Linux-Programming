#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>

#define MAX_STRING 80
#define MAX_ENTRY 1024
#define MESSAGE_LINE 6
#define ERROR_LINE 22
#define Q_LINE 20
#define PROMPT_LINE 18

static char current_cd[MAX_STRING] = "\0";
static char current_cat[MAX_STRING];
const char *title_file = "title.cdb";
const char *tracks_file="tracks.cdb";
const char *temp_file="cdb.tmp";

void clear_all_screen(void);
void get_return(void);
int get_confirm(void);
int getchoice(char *greet, char *choices[]);
void draw_menu(char *options[], int highlight, int start_row, int start_col);
void insert_title(char *cdtitle);
void get_string(char *string);
void add_record(void);
void count_cds(void);
void find_cd(void);
void list_tracks(void);
void remove_tracks(void);
void remove_cd(void);
void update_cd(void);

char *main_menu[] = 
{
    "add new CD",
    "find CD",
    "count CDs and tracks in the catalog",
    "quit",
    0,
};

char *extended_menu[] = 
{
    "add new CD",
    "find CD",
    "count CDs and tracks in the catalog",
    "    list tracks on current CD",
    "    remove current CD",
    "    update track information",
    "quit",
    0,
};

int main()
{
    int choice;
    initscr();
    do {
        choice = getchoice("Options:",
                           current_cd[0] ? extended_menu : main_menu);
        switch (choice) {
        case 'q':
            break;
        case 'a':
            add_record();
            break;
        case 'f':
            find_cd();
            break;
        case 'l':
            list_tracks();
            break;
        case 'r':
            remove_cd();
            break;
        case 'u':
            update_cd();
            break;
        }
    } while (choice != 'q');
    endwin();
    exit(EXIT_SUCCESS);
}

int getchoice(char *greet, char *choices[])
{
    static int selected_row = 0;
    int max_row = 0;
    int start_screenrow = MESSAGE_LINE;
    int start_screencol = 10;
    char **option;
    int selected;
    int key = 0;

    option = choices;
    while (*option) {
        max_row++;
        option++;
    }

    /* project against menu getting shorter when CD deleted */
    if (selected_row >= max_row){
        selected_row = 0;
    }
    clear_all_screen();
    mvprintw(start_screenrow - 2, start_screencol, greet);
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    key = 0;
    while (key != 'q' && key != KEY_ENTER && key != '\n') {
        if (key == KEY_UP) {
            if (selected_row == 0) {
                selected_row = max_row - 1;   
            } else {
                selected_row--;
            }
        }
        if (key == KEY_DOWN){
            if (selected_row == (max_row - 1)) {
                selected_row = 0;   
            } else {
                selected_row++;
            }
        }
        selected = *choices[selected_row];
        draw_menu(choices, selected_row, start_screenrow, start_screencol);
        key = getch();
    }
    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    if (key == 'q') {
        selected = 'q';
    }

    return (selected);
}

void draw_menu(char *options[], int current_highlight,
               int start_row, int start_col)
{
    int current_row = 0;
    char **option_ptr;
    char *txt_ptr;
    option_ptr = options;

    while (*option_ptr) {
        if (current_row == current_highlight) {
            attron(A_STANDOUT);
        }
        txt_ptr = options[current_row];
        txt_ptr++;
        mvprintw(start_row + current_row, start_col, "%s", txt_ptr);
        if (current_row == current_highlight) {
            sttroff(A_STANDOUT);
        }
        current_row++;
        option_ptr++;
    }

    mvprintw(start_row + current_row + 3, start_col,
             "Move highlight then press Return ");
    refresh();
}

void clear_all_screen()
{
    clear();
    mvprintw(2, 20, "%s", "CD Database Application");
    if (current_cd[0]) {
        mvprintw(ERROR_LINE, 0, "Current CD: %s: %s\n",
                 current_cat, current_cd);
    }
    refresh();
}

void add_record()
{
    char catalog_number[MAX_STRING];
    char cd_title[MAX_STRING];
    char cd_type[MAX_STRING];
    char cd_artist[MAX_STRING];
    char cd_entry[MAX_STRING];

    int screenrow = MESSAGE_LINE;
    int screencol = 10;

    clear_all_screen();
    mvprintw(screenrow, screencol, "Enter new CD details");
    screenrow += 2;

    mvprintw(screenrow, screencol, "Catalog Number: ");
    get_string(catalog_number);
    screenrow++;

    mvprintw(screenrow, screencol, "      CD Title: ");
    get_string(cd_title);
    screenrow++;

    mvprintw(screenrow, screencol, "        CD Type: ");
    get_string(cd_type);
    screenrow++;

    mvprintw(screenrow, screencol, "         Artist: ");
    get_string(cd_artist);
    screenrow++;

    mvprintw(PROMPT_LINE, 5, "%s", cd_entry);
    refresh();
    move(PROMPT_LINE, 0);
    if (get_confirm()) {
        insert_title(cd_entry);
        strcpy(current_cd, cd_title);
        strcpy(current_cat, catalog_number);
    }
}

void get_string(char *string)
{
    int len;

    wgetnstr(stdscr, string, MAX_STRING);
    len = strlen(string);
    if (len > 0 && string[len - 1] == '\n') {
        string[len - 1] = '\0';
    }
}

int get_confirm()
{
    int confirmed = 0;
    char first_char;
    mvprintw(Q_LINE, 5, "Are you sure?");
    clrtoeol();
    refresh();

    cbreak();
    first_char = getch();
    if (first_char == 'Y' || first_char == 'y') {
        confirmed = 1;
    }
    nocbreak();

    if (!confirmed) {
        mvprintw(Q_LINE, 1, "    Cancelled");
        clrtoeol();
        refresh();
        sleep(1);
    }
    return confirmed;
}

void insert_title(char *cdtitle)
{
    FILE *fp = fopen(title_file, "a");
    if (!fp) {
        mvprintw(ERROR_LINE, 0, "cannot open CD titles database");
    } else {
        fprintf(fp, "%s\n", cdtitle);
        fclose(fp);
    }
}


#define BOXED_LINES   11
#define BOXED_ROWS    60
#define BOX_LINE_POS  8
#define BOX_ROW_POS   2

void update_cd()
{
    FILE *tracks_fp;
    char track_name[MAX_STRING];
    int len;
    int track = 1;
    int screen_line = 1;
    WINDOW *box_window_ptr;
    WINDOW *sub_window_ptr;

    clear_all_screen();
    mvprintw(PROMPT_LINE, 0, "Re-entering tracks for CD.");
    if (!get_confirm()) {
        return;
    }
    move(PROMPT_LINE, 0);
    clrtoeol();

    remove_tracks();

    mvprintw(MESSAGE_LINE, 0, "Enter a blank line to finish");

    tracks_fp = fopen(tracks_file, "a");

    box_window_ptr = subwin(stdscr, BOXED_LINES + 2, BOXED_ROWS + 2,
                            BOX_LINE_POS - 1, BOX_ROW_POS - 1);

    if (!box_window_ptr) {
        return;
    }
    box(box_window_ptr, ACS_VLINE, ACS_HLINE);

    sub_window_ptr = subwin(stdscr, BOXED_LINES, BOXED_ROWS, BOX_LINE_POS, BOX_ROW_POS);
    if (!sub_window_ptr) {
        return;
    }
    scrollok(sub_window_ptr, TRUE);
    werase(sub_window_ptr);
    touchwin(stdscr);

    do {
        mvprintw(sub_window_ptr, screen_line++, BOX_ROW_POS + 2, "Track %d: ", track);
        clrtoeol();
        refresh();
        wgetnstr(sub_window_ptr, track_name, MAX_STRING);
        len = strlen(track_name);
        if (len > 0 && track_name[len - 1 ] == '\n') {
            track_name[len - 1] = '\0';
        }
        if (*track_name) {
            fprintf(tracks_fp, "%s,%d,%s\n", current_cat, track, track_name);
        }

        track++;
        if (screen_line > BOXED_LINES - 1) {
            /*time to start scrolling*/
            scroll(sub_window_ptr);
            screen_line--;
        }
    } while (*track_name);
    delwin(sub_window_ptr);

    fclose(tracks_fp);
}

void remove_cd()
{

}
