.PHONY:	all clean nuke

all:
	./heehaw qbic

nuke:
	make -C common clean
	rm -f common/plugin
	rm -f common/Makefile.protocol.inc
	find . -name '*.o' -exec rm {} \;
	find . -name '*~' -exec rm {} \;
