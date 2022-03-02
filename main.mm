#include <iostream>
#include <unistd.h>
#include "ncurses/c++/cursesp.h"

int main(int argc, const char* argv[]) {
    // There doesn't seem to be a better way to do this
    setenv("TERM", "xterm-1003", true);
    ::endwin();
    
    NCursesWindow win(::stdscr);
    NCursesPanel panel(5, 10);
    panel.printw(1, 1, "hello");
    panel.box();
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    
    for (;;) {
        NCursesPanel::redraw();
        int key = win.getch();
        if (key == KEY_MOUSE) {
            MEVENT mouseEvent = {};
            int ir = getmouse(&mouseEvent);
            if (ir != OK) continue;
            panel.mvwin(mouseEvent.y, mouseEvent.x);
        }
    }
    return 0;
}
