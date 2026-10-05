library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;


library UNISIM;
use UNISIM.VComponents.all;

entity daq is
port (
	CLK_320		: in std_logic ;
	CLK_160		: in std_logic ;
	CLK_40		: in std_logic ;


	GO		: in std_logic ;
	MODE		: in std_logic_vector(2 downto 0) ;

	LEN_EN_ACQ	: in std_logic_vector(7 downto 0) ;
	DLY_CMDPULSE	: in std_logic_vector(7 downto 0) ;

	FCMD		: out std_logic_vector(7 downto 0) ;
	SYNC40		: out std_logic ;
	DATA_AVAIL	: out std_logic ;

	WAIT_ON		: out std_logic ;
	RST_STATE	: in std_logic ;

	RDOUT_TYPE		: in std_logic_vector(2 downto 0) ;
	EXTERNAL_TRIGGER	: in std_logic ;

	START_ACQ	: out std_logic ;
	CMDPULSE	: out std_logic ;
	TRIGEXT		: out std_logic ;
	TRIGOUT		: in std_logic ;
	START_READOUT	: out std_logic 
) ;
end daq;

architecture Beh of daq is


type s_type is (S_IDLE, S_START_ACQ, S_FIRE, S_WAIT_EXT, S_ONE_MORE, S_RDOUT_WAIT, S_START_RDOUT, S_CLR_RDOUT, S_WAIT) ;

signal state	: s_type := S_IDLE ;
signal cou	: integer range 0 to 1023 := 0 ;


signal sync40_o		: std_logic := '0' ;

signal start_acq_o	: std_logic ;
signal cmdpulse_o	: std_logic ;
signal cmd_strobe	: std_logic ;
signal cmd_1		: std_logic ;
signal cmd_2		: std_logic ;
signal cmd_3		: std_logic ;

signal start_readout_o	: std_logic ;

-- constant FCMD's
signal FCMD_IDLE	: std_logic_vector(7 downto 0) := B"0011_0110" ;	-- IDLE
signal FCMD_START_ACQ	: std_logic_vector(7 downto 0) := B"0100_1011" ;	-- L1A
signal FCMD_TRIG_EXT	: std_logic_vector(7 downto 0) := B"0111_1000" ;	-- CALPULSEEXT
signal FCMD_CMD_PULSE	: std_logic_vector(7 downto 0) := B"0010_1101" ;	-- CALPULSEINT
signal FCMD_START_RDOUT	: std_logic_vector(7 downto 0) := B"1101_0001" ;	-- EBR

signal status		: std_logic_vector(7 downto 0) := X"00" ;

signal i_len_en_ack	: integer range 0 to 255 := 0 ;
signal i_dly_cmdpulse	: integer range 0 to 255 := 0 ;

signal tout_tmp		: std_logic ;
signal tout		: std_logic ;
signal tout_clr		: std_logic := '0' ;
signal tout_latch	: std_logic := '0' ;

begin

i_len_en_ack <= to_integer(unsigned(LEN_EN_ACQ)) ;
i_dly_cmdpulse <= to_integer(unsigned(DLY_CMDPULSE)) ;

SYNC40 <= sync40_o ;

--========== if MODE(0)='0' we use the old style readout scheme
START_ACQ <= start_acq_o when (MODE(0)='0') else '0' ;
CMDPULSE <= cmdpulse_o when (MODE(2 downto 0)=B"000") else '0' ;
TRIGEXT <= cmdpulse_o  when (MODE(2 downto 0)=B"010") else '0' ;
START_READOUT <= start_readout_o when (MODE(0)='0') else '0' ;


--=============== extend cmd to 4 ticks
process(CLK_40)
begin
	if(rising_edge(CLK_40)) then
		cmd_1 <= cmd_strobe ;
		cmd_2 <= cmd_1 ;
		cmd_3 <= cmd_2 ;
	end if ;
end process ;

cmdpulse_o <= cmd_strobe or cmd_1 or cmd_2 or cmd_3 ;


--============== cleanup trgout at 320
process(CLK_320, TRIGOUT)
begin
	if(rising_edge(CLK_320)) then
		tout_tmp <= TRIGOUT ;
		tout <= tout_tmp ;
	end if ;
end process ;

--=============== latch at 320 ===================
process(CLK_320, tout_clr, tout)
begin
	if(rising_edge(CLK_320)) then
		if(tout_clr='1') then
			tout_latch <= '0' ;
		elsif(tout='1') then
			tout_latch <= '1' ;
		end if ;
	end if ;
end process ;


