NAME=debase
OBJECTS=                            \
    lib/c25519/src/sha512.o         \
    lib/c25519/src/edsign.o         \
    lib/c25519/src/ed25519.o        \
    lib/c25519/src/fprime.o         \
    src/OpenURL-Linux.o             \
    src/ProcessPath-Linux.o         \
    src/machine/Machine-Linux.o     \
    src/state/StateDir-Linux.o      \
    src/ui/View.o                   \
    src/main.o

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
