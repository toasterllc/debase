NAME=debase

CC = gcc
CXX = g++

CFLAGS = $(INCDIRS) -Wall -std=c11
CXXFLAGS = $(CFLAGS) -std=c++17
COPT = -Os

ifeq ($(shell uname -s), Darwin)
	PLATFORM := Mac
else
	PLATFORM := Linux
endif

LDFLAGS = $(LDSTRIP)

ifeq ($(PLATFORM), Mac)
	LDSTRIP = -Wl,-x
else ifeq ($(PLATFORM), Linux)
	LDSTRIP = -s
endif

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

release: link

debug: COPT = -Og -g3	# Set optimization flags for debugging
debug: LDSTRIP = 		# Don't strip binary
debug: link

link: ${OBJECTS}
	$(CXX) $(CXXFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.mm
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(OBJECTS) $(NAME)
