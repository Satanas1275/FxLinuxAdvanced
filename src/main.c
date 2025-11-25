#include <gint/display.h>
#include <gint/display-fx.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <gint/bfile.h>
#include <gint/gint.h>
#include <dirent.h>
#include <gint/std/stdlib.h>
#include <gint/std/stdio.h>
#include <gint/std/string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/list.h"

#define SCREEN 100

int F2_UPPERCASE = 0;
int ALPHAPETS = 0;
int BELOW_BAR = 1;
extern bopti_image_t img_boot;
extern bopti_image_t img_e_slec;
extern bopti_image_t img_b_slec;
extern bopti_image_t img_below_bar;
char PWD[150];

// Forward decl
static int command_exec (char command_input[], list_t* scr, int x_scrl, int y_scrl);

// ===================================================================
//                          UTILS
// ===================================================================

// GET INPUT FROM BUTTONS
static char* button_reader (key_event_t key) {
    if (key.alpha) {
        if (ALPHAPETS) ALPHAPETS=0;
        else           ALPHAPETS=1;
    }
    if (ALPHAPETS) {
        switch (key.key) {
            case KEY_XOT     : if (!F2_UPPERCASE) return "a"; else return "A";
            case KEY_LOG     : if (!F2_UPPERCASE) return "b"; else return "B";
            case KEY_LN      : if (!F2_UPPERCASE) return "c"; else return "C";
            case KEY_SIN     : if (!F2_UPPERCASE) return "d"; else return "D";
            case KEY_COS     : if (!F2_UPPERCASE) return "e"; else return "E";
            case KEY_TAN     : if (!F2_UPPERCASE) return "f"; else return "F";
            case KEY_FRAC    : if (!F2_UPPERCASE) return "g"; else return "G";
            case KEY_FD      : if (!F2_UPPERCASE) return "h"; else return "H";
            case KEY_LEFTP   : if (!F2_UPPERCASE) return "i"; else return "I";
            case KEY_RIGHTP  : if (!F2_UPPERCASE) return "j"; else return "J";
            case KEY_COMMA   : if (!F2_UPPERCASE) return "k"; else return "K";
            case KEY_ARROW   : if (!F2_UPPERCASE) return "l"; else return "L";
            case KEY_7       : if (!F2_UPPERCASE) return "m"; else return "M";
            case KEY_8       : if (!F2_UPPERCASE) return "n"; else return "N";
            case KEY_9       : if (!F2_UPPERCASE) return "o"; else return "O";
            case KEY_4       : if (!F2_UPPERCASE) return "p"; else return "P";
            case KEY_5       : if (!F2_UPPERCASE) return "q"; else return "Q";
            case KEY_6       : if (!F2_UPPERCASE) return "r"; else return "R";
            case KEY_MUL     : if (!F2_UPPERCASE) return "s"; else return "S";
            case KEY_DIV     : if (!F2_UPPERCASE) return "t"; else return "T";
            case KEY_1       : if (!F2_UPPERCASE) return "u"; else return "U";
            case KEY_2       : if (!F2_UPPERCASE) return "v"; else return "V";
            case KEY_3       : if (!F2_UPPERCASE) return "w"; else return "W";
            case KEY_ADD     : if (!F2_UPPERCASE) return "x"; else return "X";
            case KEY_SUB     : if (!F2_UPPERCASE) return "y"; else return "Y";
            case KEY_0       : if (!F2_UPPERCASE) return "z"; else return "Z";
            case KEY_DOT     : if (!F2_UPPERCASE) return " "; else return " ";
            case KEY_EXP     : return "\"";
            default:           return "";
        }
    } else if ((key.alpha==0) && (key.shift==1)) {
        switch (key.key) {
            case KEY_MUL : return "{";
            case KEY_DIV : return "}";
            case KEY_ADD : return "[";
            case KEY_SUB : return "]";
            case KEY_DOT : return "=";
            default:       return "";
        }
    } else {
        switch (key.key) {
            case KEY_XOT     : return "x";
            case KEY_LEFTP   : return "(";
            case KEY_RIGHTP  : return ")";
            case KEY_COMMA   : return ",";
            case KEY_1       : return "1";
            case KEY_2       : return "2";
            case KEY_3       : return "3";
            case KEY_4       : return "4";
            case KEY_5       : return "5";
            case KEY_6       : return "6";
            case KEY_7       : return "7";
            case KEY_8       : return "8";
            case KEY_9       : return "9";
            case KEY_0       : return "0";
            case KEY_MUL     : return "*";
            case KEY_DIV     : return "/";
            case KEY_ADD     : return "+";
            case KEY_SUB     : return "-";
            case KEY_NEG     : return "-";
            case KEY_DOT     : return ".";
            default:           return "";
        }
    }
}

