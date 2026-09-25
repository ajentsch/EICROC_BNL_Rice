
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = dam_test

all: $(BINS)

dam_test : dam_test.o dam_c.o rbv1.o



depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
#	cp -a zcu106.C /home/epic/tonko

# DO NOT DELETE

axau.o: ../LOG/rtsLog.h
dam_c.o: ../LOG/rtsLog.h
dam_c.o: ../RESMEM/resmem.h
dam_c.o: dam_c.h
dam_test.o: ../LOG/rtsLog.h
dam_test.o: dam_c.h
