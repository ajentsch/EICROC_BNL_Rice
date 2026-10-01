# Writing EoC configuration, not worrying about pixels

eic_clib.write_asic_indirect_reg(0x4000, 0b11101011)

# RST GRAY COUNTER
eic_clib.write_asic_indirect_reg(0x4001, 0b11111000)  # reg 1 0b11011010
eic_clib.write_asic_indirect_reg(0x4001, 0b11111001)

eic_clib.write_asic_indirect_reg(0x4002, 0b11111100)  # reg 2
eic_clib.write_asic_indirect_reg(0x4003, 0b00100000)  # reg 3 0b10111100
eic_clib.write_asic_indirect_reg(0x4004, 0b01000000)  # reg 4
eic_clib.write_asic_indirect_reg(0x4005, 0b01000000)  # reg 5
eic_clib.write_asic_indirect_reg(0x4006, 0b00100000)  # reg 6
eic_clib.write_asic_indirect_reg(0x4007, 0b01000000)  # reg 7
eic_clib.write_asic_indirect_reg(0x4008, 0b11110000)  # reg 8 RtR! 0b11110000
eic_clib.write_asic_indirect_reg(0x4009, 0b11111111)  # reg 9 ON_buf[3:0] is [7:4] in the 8 bit, and corresponds to which block (in both halves) is active, NOT the column

eic_clib.write_asic_indirect_reg(0x400A, 0b11111111)  # reg 10 vth 7:0
eic_clib.write_asic_indirect_reg(0x400B, 0b01100011)  # reg 11 bias PA + vth 9:8

eic_clib.write_asic_indirect_reg(0x400C, 0b11100000)  # reg 12 refs + charge dacbpulser
eic_clib.write_asic_indirect_reg(0x400D, 0b01010000)  # reg 13 RtR bits <7:6> S[1:0], bits <5:0> bias RTR
eic_clib.write_asic_indirect_reg(0x400E, 0b10010111)  # reg 14 <6:3> TA_SELECT_GAIN -- high numbers seem to lower TDC mean value
eic_clib.write_asic_indirect_reg(0x400F, 0b00110000)  # reg 15 
eic_clib.write_asic_indirect_reg(0x4010, 0b01001001)  # reg 16
eic_clib.write_asic_indirect_reg(0x4011, 0b00100111)  # reg 17

eic_clib.write_asic_indirect_reg(0x4012, 0b01111000)  # reg 18 bit 4 here is dout_EN
eic_clib.write_asic_indirect_reg(0x4013, 0b00001111)  # reg 19 EN_clk column
eic_clib.write_asic_indirect_reg(0x4014, 0b00000000)  # reg 20 EN_clk column
eic_clib.write_asic_indirect_reg(0x4015, 0b00000000)  # reg 21 EN_clk column
eic_clib.write_asic_indirect_reg(0x4016, 0b00000000)  # reg 22 EN_clk column
eic_clib.write_asic_indirect_reg(0x4017, 0b11111111)  # reg 23
eic_clib.write_asic_indirect_reg(0x4018, 0b00000000)  # reg 24 12b pulser [7:0]
eic_clib.write_asic_indirect_reg(0x4019, 0b00000110)  # reg 25 12b pulser [11:8] -- 7:4 dacb_12b, 3 is 12b DACb on, 2:1 EN_buf_PA<1:0> left and right SMA output
eic_clib.write_asic_indirect_reg(0x401A, 0b00110000)  # reg 26 EN probe PA / EN_digprobe
eic_clib.write_asic_indirect_reg(0x401B, 0b00000000)  # probe dc1 / dc2



