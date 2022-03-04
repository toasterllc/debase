#include <iostream>
#include <unistd.h>
#include "lib/ncurses/c++/cursesp.h"
#include "xterm-256color.h"

int main(int argc, const char* argv[]) {
    nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
    
    // Override the terminfo 'kmous' and 'XM' properties
    //   kmous = the prefix used to detect/parse mouse events
    //   XM    = the escape string used to enable mouse events (1006=SGR 1006 mouse
    //           event mode; 1003=report mouse-moved events in addition to clicks)
    setenv("TERM_KMOUS", "\x1b[<", true);
    setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
    
    NCursesWindow win(::stdscr);
    
    // Hide cursor
    curs_set(0);
    
    NCursesPanel panel(5, 20);
    panel.box();
    panel.printw(0, 2, " hello ");
//    panel.scrollok(true);
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
        NCursesPanel::redraw();
        int key = win.getch();
        if (key == KEY_MOUSE) {
            MEVENT mouseEvent = {};
            int ir = getmouse(&mouseEvent);
            if (ir != OK) continue;
            try {
//                fprintf(debug, "event: %d %d %x\n", mouseEvent.x, mouseEvent.y, mouseEvent.bstate);
                panel.mvwin(mouseEvent.y, mouseEvent.x);
            } catch (...) {}
        
        } else if (key == KEY_RESIZE) {
//            win.erase();
//            win.printw(10, 5, "goodbye");
//            win.box();
        }
    }
    return 0;
}