// PRINT TO TERMINAL
static void print_txt (list_t* screen, char* text, int xapp, int yapp) {
    List_Push(screen, text);
    int this_idx = screen->count - 1;
    if (this_idx == 0) dtext(1+xapp , 1+yapp , C_BLACK,text);
    else dtext(1+xapp , ((this_idx)*10)+yapp , C_BLACK,text);
    if (BELOW_BAR==1) {dimage(1,56,&img_below_bar);} 
    dupdate();
}

// UPDATE TERMINAL, OR APPLY SCROLLING
static void scroll_draw (list_t* screen, int x_add ,int y_add) {
    dclear(C_WHITE);
    for (int i = 0; i<screen->count; i+=1) {
        int y;
        if (i == 0) y = 1+y_add; else y = ((i)*8)+(2*i)+y_add;
        dtext(1+x_add,y,C_BLACK, List_Get(screen,i));
    }
    if (BELOW_BAR==1) {dimage(1,56,&img_below_bar);} 
    dupdate();
}

// chat[] TO uint16_t
static uint16_t* convertToUint16(char *input) {
    int length = strlen(input);
    uint16_t *uint16String = (uint16_t*)malloc((length + 1) * sizeof(uint16_t));
    if (uint16String == NULL) { fprintf(stderr, "Memory allocation failed\n"); exit(EXIT_FAILURE);}    
    for (int i = 0; i < length; ++i) uint16String[i] = (uint16_t)input[i];
    uint16String[length] = 0; 
    return uint16String;
}

// REPLACE A STRING WITH ANOTHER FROM SOURCE
static void strrep(char *str, const char *oldSubstr, const char *newSubstr) {
    char *pos = str;
    while ((pos = strstr(pos, oldSubstr)) != NULL) {
        size_t remainingLength = strlen(pos + strlen(oldSubstr));
        memmove(pos + strlen(newSubstr), pos + strlen(oldSubstr), remainingLength + 1);
        memcpy(pos, newSubstr, strlen(newSubstr));
        pos += strlen(newSubstr);
    }
}

// PREPARE A PATH 
static void create_path(char* result, const char* path, const char* entrie, int fls0) {
    char path_cpy[ strlen(path) + 1 ];
    strcpy(path_cpy,path);
    size_t pathLen = strlen(path_cpy);
    size_t filenameLen = strlen(entrie);
    size_t joinedLen = pathLen + 1 + filenameLen;
    if (fls0) joinedLen += strlen("\\\\fls0");
    char joinedPath[joinedLen + 1];
    if (fls0) {
        strrep(path_cpy,"/","\\");
        strcpy(joinedPath, "\\\\fls0\\");
        strcat(joinedPath, path_cpy + 1);
    } else {
        strcpy(joinedPath, "/");
        strcat(joinedPath, path_cpy + 1);
    }
    if (path_cpy[pathLen - 1] != '/' && path_cpy[pathLen - 1] != '\\') {
        if (fls0) strcat(joinedPath, "\\"); else strcat(joinedPath, "/");
    }
    strcat(joinedPath, entrie);
    strcpy(result, joinedPath);
}

// SPLIT A STRING
static char *strsep2(char **stringp, const char *delim) {
    if (*stringp == NULL) return NULL;
    char *start = *stringp; char *end = strpbrk(start, delim);
    if (end != NULL) { *end = '\0'; *stringp = end + 1; } else { *stringp = NULL; }
    return start;
}

// ===================================================================
//                         LINUX COMMAND
// ===================================================================

