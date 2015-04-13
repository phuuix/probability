# Top level Makefile
# Usually you should invoke like this way: make BSP=i386, etc

PROJ_ROOTDIR = .

include $(PROJ_ROOTDIR)/config.mk

SUBDIRS = libc kernel tty net hue bsp/$(BSP)

all:
	for tdir in $(SUBDIRS) ; do make -C $$tdir all ; done

dep:
	for tdir in $(SUBDIRS) ; do make -C $$tdir dep ; done

clean:
	for tdir in $(SUBDIRS) ; do make -C $$tdir clean ; done
