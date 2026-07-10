library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library UNISIM;
use UNISIM.VComponents.all;

library work ;
use work.top_package.all ;

entity top is
port (	
	-- local clock: 125 MHz for ZCU106
	CLK_125_P	: in std_logic ;
	CLK_125_N	: in std_logic ;

        -- EICROC1 testboard
        SEL_FCMD        : out std_logic ;       -- 1: FCMD ON

        EN_ACQ_EXT      : out std_logic ;       -- enable acquisition
        TRIG_EXT        : out std_logic ;       -- external trigger to ASIC
        START_READOUT   : out std_logic ;       -- start readout to ASIC (StartRo_ext)

--      OCMDPULSE       : out std_logic ;       -- ?
--      ICMDPULSE       : out std_logic ;       -- ?

--      SPARE2          : out std_logic ;       -- ?
--      SPARE1          : out std_logic ;       -- ?

---     FPGA_READY      : out std_logic ;       -- 0: ready? just a LED?

--      K_PROBE_DC2     : in std_logic ;        -- analog probe to lpGBT ADC
--      K_PROBL_DC1     : in std_logic ;        -- analog probe to lpGBT ADC
--      K_ANALOG_PROBE  : in std_logic ;        -- analog probe to lpGBT ADC

        RESETB          : out std_logic ;       -- 0: reset active

	PMOD_EXT	: in std_logic ;	-- for external triggering

        SCL             : out std_logic ;
        SDA             : inout std_logic ;

        RST_I2C         : out std_logic ;       -- 0: reset active

        DOUT_N0         : in std_logic ;        -- use just 1 old lane...
        DOUT_P0         : in std_logic ;

        SDOUT_N         : in std_logic ;        -- EIC iface data
        SDOUT_P         : in std_logic ;

        CMDPULSE_N      : out std_logic ;       -- pulse injection command
        CMDPULSE_P      : out std_logic ;

        TRIGOUT_N       : in std_logic ;
        TRIGOUT_P       : in std_logic ;

        CLK320_N        : out std_logic ;
        CLK320_P        : out std_logic ;

        FCMD_N          : out std_logic ;       -- 00110110 is IDLE, LSb first?
        FCMD_P          : out std_logic ;

        CLK160_N        : out std_logic ;
        CLK160_P        : out std_logic ;

------  Debug -------------------
        GPIO_LED_0_LS   : out std_logic ;
        GPIO_LED_1_LS   : out std_logic ;
        GPIO_LED_2_LS   : out std_logic ;

        GPIO_SW_C       : in std_logic ;                  -- SW15

	-- terminal for the Microblaze
	USB_RX		: in std_logic ;	
	USB_TX		: out std_logic	

);
end top;

architecture Beh of top is


signal clk_160	: std_logic ;
signal clk_320	: std_logic ;
signal clk_40	: std_logic ;

signal gpio_in	: std_logic_vector(31 downto 0) ;
signal gpio_out	: std_logic_vector(31 downto 0) ;

signal led              : std_logic_vector(2 downto 0) := (others => '0') ;


signal clk_cou          : integer range 0 to 40000000 := 0 ;

signal ila0             : std_logic_vector(7 downto 0) ;
signal ila1             : std_logic_vector(7 downto 0) ;
signal ila2             : std_logic_vector(7 downto 0) ;
signal ila3             : std_logic_vector(7 downto 0) ;
signal ila4             : std_logic_vector(7 downto 0) ;
signal ila5             : std_logic_vector(0 downto 0) ;
signal ila6             : std_logic_vector(0 downto 0) ;
signal ila7             : std_logic_vector(0 downto 0) ;
signal ila8             : std_logic_vector(0 downto 0) ;

-- differentials
signal dout0            : std_logic ;   -- in
signal sdout            : std_logic ;   -- in
signal sdout_tmp	: std_logic ;	

signal fcmd             : std_logic ;   -- out
signal cmdpulse         : std_logic ;   -- out
signal trigout          : std_logic ;   -- in

signal fcmd_d		: std_logic_vector(7 downto 0) := B"0000_0000" ;


