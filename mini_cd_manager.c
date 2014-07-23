/*******************************************************************************
一个管理CD唱片的程序。[《linux程序设计第4版》6.10节例程]
使用菜单完成“增、删、改、查”四项功能，使用文本存储数据。

TITLE_FILE 存储标题信息 ： 目录编号,标题,曲目类型,艺术家
TRACKS_FILE 存储曲目信息： 目录编号,曲目编号,曲名
目录编号因为是惟一的，所以用来关联2个文件，标题文件的一个数据项一般会对应曲目文件的多行数据。

“增”操作：仅指向标题文件中增加一条CD记录。而CD的曲目信息，通过“改”操作录入。
    fopen(), wgetnstr(), fprintf(), fclose().

“查”操作：引导用户输入需要查找的标题，逐行读取标题文件，进行匹配。
    wgetnstr()（curses库函数）, fopen(), fgets(), strstr() or strcmp(), fclose().
    不会把所有CD以列表形式呈现，所以针对某张CD操作时，必须先“查”，目前版本仅支持“标题”查找。
    需要变量 current_cd 存储“当前CD标题”，“增、查”2个操作会更新该变量，“删”重置该变量为'\0'。

“删”操作：删除当前CD记录，及其所有曲目。
    曲目文件每个条目，也就是每行的开头是目录编号；
    查找曲目文件中与“当前目录编号”不匹配的行，并写入临时文件，
    而匹配的行被丢弃，以达到删除的目的。
    fopen(), fgets(), strncmp(), fputs(), fclose().

“改”操作：不支持针对某个曲目编号的修改，需要先删除某个目录编号的所有曲目，然后重新录入。
    与“删api、增api”相同。

by li2, 2014-07-22~23, ShangHai ZhaBei.

********************************************************************************

使用到的API

FILE *fopen(const char *filename, const char *mode); 打开指定的文件，并关联到文件流。

FILE *fclose(FILE *stream); 关闭指定的文件流，冲洗所有数据。

int fprintf(FILE *stream, const char *format, ...); 
格式化不同类项的参数，并写入到指定的文件流（sprintf写到指定的字符串，printf写到标准输出）。

char *fgets(char *s, int n, FILE *stream);
从文件流读取一个字符串，遇到以下情况停止：读到换行符；已经读入n-1个字符（因为需要加上表示结尾的空字符'\0'）；到达文件尾。

int fputs(const char *str, FILE *fp); 把一个以空字符终止的字符串写入到指定流，尾端的终止符不写。

curses库函数
该例程“使用curses函数函数库管理基于文本的屏幕”，即使用了非常多的curses库函数，管理屏幕输入/出、键盘输入、窗口/子窗口等。使用细节暂且不深究。
学习该例程的目的是，厘清使用C语言完成该程序的逻辑流程，及，与Shell的区别。

int getch(void); 获取键盘输入，
    在cbreak()模式下，字符一旦键入，立刻传递给程序，不需要回车，所以不需要处理回车带来的'\n';
    在预处理模式（cooked）下，由于所有处理基于行，没有按回车时程序被阻塞。
******************************************************************************/

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
const char *TITLE_FILE = "title.cdb";
const char *TRACKS_FILE = "tracks.cdb";
const char *temp_file = "cdb.tmp";

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

//todo why aadd? Repeat the first letter?
char *main_menu[] = 
{
    "aadd new CD",
    "ffind CD",
    "ccount CDs and tracks in the catalog",
    "qquit",
    0,
};

char *extended_menu[] = 
{
    "aadd new CD",
    "ffind CD",
    "ccount CDs and tracks in the catalog",
    "l    list tracks on current CD",
    "r    remove current CD",
    "u    update track information",
    "qquit",
    0,
};

int main()
{
    int choice;
    initscr();
    do {
        /* 当current_cd 不为'\0'时，需要重绘为扩展菜单 */
        choice = getchoice("Options:", current_cd[0] ? extended_menu : main_menu);
        switch (choice) {
        case 'q':
            break;
        case 'a':
            add_record();
            break;
        case 'c':
            count_cds();
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
            attroff(A_STANDOUT);
        }
        current_row++;
        option_ptr++;
    }

    mvprintw(start_row + current_row + 3, start_col, "Move highlight then press Return ");
    refresh();
}