static int exit_ (list_t* scr, int app_to_line, int x_scrl, int y_scrl) {
    char* lst = List_End(scr);
    if (lst[0] != '>') return 0;
    if (app_to_line) { List_Delete_Last(scr); List_Push(scr,">exit"); }
    scroll_draw(scr,x_scrl,y_scrl);
    print_txt(scr,"Are sure (Y/n)?",x_scrl,y_scrl);
    int do_exit = 0;
    while (true) {
        key_event_t c = getkey();
        if (c.key == KEY_SUB) { //Y
            char* last_line = List_End(scr);
            char result[strlen("Y") + strlen(last_line) + 1];
            strcpy(result, last_line); strcat(result, "Y");
            List_Delete_Last(scr); List_Push(scr,result);
            scroll_draw(scr,x_scrl,y_scrl);
            do_exit=1; sleep_us_spin(1500000); break;
        }
        if (c.key == KEY_8) {
            char* last_line = List_End(scr);
            char result[strlen("n") + strlen(last_line) + 1];
            strcpy(result, last_line); strcat(result, "n");
            List_Delete_Last(scr); List_Push(scr,result);
            scroll_draw(scr,x_scrl,y_scrl);
            sleep_us_spin(700000); break;
        }
    }
    return do_exit;
}

static void help_ (list_t* scr, int app_to_line, int x_scrl, int y_scrl) {
    char* lst = List_End(scr);
    if (lst[0] != '>') return;
    if (app_to_line) { List_Delete_Last(scr); List_Push(scr,">help"); }
    print_txt(scr , "FxLinux Commands:" , x_scrl , y_scrl);
    print_txt(scr , " help: Show this message" , x_scrl , y_scrl);
    print_txt(scr , " exit: Shutdown the system" , x_scrl , y_scrl);
    print_txt(scr , " shutdown: Shutdown the system" , x_scrl , y_scrl);
    print_txt(scr , " clear: Clear Terminal" , x_scrl , y_scrl);
    print_txt(scr , " ls: List current directory" , x_scrl , y_scrl);
    print_txt(scr , " cat <F>: Print file content" , x_scrl , y_scrl);
    print_txt(scr , " uname: OS information" , x_scrl , y_scrl);
    print_txt(scr , " echo <S>: Print text" , x_scrl , y_scrl);
    print_txt(scr , " cd <D>, cd: Change directory" , x_scrl , y_scrl);
    print_txt(scr , " bash <F>, ./<F>: Execute bash file" , x_scrl , y_scrl);
    print_txt(scr , " rm <F>: Remove file" , x_scrl , y_scrl);
    print_txt(scr , " rmdir <D>: Remove directory" , x_scrl , y_scrl);
    print_txt(scr , " sleep <I>: Sleep for seconds" , x_scrl , y_scrl);
    print_txt(scr , " pwd: Show current path" , x_scrl , y_scrl);
    print_txt(scr , " touch <F>: Create empty file" , x_scrl , y_scrl);
    print_txt(scr , " mkdir <D>: Make directory" , x_scrl , y_scrl);
    print_txt(scr , " nano <F>: Open simple editor" , x_scrl , y_scrl);
    print_txt(scr , " startx: Simple desktop GUI" , x_scrl , y_scrl);
    scroll_draw(scr,x_scrl,y_scrl);
}

static void clear (list_t* scr, int app_to_line) {
    List_Clear(scr);
    if (app_to_line) print_txt(scr,">",0,0);
    scroll_draw(scr,0,0);
}

static void uname_ (list_t* scr, int x_scrl ,int y_scrl) {
    print_txt(scr , "FxLinuxAdvanced 0.1.0" , x_scrl , y_scrl);
    print_txt(scr , "Build Date:" , x_scrl , y_scrl);
    print_txt(scr , &__TIMESTAMP__[4] , x_scrl , y_scrl);
    scroll_draw(scr, x_scrl, y_scrl);
}

static void echo_ (char line[], list_t* scr, int x_scrl ,int y_scrl){
    line+=5;
    print_txt(scr, line ,x_scrl,y_scrl);
    scroll_draw(scr,x_scrl,y_scrl);
}

static void listdir(const char* path, list_t* scr, int x_scrl ,int y_scrl) {
    DIR* dir = opendir(path);
    if (!dir) { print_txt(scr,"Error exploring directory",x_scrl,y_scrl); return; }
    print_txt(scr, "Listing directory:", x_scrl, y_scrl);
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..")==0) continue;
        if (ent->d_type == DT_DIR) {
            char out[ strlen(ent->d_name) + 3 ];
            strcpy(out, "["); strcat(out, ent->d_name); strcat(out, "]");
            print_txt(scr, out, x_scrl,y_scrl);
        }
    }
    rewinddir(dir);
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..")==0) continue;
        if (ent->d_type == DT_REG) print_txt(scr, ent->d_name, x_scrl,y_scrl);
    }
    closedir(dir);
}

