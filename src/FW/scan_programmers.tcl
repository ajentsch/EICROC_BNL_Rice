open_hw_manager

#refresh_hw_server
connect_hw_server -url localhost:3121 -allow_non_jtag

#   scan programming devices
puts "Scanning for JTAG interfaces..."
puts [get_hw_targets]

puts "Using specific JTAG chain..."
#  once I have the programmer of choice, insert it here
#  the xxx80081 is at AXAU15
#  localhost:3121/xilinx_tcf/Xilinx/16036 is the ZCU
current_hw_target [get_hw_targets */xilinx_tcf/Digilent/210512180081]
#current_hw_target [get_hw_targets */xilinx_tcf/Xilinx/16036]
open_hw_target

#  and now list devices in the JTAG chain
puts "Devices on the JTAG bus:"
puts [get_hw_devices]

#  Choose which device
current_hw_device [get_hw_devices xcau15p_0]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices xcau15p_0] 0]

puts -nonewline "TEMPERATURE (C) "
puts [get_property TEMPERATURE [lindex [get_hw_sysmons] 0]]

#set_property PROBES.FILE {} [get_hw_devices xcau15p_0]
#set_property FULL_PROBES.FILE {} [get_hw_devices xcau15p_0]
#set_property PROGRAM.FILE {axau15_nopcie_download.bin} [get_hw_devices xcau15p_0]

#puts "Programming FPGA..."
#program_hw_devices [get_hw_devices xcau15p_0]
#puts "Programming done."
