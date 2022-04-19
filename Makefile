NAME=debase
OBJECTS=src/main.o

CXX      = g++
CXXFLAGS = -std=c++17 -O0 -g3 -Wall $(IDIRS)
LFLAGS   = -L./lib/ncurses/lib -L./lib/libgit2/build-linux -lgit2 -lpthread -lz -lform -lmenu -lpanel -lncurses
IDIRS    = -isystem ./lib/ncurses/include \
           -isystem ./lib/libgit2/include \
           -iquote ./src                  \
           -iquote .

all: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LFLAGS)

clean:
	rm -Rf *.o $(NAME)
