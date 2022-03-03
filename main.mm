#include <iostream>
#include <unistd.h>
#include "ncurses/c++/cursesp.h"

int main(int argc, const char* argv[]) {
//    pause();
//    pause();
    
    // There doesn't seem to be a better way to do this
    setenv("TERM", "xterm-1003", true);
    ::endwin();
    
//    wresize(stdscr, 300, 300);
//    box(stdscr, 0, 0);
//    mvwprintw(stdscr, 10, 10, "goodbye");
    
    NCursesWindow win(::stdscr);
    win.wresize(300, 300);
    win.box();
//    win.scrollok(true);
//    win.scroll(10);
    win.printw(10, 5, "goodbye");
    
//    NCursesWindow win(::stdscr);
    NCursesPanel panel(5, 10);
    panel.printw(1, 1, "hello");
    panel.box();
//    panel.scrollok(true);
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
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
                panel.mvwin(mouseEvent.y+3, mouseEvent.x-3);
            } catch (...) {}
        }
    }
    return 0;
}
