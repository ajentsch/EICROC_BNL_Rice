
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = zcu106

all: $(BINS)

zcu106 : zcu106_c.o zcu106.o

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
#	cp -a zcu106.C /home/epic/tonko

# DO NOT DELETE

zcu106.o: ../LOG/rtsLog.h
zcu106.o: ../ZCU106/zcu106_c.h
zcu106_c.o: ../LOG/rtsLog.h
zcu106_c.o: zcu106_c.h
