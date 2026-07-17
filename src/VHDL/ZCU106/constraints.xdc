
#SEL_FCMD C26
set_property PACKAGE_PIN A8 [get_ports SEL_FCMD]
set_property IOSTANDARD LVCMOS18 [get_ports SEL_FCMD]


#EN_ACQ_EXT C22
set_property PACKAGE_PIN D11 [get_ports EN_ACQ_EXT]
set_property IOSTANDARD LVCMOS18 [get_ports EN_ACQ_EXT]

#TRIG_EXT C19
set_property PACKAGE_PIN C12 [get_ports TRIG_EXT]
set_property IOSTANDARD LVCMOS18 [get_ports TRIG_EXT]

#START_READOUT C18
set_property PACKAGE_PIN C13 [get_ports START_READOUT]
set_property IOSTANDARD LVCMOS18 [get_ports START_READOUT]

#FPGA_READY D34 -- NC????
#set_property PACKAGE_PIN AL10 [get_ports FPGA_READY]
#set_property IOSTANDARD LVCMOS18 [get_ports FPGA_READY]

#RESETB D26
set_property PACKAGE_PIN B9 [get_ports RESETB]
set_property IOSTANDARD LVCMOS18 [get_ports RESETB]

#SCL D24
set_property PACKAGE_PIN A11 [get_ports SCL]
set_property IOSTANDARD LVCMOS18 [get_ports SCL]

#SDA D23
set_property PACKAGE_PIN B11 [get_ports SDA]
set_property PULLUP true [get_ports SDA]
set_property IOSTANDARD LVCMOS18 [get_ports SDA]

#RST_I2C D21
set_property PACKAGE_PIN E10 [get_ports RST_I2C]
set_property IOSTANDARD LVCMOS18 [get_ports RST_I2C]

#DOUT_N0 G10
#DOUT_P0 G9
set_property PACKAGE_PIN K18 [get_ports DOUT_N0]
set_property PACKAGE_PIN K19 [get_ports DOUT_P0]
set_property IOSTANDARD LVDS [get_ports DOUT_N0]
set_property IOSTANDARD LVDS [get_ports DOUT_P0]
set_property DIFF_TERM_ADV TERM_100 [get_ports DOUT_P0]

#SDOUT_N G22
#SDOUT_P G21
set_property PACKAGE_PIN E12 [get_ports SDOUT_N]
set_property PACKAGE_PIN F12 [get_ports SDOUT_P]
set_property IOSTANDARD LVDS [get_ports SDOUT_N]
set_property IOSTANDARD LVDS [get_ports SDOUT_P]
set_property DIFF_TERM_ADV TERM_100 [get_ports SDOUT_P]

#CMDPULSE_N G16
#CMDPULSE_P G15
set_property PACKAGE_PIN F18 [get_ports CMDPULSE_N]
set_property PACKAGE_PIN G18 [get_ports CMDPULSE_P]
set_property IOSTANDARD LVDS [get_ports CMDPULSE_N]
set_property IOSTANDARD LVDS [get_ports CMDPULSE_P]


#TRIGOUT_N G13
#TRIGOUT_P G12
set_property PACKAGE_PIN E17 [get_ports TRIGOUT_N]
set_property PACKAGE_PIN E18 [get_ports TRIGOUT_P]
set_property IOSTANDARD LVDS [get_ports TRIGOUT_N]
set_property IOSTANDARD LVDS [get_ports TRIGOUT_P]
set_property DIFF_TERM_ADV TERM_100 [get_ports TRIGOUT_P]

#CLK320_N G7
#CLK320_P G6
set_property PACKAGE_PIN F16 [get_ports CLK320_N]
set_property PACKAGE_PIN F17 [get_ports CLK320_P]
set_property IOSTANDARD LVDS [get_ports CLK320_N]
set_property IOSTANDARD LVDS [get_ports CLK320_P]


#FCMD_N	H11
#FCMD_P H10
set_property PACKAGE_PIN L16 [get_ports FCMD_N]
set_property PACKAGE_PIN L17 [get_ports FCMD_P]
set_property IOSTANDARD LVDS [get_ports FCMD_N]
set_property IOSTANDARD LVDS [get_ports FCMD_P]


#CLK160_N H8
#CLK160_P H7
set_property PACKAGE_PIN K20 [get_ports CLK160_N]
set_property PACKAGE_PIN L20 [get_ports CLK160_P]
set_property IOSTANDARD LVDS [get_ports CLK160_N]
set_property IOSTANDARD LVDS [get_ports CLK160_P]



############## ZCU106-specific signals
set_property PACKAGE_PIN AH17 [get_ports USB_RX]
set_property IOSTANDARD LVCMOS12 [get_ports USB_RX]

set_property PACKAGE_PIN AL17 [get_ports USB_TX]
set_property IOSTANDARD LVCMOS12 [get_ports USB_TX]


set_property IOSTANDARD LVDS [get_ports CLK_125_N]
set_property PACKAGE_PIN H9 [get_ports CLK_125_P]
set_property PACKAGE_PIN G9 [get_ports CLK_125_N]
set_property IOSTANDARD LVDS [get_ports CLK_125_P]

set_property PACKAGE_PIN AL10 [get_ports GPIO_SW_C]
set_property IOSTANDARD LVCMOS12 [get_ports GPIO_SW_C]

set_property PACKAGE_PIN AL11 [get_ports GPIO_LED_0_LS]
set_property IOSTANDARD LVCMOS12 [get_ports GPIO_LED_0_LS]
set_property PACKAGE_PIN AL13 [get_ports GPIO_LED_1_LS]
set_property IOSTANDARD LVCMOS12 [get_ports GPIO_LED_1_LS]
set_property PACKAGE_PIN AK13 [get_ports GPIO_LED_2_LS]
set_property IOSTANDARD LVCMOS12 [get_ports GPIO_LED_2_LS]

# for an external trigger
set_property PACKAGE_PIN B23 [get_ports PMOD_EXT]
set_property IOSTANDARD LVCMOS18 [get_ports PMOD_EXT]


set_property BITSTREAM.CONFIG.UNUSEDPIN PULLNONE [current_design]
set_property BITSTREAM.CONFIG.USERID 32'hC106C106 [current_design]
set_property BITSTREAM.CONFIG.USR_ACCESS timestamp [current_design]

