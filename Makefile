ifeq ($(shell uname -s), Darwin)
	PLATFORM := Mac
else
	PLATFORM := Linux
endif

NAME = debase

CC = gcc
CXX = g++
STRIP = strip

CFLAGS =                                    \
	$(INCDIRS)                              \
	$(COPT)                                 \
	-Wall                                   \
	-fvisibility=hidden                     \
	-fvisibility-inlines-hidden             \
	-fstrict-aliasing                       \
	-fdiagnostics-show-note-include-stack   \
	-fno-common                             \
	-arch x86_64                            \
	-std=c11                                \

CPPFLAGS = $(CFLAGS) -std=c++17
MMFLAGS = $(CPPFLAGS)

COPT = -Os

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

LIBS =                                  \
	-lgit2                              \
	-lz                                 \
	-lcurl                              \
	-lpthread                           \
	-lformw                             \
	-lmenuw                             \
	-lpanelw                            \
	-lncursesw

ifeq ($(PLATFORM), Mac)
	CFLAGS +=                           \
		-mmacosx-version-min=10.15      \
		-arch arm64
	
	MMFLAGS +=                          \
		-fobjc-arc                      \
		-fobjc-weak
	
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

release: link
release: strip

debug: COPT = -Og -g3   # Set optimization flags for debugging
debug: link

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

%.o: %.mm
	$(CXX) $(MMFLAGS) -c $< -o $@

link: ${OBJECTS}
	$(CXX) $(CPPFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS)

strip: link
	$(STRIP) $(NAME)

clean:
	rm -f *.o $(OBJECTS) $(NAME)
