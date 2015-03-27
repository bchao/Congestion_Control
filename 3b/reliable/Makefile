

# Debug malloc support (http://dmalloc.com).  Comment out if you don't
# have dmalloc, but it is highly recommended.
#
#DMALLOC_CFLAGS = -I/usr/local/include -DDMALLOC=1
#DMALLOC_LIBS = -L/usr/local/lib -ldmalloc
#
# On Stanford machines, you need these paths for dmalloc:
#
#DMALLOC_CFLAGS = -I/afs/ir/class/cs144/dmalloc -DDMALLOC=1
#DMALLOC_LIBS = -L/afs/ir/class/cs144/dmalloc -ldmalloc

LIBRT = `test -f /usr/lib/librt.a && printf -- -lrt`

CC = gcc
CFLAGS = -g -Wall -Werror $(DMALLOC_CFLAGS)
LIBS = $(DMALLOC_LIBS)

all: reliable

.c.o:
	$(CC) $(CFLAGS) -c $<

rlib.o reliable.o: rlib.h

reliable: reliable.o rlib.o
	$(CC) $(CFLAGS) -o $@ reliable.o rlib.o $(LIBS) $(LIBRT)

.PHONY: tester reference
tester reference:
	cd tester-src && $(MAKE) $@
	cp tester-src/$@ .

TAR = reliable.tar.gz

SUBMIT = reliable/Makefile reliable/*.[ch]

.PHONY: submit
submit: clean
	ln -s . reliable
	tar -czf $(TAR) $(SUBMIT)
	rm -f reliable
	@echo '************************************************************'
	@echo '                                                            '
	@echo '  Please submit file $(TAR) to the blackboard:              '
	@echo '                                                            '
	@echo '                                                            '
	@echo '                                                            '
	@echo '************************************************************'

.PHONY: dist
dist: clean tester reference
	cd tester-src && $(MAKE) clean
	./stripsol reliable.c > reliable.c-dist
	ln -s . reliable
	tar -czf $(TAR) \
		reliable/reliable.c-dist \
		reliable/Makefile reliable/rlib.[ch] \
		reliable/stripsol \
		# reliable/tester reliable/reference
	rm -f reliable

.PHONY: clean
clean:
	@find . \( -name '*~' -o -name '*.o' -o -name '*.log' \) \
		-print0 > .clean~
	@xargs -0 echo rm -f -- < .clean~
	@xargs -0 rm -f -- < .clean~
	rm -f reliable $(TAR)

.PHONY: clobber
clobber: clean
	cd tester-src && $(MAKE) clean
	rm -f tester reference reliable.c-dist