// IF 'elem' IN 'path'
static int fd_in_path (const char* path, char elem[], unsigned char type) {
    int found = 0;
    DIR* dir = opendir(path);
    if (!dir) return 0;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, elem) == 0) {
            if (ent->d_type == type) { found = 1; break; }
        }
    }
    closedir(dir);
    return found;
}

static void cat(char file_path[], list_t* scr, int x_scrl, int y_scrl) {
    FILE *file; char line[150];
    file = fopen(file_path, "r");
    if (file == NULL) { print_txt(scr, "Error opening file", x_scrl, y_scrl); return; }
    while (fgets(line, sizeof(line), file) != NULL) { strrep(line,"\r\n",""); print_txt(scr, line, x_scrl, y_scrl); }
    fclose(file);
}

static void chdir_ (char* pwd, char input[], list_t* scr, int x_scrl, int y_scrl) {
    char *tok, *inp_p; char input_cpy [strlen(input) +1]; strcpy(input_cpy, input); inp_p = input_cpy;
    while ((tok = strsep2(&inp_p,"/")) != NULL) {
        if (strstr(input, tok) == NULL || strcmp(tok,"") == 0) continue;
        if (strcmp(tok,"") == 0) continue; if (strcmp(tok,".") == 0) continue;
        if (strcmp(tok, "..") == 0) {
            if (strcmp(pwd,"/") == 0 || strcmp(pwd,"//") == 0) continue;
            char *tok2 , *pwd_p; pwd_p=pwd; char result_p[ strlen(pwd) + 1 ]; char result_n[ strlen(pwd) + 1 ]; strcpy(result_n,"/");
            while ((tok2 = strsep2(&pwd_p, "/")) != NULL) { if (strcmp(tok2,"") == 0) continue; strcpy(result_p, result_n); strcat(result_n, tok2); strcat(result_n, "/"); }
            strcpy(pwd, result_p);
        } else {
            if (!fd_in_path(pwd,tok,DT_DIR)) {
                char msg[strlen(tok) + strlen("Directory '' not found!") + 1]; strcpy(msg, "Directory '"); strcat(msg, tok); strcat(msg, "' not found!");
                print_txt(scr, msg, x_scrl, y_scrl); return;
            }
            char pwd_cpy [ strlen(pwd) + strlen(tok) + 2]; strcpy(pwd_cpy, pwd);
            if (pwd[ strlen(pwd)-1 ] != '/') strcat(pwd_cpy,"/"); strcat(pwd_cpy, tok); strcpy(pwd, pwd_cpy);
        }
    }
}

static void bash_exec (char* file_path, list_t* scr, int x_scrl, int y_scrl) {
    FILE *file; char line[150]; file = fopen(file_path, "r");
    if (file == NULL) { print_txt(scr, "Error opening file", x_scrl, y_scrl); return; }
    while (fgets(line, sizeof(line), file) != NULL) {
        char line_cpy[150]; strcpy(line_cpy, line); strrep(line_cpy,"\r\n","");
        int continue_sys = command_exec( line_cpy, scr, x_scrl, y_scrl);
        if (!continue_sys) break;
    }
    fclose(file);
}

// ===================================================================
//                           SIMPLE NANO
// ===================================================================

static void draw_status(const char* filename, int cx, int cy) {
    drect(0, 56, 127, 63, C_BLACK);
    char status[64];
    snprintf(status, sizeof(status), "nano %s  Ln %d Col %d  F5:Save F6:Exit", filename, cy+1, cx+1);
    dtext(1, 58, C_WHITE, status);
}

