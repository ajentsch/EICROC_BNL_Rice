library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;


library UNISIM;
use UNISIM.VComponents.all;


--======================================================
--======= Serialize and align FCMD byte ================
--======================================================

entity fcmd_drv is
port (
	CLK320		: in std_logic ;

	FCMD_O		: out std_logic ;

	SYNC40		: in std_logic ;
	FCMD_I		: in std_logic_vector(7 downto 0) 
) ;
end fcmd_drv;

architecture Beh of fcmd_drv is

type s_type is (S_IDLE, S_SYNC, S_WAIT, S_START) ;

signal state		: s_type := S_IDLE ;
signal cou		: integer range 0 to 1023 := 0 ;

signal fcmd_use		: std_logic_vector(7 downto 0) ;

begin



process(CLK320, SYNC40, FCMD_I)
begin
	if(rising_edge(CLK320)) then

	FCMD_O <= '0' ;

	case(state) is
	when S_IDLE =>
		if(cou=1023) then		-- just wait a bit after startup for SYNC40 to start ticking...
			state <= S_SYNC ;
		else
			cou <= cou + 1 ;
		end if ;
	when S_SYNC =>
		if(SYNC40='1') then
			cou <= 0 ;
			state <= S_WAIT ;
		end if ;
	when S_WAIT =>
		if(cou=5) then		-- until I align to SYNC40
			cou <= 7 ;
			state <= S_START ;
		else
			cou <= cou+1 ;
		end if ;
	when S_START =>			-- should start at the same time as SYNC40
		if(cou=7) then
			fcmd_use <= FCMD_I ;	-- latch
			FCMD_O <= FCMD_I(7) ;
			cou <= 6 ;
		else
			FCMD_O <= fcmd_use(cou) ;
			if(cou=0) then
				cou <= 7 ;
			else
				cou <= cou -1 ;
			end if ;
		end if ;
	end case ;

	end if ;
end process ;

end Beh;
