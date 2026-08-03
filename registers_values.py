eic_clib.write_asic_indirect_reg(0x4000, 0b11100011)

eic_clib.write_asic_indirect_reg(0x4001, 0b11011011)
eic_clib.write_asic_indirect_reg(0x4001, 0b11011010) # RST GRAY COUNTER

eic_clib.write_asic_indirect_reg(0x4002, 0b00001000)
eic_clib.write_asic_indirect_reg(0x4003, 0b10111100)
eic_clib.write_asic_indirect_reg(0x4004, 0b00000000)
eic_clib.write_asic_indirect_reg(0x4005, 0b10000000)
eic_clib.write_asic_indirect_reg(0x4006, 0b10100000)
eic_clib.write_asic_indirect_reg(0x4007, 0b01000000)
eic_clib.write_asic_indirect_reg(0x4008, 0b11110000) # RtR
eic_clib.write_asic_indirect_reg(0x4009, 0b11110010)
eic_clib.write_asic_indirect_reg(0x400A, 0b11111111)
eic_clib.write_asic_indirect_reg(0x400B, 0b01100011)
eic_clib.write_asic_indirect_reg(0x400C, 0bPLACEHOLDER)  #dacb pulser
eic_clib.write_asic_indirect_reg(0x400D, 0b01010011)
eic_clib.write_asic_indirect_reg(0x400E, 0b10100111)
eic_clib.write_asic_indirect_reg(0x400F, 0b00110011)
eic_clib.write_asic_indirect_reg(0x4010, 0b01001001)
eic_clib.write_asic_indirect_reg(0x4011, 0b00100111)
eic_clib.write_asic_indirect_reg(0x4012, 0b11111000)
eic_clib.write_asic_indirect_reg(0x4013, 0b00001111)
eic_clib.write_asic_indirect_reg(0x4014, 0b00000000)
eic_clib.write_asic_indirect_reg(0x4015, 0b00000000)
eic_clib.write_asic_indirect_reg(0x4016, 0b00000000)
eic_clib.write_asic_indirect_reg(0x4017, 0b11111111)
eic_clib.write_asic_indirect_reg(0x4018, 0b00000000)
eic_clib.write_asic_indirect_reg(0x4019, 0b00000100)
eic_clib.write_asic_indirect_reg(0x401A, 0b00000000) 
eic_clib.write_asic_indirect_reg(0x401B, 0b00000100)


#  writing those values in the 1024pixels
eic_clib.write_asic_indirect_reg(0x1, 0b10000000)
eic_clib.write_asic_indirect_reg(0x2, 0b01101100) #pix31
eic_clib.write_asic_indirect_reg(0x3, 0b0_000_01_00)
eic_clib.write_asic_indirect_reg(0x4, 0b00000001)
eic_clib.write_asic_indirect_reg(0x5, 0b00100000)

# masking pixel 31 and rewriting the 1023 other pixels
'''eic_clib.write_asic_indirect_reg(0x20F9, 0b10000000)
eic_clib.write_asic_indirect_reg(0x20FA, 0b00000000)
eic_clib.write_asic_indirect_reg(0x20FB, 0b0_000_00_00)
eic_clib.write_asic_indirect_reg(0x20FC, 0b00000001)
eic_clib.write_asic_indirect_reg(0x20FD, 0b00100000)'''