signal regs_ro		: vec16_t(3 downto 0) ;
signal regs_rw		: vec16_t(4 downto 0) ;

signal sdout_8b		: std_logic_vector(7 downto 0) ;
signal dout_8b		: std_logic_vector(7 downto 0) ;

signal enable_acq	: std_logic := '0' ;
signal trig_external	: std_logic := '0' ;
signal start_rdout	: std_logic := '0' ;

signal sync40		: std_logic := '0' ;

signal data_avail	: std_logic ;
signal fifo_srst	: std_logic ;
signal fifo_din		: std_logic_vector(31 downto 0) ;
signal fifo_wr_en	: std_logic ;
signal fifo_rd_en	: std_logic ;
signal fifo_dout	: std_logic_vector(31 downto 0) ;
signal fifo_full	: std_logic ;
signal fifo_empty	: std_logic ;
signal fifo_rd_rst_busy	: std_logic ;
signal fifo_wr_rst_busy	: std_logic ;

signal word_aux_0	: std_logic_vector(31 downto 0) ;

signal external_trigger	: std_logic ;

begin


--FPGA_READY <= '0' ;   -- 0: ready??? This is not connected on ZCU106


--================= Driven by component daq ==========================
EN_ACQ_EXT <= enable_acq when(regs_rw(0)(3)='0') else 'Z' ;
TRIG_EXT <= trig_external when(regs_rw(0)(3)='0') else 'Z' ;
START_READOUT <= start_rdout when(regs_rw(0)(3)='0') else 'Z' ;




--============ ILA setup ======================================================
ila0 <= B"00" & external_trigger & data_avail & enable_acq & start_rdout & cmdpulse & trig_external ;
ila1 <= sdout & dout0 & B"00" & B"000" & fcmd ;
ila2 <= sdout_8b ;
ila3 <= dout_8b ;
ila4 <= fcmd_d ;
ila5(0) <= regs_rw(0)(8) ;	-- SCL
ila6(0) <= regs_rw(0)(9) ;	-- SDA
ila7(0) <= trigout ;		-- from ASIC
ila8(0) <= sync40 ;		-- sync pulse at 40/2 MHz

--============= differential stuff =====================
fcmd_i : OBUFDS port map (i=>fcmd, o=>FCMD_P, ob =>FCMD_N) ;
cmdpulse_i : OBUFTDS port map (i=>cmdpulse, o=>CMDPULSE_P, ob =>CMDPULSE_N, t => regs_rw(0)(3)) ;

dout0_i : IBUFDS port map (i=>DOUT_P0  , ib=>DOUT_N0, o=>dout0) ;

-- EICROC0 or its Testboard have a bug where they swap N & P sides!
sdout_i : IBUFDS port map (i=>SDOUT_P, ib=>SDOUT_N, o=>sdout_tmp) ;
sdout <= sdout_tmp when (regs_rw(0)(5)='0') else (not sdout_tmp) ;

trigout_i : IBUFDS port map (i=>TRIGOUT_P, ib=>TRIGOUT_N, o=>trigout) ;

RESETB <= regs_rw(0)(0) ;	-- 0:reset: DEFAULT IS KEEP IN RESET
RST_I2C <= regs_rw(0)(1) ;	-- 0:reset: DEFAULT IS KEEP IN RESET
SEL_FCMD <= regs_rw(0)(2) ;	-- 0:disable FCMD logic


external_trigger <= PMOD_EXT ;

--===================================
--======== I2C ======================
--===================================

SCL <= regs_rw(0)(8) ;
SDA <= regs_rw(0)(9) when regs_rw(0)(10)='1' else 'Z' ;
regs_ro(0)(9) <= SDA ;



regs_ro(0)(0) <= dout0 ;
regs_ro(0)(1) <= sdout ;
regs_ro(0)(2) <= trigout ;

--=========== FIFO Status and Control

regs_ro(1)(0) <= data_avail ;
regs_ro(1)(1) <= fifo_empty ;
regs_ro(1)(2) <= fifo_full ;
regs_ro(1)(3) <= fifo_wr_rst_busy ;
regs_ro(1)(4) <= fifo_rd_rst_busy ;