static void nano_editor(const char* filepath, const char* display_name) {
    size_t cap = 4096; // 4 KiB buffer to limit RAM usage
    char *buf = malloc(cap);
    if(!buf) return;
    int len = 0; memset(buf, 0, cap);
    FILE* f = fopen(filepath, "r"); if (f) { len = fread(buf, 1, cap-1, f); fclose(f);}    
    int cx=0, cy=0; int scroll=0;
    while (1) {
        dclear(C_WHITE);
        int line=0; int visible=0; int i=0; int start=0;
        for (i=0; i<=len; i++) {
            if (buf[i]=='\n' || i==len) {
                if (line>=scroll && visible<7) {
                    char saved = buf[i]; buf[i]=0; dtext(1, visible*8, C_BLACK, &buf[start]); buf[i]=saved; visible++;
                }
                start = i+1; line++;
            }
        }
        draw_status(display_name, cx, cy); dupdate();
        key_event_t k = getkey();
        if (k.key == KEY_F6 || k.key == KEY_EXIT) break;
        if (k.key == KEY_F5) { FILE* fw = fopen(filepath, "w"); if (fw) { fwrite(buf, 1, len, fw); fclose(fw);} continue; }
        if (k.key == KEY_UP) { if (scroll>0) scroll--; if (cy>0) cy--; }
        else if (k.key == KEY_DOWN) { scroll++; cy++; }
        else if (k.key == KEY_DEL) { if (len>0) { len--; buf[len]=0; if (cx>0) cx--; } }
        else if (k.key == KEY_EXE) { if (len < (int)cap-1) { buf[len++]='\n'; buf[len]=0; cy++; cx=0; } }
        else {
            char* s = button_reader(k);
            if (s && s[0]) { int sl = strlen(s); if (len+sl < (int)cap-1) { memcpy(&buf[len], s, sl); len += sl; buf[len]=0; cx += sl; } }
        }
    }
    free(buf);
}

// ===================================================================
//                           SIMPLE GUI
// ===================================================================

static void draw_rect_border(int x1,int y1,int x2,int y2,int color) {
    drect(x1,y1,x2,y2,C_WHITE);
    dline(x1,y1,x2,y1,color);
    dline(x2,y1,x2,y2,color);
    dline(x2,y2,x1,y2,color);
    dline(x1,y2,x1,y1,color);
}

static void gui_loop(void) {
    int mx=10,my=10;
    while (1) {
        dclear(C_WHITE);
        dtext(4,4,C_BLACK,"FxLinux Desktop");
        draw_rect_border(10, 20, 40, 40, C_BLACK);
        dtext(12,42,C_BLACK,"Terminal");
        draw_rect_border(60, 20, 90, 40, C_BLACK);
        dtext(66,42,C_BLACK,"Exit");
        drect(0,56,127,63,C_BLACK);
        dtext(2,58,C_WHITE,"Apps");
        dpixel(mx,my,C_BLACK); dpixel(mx+1,my,C_BLACK); dpixel(mx,my+1,C_BLACK); dpixel(mx+1,my+1,C_BLACK);
        dupdate();
        key_event_t k = getkey();
        if (k.key==KEY_EXIT || k.key==KEY_F6) break;
        if (k.key==KEY_LEFT && mx>0) mx--; if (k.key==KEY_RIGHT && mx<127) mx++;
        if (k.key==KEY_UP && my>0) my--; if (k.key==KEY_DOWN && my<63) my++;
        if (k.key==KEY_EXE) {
            if (mx>=10 && mx<=40 && my>=20 && my<=40) { break; }
            if (mx>=60 && mx<=90 && my>=20 && my<=40) { break; }
        }
    }
}

// ===================================================================
//                        COMMAND EXECUTION
// ===================================================================

