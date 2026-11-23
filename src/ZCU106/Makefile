
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto

LIBS = zcu106lib.o
BINS = decoder ana_dta zcu106

all: $(BINS) $(LIBS)

zcu106 : zcu106.o ../ALEX/alex_zcu.o

decoder : eicroc_decoder_c.o decoder.o

ana_dta : eicroc_decoder_c.o ana_dta.o 

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

zcu106lib.o : zcu106lib.C

save:
	cp -a $(BINS) /RTS/bin
#	scp -p zcu106 ana_dta decoder Ljubicic@bonner-muon.rice.edu:code

install:
	@$(INSTALL) ana_dta decoder /RTS/bin

# DO NOT DELETE

ana_dta.o: ../LOG/rtsLog.h
ana_dta.o: eicroc_decoder_c.h
decoder.o: ../LOG/rtsLog.h
decoder.o: eicroc_decoder_c.h
eicroc_decoder_c.o: ../LOG/rtsLog.h
eicroc_decoder_c.o: eicroc_decoder_c.h
zcu106.o: ../LOG/rtsLog.h
zcu106.o: ../ZCU106/zcu106.h
zcu106.o: ../ALEX/alex_zcu.h
zcu106_c.o: ../LOG/rtsLog.h
zcu106_c.o: zcu106_c.h
zcu106lib.o: ../LOG/rtsLog.h
zcu106lib.o: ../ZCU106/zcu106lib.h