fifo_srst <= regs_rw(1)(0) ;


word_aux_0 <= x"00" & B"0" & regs_rw(2)(3 downto 1) & B"0" & regs_rw(1)(10 downto 8)  & regs_rw(3) ;

--=======================================================
--========== Microblaze registers =======================
--=======================================================

regs_bl : block

type s_type is (S_IDLE, S_READ, S_WRITE, S_WAIT) ;

signal state	: s_type := S_IDLE ;

signal addr	: std_logic_vector(3 downto 0) := (others => '0') ;
signal data	: std_logic_vector(15 downto 0) := (others => '0') ;

begin

process(clk_160, gpio_in)
begin
	if(rising_edge(clk_160)) then

	fifo_rd_en <= '0' ;

	case(state) is
	when S_IDLE =>
		if(gpio_in(31)='1') then
			addr <= gpio_in(19 downto 16) ;
			if(gpio_in(30)='1') then
				data <= gpio_in(15 downto 0) ;
				state <= S_WRITE ;
			else
				state <= S_READ ;
			end if ;
		end if ;
	when S_WRITE =>
		case(addr) is
		when X"0" =>
			regs_rw(0) <= data ;
		when X"1" =>
			regs_rw(1) <= data ;
		when X"2" =>
			regs_rw(2) <= data ;
		when X"3" =>
			regs_rw(3) <= data ;
		when X"4" =>
			regs_rw(4) <= data ;
		when others =>
		end case ;

		state <= S_WAIT ;
	when S_READ =>
		case(addr) is
		when X"0" =>
			gpio_out <= regs_ro(0) & regs_rw(0) ;
		when X"1" =>
			gpio_out <= regs_ro(1) & regs_rw(1) ;
		when X"2" =>
			gpio_out <= regs_ro(2) & regs_rw(2) ;
		when X"3" =>
			gpio_out <= regs_ro(3) & regs_rw(3) ;
		when X"4" => 
			gpio_out <= X"00" & sdout_8b & regs_rw(4);
		when X"5" => 
			gpio_out <= X"0000_00" & dout_8b ;
		when X"6" =>
			gpio_out <= fifo_dout ;
			fifo_rd_en <= not fifo_empty ;
		when X"7" =>
			gpio_out <= X"0607_2026" ;	-- kinda version
		when others =>
			gpio_out <= x"DEAD_C0DE" ;
		end case ;

		state <= S_WAIT ;
	when S_WAIT =>
		if(gpio_in(31)='0') then
			state <= S_IDLE ;
		end if ;	
	end case ;
	end if ;
end process ;

end block ;


--============ Debugging -- ignore ======================

GPIO_LED_0_LS <= led(0) ;
GPIO_LED_1_LS <= led(1) ;
GPIO_LED_2_LS <= led(2) ;

led(0) <= GPIO_SW_C ; -- also trigger?



process(clk_40)
begin
        if(rising_edge(clk_40)) then
                if(clk_cou=40000000) then
                        clk_cou <= 0 ;
                else
                        clk_cou <= clk_cou + 1 ;
                end if ;
        end if ;
end process ;


process(clk_40)
begin
        if(rising_edge(clk_40)) then
                if(clk_cou<20000000) then
                        led(1) <= '0' ;
                else
                        led(1) <= '1' ;
                end if ;
        end if ;
end process ;

process(clk_40)
begin
        if(rising_edge(clk_40)) then
                if(clk_cou<10000000) then
                        led(2) <= '0' ;
                else
                        led(2) <= '1' ;
                end if ;
        end if ;
end process ;

--=========================================
--========== clock outputs to EICROC ======
--=========================================

clk_o_bl : block

signal clk_320_o        : std_logic ;
signal clk_160_o        : std_logic ;

begin

--======== HACK: the output clock is 40 MHz! Not 320! ==========
oddr_320 : oddre1
port map (
        C => clk_320,
        Q => clk_320_o,
        D1 => '1',
        D2 => '0',
        SR => '0'
) ;

