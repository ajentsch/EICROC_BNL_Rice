library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library UNISIM;
use UNISIM.VComponents.all;

library work ;
use work.top_package.all ;


--===============================================================
--========== Aligns serial data coming from EICROC into =========
--========== 8bit words for both SDOUT (which we use) and =======
--========== DOUT0 (which we don't use ==========================
--===============================================================


entity deser is
port (	
	CLK_320		: in std_logic ;
	SYNC40		: in std_logic ;

	DELAY		: in std_logic_vector(2 downto 0) ;

	DOUT		: in std_logic ;	-- serial link 0 from the OLD EICROC0
	SDOUT		: in std_logic ;	-- serial link from the new EICROC1

	DOUT_8B		: out std_logic_vector(7 downto 0) ;
	SDOUT_8B	: out std_logic_vector(7 downto 0) 
);
end deser;

architecture Beh of deser is

type s_type is (S_IDLE, S_SYNC, S_WAIT, S_START) ;
signal state		: s_type := S_IDLE ;

signal d_out8		: std_logic_vector(7 downto 0) ;
signal sd_out8		: std_logic_vector(7 downto 0) ;

signal d_out8_tmp	: std_logic_vector(7 downto 0) ;
signal sd_out8_tmp	: std_logic_vector(7 downto 0) ;

signal cou		: integer range 0 to 1023 := 0 ;

signal i_delay		: integer range 0 to 7 := 0 ;


begin

i_delay <= to_integer(unsigned(DELAY)) ;	-- experimentaly determined

DOUT_8B <= d_out8 ;
SDOUT_8B <= sd_out8 ;

--========== NOTE: From the Manual: data is MSB first ==========================

process(CLK_320, SYNC40, i_delay)
begin
	if(rising_edge(CLK_320)) then
	
	case(state) is
	when S_IDLE =>
		sd_out8_tmp <= (others => '0') ;
		d_out8_tmp <= (others => '0') ;

		sd_out8 <= (others => '0') ;
		d_out8 <= (others => '0') ;

		if(cou=1023) then
			state <= S_SYNC ;
		else
			cou <= cou + 1 ;
		end if ;
	when S_SYNC =>
		if(SYNC40='1') then
			cou <= 0 ;
			state <= S_WAIT ;
		end if ;
	when S_WAIT =>			-- wait for alignment
--		if(cou=1) then		-- this is for EICROC1 and its SDOUT; was 1 for EICROC1
		if(cou=i_delay) then		
			cou <= 0 ;
			state <= S_START ;
		else
			cou <= cou+1 ;
		end if ;
	when S_START =>
		sd_out8_tmp(cou) <= SDOUT ;
		d_out8_tmp(cou) <= DOUT ;


		if(cou=7) then
			-- latch
			sd_out8 <= SDOUT & sd_out8_tmp(6 downto 0) ;
			d_out8 <= DOUT & d_out8_tmp(6 downto 0) ;

			cou <= 0 ;
		else
			cou <= cou+1 ;
		end if ;
	end case ;
	end if ;
end process ;

end ;
