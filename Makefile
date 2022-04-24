NAME = debase
DEBUG ?= 0
ARCHS ?= x86_64 arm64

ifeq ($(shell uname -s), Darwin)
	PLATFORM = mac
else
	PLATFORM = linux
endif

BUILDROOT = build-$(PLATFORM)
BUILDDIR = $(BUILDROOT)/release
ifeq ($(DEBUG), 1)
	BUILDDIR = $(BUILDROOT)/debug
endif

GITHASHHEADER = src/DebaseGitHash.h

SRCS =											\
	lib/c25519/src/sha512.c						\
	lib/c25519/src/edsign.c						\
	lib/c25519/src/ed25519.c					\
	lib/c25519/src/fprime.c						\
	lib/c25519/src/f25519.c						\
	src/OpenURL-$(PLATFORM).*					\
	src/ProcessPath-$(PLATFORM).*				\
	src/machine/Machine-$(PLATFORM).*			\
	src/state/StateDir-$(PLATFORM).*			\
	src/ui/View.cpp								\
	src/main.cpp

# Using CPPFLAGS for the common flags between C/C++.
# We can't just include CFLAGS in the definition of
# CXXFLAGS because GCC complains when supplying
# -std=c11 to C++ files.
CPPFLAGS =										\
	$(INCDIRS)									\
	$(OPTFLAGS)									\
	-g											\
	-fvisibility=hidden							\
	-fstrict-aliasing							\
	-fno-common									\
	-MMD										\
	-MP											\
	-Wall										\

CFLAGS =										\
	-std=c11									\

CXXFLAGS =										\
	-std=c++17									\
	-fvisibility-inlines-hidden

OBJCXXFLAGS =									\
	-fobjc-arc									\
	-fobjc-weak

OPTFLAGS = -Os
ifeq ($(DEBUG), 1)
	OPTFLAGS = -O0 -g3
endif

INCDIRS =										\
	-isystem ./lib/ncurses/include				\
	-iquote ./lib/libgit2/include				\
	-iquote ./src								\
	-iquote .

LIBDIRS =										\
	-L./lib/libgit2/build-$(PLATFORM)			\
	-L./lib/libcurl/build-$(PLATFORM)			\
	-L./lib/ncurses/build-$(PLATFORM)

LIBS =											\
	-lgit2										\
	-lz											\
	-lcurl										\
	-lpthread									\
	-lformw										\
	-lmenuw										\
	-lpanelw									\
	-lncursesw

ifeq ($(PLATFORM), mac)
	CPPFLAGS +=									\
		$(addprefix -arch , $(ARCHS))			\
		-mmacosx-version-min=10.15				\
		-fdiagnostics-show-note-include-stack
	
	LIBS +=										\
		-framework Foundation					\
		-framework IOKit						\
		-framework CoreServices					\
		-framework Security						\
		-framework SystemConfiguration			\
		-liconv

else ifeq ($(PLATFORM), linux)
	LIBS +=										\
		-lssl									\
		-lcrypto
endif

OBJS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRCS))))

$(NAME): $(BUILDDIR)/$(NAME)

# Objects depend on libs being built first
# We explicitly depend on GITHASHHEADER too, for the initial build where
# our .d dependency files don't exist, so make doesn't know what depends on
$(OBJS): | lib $(GITHASHHEADER)

# Libs: execute make from `lib` directory
.PHONY: lib
lib:
	$(MAKE) -C $@

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

# Link, generate dsym, and strip
$(BUILDDIR)/$(NAME): $(OBJS)
	$(LINK.cc) $^ -o $@ $(LIBDIRS) $(LIBS)
ifeq ($(PLATFORM), mac)
	xcrun dsymutil $@ -o $@.dSYM
endif
ifeq ($(DEBUG), 0)
	strip $@
endif

# Output the git HEAD hash to $(GITHASHHEADER)
$(GITHASHHEADER): .git/HEAD .git/index
	echo '#pragma once' > $@
	echo '#define _DebaseGitHash "$(shell git rev-parse HEAD)"' >> $@
	echo '#define _DebaseGitHashShort "$(shell git rev-parse HEAD | cut -c -10)"' >> $@

codesign: $(BUILDDIR)/$(NAME)
	codesign														\
		-vvvv														\
		--force														\
		--timestamp													\
		--options=runtime											\
		-s 'Developer ID Application: Toaster LLC (5VXGM37B6Z)'		\
		$<

notarize: codesign
	cd $(BUILDDIR) && zip $(NAME).zip $(NAME)
	
	@echo "Uploading binary to be notarized..."
	
	xcrun altool									\
		 --notarize-app								\
		 --primary-bundle-id com.heytoaster.debase	\
		 --username 'apple@heytoaster.com'			\
		 --password '@keychain:altool'				\
		 --file $(BUILDDIR)/$(NAME).zip
	
	rm $(BUILDDIR)/$(NAME).zip
	
	xcrun altool									\
		--notarization-history 0					\
		-p @keychain:altool

clean:
	$(MAKE) -C lib clean
	rm -Rf $(BUILDROOT)

# Include all .d files
-include $(OBJS:%.o=%.d)
