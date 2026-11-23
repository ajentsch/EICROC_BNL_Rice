
include ../rtsmakefile.def

#LDLIBS+= -lmosquitto


BINS = eicroc machine parser dam_test eicroc_old

all: $(BINS)

# also search here for sources:
vpath %.C ../ZCU106 ../ALEX

dam_test : dam_test.o dam_c.o rbv1.o

eicroc : eicroc.o eicroc_c.o dam_c.o regjson.o jparse.o rbv1.o zcu106lib.o alex_zcu.o

parser : parser.o

eicroc_old : eicroc_old.o dam_c.o rbv1.o 

depend:
	@$(MAKEDEPEND)

clean:
	@$(CLEAN) $(BINS)

save:
	cp -a eicroc /RTS/bin

TARGET ?= petalinux@peta1
DEST ?= src/AXAU
PW ?= user

FILES := eicroc.C eicroc_c.C eicroc_c.h dam_c.C dam_c.h

scp: $(FILES)
	sshpass -p '$(PW)' scp -p -o StrictHostKeyChecking=no $(FILES) $(TARGET):$(DEST)/

# DO NOT DELETE

dam_c.o: ../LOG/rtsLog.h
dam_c.o: ../RESMEM/resmem.h
dam_c.o: dam_c.h
dam_test.o: ../LOG/rtsLog.h
dam_test.o: dam_c.h
eicroc.o: ../LOG/rtsLog.h
eicroc.o: ../AXAU/eicroc_c.h
eicroc_c.o: ../LOG/rtsLog.h
eicroc_c.o: ../AXAU/dam_c.h
eicroc_c.o: ../ZCU106/zcu106lib.h
eicroc_c.o: jparse.h
eicroc_c.o: regjson.h
eicroc_c.o: eicroc_c.h
eicroc_old.o: ../LOG/rtsLog.h
eicroc_old.o: dam_c.h
jparse.o: jparse.h
parser.o: ../LOG/rtsLog.h
regjson.o: regjson.h
regjson.o: jparse.h
