#include <iostream>
#include <unistd.h>
#include "ncurses/c++/cursesp.h"

int main(int argc, const char* argv[]) {
//    pause();
//    pause();
    
    setenv("TERM_KMOUS", "\x1b[<", true);
    setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
    
//    wresize(stdscr, 300, 300);
//    box(stdscr, 0, 0);
//    mvwprintw(stdscr, 10, 10, "goodbye");
    
    NCursesWindow win(::stdscr);
//    win.wresize(300, 300);
//    win.scrollok(true);
//    win.scroll(10);
    
    FILE* debug = fopen("/Users/dave/Desktop/debug", "w");
    setvbuf(debug, NULL, _IOLBF, 0);
    
//    NCursesWindow win(::stdscr);
    NCursesPanel panel(5, 20);
    panel.box();
    panel.printw(0, 2, " hello ");
//    panel.scrollok(true);
    
    curs_set(0);
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
//        win.refresh();
        NCursesPanel::redraw();
//        panel.scroll(1);
//        win.scroll(1);
        
//        NCursesPanel::redraw();
        int key = win.getch();
        if (key == KEY_MOUSE) {
            MEVENT mouseEvent = {};
            int ir = getmouse(&mouseEvent);
            if (ir != OK) continue;
            try {
                fprintf(debug, "event: %d %d %x\n", mouseEvent.x, mouseEvent.y, mouseEvent.bstate);
                panel.mvwin(mouseEvent.y, mouseEvent.x);
            } catch (...) {}
        } else if (key == KEY_RESIZE) {
            win.erase();
            win.printw(10, 5, "goodbye");
            win.box();
        }
    }
    return 0;
}
