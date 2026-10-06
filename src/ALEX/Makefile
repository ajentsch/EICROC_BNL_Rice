
include ../rtsmakefile.def


BINS = alex

all: $(BINS)

alex: ../ZCU106/zcu106.o alex_zcu.o
	$(LINK.o) -o $@ $^

# Leave stuff below
depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
# DO NOT DELETE

alex_zcu.o: ../LOG/rtsLog.h
alex_zcu.o: ../ZCU106/zcu106.h
