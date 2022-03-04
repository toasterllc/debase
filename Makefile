NAME=CursesTest
OBJECTS=main.o

CXX      = g++
CXXFLAGS = -std=c++17 -O0 -g3 -Wall -Weffc++ $(IDIRS)
LFLAGS   = -L./ncurses/lib -lncurses++ -lform -lmenu -lpanel -lncurses
IDIRS    = -isystem ./ncurses/include   \
           -isystem ./ncurses/c++

all: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LFLAGS)

clean:
	rm -Rf *.o $(NAME)