process(CLK_40, GO, MODE, tout_latch, EXTERNAL_TRIGGER, RDOUT_TYPE)
begin
	if(rising_edge(CLK_40)) then

	sync40_o <= not sync40_o ;
	WAIT_ON <= '0' ;
	tout_clr <= '0' ;

	case(state) is
	when S_IDLE =>
		start_acq_o <= '0'; 
		cmd_strobe <= '0' ;
		start_readout_o <= '0' ;

		FCMD <= FCMD_IDLE ;

		cou <= 0 ;

		DATA_AVAIL <= '0' ;	-- tell the FIFO machine to stop...

		if(GO='1') then
			status <= (others => '0') ;
			state <= S_START_ACQ ;
		end if ;

	when S_START_ACQ =>
		start_acq_o <= '1' ;			-- start acquisition ON

		if(cou=0) then
			FCMD <= FCMD_START_ACQ ;	-- also fire FCMD but just once
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		tout_clr <= '1' ;	-- clear latch here

		if(cou=i_dly_cmdpulse) then		
			state <= S_FIRE ;
			cou <= 0 ;
		else				
			cou <= cou+1 ;
		end if ;
	when S_FIRE =>					-- either fire the command pulse or nothing...
		cmd_strobe <= '1' ;			-- keep it on
		start_acq_o <= '1' ;

		
		-- at this point I either:
		--	pedestal: do nothing, move to readout
		--	pulser: fire pulser
		--	trig_ext: fire TRIG_EXT
		--	trgout: wait for TRGOUT
		--	external_trigger: wait for external trigger
		if(cou=0 and MODE(2)='0') then	-- hm, cou is always 0 on entry
			if(MODE(1)='0') then
				FCMD <= FCMD_CMD_PULSE ;
			else
				FCMD <= FCMD_TRIG_EXT ;
			end if ;
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		cou <= 0 ;



		case(RDOUT_TYPE) is
		when B"011" =>
			state <= S_WAIT_EXT ;
		when B"100" =>
			state <= S_WAIT_EXT ;
		when others =>
			state <= S_ONE_MORE ;
		end case ;

	when S_WAIT_EXT	=>	-- wait for external occurence
		start_acq_o <= '1' ;	-- keep high
		cmd_strobe <= '0' ;	-- negate

		WAIT_ON <= '1' ;		-- show external code
		FCMD <= FCMD_IDLE ;
	
		case(RDOUT_TYPE) is
		when B"011" =>
			if(tout_latch='1') then
				state <= S_ONE_MORE ;
			elsif(RST_STATE='1') then
				state <= S_WAIT ;
			end if ;
		when B"100" =>
			if(EXTERNAL_TRIGGER='1') then
				state <= S_ONE_MORE ;
			elsif(RST_STATE='1') then
				state <= S_WAIT ;
			end if ;
		when others =>			-- this shouldn't be!
			state <= S_ONE_MORE ;
		end case ;		

	when S_ONE_MORE =>			-- I wait with START_ACQ enabled, typically 4 ticks...
		start_acq_o <= '1' ;
		cmd_strobe <= '0' ;

		FCMD <= FCMD_IDLE ;

		if(cou=i_len_en_ack) then
			cou <= 0 ;
			state <= S_RDOUT_WAIT ;
		else
			cou <= cou + 1 ;
		end if ;
	when S_RDOUT_WAIT =>			-- this stops the circular buffer
		cmd_strobe <= '0' ;
		start_acq_o <= '0' ;

		if(cou=0) then
			FCMD <= FCMD_START_ACQ ; -- repeat to clear
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		if(cou=10) then			-- fixed time from end of acq to start readout
			state <= S_START_RDOUT ;
			cou <= 0 ;
		else
			cou <= cou + 1 ;
		end if ;
	when S_START_RDOUT =>
		start_readout_o <= '1' ;

		DATA_AVAIL <= '1' ;

		if(cou=0) then
			FCMD <= FCMD_START_RDOUT ;
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		if(cou=5) then
			state <= S_CLR_RDOUT ;
		else
			cou <= cou+1 ;
		end if ;
	when S_CLR_RDOUT =>
		start_readout_o <= '0' ;
		FCMD <= FCMD_START_RDOUT ;	-- repeat to clear
		state <= S_WAIT ;
	when S_WAIT =>

		FCMD <= FCMD_IDLE ;
		start_readout_o <= '0' ;
		cmd_strobe <= '0' ;
		start_acq_o <= '0' ;

		if(GO='0') then
			state <= S_IDLE ;
		end if ;
	end case ;
	end if ;
end process ;

end Beh;
