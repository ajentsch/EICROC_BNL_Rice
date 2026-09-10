open_hw_manager
connect_hw_server -url localhost:3121 -allow_non_jtag
open_hw_target {localhost:3121/xilinx_tcf/Xilinx/16036}




set_property PROBES.FILE {zcu106_blaze.ltx} [get_hw_devices xczu7_0]
set_property FULL_PROBES.FILE {zcu106_blaze.ltx} [get_hw_devices xczu7_0]
set_property PROGRAM.FILE {zcu106_blaze_download.bit} [get_hw_devices xczu7_0]


current_hw_device [get_hw_devices xczu7_0]

program_hw_devices [get_hw_devices xczu7_0]
refresh_hw_device [lindex [get_hw_devices xczu7_0] 0]

