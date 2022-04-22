ifeq ($(shell uname -s), Darwin)
	PLATFORM := Mac
else
	PLATFORM := Linux
endif







BUILD := debug
BUILDIR := $(CURDIR)/$(BUILD)



NAME = debase

# CC = gcc
# CXX = g++
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

SOURCE =                                \
	lib/c25519/src/sha512.c             \
	lib/c25519/src/edsign.c             \
	lib/c25519/src/ed25519.c            \
	lib/c25519/src/fprime.c             \
	lib/c25519/src/f25519.c             \
	src/OpenURL-$(PLATFORM).mm          \
	src/ProcessPath-$(PLATFORM).mm      \
	src/machine/Machine-$(PLATFORM).mm  \
	src/state/StateDir-$(PLATFORM).mm   \
	src/ui/View.cpp                     \
	src/main.cpp

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

debug: COPT = -Og -g3	# Set optimization flags for debugging
debug: link

$(BUILDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BUILDIR)/%.o: %.mm
	$(CXX) $(MMFLAGS) -c $< -o $@

$(BUILDIR):
	mkdir $@

link: $(OBJECTS)
	$(CXX) $(CPPFLAGS) $? -o $(NAME) $(LIBDIRS) $(LIBS) $(LDFLAGS)

strip: link
	$(STRIP) $(NAME)

clean:
	rm -f *.o $(OBJECTS) $(NAME)
