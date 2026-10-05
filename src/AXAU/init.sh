
### For bonner-muon

# AXAU15 is on 65:00.0

rm -f /dev/dam1

#/bin/chmod 666 /sys/bus/pci/devices/0000:65:00.0/resource0
#/usr/bin/ln -s /sys/bus/pci/devices/0000:65:00.0 /dev/dam1

/bin/chmod 666 /sys/bus/pci/devices/0000:22:00.0/resource0
/usr/bin/ln -s /sys/bus/pci/devices/0000:22:00.0 /dev/dam1

/sbin/setpci -d 10ee: COMMAND=7


insmod ../RESMEM/resmem.ko resmem_hwaddr=0x20000000 resmem_length=0x08000000
sleep 1
chmod 666 /dev/resmem
