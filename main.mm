#include <iostream>
#include "curses.h"
#include "ncurses/c++/cursesapp.h"
#include "ncurses/c++/cursslk.h"
#include "ncurses/c++/cursesm.h"
#include "ncurses/c++/cursesf.h"

int main(int argc, const char* argv[]) {
    
//    setlocale(LC_ALL, "");
    
    NCursesApplication* A = NCursesApplication::getApplication();
    int res;
    
    A->handleArgs(argc, (char**)argv);
    ::endwin();
    try {
        bool bColors = A->b_Colors;
        Soft_Label_Key_Set* S = 0;

        int ts = A->titlesize();
        if (ts>0) NCursesWindow::ripoffline(ts,A->rinit);
        Soft_Label_Key_Set::Label_Layout fmt = A->useSLKs();
        
        if (fmt!=Soft_Label_Key_Set::None) {
            S = new Soft_Label_Key_Set(fmt);
            assert(S != 0);
            A->init_labels(*S);
        }

        A->Root_Window = new NCursesWindow(::stdscr);
        A->init(bColors);

        if (ts>0) A->title();
        if (fmt!=Soft_Label_Key_Set::None) {
            A->push(*S);
        }
        
        A->run();
        
        ::endwin();
    } catch(const NCursesException& e) {
        ::endwin();
        std::cerr << e.message << std::endl;
        res = e.errorno;
    }
    
    delete A;
    exit_curses(res);
    return(res);
    
    
    
    
//    bool bColors = b_Colors;
//    Soft_Label_Key_Set* S = 0;
//
//    int ts = titlesize();
//    if (ts>0) NCursesWindow::ripoffline(ts,rinit);
//    Soft_Label_Key_Set::Label_Layout fmt = useSLKs();
//    if (fmt!=Soft_Label_Key_Set::None) {
//        S = new Soft_Label_Key_Set(fmt);
//        assert(S != 0);
//        init_labels(*S);
//    }
//
//    Root_Window = new NCursesWindow(::stdscr);
//    init(bColors);
//
//    if (ts>0) title();
//    
//    if (fmt!=Soft_Label_Key_Set::None) {
//        push(*S);
//    }
//    
//    return run();
    
    
    
    return 0;
}
