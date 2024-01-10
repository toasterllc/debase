BUILDDIRS = $(addsuffix /$(BUILDDIR), $(LIBS))
BUILDDIR = build-$(PLATFORM)
LIBS = libgit2 ncurses
ARCHS ?= x86_64 arm64

ifeq ($(shell uname -s), Darwin)
	PLATFORM = mac
else
	PLATFORM = linux
endif

# LIBGIT2.BUILDDIR = $(LIBGIT2)/$(BUILDDIR)
# NCURSES.BUILDDIR = $(NCURSES)/$(BUILDDIR)

# LIBGIT2.TMP = $(LIBGIT2).tmp
# NCURSES.TMP = $(NCURSES).tmp

all: $(addsuffix /$(BUILDDIR), $(LIBS))

# all: libgit2/build-Mac
# all: ncurses/build-Mac
# all: $(addsuffix /$(BUILDDIR), $(LIBS))
# all: $(LIBGIT2) $(NCURSES)

libgit2/%:
	$(eval TMP = $@.tmp)
	
	mkdir -p $(TMP)
	
ifeq ($(PLATFORM), mac)
	$(eval PLATFORMFLAGS =													\
		-DCMAKE_OSX_ARCHITECTURES="$(shell echo $(ARCHS) | sed 's/ /;/')"	\
		-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15									\
	)
endif
	
	cd $(TMP) && cmake														\
		$(PLATFORMFLAGS)													\
		-DCMAKE_BUILD_TYPE=Release											\
		-DBUILD_SHARED_LIBS=OFF												\
		-DUSE_HTTPS=OFF ..
	
	$(MAKE) -C $(TMP) clean
	$(MAKE) -C $(TMP) git2
	
	mv $(TMP) $@

ncurses/%:
	$(eval DIR = $(dir $@))
	
ifeq ($(PLATFORM), mac)
	$(eval PLATFORMFLAGS =													\
		CFLAGS="-mmacosx-version-min=10.15 $(addprefix -arch , $(ARCHS))"	\
	)
endif
	
	cd $(DIR) && ./configure												\
		$(PLATFORMFLAGS)													\
		--prefix=/usr														\
		--enable-widec														\
		--without-cxx-binding 	# ncurses c++ bindings fail to build when
								# host=arm64 & target=x86_64, but we use
								# them them anyway.
	
	$(MAKE) -C $(DIR) clean
	$(MAKE) -C $(DIR)
	
	mv $(DIR)/lib $@
	cp -R $(DIR)/include $@/include

# Clean: execute `make clean` for each lib, and then delete $(BUILDDIRS)
CLEANLIBS := $(addsuffix .clean, $(LIBS))

clean: $(CLEANLIBS)
	rm -Rf $(BUILDDIRS)

$(CLEANLIBS):
	$(MAKE) -C $(basename $@) clean || true

# clean:
# 	$(foreach lib, $(LIBS), $(MAKE) -C $(lib) clean || echo hello)
# 	rm -Rf $(BUILDDIRS)
