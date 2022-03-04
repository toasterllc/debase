NAME=CursesTest
OBJECTS=main.o

CXX      = g++
CXXFLAGS = -std=c++17 -O0 -g3 -Wall -Weffc++ $(IDIRS)
LFLAGS   = -L./lib/ncurses/lib -lform -lmenu -lpanel -lncurses
IDIRS    = -isystem ./lib/ncurses/include \
           -isystem ./lib/libgit2/include

all: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LFLAGS)

clean:
	rm -Rf *.o $(NAME)
