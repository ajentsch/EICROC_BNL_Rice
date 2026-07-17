library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library UNISIM;
use UNISIM.VComponents.all;

library work ;
use work.top_package.all ;

--====================================================================
--========== Create 4x8bit words and dump them with header/trailer ===
--========== to a 32bit FIFO                                       ===
--====================================================================

entity rdout is
port (	
	CLK_40		: in std_logic ;

	DATA_AVAIL	: in std_logic ;

	DATA8_IN	: in std_logic_vector(7 downto 0) ;
	
	DATA32_OUT	: out std_logic_vector(31 downto 0) ;

	WR_EN		: out std_logic ;

	WORDS_REQ	: in std_logic_vector(15 downto 0) ;
	ASIC_TYPE	: in std_logic_vector(3 downto 0) ;
	WORD_AUX_0	: in std_logic_vector(31 downto 0) 
);
end rdout;

architecture Beh of rdout is

type s_type is (S_IDLE, S_HDR, S_DTA_WAIT, S_DTA_0, S_DTA_1, S_DTA_2, S_DTA_3, S_TRL,S_WAIT) ;

signal state	: s_type := S_IDLE ;
 
signal fifo_l	: std_logic_vector(31 downto 0) ;
signal fifo_d	: std_logic_vector(31 downto 0) ;

signal word_cou	: integer range 0 to 127000 ;

signal hdr	: vec32_t(7 downto 0) ;
signal trl	: vec32_t(7 downto 0) := (others => (others => '0')) ;

signal cou	: integer range 0 to 127000 ;

signal asic_word_cou	: integer range 0 to 127000 ;
signal fmt_type		: std_logic_vector(11 downto 0) ;

begin


asic_word_cou <= to_integer(unsigned(WORDS_REQ)) ;
fmt_type <= x"000" ;

hdr(0) <= x"ABCD_EC01" ;
hdr(1)(31 downto 16) <= ASIC_TYPE & fmt_type ;		-- normally: geo_id

-- not used below!!!
hdr(2) <= (others => '0') ;	-- word cou
hdr(3) <= (others => '0') ;	-- status
hdr(4) <= (others => '0') ;	-- xing
hdr(5) <= (others => '0') ;	-- xing
hdr(6) <= WORD_AUX_0 ;	-- type
hdr(7) <= x"FEED_EC01" ;

trl(0) <= x"CDEF_EC01" ;
trl(1)(31 downto 16) <= ASIC_TYPE & fmt_type ;

trl(2) <= (others => '0') ;			-- unused
trl(3)(31 downto 8) <= (others => '0') ;	-- status in lower 8 bits
trl(4) <= (others => '0') ;
trl(5) <= (others => '0') ;
trl(6) <= WORD_AUX_0 ;				-- whatever, from upper code
trl(7) <= x"EEEE_EC01" ;
	
DATA32_OUT <= fifo_d ;

hdr(1)(15 downto 0) <= WORDS_REQ ;
trl(1)(15 downto 0) <= WORDS_REQ ;

process(CLK_40, DATA_AVAIL, DATA8_IN, asic_word_cou)
begin
	if(rising_edge(CLK_40)) then

	WR_EN <= '0' ;

	case(state) is
	when S_IDLE =>
		word_cou <= 0 ;
		cou <= 0 ;
		if(DATA_AVAIL='1') then
			trl(3)(7 downto 0) <= (others => '0') ;	-- clear status
			state <= S_HDR ;
		end if ;
	when S_HDR =>			-- dump 2 header words
		word_cou <= 1 ;
		fifo_d <= hdr(cou) ;
		WR_EN <= '1' ;

		if(cou=1) then
			cou <= 0 ;
			state <= S_DTA_WAIT ;
		else	
			cou <= cou+1 ;
		end if ;
	when S_DTA_WAIT =>
		fifo_l(7 downto 0) <= DATA8_IN ;
		if(DATA8_IN/=X"00") then
			state <= S_DTA_1 ;
		else
			if(cou>6000) then
				trl(3)(0) <= '1' ;	-- timeout occured
				cou <= 0 ;
				state <= S_TRL ;
			else
				cou <= cou + 1 ;
			end if ;
		end if ;
	when S_DTA_0 =>
		fifo_l(7 downto 0) <= DATA8_IN ;
		state <= S_DTA_1 ;
	when S_DTA_1 =>
		fifo_l(15 downto 8) <= DATA8_IN ;
		state <= S_DTA_2 ;
	when S_DTA_2 =>
		fifo_l(23 downto 16) <= DATA8_IN ;
		state <= S_DTA_3 ;
	when S_DTA_3 =>
		fifo_d(31 downto 24) <= DATA8_IN ;
		fifo_d(23 downto 0) <= fifo_l(23 downto 0) ;
		WR_EN <= '1' ;
	
		if(word_cou=asic_word_cou) then
			cou <= 0 ;
			state <= S_TRL ;	
		else
			word_cou <= word_cou+1 ;
			state <= S_DTA_0 ;
		end if ;
	when S_TRL =>
		fifo_d <= trl(cou) ;
		WR_EN <= '1' ;

		if(cou=7) then
			state <= S_WAIT ;
		else
			cou <= cou+1 ;
		end if ;
	when S_WAIT =>
		if(DATA_AVAIL='0') then
			state <= S_IDLE ;
		end if ;
	end case ;
	end if ;

end process ;

end ;
