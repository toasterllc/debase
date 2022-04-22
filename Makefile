NAME=debase

CXX = g++ -v

CXXFLAGS = -std=c++17 -Wall $(INCDIRS) -v

ifeq ($(shell uname -s), Darwin)
	PLATFORM := Mac
else
	PLATFORM := Linux
endif

LDFLAGS =

OBJECTS =                               \
	lib/c25519/src/sha512.o             \
	lib/c25519/src/edsign.o             \
	lib/c25519/src/ed25519.o            \
	lib/c25519/src/fprime.o             \
	lib/c25519/src/f25519.o             \
	src/OpenURL-$(PLATFORM).o           \
	src/ProcessPath-$(PLATFORM).o       \
	src/machine/Machine-$(PLATFORM).o   \
	src/state/StateDir-$(PLATFORM).o    \
	src/ui/View.o                       \
	src/main.o

INCDIRS =                               \
	-isystem ./lib/ncurses/include      \
	-iquote ./lib/libgit2/include       \
	-iquote ./src                       \
	-iquote .

LIBDIRS =                               \
	-L./lib/libgit2/build-$(PLATFORM)   \
	-L./lib/libcurl/build/lib           \
	-L./lib/ncurses/lib

LIBS =

ifeq ($(PLATFORM), Mac)
	LIBS +=                             \
		-framework Foundation           \
		-framework IOKit                \
		-framework CoreServices         \
		-framework Security             \
		-framework SystemConfiguration  \
		-liconv

else ifeq ($(PLATFORM), Linux)
	LIBS +=                             \
		-lssl                           \
		-lcrypto
endif

LIBS +=                                 \
	-lgit2                              \
	-lz                                 \
	-lcurl                              \
	-lpthread                           \
	-lformw                             \
	-lmenuw                             \
	-lpanelw                            \
	-lncursesw

release:
	CXXFLAGS += -Os
	LDFLAGS += -s
release: bin

debug:
	CXXFLAGS += -Og -g3
debug: bin

bin: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS) -v

clean:
	rm -f *.o $(OBJECTS) $(NAME)


.SUFFIXES: .mm
.mm.o:
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS) -v
