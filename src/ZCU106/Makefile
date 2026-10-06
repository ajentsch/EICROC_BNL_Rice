
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = zcu106 decoder ana_dta

all: $(BINS)

zcu106 : zcu106.o ../ALEX/alex_zcu.o

decoder : eicroc_decoder_c.o decoder.o

ana_dta : eicroc_decoder_c.o ana_dta.o 

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
#	cp -a zcu106.C /home/epic/tonko

# DO NOT DELETE

ana_dta.o: ../LOG/rtsLog.h
ana_dta.o: eicroc_decoder_c.h
decoder.o: ../LOG/rtsLog.h
decoder.o: eicroc_decoder_c.h
eicroc_decoder_c.o: ../LOG/rtsLog.h
eicroc_decoder_c.o: eicroc_decoder_c.h
zcu106.o: ../LOG/rtsLog.h
zcu106.o: ../ZCU106/zcu106.h
zcu106_c.o: ../LOG/rtsLog.h
zcu106_c.o: zcu106_c.h
