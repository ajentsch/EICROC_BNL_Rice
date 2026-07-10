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


type s_type is (S_IDLE, S_START_ACQ, S_FIRE, S_ONE_MORE, S_RDOUT_WAIT, S_START_RDOUT, S_CLR_RDOUT, S_WAIT) ;

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

begin

i_len_en_ack <= to_integer(unsigned(LEN_EN_ACQ)) ;
i_dly_cmdpulse <= to_integer(unsigned(DLY_CMDPULSE)) ;

SYNC40 <= sync40_o ;

START_ACQ <= start_acq_o when (MODE(0)='0') else '0' ;
CMDPULSE <= cmdpulse_o when (MODE(2 downto 0)=B"000") else '0' ;
TRIGEXT <= cmdpulse_o  when (MODE(2 downto 0)=B"010") else '0' ;
START_READOUT <= start_readout_o when (MODE(0)='0') else '0' ;


process(CLK_40)
begin
	if(rising_edge(CLK_40)) then
		cmd_1 <= cmd_strobe ;
		cmd_2 <= cmd_1 ;
		cmd_3 <= cmd_2 ;
	end if ;
end process ;

cmdpulse_o <= cmd_strobe or cmd_1 or cmd_2 or cmd_3 ;


process(CLK_40, GO, MODE, TRIGOUT)
begin
	if(rising_edge(CLK_40)) then

	sync40_o <= not sync40_o ;

		
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
		start_acq_o <= '1' ;

		if(cou=0) then
			FCMD <= FCMD_START_ACQ ;	-- also fire FCMD but just once
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		if(cou=i_dly_cmdpulse) then		
			state <= S_FIRE ;
			cou <= 0 ;
		else
			cou <= cou+1 ;
		end if ;
	when S_FIRE =>
		cmd_strobe <= '1' ;
		start_acq_o <= '1' ;

		if(cou=0 and MODE(2)='0') then
			if(MODE(1)='0') then
				FCMD <= FCMD_CMD_PULSE ;
			else
				FCMD <= FCMD_TRIG_EXT ;
			end if ;
		else
			FCMD <= FCMD_IDLE ;
		end if ;

		cou <= 0 ;
		state <= S_ONE_MORE ;


	when S_ONE_MORE =>
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
