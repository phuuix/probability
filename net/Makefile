
SUBDIRS = lwip-1.5beta/src

all:
	for tdir in $(SUBDIRS) ; do make -C $$tdir all ; done

dep:
	for tdir in $(SUBDIRS) ; do make -C $$tdir dep ; done

clean:
	for tdir in $(SUBDIRS) ; do make -C $$tdir clean ; done
