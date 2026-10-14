#puts "Entering various.tcl with arguments"
#puts $argv
set myarg1 [lindex $argv 0]
#puts "File [lindex $argv 0], argument $myarg1"

if {$myarg1 == "prog"} {
	puts "Programming FPGA."
} elseif {$myarg1 == "burn"} {
	puts "Burning PROM." 
}

open_hw_manager

#connect_hw_server -url localhost:3121 -allow_non_jtag
connect_hw_server 
refresh_hw_server

#   scan programming devices
puts "Scanning for JTAG interfaces..."
puts [get_hw_targets]

#  once I have the programmer of choice, insert it here
current_hw_target [get_hw_targets */xilinx_tcf/Digilent/210512180081]
# AXAU15
#current_hw_target [get_hw_targets */xilinx_tcf/Digilent/210512180081]
open_hw_target

#  and now list devices in the JTAG chain
puts "Devices on the JTAG bus:"
puts [get_hw_devices]

#  Choose which device
current_hw_device [get_hw_devices xcau15p_0]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices xcau15p_0] 0]

puts -nonewline "TEMPERATURE (C) "
puts [get_property TEMPERATURE [lindex [get_hw_sysmons] 0]]

set_property PROBES.FILE {} [get_hw_devices xcau15p_0]
set_property FULL_PROBES.FILE {} [get_hw_devices xcau15p_0]
set_property PROGRAM.FILE {axau15_gth_download.bin} [get_hw_devices xcau15p_0]

if {$myarg1 == "prog"} {

	puts "Programming FPGA..."
	program_hw_devices [get_hw_devices xcau15p_0]
	puts "Programming Done."
} elseif {$myarg1 == "burn"} {
	puts "Burning PROM..." 

	create_hw_cfgmem -hw_device [get_hw_devices xcau15p_0] -mem_dev [lindex [get_cfgmem_parts {mt25qu256-spi-x1_x2_x4}] 0]
	set_property PROGRAM.ADDRESS_RANGE  {use_file} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.FILES [list "axau15_gth_download.bin" ] [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.PRM_FILE {axau15_gth_download.prm} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.UNUSED_PIN_TERMINATION {pull-none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.CFG_PROGRAM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.VERIFY  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	startgroup 
	create_hw_bitstream -hw_device [lindex [get_hw_devices xcau15p_0] 0] [get_property PROGRAM.HW_CFGMEM_BITFILE [ lindex [get_hw_devices xcau15p_0] 0]]; program_hw_devices [lindex [get_hw_devices xcau15p_0] 0]; refresh_hw_device [lindex [get_hw_devices xcau15p_0] 0];
# This below take a longer time, where PROM burning actually occurs
	puts "Burning PROM -- this can take a while..."

	program_hw_cfgmem -hw_cfgmem [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
	endgroup
	boot_hw_device [lindex [get_hw_devices xcau15p_0] 0]
	refresh_hw_device [lindex [get_hw_devices xcau15p_0] 0]

	puts "Burning PROM Done."
}