static int command_exec (char command_input[], list_t* scr, int x_scrl, int y_scrl) {
    if (strncmp(command_input,"\0",1) == 0) return 1;
    if (strcmp(command_input, "help") == 0) { help_(scr,0,x_scrl,y_scrl); }
    else if ((strcmp(command_input, "exit") == 0 || (strcmp(command_input, "shutdown") == 0))) {
        if (scr->count >= 5) y_scrl = -((scr->count-4)*10);
        int do_exit = exit_(scr,0,x_scrl,y_scrl); if (do_exit) return 0;
    }
    else if (strcmp(command_input, "clear") == 0) {
        clear(scr,0); x_scrl=0; y_scrl=0; scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input, "cat ",4) == 0) {
        strcpy(command_input, command_input+4);
        if (!fd_in_path(PWD,command_input,DT_REG)) { print_txt(scr, "File is not found!", x_scrl, y_scrl); return 1; }
        char CatPath[strlen(command_input) + strlen(PWD) + 4]; create_path(CatPath, PWD, command_input, 0);
        cat( CatPath ,scr,x_scrl,y_scrl); scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strcmp(command_input,"ls") == 0) { listdir(PWD,scr,x_scrl,y_scrl); }
    else if (strcmp(command_input,"uname") == 0) { uname_(scr,x_scrl,y_scrl); }
    else if (strcmp(command_input,"echo") == 0) { print_txt(scr,"",x_scrl,y_scrl); scroll_draw(scr,x_scrl,y_scrl); }
    else if (strncmp(command_input,"echo ",5) == 0) { echo_(command_input,scr,x_scrl,y_scrl); }
    else if (strcmp(command_input,"cd") == 0) {
        strcpy(PWD, "/"); char current_path[ strlen(PWD) + strlen("///fls0") + 2 ]; create_path(current_path, PWD, "",1); strrep(current_path,"\\","/"); print_txt(scr, current_path, x_scrl, y_scrl);
    }
    else if (strncmp(command_input,"cd ",3) == 0) {
        chdir_(PWD, command_input+3, scr, x_scrl, y_scrl); scroll_draw(scr, x_scrl, y_scrl);
        char current_path[ strlen(PWD) + strlen("///fls0") + 2 ]; create_path(current_path, PWD, "",1); strrep(current_path,"\\","/"); print_txt(scr, current_path, x_scrl, y_scrl);
    }
    else if (strcmp(command_input,"bash") == 0) {
        print_txt(scr,"Bash Script for FxLinux",x_scrl,y_scrl); print_txt(scr,"create .sh file , and write your fxlinux commands.",x_scrl,y_scrl); print_txt(scr,"./file.sh  or  bash  file.sh",x_scrl,y_scrl); scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input,"bash ",5) == 0) {
        strcpy(command_input, command_input+5);
        if (!fd_in_path(PWD,command_input,DT_REG)) { print_txt(scr, "File is not found!", x_scrl, y_scrl); return 1; }
        char BPath[strlen(command_input) + strlen(PWD) + 4]; create_path(BPath, PWD, command_input, 0);
        bash_exec( BPath ,scr,x_scrl,y_scrl); scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input,"./",2) == 0) {
        strcpy(command_input, command_input+2);
        if (!fd_in_path(PWD,command_input,DT_REG)) { print_txt(scr, "File is not found!", x_scrl, y_scrl); return 1; }
        char BPath[strlen(command_input) + strlen(PWD) + 4]; create_path(BPath, PWD, command_input, 0);
        bash_exec( BPath ,scr,x_scrl,y_scrl); scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input,"rm ",3) == 0) {
        strcpy(command_input, command_input+3);
        char RmPath[strlen(command_input) + strlen(PWD) + 4]; create_path(RmPath, PWD, command_input, 0);
        if (remove( RmPath ) != 0) { print_txt(scr, "Error removing file", x_scrl, y_scrl); }
        scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input,"rmdir ",6) == 0) {
        strcpy(command_input, command_input+6);
        char RmPath[strlen(command_input) + strlen(PWD) + 4]; create_path(RmPath, PWD, command_input, 0);
        if (rmdir( RmPath ) != 0) { print_txt(scr, "Error removing directory", x_scrl, y_scrl); }
        scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input,"sleep ",6) == 0) { float sec = atof(command_input+6) * 1000000; sleep_us_spin((int)sec); }
    else if (strcmp(command_input,"pwd") == 0) {
        char current_path[ strlen(PWD) + strlen("///fls0") + 2 ]; create_path(current_path, PWD, "",1); strrep(current_path,"\\","/"); print_txt(scr, current_path, x_scrl, y_scrl);
    }
    else if (strncmp(command_input, "touch ",6) == 0) {
        strcpy(command_input, command_input+6);
        char tPath[strlen(command_input) + strlen(PWD) + 4]; create_path(tPath, PWD, command_input, 0);
        FILE *file = fopen(tPath, "w"); if (file != NULL) fclose(file); else print_txt(scr, "Error creating file", x_scrl, y_scrl);
        scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input, "mkdir ",6) == 0) {
        strcpy(command_input, command_input+6);
        char dPath[strlen(command_input) + strlen(PWD) + 4]; create_path(dPath, PWD, command_input, 0);
        print_txt(scr, dPath, x_scrl, y_scrl);
        if (mkdir(dPath, 0777) != 0) { print_txt(scr, "Error creating directory", x_scrl, y_scrl); }
        scroll_draw(scr,x_scrl,y_scrl);
    }
    else if (strncmp(command_input, "nano ",5) == 0) {
        char* name = command_input+5; char nPath[strlen(name) + strlen(PWD) + 4];
        create_path(nPath, PWD, name, 0); nano_editor(nPath, name);
    }
    else if (strcmp(command_input, "nano") == 0) { print_txt(scr, "Usage: nano <file>", x_scrl, y_scrl); }
    else if (strcmp(command_input, "startx") == 0) { gui_loop(); }
    else {
        char* tok = strtok(command_input," ");
        char *words[] = {"help", "exit", "shutdown", "clear", "cat", "ls", "uname", "rm", "touch", "rmdir", "mkdir", "nano", "startx"};
        int found = 0; for (int i = 0; i < (int)(sizeof(words) / sizeof(words[0])); ++i) { if (strcmp(words[i], tok) == 0) { found = 1; break; } }
        if (found) {
            char msg [strlen(tok) + strlen("Invaild call for ''") + 1]; strcpy(msg, "Invaild call for '"); strcat(msg, tok); strcat(msg, "'"); print_txt(scr, msg, x_scrl, y_scrl);
        } else {
            char msg [strlen(tok) + strlen("Command '' not found!") + 1]; strcpy(msg, "Command '"); strcat(msg, tok); strcat(msg, "' not found!"); print_txt(scr, msg, x_scrl, y_scrl);
        }
    }
    return 1;
}

