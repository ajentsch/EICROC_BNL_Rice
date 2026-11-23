
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = tdaq

all: $(BINS)

tdaq: tdaq.o

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
	cp -a $(BINS) /RTS/bin

# DO NOT DELETE

tdaq.o: ../LOG/rtsLog.h
