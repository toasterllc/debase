NAME = debase
DEBUG ?= 0

ifeq ($(shell uname -s), Darwin)
	PLATFORM = mac
else
	PLATFORM = linux
endif

BUILDDIR = build-$(PLATFORM)/release
ifeq ($(DEBUG), 1)
    BUILDDIR = build-$(PLATFORM)/debug
endif

SRCS =										\
	lib/c25519/src/sha512.c					\
	lib/c25519/src/edsign.c					\
	lib/c25519/src/ed25519.c				\
	lib/c25519/src/fprime.c					\
	lib/c25519/src/f25519.c					\
	src/OpenURL-$(PLATFORM).*				\
	src/ProcessPath-$(PLATFORM).*			\
	src/machine/Machine-$(PLATFORM).*		\
	src/state/StateDir-$(PLATFORM).*		\
	src/ui/View.cpp							\
	src/main.cpp

CFLAGS =									\
	$(INCDIRS)								\
	$(OPTFLAGS)								\
	-Wall									\
	-fvisibility=hidden						\
	-fvisibility-inlines-hidden				\
	-fstrict-aliasing						\
	-fdiagnostics-show-note-include-stack	\
	-fno-common								\
	-arch x86_64							\
	-std=c11

CXXFLAGS = $(CFLAGS) -std=c++17

OBJCXXFLAGS = $(CXXFLAGS)

OPTFLAGS = -Os
ifeq ($(DEBUG), 1)
    OPTFLAGS = -O0 -g3
endif

INCDIRS =									\
	-isystem ./lib/ncurses/include			\
	-iquote ./lib/libgit2/include			\
	-iquote ./src							\
	-iquote .

LIBDIRS =									\
	-L./lib/libgit2/build-$(PLATFORM)		\
	-L./lib/libcurl/build-$(PLATFORM)		\
	-L./lib/ncurses/build-$(PLATFORM)

LIBS =										\
	-lgit2									\
	-lz										\
	-lcurl									\
	-lpthread								\
	-lformw									\
	-lmenuw									\
	-lpanelw								\
	-lncursesw

ifeq ($(PLATFORM), Mac)
	CFLAGS +=								\
		-mmacosx-version-min=10.15			\
		-arch arm64
	
	OBJCXXFLAGS +=							\
		-fobjc-arc							\
		-fobjc-weak
	
	LIBS +=									\
		-framework Foundation				\
		-framework IOKit					\
		-framework CoreServices				\
		-framework Security					\
		-framework SystemConfiguration		\
		-liconv

else ifeq ($(PLATFORM), Linux)
	LIBS +=									\
		-lssl								\
		-lcrypto
endif

OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRCS))))

# Link
$(BUILDDIR)/$(NAME): $(OBJS)
	$(LINK.cc) $? -o $@ $(LIBDIRS) $(LIBS)
ifeq ($(DEBUG), 0)
	strip $@
endif

# C rule
$(BUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(COMPILE.c) $< -o $@

# C++ rule
$(BUILDDIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(COMPILE.cc) $< -o $@

# Objective-C++ rule
$(BUILDDIR)/%.o: %.mm
	mkdir -p $(dir $@)
	$(COMPILE.cc) $(OBJCXXFLAGS) $< -o $@