void clear_all_screen()
{
    clear();
    mvprintw(2, 20, "%s", "CD Database Application");
    if (current_cd[0]) {
        mvprintw(ERROR_LINE, 0, "Current CD: %s, %s\n",
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

    mvprintw(screenrow, screencol, "       CD Type: ");
    get_string(cd_type);
    screenrow++;

    mvprintw(screenrow, screencol, "        Artist: ");
    get_string(cd_artist);
    screenrow++;

    mvprintw(PROMPT_LINE-2, 5, "%s", "About to add this new entry");
    sprintf(cd_entry, "%s,%s,%s,%s", catalog_number, cd_title, cd_type, cd_artist);
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
    FILE *fp = fopen(TITLE_FILE, "a");
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

    tracks_fp = fopen(TRACKS_FILE, "a");

    /* Just to show how, enter the information in a scrolling, boxed,
       window. The trick is to set-up a sub-window, draw a box around the
       edge, then add a new, scrolling, sub-window just inside the boxed
       sub-window. */
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
        //todo mvprintw
        mvwprintw(sub_window_ptr, screen_line++, BOX_ROW_POS + 2, "Track %d: ", track);
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
    FILE *titles_fp, *temp_fp;
    char entry[MAX_ENTRY];
    int cat_length;

    if (current_cd[0] == '\0') {
        return;
    }

    clear_all_screen();
    mvprintw(PROMPT_LINE, 0, "About to remove CD %s: %s.",
             current_cat, current_cd);
    if (!get_confirm()) {
        return;
    }
    
    //todo whey current_cd remove all?
    cat_length = strlen(current_cat);

    /* Copy the titles file to a tempory, ignoring this CD */
    titles_fp = fopen(TITLE_FILE, "r");
    temp_fp = fopen(temp_file, "w");

    while (fgets(entry, MAX_ENTRY, titles_fp)) {
        /* Compare catalog number and copy entry if no match */
        if (strncmp(current_cat, entry, cat_length) != 0) {
            fputs(entry, temp_fp);
        }
    }
    fclose(titles_fp);
    fclose(temp_fp);

    /* Delete the titles file, and rename the temporary file */
    unlink(TITLE_FILE);
    rename(temp_file, TITLE_FILE);

    /* Now do the same for the tracks file */
    remove_tracks();

    /* Reset current CD to 'None' */
    current_cd[0] = '\0';
}

void remove_tracks()
{
    FILE *tracks_fp, *temp_fp;
    char entry[MAX_ENTRY];
    int cat_length;

    if (current_cd[0] == '\0') {
        return;
    }
    //todo whey current_cd remove all?
    cat_length = strlen(current_cat);

    tracks_fp = fopen(TRACKS_FILE, "r");
    if (tracks_fp == (FILE *)NULL) {
        return;
    }
    temp_fp = fopen(temp_file, "w");

    while (fgets(entry, MAX_ENTRY, tracks_fp)) {
        /* Compare catalog number and copy entry if no match */
        if (strncmp(current_cat, entry, cat_length) != 0) {
            fputs(entry, temp_fp);
        }
    }
    fclose(tracks_fp);
    fclose(temp_fp);

    /* Delete the tracks file, and rename the temporary file */
    unlink(TRACKS_FILE);
    rename(temp_file, TRACKS_FILE);
}

void count_cds()
{
    FILE *titles_fp, *tracks_fp;
    char entry[MAX_ENTRY];
    int titles = 0;
    int tracks = 0;

    titles_fp = fopen(TITLE_FILE, "r");
    if (titles_fp) {
        while (fgets(entry, MAX_ENTRY, titles_fp)){
            titles++;
        }
        fclose(titles_fp);
    }

    tracks_fp = fopen(TRACKS_FILE, "r");
    if (tracks_fp) {
        while (fgets(entry, MAX_ENTRY, tracks_fp)){
            tracks++;
        }
        fclose(tracks_fp);
    }

    mvprintw(ERROR_LINE, 0,
             "Database contains %d titles, with a total of %d tracks.",
             titles, tracks);
    get_return();
}

void find_cd()
{
    char match[MAX_STRING], entry[MAX_ENTRY];
    FILE *titles_fp;
    int count = 0;
    char *found, *title, *catalog;

    mvprintw(Q_LINE, 0, "Enter a string to search for in CD titles: ");
    get_string(match);

    titles_fp = fopen(TITLE_FILE, "r");
    if (titles_fp) {
        while (fgets(entry, MAX_ENTRY, titles_fp)) {
            /* Skip past catalog number */
            catalog = entry;
            if (found = strstr(catalog, ",")) {
                *found = '\0';
                title = found + 1;

                /* Zap the next comma in the entry to reduce it to title only */
                if (found = strstr(title, ",")) {
                    *found = '\0';
                    
                    /* Now see if the match substring is present */
                    if (found = strstr(title, match)) {
                        count++;
                        strcpy(current_cd, title);
                        strcpy(current_cat, catalog);
                    }
                }
            }
        }
        fclose(titles_fp);
    }
    if (count != 1) {
        if (count == 0) {
            mvprintw(ERROR_LINE, 0, "Sorry, no matching CD found.");
        }
        if (count > 1) {
            mvprintw(ERROR_LINE, 0, "Sorry, match is ambiguous: %d CDs found.", count);
        }
        current_cd[0] = '\0';
        get_return();
    }
}

void list_tracks()
{
    FILE *tracks_fp;
    char entry[MAX_ENTRY];
    int cat_length;
    int lines_op = 0;
    WINDOW *track_pad_ptr;
    int tracks = 0;
    int key;
    int first_line = 0;

    if (current_cd[0] == '\0') {
        mvprintw(ERROR_LINE, 0, "You must select a CD first.");
        get_return();
        return;
    }
    clear_all_screen();
    cat_length = strlen(current_cat);

    /* First count the number of tracks for the current CD */
    tracks_fp = fopen(TRACKS_FILE, "r");
    if (!tracks_fp) {
        return;
    }
    while (fgets(entry, MAX_ENTRY, tracks_fp)) {
        if (strncmp(current_cat, entry, cat_length) == 0) {
            tracks++;
        }
    }
    fclose(tracks_fp);

    /* Make a new pad, ensure that even if there is only a single
       track the PAD is large enough so the later prefresh() is always valid  */
    track_pad_ptr = newpad(tracks + 1 + BOXED_LINES, BOXED_ROWS + 1);
    if (!track_pad_ptr)
	return;

    tracks_fp = fopen(TRACKS_FILE, "r");
    if (!tracks_fp)
	return;

    mvprintw(4, 0, "CD Track Listing\n");

    /* write the track information into the pad */
    while (fgets(entry, MAX_ENTRY, tracks_fp)) {
        /* Compare catalog number and output rest of entry */
        if (strncmp(current_cat, entry, cat_length) == 0) {
            mvwprintw(track_pad_ptr, lines_op++, 0, "%s", entry + cat_length + 1);
        }
    }
    fclose(tracks_fp);

    if (lines_op > BOXED_LINES) {
        mvprintw(MESSAGE_LINE, 0, "Cursor keys to scroll, RETURN or q to exit");
    } else {
        mvprintw(MESSAGE_LINE, 0, "RETURN or q to exit");
    }
    wrefresh(stdscr);
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    key = 0;
    while (key != 'q' && key != KEY_ENTER && key != '\n') {
        if (key == KEY_UP) {
            if (first_line > 0) {
                first_line--;
            }
        }
        if (key == KEY_DOWN) {
            if (first_line + BOXED_LINES + 1 < tracks) {
                first_line++;
            }
        }
        /* now draw the appropriate part of the pad on the screen */
        prefresh(track_pad_ptr, first_line, 0,
                 BOX_LINE_POS, BOX_ROW_POS,
                 BOX_LINE_POS + BOXED_LINES, BOX_ROW_POS + BOXED_ROWS);
        key = getch();
    }

    delwin(track_pad_ptr);
    keypad(stdscr, FALSE);
    nocbreak();
    echo();
}

void get_return()
{
    int ch;
    mvprintw(23, 0, "%s", "Press return");
    refresh();
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
}
