NAME = debase
DEBUG ?= 0

BUILDDIR = build-$(PLATFORM)/release
ifeq ($(DEBUG), 1)
    BUILDDIR = build-$(PLATFORM)/debug
endif

SRCS =                                      \
	lib/c25519/src/sha512.c                 \
	lib/c25519/src/edsign.c                 \
	lib/c25519/src/ed25519.c                \
	lib/c25519/src/fprime.c                 \
	lib/c25519/src/f25519.c                 \
	src/OpenURL-$(PLATFORM).*               \
	src/ProcessPath-$(PLATFORM).*           \
	src/machine/Machine-$(PLATFORM).*       \
	src/state/StateDir-$(PLATFORM).*        \
	src/ui/View.cpp                         \
	src/main.cpp

CFLAGS =                                    \
	$(INCDIRS)                              \
	$(OPTFLAGS)                             \
	-Wall                                   \
	-fvisibility=hidden                     \
	-fvisibility-inlines-hidden             \
	-fstrict-aliasing                       \
	-fdiagnostics-show-note-include-stack   \
	-fno-common                             \
	-arch x86_64                            \
	-std=c11

CXXFLAGS = $(CFLAGS) -std=c++17

OBJCXXFLAGS = $(CXXFLAGS)

OPTFLAGS = -Os
ifeq ($(DEBUG), 1)
    OPTFLAGS = -O0 -g3
endif

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

ifeq ($(shell uname -s), Darwin)
	PLATFORM = Mac
else
	PLATFORM = Linux
endif

ifeq ($(PLATFORM), Mac)
	CFLAGS +=                           \
		-mmacosx-version-min=10.15      \
		-arch arm64
	
	OBJCXXFLAGS +=                      \
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

# OBJS = $(addsuffix .o, $(basename $(SRCS)))

OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRCS))))

# OBJDIRS = $(dir $(OBJS))
# OBJDIRS = build/lib/c25519/src build/src build/src/machine build/src/state build/src/ui

# $(BUILDDIR)/$(NAME): $(OBJS)
# 	$(LINK.cc) $? -o $@ $(LIBDIRS) $(LIBS)
# 	strip $@

# $(BUILDDIR)/%.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@
#
# $(BUILDDIR)/%.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
#
# $(BUILDDIR)/%.o: %.mm
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# release: BUILDDIR = build-$(PLATFORM)/release
# release: $(BUILDDIR)/$(NAME)
#
# debug: BUILDDIR = build-$(PLATFORM)/debug
# debug: $(BUILDDIR)/$(NAME)

# release: BUILDDIR := build/release
# release: $(BUILDDIR)/$(NAME)

$(BUILDDIR)/$(NAME): $(OBJS)
	$(LINK.cc) $? -o $@ $(LIBDIRS) $(LIBS)
ifeq ($(DEBUG), 0)
	strip $@
endif

$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(COMPILE.c) $< -o $@

