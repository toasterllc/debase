NAME=debase

CXX = g++

CXXFLAGS = -std=c++17 -Wall $(INCDIRS)

LDFLAGS =                           \

OBJECTS =                           \
	lib/c25519/src/sha512.o         \
	lib/c25519/src/edsign.o         \
	lib/c25519/src/ed25519.o        \
	lib/c25519/src/fprime.o         \
	lib/c25519/src/f25519.o         \
	src/OpenURL-Linux.o             \
	src/ProcessPath-Linux.o         \
	src/machine/Machine-Linux.o     \
	src/state/StateDir-Linux.o      \
	src/ui/View.o                   \
	src/main.o                      \

INCDIRS =                           \
	-isystem ./lib/ncurses/include  \
	-iquote ./lib/libgit2/include   \
	-iquote ./src                   \
	-iquote .                       \

LIBDIRS =                           \
	-L./lib/libgit2/build-linux     \
	-L./lib/libcurl/build/lib       \
	-L./lib/ncurses/lib             \

LIBS =                              \
	-lgit2                          \
	-lz                             \
	-lcurl                          \
	-lssl                           \
	-lcrypto                        \
	-lpthread                       \
	-lformw                         \
	-lmenuw                         \
	-lpanelw                        \
	-lncursesw                      \

release: CXXFLAGS += -Os
release: LDFLAGS += -s
release: bin

debug: CXXFLAGS += -Og -g3
debug: bin

bin: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS)

clean:
	rm -f *.o $(OBJECTS) $(NAME)