// ===================================================================
//                                MAIN
// ===================================================================

int main(void) {
    strcpy(PWD,"/");
    dclear(C_WHITE); dimage(1,1,&img_boot); dupdate(); sleep_us_spin(3000000);

    dclear(C_WHITE);
    struct List scr_obj; list_t* scr = &scr_obj; List_Init(scr,(size_t)SCREEN);
    print_txt(scr , "FxLinux 0.1.0",0,0);
    print_txt(scr , "Type \"help\" for Commands",0,0);
    print_txt(scr , ">",0,0);

    int x_scrl = 0; int y_scrl = 0;
    while (true){
        key_event_t key = getkey();
        if      (key.key == KEY_F1) { clear(scr,1); x_scrl=0; y_scrl=0; scroll_draw(scr,x_scrl,y_scrl); }
        else if (key.key == KEY_F3) { help_(scr,1,x_scrl,y_scrl); print_txt(scr,">",x_scrl,y_scrl); }
        else if (key.key == KEY_EXIT || key.key == KEY_F6) {
            if (scr->count >= 5) y_scrl = -((scr->count-4)*10);
            int do_exit = exit_(scr,1,x_scrl,y_scrl); if (do_exit) break; else print_txt(scr,">",x_scrl,y_scrl);
        }
        else if (key.key == KEY_UP) { if (y_scrl+2 <= 1) { y_scrl+=2; scroll_draw( scr , x_scrl , y_scrl);} }
        else if (key.key == KEY_DOWN) { y_scrl-=2; scroll_draw( scr , x_scrl , y_scrl); }
        else if (key.key == KEY_LEFT) { if (x_scrl < 0) { x_scrl+=2; scroll_draw( scr , x_scrl , y_scrl);} }
        else if (key.key == KEY_RIGHT) { x_scrl-=2; scroll_draw( scr , x_scrl , y_scrl); }
        else if (key.key == KEY_F2) { if (!F2_UPPERCASE) F2_UPPERCASE = 1; else F2_UPPERCASE = 0; }
        else if (key.key == KEY_DEL) { if (strlen(List_End(scr)) > 1) List_Delete_Last_Chr(scr); scroll_draw( scr , x_scrl , y_scrl); }
        else if (key.key == KEY_EXE) {
            char command_input[strlen(List_End(scr)) + 1]; strcpy(command_input, List_End(scr)+1);
            int continue_sys = command_exec( command_input, scr, x_scrl, y_scrl); if (!continue_sys) break; print_txt(scr,">",x_scrl,y_scrl);
        }
        else {
            char* c = button_reader(key); char* last_line = List_End(scr);
            char result[strlen(c) + strlen(last_line) + 1]; strcpy(result, last_line); strcat(result, c);
            List_Delete_Last(scr); List_Push(scr,result); scroll_draw( scr , x_scrl , y_scrl);
        }
    }
    List_Destroy(scr);
    return 1;
}