$(BUILDDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(COMPILE.cc) $< -o $@

$(BUILDDIR)/%.o: %.mm
	mkdir -p $(dir $@)
	$(COMPILE.cc) $(OBJCXXFLAGS) $< -o $@

# ifeq ($(shell uname -s), Darwin)
# 	@echo HELLO
# else
# 	@echo GOODBYE
# endif

# %.o: $(BUILDDIR)/%.o
# 	mkdir -p $(dir $(BUILDDIR)/$@)
# 
# $(OBJDIRS):
# 	mkdir -p $@
























# NAME = debase
# BUILDDIR = build
#
# SRCS =                                      \
# 	lib/c25519/src/sha512.c                 \
# 	lib/c25519/src/edsign.c                 \
# 	lib/c25519/src/ed25519.c                \
# 	lib/c25519/src/fprime.c                 \
# 	lib/c25519/src/f25519.c                 \
# 	src/OpenURL-$(PLATFORM).mm              \
# 	src/ProcessPath-$(PLATFORM).mm          \
# 	src/machine/Machine-$(PLATFORM).mm      \
# 	src/state/StateDir-$(PLATFORM).mm       \
# 	src/ui/View.cpp                         \
# 	src/main.cpp
#
# CFLAGS =                                    \
# 	$(INCDIRS)                              \
# 	$(COPT)                                 \
# 	-Wall                                   \
# 	-fvisibility=hidden                     \
# 	-fvisibility-inlines-hidden             \
# 	-fstrict-aliasing                       \
# 	-fdiagnostics-show-note-include-stack   \
# 	-fno-common                             \
# 	-arch x86_64                            \
# 	-std=c11
#
# CPPFLAGS = $(CFLAGS) -std=c++17
#
# INCDIRS =                               \
# 	-isystem ./lib/ncurses/include      \
# 	-iquote ./lib/libgit2/include       \
# 	-iquote ./src                       \
# 	-iquote .
#
# LIBDIRS =                               \
# 	-L./lib/libgit2/build-$(PLATFORM)   \
# 	-L./lib/libcurl/build/lib           \
# 	-L./lib/ncurses/lib
#
# LIBS =                                  \
# 	-lgit2                              \
# 	-lz                                 \
# 	-lcurl                              \
# 	-lpthread                           \
# 	-lformw                             \
# 	-lmenuw                             \
# 	-lpanelw                            \
# 	-lncursesw
#
# OBJS = $(addsuffix .o, $(basename $(SRCS)))
#
# # OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRCS))))
# # OBJDIRS = $(dir $(OBJS))
# # OBJDIRS = build/lib/c25519/src build/src build/src/machine build/src/state build/src/ui
#
# ifeq ($(shell uname -s), Darwin)
# 	PLATFORM = Mac
# else
# 	PLATFORM = Linux
# endif
#
# $(NAME): $(BUILDDIR)/$(NAME)
#
# # $(BUILDDIR)/%.o: %.c
# # 	$(CC) $(CFLAGS) -c $< -o $@
# #
# # $(BUILDDIR)/%.o: %.cpp
# # 	$(CXX) $(CPPFLAGS) -c $< -o $@
# #
# # $(BUILDDIR)/%.o: %.mm
# # 	$(CXX) $(CPPFLAGS) -c $< -o $@
#
#
#
# $(BUILDDIR)/%.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@
#
# $(BUILDDIR)/%.o: %.cpp
# 	$(CXX) $(CPPFLAGS) -c $< -o $@
#
# $(BUILDDIR)/%.o: %.mm
# 	$(CXX) $(CPPFLAGS) -c $< -o $@
#
# %.o: $(BUILDDIR)/%.o
# 	mkdir -p $(dir $(BUILDDIR)/$@)
#
# # $(OBJDIRS):
# # 	mkdir -p $@
#
# $(BUILDDIR)/$(NAME): $(OBJS)
# 	$(CC) $^ -o $@
# 	strip $@















# BUILDDIR := build
#
# PROGS = debase
# SRCS = src/main.cpp
# OBJS := $(addprefix $(BUILDDIR)/,$(patsubst %.c,%.o,$(SRCS)))
#
#
# OBJECTS = $(addsuffix .o, $(basename $(SRC)))
#
# # OBJECTS = src/main.o
# # Shouldn't need to change anything below here...
#
# OBJDIR = obj
#
# # VPATH = $(OBJDIR)
#
# $(OBJDIR)/%.o : %.c
# 	$(COMPILE.c) $< -o $@
#
# $(OBJDIR)/%.o : %.cpp
# 	$(COMPILE.cpp) $< -o $@
#
# OBJPROG = $(addprefix $(OBJDIR)/, $(PROGS))
#
# all: $(OBJPROG)
#
# # $(OBJDIR):
# # 	mkdir -p $(OBJDIR)
#
# # $(OBJECTS):
# # 	mkdir -p $@
#
# $(OBJPROG): $(addprefix $(OBJDIR)/, $(OBJECTS))
# 	# echo hello
# 	# @mkdir -p $(@D)
# 	$(LINK.o) $^ $(LDLIBS) -o $@
