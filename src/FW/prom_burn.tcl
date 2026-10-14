
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
program_hw_cfgmem -hw_cfgmem [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices xcau15p_0] 0]]
endgroup
boot_hw_device [lindex [get_hw_devices xcau15p_0] 0]
refresh_hw_device [lindex [get_hw_devices xcau15p_0] 0]
