
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = zcu106 decoder

all: $(BINS)

zcu106 : zcu106.o

decoder : eicroc_decoder_c.o decoder.o

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
#	cp -a zcu106.C /home/epic/tonko

# DO NOT DELETE

decoder.o: ../LOG/rtsLog.h
decoder.o: eicroc_decoder_c.h
eicroc_decoder_c.o: ../LOG/rtsLog.h
eicroc_decoder_c.o: eicroc_decoder_c.h
zcu106.o: ../LOG/rtsLog.h
zcu106_c.o: ../LOG/rtsLog.h
zcu106_c.o: zcu106_c.h
