LIBRARY ieee;
USE ieee.std_logic_1164.all;
USE ieee.numeric_std.all;

PACKAGE top_package is

type vec32_t is array (integer range <>) of std_logic_vector(31 downto 0) ;
type vec16_t is array (integer range <>) of std_logic_vector(15 downto 0) ;
type vec8_t is array (integer range <>) of std_logic_vector(7 downto 0) ;

constant ASIC_CMD_IDLE	: std_logic_vector(7 downto 0) := x"F0" ;
constant ASIC_CMD_BCR	: std_logic_vector(7 downto 0) := x"CC" ;

constant TSLICE_40_TICKS	: integer := 40000 ;	-- 20000 is normal 0.5 ms ; 200 is for debugging

function to_hex_string(
	slv: in std_logic_vector)
	return string ;

END top_package;

PACKAGE BODY top_package is

function to_hex_string(slv: in std_logic_vector) return string is
            variable hex_string : string(1 to slv'length / 4);
            variable nibble : std_logic_vector(3 downto 0);
            constant hex_chars : string := "0123456789ABCDEF";
begin
     	    for i in 0 to slv'length/4-1 loop
        	nibble := slv(i * 4 + 3 downto i * 4);
                hex_string(8 - i) := hex_chars(to_integer(unsigned(nibble)) + 1);
            end loop;
            return hex_string;
end function;

END PACKAGE BODY top_package ;