obufds_320: obuftds
port map (
        O => CLK320_P,
        OB => CLK320_N,
        I => clk_320_o,
	T => regs_rw(0)(4)	-- tristate for EICROC0
) ;

oddr_160 : oddre1
port map (
        C => clk_160,
        Q => clk_160_o,
        D1 => '1',
        D2 => '0',
        SR => '0'
) ;

obufds_160: obuftds
port map (
        O => CLK160_P,
        OB => CLK160_N,
        I => clk_160_o,
	T => regs_rw(0)(3)	-- tri-state is a '1'
) ;

end block ; -------------------------------


--=========================================
--=========== External entities ===========
--=========================================

ila_0_i : entity work.ila_0
port map (
        clk => clk_320,

        probe0 => ila0,
        probe1 => ila1,
        probe2 => ila2,
        probe3 => ila3,
        probe4 => ila4,
        probe5 => ila5,
        probe6 => ila6,
        probe7 => ila7,
        probe8 => ila8
) ;

fcmd_ii : entity work.fcmd_drv
port map (
        clk320 => clk_320,

	sync40 => sync40,

        fcmd_i => fcmd_d,

        fcmd_o => fcmd	-- serialized output
) ;

deser_i	: entity work.deser
port map (
	clk_320	=> clk_320,

	sync40 => sync40,
	
	delay => regs_rw(1)(3 downto 1),

	sdout => sdout,
	dout => dout0,

	sdout_8b => sdout_8b,
	dout_8b => dout_8b 
) ;

daq_i : entity work.daq
port map (
	clk_320 => clk_320,
	clk_160 => clk_160,
	clk_40 => clk_40,

	go => regs_rw(2)(0),
	mode => regs_rw(2)(3 downto 1),

	len_en_acq	=> regs_rw(3)(7 downto 0),
	dly_cmdpulse	=> regs_rw(3)(15 downto 8),

	sync40	=> sync40,
	fcmd 	=> fcmd_d,
	data_avail => data_avail,

	rdout_type => regs_rw(1)(10 downto 8),
	external_trigger => external_trigger,

	start_acq => enable_acq,	-- to ASIC
	cmdpulse => cmdpulse,		-- to ASIC, diff
	trigext => trig_external,	-- to ASIC
	trigout => trigout,		-- from ASIC
	start_readout => start_rdout	-- to ASIC
) ;

fifo_i : entity work.fifo_generator_0
port map (
	srst => fifo_srst,

	wr_clk => clk_40,
	rd_clk => clk_160,

	din => fifo_din,
	wr_en => fifo_wr_en,
	rd_en => fifo_rd_en,
	dout => fifo_dout,
	full => fifo_full,
	empty => fifo_empty,
	wr_rst_busy => fifo_wr_rst_busy,
	rd_rst_busy => fifo_rd_rst_busy
) ;

rdout_i : entity work.rdout
port map (
	clk_40 => clk_40,

	data_avail 	=> data_avail,	-- in
	
	data8_in	=> sdout_8b,	-- in

	data32_out	=> fifo_din,	-- out

	wr_en		=> fifo_wr_en,	-- out

	words_req	=> regs_rw(4),
	asic_type	=> regs_rw(1)(7 downto 4),
	word_aux_0	=> word_aux_0
) ;

--====================================================================
--============== Microblaze stuff ====================================
--====================================================================
design_1_i: entity work.design_1
port map (

      diff_clock_rtl_0_clk_n => CLK_125_N,
      diff_clock_rtl_0_clk_p => CLK_125_P,

      clk_axi => clk_160,
      clk_out2_0 => clk_320,
      clk_out3_0 => clk_40,

      reset_rtl_0 => '0',	-- designs reset is active HIGH so no reset


      uart_rtl_0_rxd => USB_RX,
      uart_rtl_0_txd => USB_TX,

      gpio_rtl_0_tri_i => gpio_out,
      gpio_rtl_1_tri_o => gpio_in
) ;

end ;
