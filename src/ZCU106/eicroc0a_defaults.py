eic_clib.write_asic_indirect_reg(0x4000, 0b11100000)
# GRAY COUNTER RESET 
eic_clib.write_asic_indirect_reg(0x4001, 0b11011000) 
eic_clib.write_asic_indirect_reg(0x4001, 0b11011001) 
eic_clib.write_asic_indirect_reg(0x4001, 0b11011000) 
eic_clib.write_asic_indirect_reg(0x4002, 0b10000000) 
eic_clib.write_asic_indirect_reg(0x4003, 0b10111100) 
eic_clib.write_asic_indirect_reg(0x4004, 0b00000000) 
eic_clib.write_asic_indirect_reg(0x4005, 0b01000000) 
eic_clib.write_asic_indirect_reg(0x4006, 0b10100000) 
eic_clib.write_asic_indirect_reg(0x4007, 0b01000000) 
eic_clib.write_asic_indirect_reg(0x4008, 0b11110000) # RtR! 
eic_clib.write_asic_indirect_reg(0x4009, 0b11101111) 
eic_clib.write_asic_indirect_reg(0x400A, 0b00000000) # vth 7:0 
eic_clib.write_asic_indirect_reg(0x400B, 0b0000_00_00) # bias PA + vth 9:8 eic_clib.write_asic_indirect_reg(0x400C, 0b11000000) # refs + charge dacbpulser
eic_clib.write_asic_indirect_reg(0x400D, 0b01_1000_00) # RtR 
eic_clib.write_asic_indirect_reg(0x400E, 0b10100111) 
eic_clib.write_asic_indirect_reg(0x400F, 0b00110011) 
eic_clib.write_asic_indirect_reg(0x4010, 0b010_010_01) 
eic_clib.write_asic_indirect_reg(0x4011, 0b0_010_0110) 
eic_clib.write_asic_indirect_reg(0x4012, 0b0000_0000) # Probes

eic_clib.write_asic_indirect_reg(0x1, 0b10000000) 
eic_clib.write_asic_indirect_reg(0x2, 0b01000000) 
eic_clib.write_asic_indirect_reg(0x3, 0b0_000_0_1_00) 
eic_clib.write_asic_indirect_reg(0x4, 0b00101001) 
eic_clib.write_asic_indirect_reg(0x5, 0b00000000)
