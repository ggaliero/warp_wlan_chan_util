--------------------------------------------------------------------------------
--    This file is owned and controlled by Xilinx and must be used solely     --
--    for design, simulation, implementation and creation of design files     --
--    limited to Xilinx devices or technologies. Use with non-Xilinx          --
--    devices or technologies is expressly prohibited and immediately         --
--    terminates your license.                                                --
--                                                                            --
--    XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" SOLELY    --
--    FOR USE IN DEVELOPING PROGRAMS AND SOLUTIONS FOR XILINX DEVICES.  BY    --
--    PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE             --
--    IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, XILINX IS      --
--    MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM ANY      --
--    CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING ANY       --
--    RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY       --
--    DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE   --
--    IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR          --
--    REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF         --
--    INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A   --
--    PARTICULAR PURPOSE.                                                     --
--                                                                            --
--    Xilinx products are not intended for use in life support appliances,    --
--    devices, or systems.  Use in such applications are expressly            --
--    prohibited.                                                             --
--                                                                            --
--    (c) Copyright 1995-2015 Xilinx, Inc.                                    --
--    All rights reserved.                                                    --
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- You must compile the wrapper file cntr_11_0_5e62871cb125c52e.vhd when simulating
-- the core, cntr_11_0_5e62871cb125c52e. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

-- The synthesis directives "translate_off/translate_on" specified
-- below are supported by Xilinx, Mentor Graphics and Synplicity
-- synthesis tools. Ensure they are correct for your synthesis tool(s).

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
-- synthesis translate_off
LIBRARY XilinxCoreLib;
-- synthesis translate_on
ENTITY cntr_11_0_5e62871cb125c52e IS
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    load : IN STD_LOGIC;
    l : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    q : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END cntr_11_0_5e62871cb125c52e;

ARCHITECTURE cntr_11_0_5e62871cb125c52e_a OF cntr_11_0_5e62871cb125c52e IS
-- synthesis translate_off
COMPONENT wrapped_cntr_11_0_5e62871cb125c52e
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    load : IN STD_LOGIC;
    l : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    q : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

-- Configuration specification
  FOR ALL : wrapped_cntr_11_0_5e62871cb125c52e USE ENTITY XilinxCoreLib.c_counter_binary_v11_0(behavioral)
    GENERIC MAP (
      c_ainit_val => "0",
      c_ce_overrides_sync => 0,
      c_count_by => "1",
      c_count_mode => 0,
      c_count_to => "1",
      c_fb_latency => 0,
      c_has_ce => 1,
      c_has_load => 1,
      c_has_sclr => 0,
      c_has_sinit => 1,
      c_has_sset => 0,
      c_has_thresh0 => 0,
      c_implementation => 0,
      c_latency => 1,
      c_load_low => 0,
      c_restrict_count => 0,
      c_sclr_overrides_sset => 1,
      c_sinit_val => "0",
      c_thresh0_value => "1",
      c_verbosity => 0,
      c_width => 64,
      c_xdevicefamily => "virtex6"
    );
-- synthesis translate_on
BEGIN
-- synthesis translate_off
U0 : wrapped_cntr_11_0_5e62871cb125c52e
  PORT MAP (
    clk => clk,
    ce => ce,
    sinit => sinit,
    load => load,
    l => l,
    q => q
  );
-- synthesis translate_on

END cntr_11_0_5e62871cb125c52e_a;
--------------------------------------------------------------------------------
--    This file is owned and controlled by Xilinx and must be used solely     --
--    for design, simulation, implementation and creation of design files     --
--    limited to Xilinx devices or technologies. Use with non-Xilinx          --
--    devices or technologies is expressly prohibited and immediately         --
--    terminates your license.                                                --
--                                                                            --
--    XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" SOLELY    --
--    FOR USE IN DEVELOPING PROGRAMS AND SOLUTIONS FOR XILINX DEVICES.  BY    --
--    PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE             --
--    IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, XILINX IS      --
--    MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM ANY      --
--    CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING ANY       --
--    RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY       --
--    DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE   --
--    IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR          --
--    REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF         --
--    INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A   --
--    PARTICULAR PURPOSE.                                                     --
--                                                                            --
--    Xilinx products are not intended for use in life support appliances,    --
--    devices, or systems.  Use in such applications are expressly            --
--    prohibited.                                                             --
--                                                                            --
--    (c) Copyright 1995-2015 Xilinx, Inc.                                    --
--    All rights reserved.                                                    --
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- You must compile the wrapper file cntr_11_0_78c1092db561b4bc.vhd when simulating
-- the core, cntr_11_0_78c1092db561b4bc. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

-- The synthesis directives "translate_off/translate_on" specified
-- below are supported by Xilinx, Mentor Graphics and Synplicity
-- synthesis tools. Ensure they are correct for your synthesis tool(s).

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
-- synthesis translate_off
LIBRARY XilinxCoreLib;
-- synthesis translate_on
ENTITY cntr_11_0_78c1092db561b4bc IS
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    q : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END cntr_11_0_78c1092db561b4bc;

ARCHITECTURE cntr_11_0_78c1092db561b4bc_a OF cntr_11_0_78c1092db561b4bc IS
-- synthesis translate_off
COMPONENT wrapped_cntr_11_0_78c1092db561b4bc
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    q : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

-- Configuration specification
  FOR ALL : wrapped_cntr_11_0_78c1092db561b4bc USE ENTITY XilinxCoreLib.c_counter_binary_v11_0(behavioral)
    GENERIC MAP (
      c_ainit_val => "0",
      c_ce_overrides_sync => 0,
      c_count_by => "1",
      c_count_mode => 0,
      c_count_to => "1",
      c_fb_latency => 0,
      c_has_ce => 1,
      c_has_load => 0,
      c_has_sclr => 0,
      c_has_sinit => 1,
      c_has_sset => 0,
      c_has_thresh0 => 0,
      c_implementation => 0,
      c_latency => 1,
      c_load_low => 0,
      c_restrict_count => 0,
      c_sclr_overrides_sset => 1,
      c_sinit_val => "0",
      c_thresh0_value => "1",
      c_verbosity => 0,
      c_width => 64,
      c_xdevicefamily => "virtex6"
    );
-- synthesis translate_on
BEGIN
-- synthesis translate_off
U0 : wrapped_cntr_11_0_78c1092db561b4bc
  PORT MAP (
    clk => clk,
    ce => ce,
    sinit => sinit,
    q => q
  );
-- synthesis translate_on

END cntr_11_0_78c1092db561b4bc_a;
--------------------------------------------------------------------------------
--    This file is owned and controlled by Xilinx and must be used solely     --
--    for design, simulation, implementation and creation of design files     --
--    limited to Xilinx devices or technologies. Use with non-Xilinx          --
--    devices or technologies is expressly prohibited and immediately         --
--    terminates your license.                                                --
--                                                                            --
--    XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" SOLELY    --
--    FOR USE IN DEVELOPING PROGRAMS AND SOLUTIONS FOR XILINX DEVICES.  BY    --
--    PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE             --
--    IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, XILINX IS      --
--    MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM ANY      --
--    CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING ANY       --
--    RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY       --
--    DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE   --
--    IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR          --
--    REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF         --
--    INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A   --
--    PARTICULAR PURPOSE.                                                     --
--                                                                            --
--    Xilinx products are not intended for use in life support appliances,    --
--    devices, or systems.  Use in such applications are expressly            --
--    prohibited.                                                             --
--                                                                            --
--    (c) Copyright 1995-2015 Xilinx, Inc.                                    --
--    All rights reserved.                                                    --
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- You must compile the wrapper file cntr_11_0_86806e294f737f4c.vhd when simulating
-- the core, cntr_11_0_86806e294f737f4c. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

-- The synthesis directives "translate_off/translate_on" specified
-- below are supported by Xilinx, Mentor Graphics and Synplicity
-- synthesis tools. Ensure they are correct for your synthesis tool(s).

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
-- synthesis translate_off
LIBRARY XilinxCoreLib;
-- synthesis translate_on
ENTITY cntr_11_0_86806e294f737f4c IS
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    q : OUT STD_LOGIC_VECTOR(7 DOWNTO 0)
  );
END cntr_11_0_86806e294f737f4c;

ARCHITECTURE cntr_11_0_86806e294f737f4c_a OF cntr_11_0_86806e294f737f4c IS
-- synthesis translate_off
COMPONENT wrapped_cntr_11_0_86806e294f737f4c
  PORT (
    clk : IN STD_LOGIC;
    ce : IN STD_LOGIC;
    sinit : IN STD_LOGIC;
    q : OUT STD_LOGIC_VECTOR(7 DOWNTO 0)
  );
END COMPONENT;

-- Configuration specification
  FOR ALL : wrapped_cntr_11_0_86806e294f737f4c USE ENTITY XilinxCoreLib.c_counter_binary_v11_0(behavioral)
    GENERIC MAP (
      c_ainit_val => "0",
      c_ce_overrides_sync => 0,
      c_count_by => "1",
      c_count_mode => 0,
      c_count_to => "1",
      c_fb_latency => 0,
      c_has_ce => 1,
      c_has_load => 0,
      c_has_sclr => 0,
      c_has_sinit => 1,
      c_has_sset => 0,
      c_has_thresh0 => 0,
      c_implementation => 0,
      c_latency => 1,
      c_load_low => 0,
      c_restrict_count => 0,
      c_sclr_overrides_sset => 1,
      c_sinit_val => "0",
      c_thresh0_value => "1",
      c_verbosity => 0,
      c_width => 8,
      c_xdevicefamily => "virtex6"
    );
-- synthesis translate_on
BEGIN
-- synthesis translate_off
U0 : wrapped_cntr_11_0_86806e294f737f4c
  PORT MAP (
    clk => clk,
    ce => ce,
    sinit => sinit,
    q => q
  );
-- synthesis translate_on

END cntr_11_0_86806e294f737f4c_a;

-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
package conv_pkg is
    constant simulating : boolean := false
      -- synopsys translate_off
        or true
      -- synopsys translate_on
    ;
    constant xlUnsigned : integer := 1;
    constant xlSigned : integer := 2;
    constant xlFloat : integer := 3;
    constant xlWrap : integer := 1;
    constant xlSaturate : integer := 2;
    constant xlTruncate : integer := 1;
    constant xlRound : integer := 2;
    constant xlRoundBanker : integer := 3;
    constant xlAddMode : integer := 1;
    constant xlSubMode : integer := 2;
    attribute black_box : boolean;
    attribute syn_black_box : boolean;
    attribute fpga_dont_touch: string;
    attribute box_type :  string;
    attribute keep : string;
    attribute syn_keep : boolean;
    function std_logic_vector_to_unsigned(inp : std_logic_vector) return unsigned;
    function unsigned_to_std_logic_vector(inp : unsigned) return std_logic_vector;
    function std_logic_vector_to_signed(inp : std_logic_vector) return signed;
    function signed_to_std_logic_vector(inp : signed) return std_logic_vector;
    function unsigned_to_signed(inp : unsigned) return signed;
    function signed_to_unsigned(inp : signed) return unsigned;
    function pos(inp : std_logic_vector; arith : INTEGER) return boolean;
    function all_same(inp: std_logic_vector) return boolean;
    function all_zeros(inp: std_logic_vector) return boolean;
    function is_point_five(inp: std_logic_vector) return boolean;
    function all_ones(inp: std_logic_vector) return boolean;
    function convert_type (inp : std_logic_vector; old_width, old_bin_pt,
                           old_arith, new_width, new_bin_pt, new_arith,
                           quantization, overflow : INTEGER)
        return std_logic_vector;
    function cast (inp : std_logic_vector; old_bin_pt,
                   new_width, new_bin_pt, new_arith : INTEGER)
        return std_logic_vector;
    function shift_division_result(quotient, fraction: std_logic_vector;
                                   fraction_width, shift_value, shift_dir: INTEGER)
        return std_logic_vector;
    function shift_op (inp: std_logic_vector;
                       result_width, shift_value, shift_dir: INTEGER)
        return std_logic_vector;
    function vec_slice (inp : std_logic_vector; upper, lower : INTEGER)
        return std_logic_vector;
    function s2u_slice (inp : signed; upper, lower : INTEGER)
        return unsigned;
    function u2u_slice (inp : unsigned; upper, lower : INTEGER)
        return unsigned;
    function s2s_cast (inp : signed; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return signed;
    function u2s_cast (inp : unsigned; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return signed;
    function s2u_cast (inp : signed; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return unsigned;
    function u2u_cast (inp : unsigned; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return unsigned;
    function u2v_cast (inp : unsigned; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return std_logic_vector;
    function s2v_cast (inp : signed; old_bin_pt,
                   new_width, new_bin_pt : INTEGER)
        return std_logic_vector;
    function trunc (inp : std_logic_vector; old_width, old_bin_pt, old_arith,
                    new_width, new_bin_pt, new_arith : INTEGER)
        return std_logic_vector;
    function round_towards_inf (inp : std_logic_vector; old_width, old_bin_pt,
                                old_arith, new_width, new_bin_pt,
                                new_arith : INTEGER) return std_logic_vector;
    function round_towards_even (inp : std_logic_vector; old_width, old_bin_pt,
                                old_arith, new_width, new_bin_pt,
                                new_arith : INTEGER) return std_logic_vector;
    function max_signed(width : INTEGER) return std_logic_vector;
    function min_signed(width : INTEGER) return std_logic_vector;
    function saturation_arith(inp:  std_logic_vector;  old_width, old_bin_pt,
                              old_arith, new_width, new_bin_pt, new_arith
                              : INTEGER) return std_logic_vector;
    function wrap_arith(inp:  std_logic_vector;  old_width, old_bin_pt,
                        old_arith, new_width, new_bin_pt, new_arith : INTEGER)
                        return std_logic_vector;
    function fractional_bits(a_bin_pt, b_bin_pt: INTEGER) return INTEGER;
    function integer_bits(a_width, a_bin_pt, b_width, b_bin_pt: INTEGER)
        return INTEGER;
    function sign_ext(inp : std_logic_vector; new_width : INTEGER)
        return std_logic_vector;
    function zero_ext(inp : std_logic_vector; new_width : INTEGER)
        return std_logic_vector;
    function zero_ext(inp : std_logic; new_width : INTEGER)
        return std_logic_vector;
    function extend_MSB(inp : std_logic_vector; new_width, arith : INTEGER)
        return std_logic_vector;
    function align_input(inp : std_logic_vector; old_width, delta, new_arith,
                          new_width: INTEGER)
        return std_logic_vector;
    function pad_LSB(inp : std_logic_vector; new_width: integer)
        return std_logic_vector;
    function pad_LSB(inp : std_logic_vector; new_width, arith : integer)
        return std_logic_vector;
    function max(L, R: INTEGER) return INTEGER;
    function min(L, R: INTEGER) return INTEGER;
    function "="(left,right: STRING) return boolean;
    function boolean_to_signed (inp : boolean; width: integer)
        return signed;
    function boolean_to_unsigned (inp : boolean; width: integer)
        return unsigned;
    function boolean_to_vector (inp : boolean)
        return std_logic_vector;
    function std_logic_to_vector (inp : std_logic)
        return std_logic_vector;
    function integer_to_std_logic_vector (inp : integer;  width, arith : integer)
        return std_logic_vector;
    function std_logic_vector_to_integer (inp : std_logic_vector;  arith : integer)
        return integer;
    function std_logic_to_integer(constant inp : std_logic := '0')
        return integer;
    function bin_string_element_to_std_logic_vector (inp : string;  width, index : integer)
        return std_logic_vector;
    function bin_string_to_std_logic_vector (inp : string)
        return std_logic_vector;
    function hex_string_to_std_logic_vector (inp : string; width : integer)
        return std_logic_vector;
    function makeZeroBinStr (width : integer) return STRING;
    function and_reduce(inp: std_logic_vector) return std_logic;
    -- synopsys translate_off
    function is_binary_string_invalid (inp : string)
        return boolean;
    function is_binary_string_undefined (inp : string)
        return boolean;
    function is_XorU(inp : std_logic_vector)
        return boolean;
    function to_real(inp : std_logic_vector; bin_pt : integer; arith : integer)
        return real;
    function std_logic_to_real(inp : std_logic; bin_pt : integer; arith : integer)
        return real;
    function real_to_std_logic_vector (inp : real;  width, bin_pt, arith : integer)
        return std_logic_vector;
    function real_string_to_std_logic_vector (inp : string;  width, bin_pt, arith : integer)
        return std_logic_vector;
    constant display_precision : integer := 20;
    function real_to_string (inp : real) return string;
    function valid_bin_string(inp : string) return boolean;
    function std_logic_vector_to_bin_string(inp : std_logic_vector) return string;
    function std_logic_to_bin_string(inp : std_logic) return string;
    function std_logic_vector_to_bin_string_w_point(inp : std_logic_vector; bin_pt : integer)
        return string;
    function real_to_bin_string(inp : real;  width, bin_pt, arith : integer)
        return string;
    type stdlogic_to_char_t is array(std_logic) of character;
    constant to_char : stdlogic_to_char_t := (
        'U' => 'U',
        'X' => 'X',
        '0' => '0',
        '1' => '1',
        'Z' => 'Z',
        'W' => 'W',
        'L' => 'L',
        'H' => 'H',
        '-' => '-');
    -- synopsys translate_on
end conv_pkg;
package body conv_pkg is
    function std_logic_vector_to_unsigned(inp : std_logic_vector)
        return unsigned
    is
    begin
        return unsigned (inp);
    end;
    function unsigned_to_std_logic_vector(inp : unsigned)
        return std_logic_vector
    is
    begin
        return std_logic_vector(inp);
    end;
    function std_logic_vector_to_signed(inp : std_logic_vector)
        return signed
    is
    begin
        return  signed (inp);
    end;
    function signed_to_std_logic_vector(inp : signed)
        return std_logic_vector
    is
    begin
        return std_logic_vector(inp);
    end;
    function unsigned_to_signed (inp : unsigned)
        return signed
    is
    begin
        return signed(std_logic_vector(inp));
    end;
    function signed_to_unsigned (inp : signed)
        return unsigned
    is
    begin
        return unsigned(std_logic_vector(inp));
    end;
    function pos(inp : std_logic_vector; arith : INTEGER)
        return boolean
    is
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
    begin
        vec := inp;
        if arith = xlUnsigned then
            return true;
        else
            if vec(width-1) = '0' then
                return true;
            else
                return false;
            end if;
        end if;
        return true;
    end;
    function max_signed(width : INTEGER)
        return std_logic_vector
    is
        variable ones : std_logic_vector(width-2 downto 0);
        variable result : std_logic_vector(width-1 downto 0);
    begin
        ones := (others => '1');
        result(width-1) := '0';
        result(width-2 downto 0) := ones;
        return result;
    end;
    function min_signed(width : INTEGER)
        return std_logic_vector
    is
        variable zeros : std_logic_vector(width-2 downto 0);
        variable result : std_logic_vector(width-1 downto 0);
    begin
        zeros := (others => '0');
        result(width-1) := '1';
        result(width-2 downto 0) := zeros;
        return result;
    end;
    function and_reduce(inp: std_logic_vector) return std_logic
    is
        variable result: std_logic;
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
    begin
        vec := inp;
        result := vec(0);
        if width > 1 then
            for i in 1 to width-1 loop
                result := result and vec(i);
            end loop;
        end if;
        return result;
    end;
    function all_same(inp: std_logic_vector) return boolean
    is
        variable result: boolean;
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
    begin
        vec := inp;
        result := true;
        if width > 0 then
            for i in 1 to width-1 loop
                if vec(i) /= vec(0) then
                    result := false;
                end if;
            end loop;
        end if;
        return result;
    end;
    function all_zeros(inp: std_logic_vector)
        return boolean
    is
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
        variable zero : std_logic_vector(width-1 downto 0);
        variable result : boolean;
    begin
        zero := (others => '0');
        vec := inp;
        -- synopsys translate_off
        if (is_XorU(vec)) then
            return false;
        end if;
         -- synopsys translate_on
        if (std_logic_vector_to_unsigned(vec) = std_logic_vector_to_unsigned(zero)) then
            result := true;
        else
            result := false;
        end if;
        return result;
    end;
    function is_point_five(inp: std_logic_vector)
        return boolean
    is
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
        variable result : boolean;
    begin
        vec := inp;
        -- synopsys translate_off
        if (is_XorU(vec)) then
            return false;
        end if;
         -- synopsys translate_on
        if (width > 1) then
           if ((vec(width-1) = '1') and (all_zeros(vec(width-2 downto 0)) = true)) then
               result := true;
           else
               result := false;
           end if;
        else
           if (vec(width-1) = '1') then
               result := true;
           else
               result := false;
           end if;
        end if;
        return result;
    end;
    function all_ones(inp: std_logic_vector)
        return boolean
    is
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
        variable one : std_logic_vector(width-1 downto 0);
        variable result : boolean;
    begin
        one := (others => '1');
        vec := inp;
        -- synopsys translate_off
        if (is_XorU(vec)) then
            return false;
        end if;
         -- synopsys translate_on
        if (std_logic_vector_to_unsigned(vec) = std_logic_vector_to_unsigned(one)) then
            result := true;
        else
            result := false;
        end if;
        return result;
    end;
    function full_precision_num_width(quantization, overflow, old_width,
                                      old_bin_pt, old_arith,
                                      new_width, new_bin_pt, new_arith : INTEGER)
        return integer
    is
        variable result : integer;
    begin
        result := old_width + 2;
        return result;
    end;
    function quantized_num_width(quantization, overflow, old_width, old_bin_pt,
                                 old_arith, new_width, new_bin_pt, new_arith
                                 : INTEGER)
        return integer
    is
        variable right_of_dp, left_of_dp, result : integer;
    begin
        right_of_dp := max(new_bin_pt, old_bin_pt);
        left_of_dp := max((new_width - new_bin_pt), (old_width - old_bin_pt));
        result := (old_width + 2) + (new_bin_pt - old_bin_pt);
        return result;
    end;
    function convert_type (inp : std_logic_vector; old_width, old_bin_pt,
                           old_arith, new_width, new_bin_pt, new_arith,
                           quantization, overflow : INTEGER)
        return std_logic_vector
    is
        constant fp_width : integer :=
            full_precision_num_width(quantization, overflow, old_width,
                                     old_bin_pt, old_arith, new_width,
                                     new_bin_pt, new_arith);
        constant fp_bin_pt : integer := old_bin_pt;
        constant fp_arith : integer := old_arith;
        variable full_precision_result : std_logic_vector(fp_width-1 downto 0);
        constant q_width : integer :=
            quantized_num_width(quantization, overflow, old_width, old_bin_pt,
                                old_arith, new_width, new_bin_pt, new_arith);
        constant q_bin_pt : integer := new_bin_pt;
        constant q_arith : integer := old_arith;
        variable quantized_result : std_logic_vector(q_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        result := (others => '0');
        full_precision_result := cast(inp, old_bin_pt, fp_width, fp_bin_pt,
                                      fp_arith);
        if (quantization = xlRound) then
            quantized_result := round_towards_inf(full_precision_result,
                                                  fp_width, fp_bin_pt,
                                                  fp_arith, q_width, q_bin_pt,
                                                  q_arith);
        elsif (quantization = xlRoundBanker) then
            quantized_result := round_towards_even(full_precision_result,
                                                  fp_width, fp_bin_pt,
                                                  fp_arith, q_width, q_bin_pt,
                                                  q_arith);
        else
            quantized_result := trunc(full_precision_result, fp_width, fp_bin_pt,
                                      fp_arith, q_width, q_bin_pt, q_arith);
        end if;
        if (overflow = xlSaturate) then
            result := saturation_arith(quantized_result, q_width, q_bin_pt,
                                       q_arith, new_width, new_bin_pt, new_arith);
        else
             result := wrap_arith(quantized_result, q_width, q_bin_pt, q_arith,
                                  new_width, new_bin_pt, new_arith);
        end if;
        return result;
    end;
    function cast (inp : std_logic_vector; old_bin_pt, new_width,
                   new_bin_pt, new_arith : INTEGER)
        return std_logic_vector
    is
        constant old_width : integer := inp'length;
        constant left_of_dp : integer := (new_width - new_bin_pt)
                                         - (old_width - old_bin_pt);
        constant right_of_dp : integer := (new_bin_pt - old_bin_pt);
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
        variable j   : integer;
    begin
        vec := inp;
        for i in new_width-1 downto 0 loop
            j := i - right_of_dp;
            if ( j > old_width-1) then
                if (new_arith = xlUnsigned) then
                    result(i) := '0';
                else
                    result(i) := vec(old_width-1);
                end if;
            elsif ( j >= 0) then
                result(i) := vec(j);
            else
                result(i) := '0';
            end if;
        end loop;
        return result;
    end;
    function shift_division_result(quotient, fraction: std_logic_vector;
                                   fraction_width, shift_value, shift_dir: INTEGER)
        return std_logic_vector
    is
        constant q_width : integer := quotient'length;
        constant f_width : integer := fraction'length;
        constant vec_MSB : integer := q_width+f_width-1;
        constant result_MSB : integer := q_width+fraction_width-1;
        constant result_LSB : integer := vec_MSB-result_MSB;
        variable vec : std_logic_vector(vec_MSB downto 0);
        variable result : std_logic_vector(result_MSB downto 0);
    begin
        vec := ( quotient & fraction );
        if shift_dir = 1 then
            for i in vec_MSB downto 0 loop
                if (i < shift_value) then
                     vec(i) := '0';
                else
                    vec(i) := vec(i-shift_value);
                end if;
            end loop;
        else
            for i in 0 to vec_MSB loop
                if (i > vec_MSB-shift_value) then
                    vec(i) := vec(vec_MSB);
                else
                    vec(i) := vec(i+shift_value);
                end if;
            end loop;
        end if;
        result := vec(vec_MSB downto result_LSB);
        return result;
    end;
    function shift_op (inp: std_logic_vector;
                       result_width, shift_value, shift_dir: INTEGER)
        return std_logic_vector
    is
        constant inp_width : integer := inp'length;
        constant vec_MSB : integer := inp_width-1;
        constant result_MSB : integer := result_width-1;
        constant result_LSB : integer := vec_MSB-result_MSB;
        variable vec : std_logic_vector(vec_MSB downto 0);
        variable result : std_logic_vector(result_MSB downto 0);
    begin
        vec := inp;
        if shift_dir = 1 then
            for i in vec_MSB downto 0 loop
                if (i < shift_value) then
                     vec(i) := '0';
                else
                    vec(i) := vec(i-shift_value);
                end if;
            end loop;
        else
            for i in 0 to vec_MSB loop
                if (i > vec_MSB-shift_value) then
                    vec(i) := vec(vec_MSB);
                else
                    vec(i) := vec(i+shift_value);
                end if;
            end loop;
        end if;
        result := vec(vec_MSB downto result_LSB);
        return result;
    end;
    function vec_slice (inp : std_logic_vector; upper, lower : INTEGER)
      return std_logic_vector
    is
    begin
        return inp(upper downto lower);
    end;
    function s2u_slice (inp : signed; upper, lower : INTEGER)
      return unsigned
    is
    begin
        return unsigned(vec_slice(std_logic_vector(inp), upper, lower));
    end;
    function u2u_slice (inp : unsigned; upper, lower : INTEGER)
      return unsigned
    is
    begin
        return unsigned(vec_slice(std_logic_vector(inp), upper, lower));
    end;
    function s2s_cast (inp : signed; old_bin_pt, new_width, new_bin_pt : INTEGER)
        return signed
    is
    begin
        return signed(cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlSigned));
    end;
    function s2u_cast (inp : signed; old_bin_pt, new_width,
                   new_bin_pt : INTEGER)
        return unsigned
    is
    begin
        return unsigned(cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlSigned));
    end;
    function u2s_cast (inp : unsigned; old_bin_pt, new_width,
                   new_bin_pt : INTEGER)
        return signed
    is
    begin
        return signed(cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlUnsigned));
    end;
    function u2u_cast (inp : unsigned; old_bin_pt, new_width,
                   new_bin_pt : INTEGER)
        return unsigned
    is
    begin
        return unsigned(cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlUnsigned));
    end;
    function u2v_cast (inp : unsigned; old_bin_pt, new_width,
                   new_bin_pt : INTEGER)
        return std_logic_vector
    is
    begin
        return cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlUnsigned);
    end;
    function s2v_cast (inp : signed; old_bin_pt, new_width,
                   new_bin_pt : INTEGER)
        return std_logic_vector
    is
    begin
        return cast(std_logic_vector(inp), old_bin_pt, new_width, new_bin_pt, xlSigned);
    end;
    function boolean_to_signed (inp : boolean; width : integer)
        return signed
    is
        variable result : signed(width - 1 downto 0);
    begin
        result := (others => '0');
        if inp then
          result(0) := '1';
        else
          result(0) := '0';
        end if;
        return result;
    end;
    function boolean_to_unsigned (inp : boolean; width : integer)
        return unsigned
    is
        variable result : unsigned(width - 1 downto 0);
    begin
        result := (others => '0');
        if inp then
          result(0) := '1';
        else
          result(0) := '0';
        end if;
        return result;
    end;
    function boolean_to_vector (inp : boolean)
        return std_logic_vector
    is
        variable result : std_logic_vector(1 - 1 downto 0);
    begin
        result := (others => '0');
        if inp then
          result(0) := '1';
        else
          result(0) := '0';
        end if;
        return result;
    end;
    function std_logic_to_vector (inp : std_logic)
        return std_logic_vector
    is
        variable result : std_logic_vector(1 - 1 downto 0);
    begin
        result(0) := inp;
        return result;
    end;
    function trunc (inp : std_logic_vector; old_width, old_bin_pt, old_arith,
                                new_width, new_bin_pt, new_arith : INTEGER)
        return std_logic_vector
    is
        constant right_of_dp : integer := (old_bin_pt - new_bin_pt);
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if right_of_dp >= 0 then
            if new_arith = xlUnsigned then
                result := zero_ext(vec(old_width-1 downto right_of_dp), new_width);
            else
                result := sign_ext(vec(old_width-1 downto right_of_dp), new_width);
            end if;
        else
            if new_arith = xlUnsigned then
                result := zero_ext(pad_LSB(vec, old_width +
                                           abs(right_of_dp)), new_width);
            else
                result := sign_ext(pad_LSB(vec, old_width +
                                           abs(right_of_dp)), new_width);
            end if;
        end if;
        return result;
    end;
    function round_towards_inf (inp : std_logic_vector; old_width, old_bin_pt,
                                old_arith, new_width, new_bin_pt, new_arith
                                : INTEGER)
        return std_logic_vector
    is
        constant right_of_dp : integer := (old_bin_pt - new_bin_pt);
        constant expected_new_width : integer :=  old_width - right_of_dp  + 1;
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable one_or_zero : std_logic_vector(new_width-1 downto 0);
        variable truncated_val : std_logic_vector(new_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if right_of_dp >= 0 then
            if new_arith = xlUnsigned then
                truncated_val := zero_ext(vec(old_width-1 downto right_of_dp),
                                          new_width);
            else
                truncated_val := sign_ext(vec(old_width-1 downto right_of_dp),
                                          new_width);
            end if;
        else
            if new_arith = xlUnsigned then
                truncated_val := zero_ext(pad_LSB(vec, old_width +
                                                  abs(right_of_dp)), new_width);
            else
                truncated_val := sign_ext(pad_LSB(vec, old_width +
                                                  abs(right_of_dp)), new_width);
            end if;
        end if;
        one_or_zero := (others => '0');
        if (new_arith = xlSigned) then
            if (vec(old_width-1) = '0') then
                one_or_zero(0) := '1';
            end if;
            if (right_of_dp >= 2) and (right_of_dp <= old_width) then
                if (all_zeros(vec(right_of_dp-2 downto 0)) = false) then
                    one_or_zero(0) := '1';
                end if;
            end if;
            if (right_of_dp >= 1) and (right_of_dp <= old_width) then
                if vec(right_of_dp-1) = '0' then
                    one_or_zero(0) := '0';
                end if;
            else
                one_or_zero(0) := '0';
            end if;
        else
            if (right_of_dp >= 1) and (right_of_dp <= old_width) then
                one_or_zero(0) :=  vec(right_of_dp-1);
            end if;
        end if;
        if new_arith = xlSigned then
            result := signed_to_std_logic_vector(std_logic_vector_to_signed(truncated_val) +
                                                 std_logic_vector_to_signed(one_or_zero));
        else
            result := unsigned_to_std_logic_vector(std_logic_vector_to_unsigned(truncated_val) +
                                                  std_logic_vector_to_unsigned(one_or_zero));
        end if;
        return result;
    end;
    function round_towards_even (inp : std_logic_vector; old_width, old_bin_pt,
                                old_arith, new_width, new_bin_pt, new_arith
                                : INTEGER)
        return std_logic_vector
    is
        constant right_of_dp : integer := (old_bin_pt - new_bin_pt);
        constant expected_new_width : integer :=  old_width - right_of_dp  + 1;
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable one_or_zero : std_logic_vector(new_width-1 downto 0);
        variable truncated_val : std_logic_vector(new_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if right_of_dp >= 0 then
            if new_arith = xlUnsigned then
                truncated_val := zero_ext(vec(old_width-1 downto right_of_dp),
                                          new_width);
            else
                truncated_val := sign_ext(vec(old_width-1 downto right_of_dp),
                                          new_width);
            end if;
        else
            if new_arith = xlUnsigned then
                truncated_val := zero_ext(pad_LSB(vec, old_width +
                                                  abs(right_of_dp)), new_width);
            else
                truncated_val := sign_ext(pad_LSB(vec, old_width +
                                                  abs(right_of_dp)), new_width);
            end if;
        end if;
        one_or_zero := (others => '0');
        if (right_of_dp >= 1) and (right_of_dp <= old_width) then
            if (is_point_five(vec(right_of_dp-1 downto 0)) = false) then
                one_or_zero(0) :=  vec(right_of_dp-1);
            else
                one_or_zero(0) :=  vec(right_of_dp);
            end if;
        end if;
        if new_arith = xlSigned then
            result := signed_to_std_logic_vector(std_logic_vector_to_signed(truncated_val) +
                                                 std_logic_vector_to_signed(one_or_zero));
        else
            result := unsigned_to_std_logic_vector(std_logic_vector_to_unsigned(truncated_val) +
                                                  std_logic_vector_to_unsigned(one_or_zero));
        end if;
        return result;
    end;
    function saturation_arith(inp:  std_logic_vector;  old_width, old_bin_pt,
                              old_arith, new_width, new_bin_pt, new_arith
                              : INTEGER)
        return std_logic_vector
    is
        constant left_of_dp : integer := (old_width - old_bin_pt) -
                                         (new_width - new_bin_pt);
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
        variable overflow : boolean;
    begin
        vec := inp;
        overflow := true;
        result := (others => '0');
        if (new_width >= old_width) then
            overflow := false;
        end if;
        if ((old_arith = xlSigned and new_arith = xlSigned) and (old_width > new_width)) then
            if all_same(vec(old_width-1 downto new_width-1)) then
                overflow := false;
            end if;
        end if;
        if (old_arith = xlSigned and new_arith = xlUnsigned) then
            if (old_width > new_width) then
                if all_zeros(vec(old_width-1 downto new_width)) then
                    overflow := false;
                end if;
            else
                if (old_width = new_width) then
                    if (vec(new_width-1) = '0') then
                        overflow := false;
                    end if;
                end if;
            end if;
        end if;
        if (old_arith = xlUnsigned and new_arith = xlUnsigned) then
            if (old_width > new_width) then
                if all_zeros(vec(old_width-1 downto new_width)) then
                    overflow := false;
                end if;
            else
                if (old_width = new_width) then
                    overflow := false;
                end if;
            end if;
        end if;
        if ((old_arith = xlUnsigned and new_arith = xlSigned) and (old_width > new_width)) then
            if all_same(vec(old_width-1 downto new_width-1)) then
                overflow := false;
            end if;
        end if;
        if overflow then
            if new_arith = xlSigned then
                if vec(old_width-1) = '0' then
                    result := max_signed(new_width);
                else
                    result := min_signed(new_width);
                end if;
            else
                if ((old_arith = xlSigned) and vec(old_width-1) = '1') then
                    result := (others => '0');
                else
                    result := (others => '1');
                end if;
            end if;
        else
            if (old_arith = xlSigned) and (new_arith = xlUnsigned) then
                if (vec(old_width-1) = '1') then
                    vec := (others => '0');
                end if;
            end if;
            if new_width <= old_width then
                result := vec(new_width-1 downto 0);
            else
                if new_arith = xlUnsigned then
                    result := zero_ext(vec, new_width);
                else
                    result := sign_ext(vec, new_width);
                end if;
            end if;
        end if;
        return result;
    end;
   function wrap_arith(inp:  std_logic_vector;  old_width, old_bin_pt,
                       old_arith, new_width, new_bin_pt, new_arith : INTEGER)
        return std_logic_vector
    is
        variable result : std_logic_vector(new_width-1 downto 0);
        variable result_arith : integer;
    begin
        if (old_arith = xlSigned) and (new_arith = xlUnsigned) then
            result_arith := xlSigned;
        end if;
        result := cast(inp, old_bin_pt, new_width, new_bin_pt, result_arith);
        return result;
    end;
    function fractional_bits(a_bin_pt, b_bin_pt: INTEGER) return INTEGER is
    begin
        return max(a_bin_pt, b_bin_pt);
    end;
    function integer_bits(a_width, a_bin_pt, b_width, b_bin_pt: INTEGER)
        return INTEGER is
    begin
        return  max(a_width - a_bin_pt, b_width - b_bin_pt);
    end;
    function pad_LSB(inp : std_logic_vector; new_width: integer)
        return STD_LOGIC_VECTOR
    is
        constant orig_width : integer := inp'length;
        variable vec : std_logic_vector(orig_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
        variable pos : integer;
        constant pad_pos : integer := new_width - orig_width - 1;
    begin
        vec := inp;
        pos := new_width-1;
        if (new_width >= orig_width) then
            for i in orig_width-1 downto 0 loop
                result(pos) := vec(i);
                pos := pos - 1;
            end loop;
            if pad_pos >= 0 then
                for i in pad_pos downto 0 loop
                    result(i) := '0';
                end loop;
            end if;
        end if;
        return result;
    end;
    function sign_ext(inp : std_logic_vector; new_width : INTEGER)
        return std_logic_vector
    is
        constant old_width : integer := inp'length;
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if new_width >= old_width then
            result(old_width-1 downto 0) := vec;
            if new_width-1 >= old_width then
                for i in new_width-1 downto old_width loop
                    result(i) := vec(old_width-1);
                end loop;
            end if;
        else
            result(new_width-1 downto 0) := vec(new_width-1 downto 0);
        end if;
        return result;
    end;
    function zero_ext(inp : std_logic_vector; new_width : INTEGER)
        return std_logic_vector
    is
        constant old_width : integer := inp'length;
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if new_width >= old_width then
            result(old_width-1 downto 0) := vec;
            if new_width-1 >= old_width then
                for i in new_width-1 downto old_width loop
                    result(i) := '0';
                end loop;
            end if;
        else
            result(new_width-1 downto 0) := vec(new_width-1 downto 0);
        end if;
        return result;
    end;
    function zero_ext(inp : std_logic; new_width : INTEGER)
        return std_logic_vector
    is
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        result(0) := inp;
        for i in new_width-1 downto 1 loop
            result(i) := '0';
        end loop;
        return result;
    end;
    function extend_MSB(inp : std_logic_vector; new_width, arith : INTEGER)
        return std_logic_vector
    is
        constant orig_width : integer := inp'length;
        variable vec : std_logic_vector(orig_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if arith = xlUnsigned then
            result := zero_ext(vec, new_width);
        else
            result := sign_ext(vec, new_width);
        end if;
        return result;
    end;
    function pad_LSB(inp : std_logic_vector; new_width, arith: integer)
        return STD_LOGIC_VECTOR
    is
        constant orig_width : integer := inp'length;
        variable vec : std_logic_vector(orig_width-1 downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
        variable pos : integer;
    begin
        vec := inp;
        pos := new_width-1;
        if (arith = xlUnsigned) then
            result(pos) := '0';
            pos := pos - 1;
        else
            result(pos) := vec(orig_width-1);
            pos := pos - 1;
        end if;
        if (new_width >= orig_width) then
            for i in orig_width-1 downto 0 loop
                result(pos) := vec(i);
                pos := pos - 1;
            end loop;
            if pos >= 0 then
                for i in pos downto 0 loop
                    result(i) := '0';
                end loop;
            end if;
        end if;
        return result;
    end;
    function align_input(inp : std_logic_vector; old_width, delta, new_arith,
                         new_width: INTEGER)
        return std_logic_vector
    is
        variable vec : std_logic_vector(old_width-1 downto 0);
        variable padded_inp : std_logic_vector((old_width + delta)-1  downto 0);
        variable result : std_logic_vector(new_width-1 downto 0);
    begin
        vec := inp;
        if delta > 0 then
            padded_inp := pad_LSB(vec, old_width+delta);
            result := extend_MSB(padded_inp, new_width, new_arith);
        else
            result := extend_MSB(vec, new_width, new_arith);
        end if;
        return result;
    end;
    function max(L, R: INTEGER) return INTEGER is
    begin
        if L > R then
            return L;
        else
            return R;
        end if;
    end;
    function min(L, R: INTEGER) return INTEGER is
    begin
        if L < R then
            return L;
        else
            return R;
        end if;
    end;
    function "="(left,right: STRING) return boolean is
    begin
        if (left'length /= right'length) then
            return false;
        else
            test : for i in 1 to left'length loop
                if left(i) /= right(i) then
                    return false;
                end if;
            end loop test;
            return true;
        end if;
    end;
    -- synopsys translate_off
    function is_binary_string_invalid (inp : string)
        return boolean
    is
        variable vec : string(1 to inp'length);
        variable result : boolean;
    begin
        vec := inp;
        result := false;
        for i in 1 to vec'length loop
            if ( vec(i) = 'X' ) then
                result := true;
            end if;
        end loop;
        return result;
    end;
    function is_binary_string_undefined (inp : string)
        return boolean
    is
        variable vec : string(1 to inp'length);
        variable result : boolean;
    begin
        vec := inp;
        result := false;
        for i in 1 to vec'length loop
            if ( vec(i) = 'U' ) then
                result := true;
            end if;
        end loop;
        return result;
    end;
    function is_XorU(inp : std_logic_vector)
        return boolean
    is
        constant width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
        variable result : boolean;
    begin
        vec := inp;
        result := false;
        for i in 0 to width-1 loop
            if (vec(i) = 'U') or (vec(i) = 'X') then
                result := true;
            end if;
        end loop;
        return result;
    end;
    function to_real(inp : std_logic_vector; bin_pt : integer; arith : integer)
        return real
    is
        variable  vec : std_logic_vector(inp'length-1 downto 0);
        variable result, shift_val, undefined_real : real;
        variable neg_num : boolean;
    begin
        vec := inp;
        result := 0.0;
        neg_num := false;
        if vec(inp'length-1) = '1' then
            neg_num := true;
        end if;
        for i in 0 to inp'length-1 loop
            if  vec(i) = 'U' or vec(i) = 'X' then
                return undefined_real;
            end if;
            if arith = xlSigned then
                if neg_num then
                    if vec(i) = '0' then
                        result := result + 2.0**i;
                    end if;
                else
                    if vec(i) = '1' then
                        result := result + 2.0**i;
                    end if;
                end if;
            else
                if vec(i) = '1' then
                    result := result + 2.0**i;
                end if;
            end if;
        end loop;
        if arith = xlSigned then
            if neg_num then
                result := result + 1.0;
                result := result * (-1.0);
            end if;
        end if;
        shift_val := 2.0**(-1*bin_pt);
        result := result * shift_val;
        return result;
    end;
    function std_logic_to_real(inp : std_logic; bin_pt : integer; arith : integer)
        return real
    is
        variable result : real := 0.0;
    begin
        if inp = '1' then
            result := 1.0;
        end if;
        if arith = xlSigned then
            assert false
                report "It doesn't make sense to convert a 1 bit number to a signed real.";
        end if;
        return result;
    end;
    -- synopsys translate_on
    function integer_to_std_logic_vector (inp : integer;  width, arith : integer)
        return std_logic_vector
    is
        variable result : std_logic_vector(width-1 downto 0);
        variable unsigned_val : unsigned(width-1 downto 0);
        variable signed_val : signed(width-1 downto 0);
    begin
        if (arith = xlSigned) then
            signed_val := to_signed(inp, width);
            result := signed_to_std_logic_vector(signed_val);
        else
            unsigned_val := to_unsigned(inp, width);
            result := unsigned_to_std_logic_vector(unsigned_val);
        end if;
        return result;
    end;
    function std_logic_vector_to_integer (inp : std_logic_vector;  arith : integer)
        return integer
    is
        constant width : integer := inp'length;
        variable unsigned_val : unsigned(width-1 downto 0);
        variable signed_val : signed(width-1 downto 0);
        variable result : integer;
    begin
        if (arith = xlSigned) then
            signed_val := std_logic_vector_to_signed(inp);
            result := to_integer(signed_val);
        else
            unsigned_val := std_logic_vector_to_unsigned(inp);
            result := to_integer(unsigned_val);
        end if;
        return result;
    end;
    function std_logic_to_integer(constant inp : std_logic := '0')
        return integer
    is
    begin
        if inp = '1' then
            return 1;
        else
            return 0;
        end if;
    end;
    function makeZeroBinStr (width : integer) return STRING is
        variable result : string(1 to width+3);
    begin
        result(1) := '0';
        result(2) := 'b';
        for i in 3 to width+2 loop
            result(i) := '0';
        end loop;
        result(width+3) := '.';
        return result;
    end;
    -- synopsys translate_off
    function real_string_to_std_logic_vector (inp : string;  width, bin_pt, arith : integer)
        return std_logic_vector
    is
        variable result : std_logic_vector(width-1 downto 0);
    begin
        result := (others => '0');
        return result;
    end;
    function real_to_std_logic_vector (inp : real;  width, bin_pt, arith : integer)
        return std_logic_vector
    is
        variable real_val : real;
        variable int_val : integer;
        variable result : std_logic_vector(width-1 downto 0) := (others => '0');
        variable unsigned_val : unsigned(width-1 downto 0) := (others => '0');
        variable signed_val : signed(width-1 downto 0) := (others => '0');
    begin
        real_val := inp;
        int_val := integer(real_val * 2.0**(bin_pt));
        if (arith = xlSigned) then
            signed_val := to_signed(int_val, width);
            result := signed_to_std_logic_vector(signed_val);
        else
            unsigned_val := to_unsigned(int_val, width);
            result := unsigned_to_std_logic_vector(unsigned_val);
        end if;
        return result;
    end;
    -- synopsys translate_on
    function valid_bin_string (inp : string)
        return boolean
    is
        variable vec : string(1 to inp'length);
    begin
        vec := inp;
        if (vec(1) = '0' and vec(2) = 'b') then
            return true;
        else
            return false;
        end if;
    end;
    function hex_string_to_std_logic_vector(inp: string; width : integer)
        return std_logic_vector is
        constant strlen       : integer := inp'LENGTH;
        variable result       : std_logic_vector(width-1 downto 0);
        variable bitval       : std_logic_vector((strlen*4)-1 downto 0);
        variable posn         : integer;
        variable ch           : character;
        variable vec          : string(1 to strlen);
    begin
        vec := inp;
        result := (others => '0');
        posn := (strlen*4)-1;
        for i in 1 to strlen loop
            ch := vec(i);
            case ch is
                when '0' => bitval(posn downto posn-3) := "0000";
                when '1' => bitval(posn downto posn-3) := "0001";
                when '2' => bitval(posn downto posn-3) := "0010";
                when '3' => bitval(posn downto posn-3) := "0011";
                when '4' => bitval(posn downto posn-3) := "0100";
                when '5' => bitval(posn downto posn-3) := "0101";
                when '6' => bitval(posn downto posn-3) := "0110";
                when '7' => bitval(posn downto posn-3) := "0111";
                when '8' => bitval(posn downto posn-3) := "1000";
                when '9' => bitval(posn downto posn-3) := "1001";
                when 'A' | 'a' => bitval(posn downto posn-3) := "1010";
                when 'B' | 'b' => bitval(posn downto posn-3) := "1011";
                when 'C' | 'c' => bitval(posn downto posn-3) := "1100";
                when 'D' | 'd' => bitval(posn downto posn-3) := "1101";
                when 'E' | 'e' => bitval(posn downto posn-3) := "1110";
                when 'F' | 'f' => bitval(posn downto posn-3) := "1111";
                when others => bitval(posn downto posn-3) := "XXXX";
                               -- synopsys translate_off
                               ASSERT false
                                   REPORT "Invalid hex value" SEVERITY ERROR;
                               -- synopsys translate_on
            end case;
            posn := posn - 4;
        end loop;
        if (width <= strlen*4) then
            result :=  bitval(width-1 downto 0);
        else
            result((strlen*4)-1 downto 0) := bitval;
        end if;
        return result;
    end;
    function bin_string_to_std_logic_vector (inp : string)
        return std_logic_vector
    is
        variable pos : integer;
        variable vec : string(1 to inp'length);
        variable result : std_logic_vector(inp'length-1 downto 0);
    begin
        vec := inp;
        pos := inp'length-1;
        result := (others => '0');
        for i in 1 to vec'length loop
            -- synopsys translate_off
            if (pos < 0) and (vec(i) = '0' or vec(i) = '1' or vec(i) = 'X' or vec(i) = 'U')  then
                assert false
                    report "Input string is larger than output std_logic_vector. Truncating output.";
                return result;
            end if;
            -- synopsys translate_on
            if vec(i) = '0' then
                result(pos) := '0';
                pos := pos - 1;
            end if;
            if vec(i) = '1' then
                result(pos) := '1';
                pos := pos - 1;
            end if;
            -- synopsys translate_off
            if (vec(i) = 'X' or vec(i) = 'U') then
                result(pos) := 'U';
                pos := pos - 1;
            end if;
            -- synopsys translate_on
        end loop;
        return result;
    end;
    function bin_string_element_to_std_logic_vector (inp : string;  width, index : integer)
        return std_logic_vector
    is
        constant str_width : integer := width + 4;
        constant inp_len : integer := inp'length;
        constant num_elements : integer := (inp_len + 1)/str_width;
        constant reverse_index : integer := (num_elements-1) - index;
        variable left_pos : integer;
        variable right_pos : integer;
        variable vec : string(1 to inp'length);
        variable result : std_logic_vector(width-1 downto 0);
    begin
        vec := inp;
        result := (others => '0');
        if (reverse_index = 0) and (reverse_index < num_elements) and (inp_len-3 >= width) then
            left_pos := 1;
            right_pos := width + 3;
            result := bin_string_to_std_logic_vector(vec(left_pos to right_pos));
        end if;
        if (reverse_index > 0) and (reverse_index < num_elements) and (inp_len-3 >= width) then
            left_pos := (reverse_index * str_width) + 1;
            right_pos := left_pos + width + 2;
            result := bin_string_to_std_logic_vector(vec(left_pos to right_pos));
        end if;
        return result;
    end;
   -- synopsys translate_off
    function std_logic_vector_to_bin_string(inp : std_logic_vector)
        return string
    is
        variable vec : std_logic_vector(1 to inp'length);
        variable result : string(vec'range);
    begin
        vec := inp;
        for i in vec'range loop
            result(i) := to_char(vec(i));
        end loop;
        return result;
    end;
    function std_logic_to_bin_string(inp : std_logic)
        return string
    is
        variable result : string(1 to 3);
    begin
        result(1) := '0';
        result(2) := 'b';
        result(3) := to_char(inp);
        return result;
    end;
    function std_logic_vector_to_bin_string_w_point(inp : std_logic_vector; bin_pt : integer)
        return string
    is
        variable width : integer := inp'length;
        variable vec : std_logic_vector(width-1 downto 0);
        variable str_pos : integer;
        variable result : string(1 to width+3);
    begin
        vec := inp;
        str_pos := 1;
        result(str_pos) := '0';
        str_pos := 2;
        result(str_pos) := 'b';
        str_pos := 3;
        for i in width-1 downto 0  loop
            if (((width+3) - bin_pt) = str_pos) then
                result(str_pos) := '.';
                str_pos := str_pos + 1;
            end if;
            result(str_pos) := to_char(vec(i));
            str_pos := str_pos + 1;
        end loop;
        if (bin_pt = 0) then
            result(str_pos) := '.';
        end if;
        return result;
    end;
    function real_to_bin_string(inp : real;  width, bin_pt, arith : integer)
        return string
    is
        variable result : string(1 to width);
        variable vec : std_logic_vector(width-1 downto 0);
    begin
        vec := real_to_std_logic_vector(inp, width, bin_pt, arith);
        result := std_logic_vector_to_bin_string(vec);
        return result;
    end;
    function real_to_string (inp : real) return string
    is
        variable result : string(1 to display_precision) := (others => ' ');
    begin
        result(real'image(inp)'range) := real'image(inp);
        return result;
    end;
    -- synopsys translate_on
end conv_pkg;

-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
-- synopsys translate_off
library unisim;
use unisim.vcomponents.all;
-- synopsys translate_on
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity srl17e is
    generic (width : integer:=16;
             latency : integer :=8);
    port (clk   : in std_logic;
          ce    : in std_logic;
          d     : in std_logic_vector(width-1 downto 0);
          q     : out std_logic_vector(width-1 downto 0));
end srl17e;
architecture structural of srl17e is
    component SRL16E
        port (D   : in STD_ULOGIC;
              CE  : in STD_ULOGIC;
              CLK : in STD_ULOGIC;
              A0  : in STD_ULOGIC;
              A1  : in STD_ULOGIC;
              A2  : in STD_ULOGIC;
              A3  : in STD_ULOGIC;
              Q   : out STD_ULOGIC);
    end component;
    attribute syn_black_box of SRL16E : component is true;
    attribute fpga_dont_touch of SRL16E : component is "true";
    component FDE
        port(
            Q  :        out   STD_ULOGIC;
            D  :        in    STD_ULOGIC;
            C  :        in    STD_ULOGIC;
            CE :        in    STD_ULOGIC);
    end component;
    attribute syn_black_box of FDE : component is true;
    attribute fpga_dont_touch of FDE : component is "true";
    constant a : std_logic_vector(4 downto 0) :=
        integer_to_std_logic_vector(latency-2,5,xlSigned);
    signal d_delayed : std_logic_vector(width-1 downto 0);
    signal srl16_out : std_logic_vector(width-1 downto 0);
begin
    d_delayed <= d after 200 ps;
    reg_array : for i in 0 to width-1 generate
        srl16_used: if latency > 1 generate
            u1 : srl16e port map(clk => clk,
                                 d => d_delayed(i),
                                 q => srl16_out(i),
                                 ce => ce,
                                 a0 => a(0),
                                 a1 => a(1),
                                 a2 => a(2),
                                 a3 => a(3));
        end generate;
        srl16_not_used: if latency <= 1 generate
            srl16_out(i) <= d_delayed(i);
        end generate;
        fde_used: if latency /= 0  generate
            u2 : fde port map(c => clk,
                              d => srl16_out(i),
                              q => q(i),
                              ce => ce);
        end generate;
        fde_not_used: if latency = 0  generate
            q(i) <= srl16_out(i);
        end generate;
    end generate;
 end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity synth_reg is
    generic (width           : integer := 8;
             latency         : integer := 1);
    port (i       : in std_logic_vector(width-1 downto 0);
          ce      : in std_logic;
          clr     : in std_logic;
          clk     : in std_logic;
          o       : out std_logic_vector(width-1 downto 0));
end synth_reg;
architecture structural of synth_reg is
    component srl17e
        generic (width : integer:=16;
                 latency : integer :=8);
        port (clk : in std_logic;
              ce  : in std_logic;
              d   : in std_logic_vector(width-1 downto 0);
              q   : out std_logic_vector(width-1 downto 0));
    end component;
    function calc_num_srl17es (latency : integer)
        return integer
    is
        variable remaining_latency : integer;
        variable result : integer;
    begin
        result := latency / 17;
        remaining_latency := latency - (result * 17);
        if (remaining_latency /= 0) then
            result := result + 1;
        end if;
        return result;
    end;
    constant complete_num_srl17es : integer := latency / 17;
    constant num_srl17es : integer := calc_num_srl17es(latency);
    constant remaining_latency : integer := latency - (complete_num_srl17es * 17);
    type register_array is array (num_srl17es downto 0) of
        std_logic_vector(width-1 downto 0);
    signal z : register_array;
begin
    z(0) <= i;
    complete_ones : if complete_num_srl17es > 0 generate
        srl17e_array: for i in 0 to complete_num_srl17es-1 generate
            delay_comp : srl17e
                generic map (width => width,
                             latency => 17)
                port map (clk => clk,
                          ce  => ce,
                          d       => z(i),
                          q       => z(i+1));
        end generate;
    end generate;
    partial_one : if remaining_latency > 0 generate
        last_srl17e : srl17e
            generic map (width => width,
                         latency => remaining_latency)
            port map (clk => clk,
                      ce  => ce,
                      d   => z(num_srl17es-1),
                      q   => z(num_srl17es));
    end generate;
    o <= z(num_srl17es);
end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity synth_reg_reg is
    generic (width           : integer := 8;
             latency         : integer := 1);
    port (i       : in std_logic_vector(width-1 downto 0);
          ce      : in std_logic;
          clr     : in std_logic;
          clk     : in std_logic;
          o       : out std_logic_vector(width-1 downto 0));
end synth_reg_reg;
architecture behav of synth_reg_reg is
  type reg_array_type is array (latency-1 downto 0) of std_logic_vector(width -1 downto 0);
  signal reg_bank : reg_array_type := (others => (others => '0'));
  signal reg_bank_in : reg_array_type := (others => (others => '0'));
  attribute syn_allow_retiming : boolean;
  attribute syn_srlstyle : string;
  attribute syn_allow_retiming of reg_bank : signal is true;
  attribute syn_allow_retiming of reg_bank_in : signal is true;
  attribute syn_srlstyle of reg_bank : signal is "registers";
  attribute syn_srlstyle of reg_bank_in : signal is "registers";
begin
  latency_eq_0: if latency = 0 generate
    o <= i;
  end generate latency_eq_0;
  latency_gt_0: if latency >= 1 generate
    o <= reg_bank(latency-1);
    reg_bank_in(0) <= i;
    loop_gen: for idx in latency-2 downto 0 generate
      reg_bank_in(idx+1) <= reg_bank(idx);
    end generate loop_gen;
    sync_loop: for sync_idx in latency-1 downto 0 generate
      sync_proc: process (clk)
      begin
        if clk'event and clk = '1' then
          if clr = '1' then
            reg_bank_in <= (others => (others => '0'));
          elsif ce = '1'  then
            reg_bank(sync_idx) <= reg_bank_in(sync_idx);
          end if;
        end if;
      end process sync_proc;
    end generate sync_loop;
  end generate latency_gt_0;
end behav;

-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
-- synopsys translate_off
library unisim;
use unisim.vcomponents.all;
-- synopsys translate_on
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity single_reg_w_init is
  generic (
    width: integer := 8;
    init_index: integer := 0;
    init_value: bit_vector := b"0000"
  );
  port (
    i: in std_logic_vector(width - 1 downto 0);
    ce: in std_logic;
    clr: in std_logic;
    clk: in std_logic;
    o: out std_logic_vector(width - 1 downto 0)
  );
end single_reg_w_init;
architecture structural of single_reg_w_init is
  function build_init_const(width: integer;
                            init_index: integer;
                            init_value: bit_vector)
    return std_logic_vector
  is
    variable result: std_logic_vector(width - 1 downto 0);
  begin
    if init_index = 0 then
      result := (others => '0');
    elsif init_index = 1 then
      result := (others => '0');
      result(0) := '1';
    else
      result := to_stdlogicvector(init_value);
    end if;
    return result;
  end;
  component fdre
    port (
      q: out std_ulogic;
      d: in  std_ulogic;
      c: in  std_ulogic;
      ce: in  std_ulogic;
      r: in  std_ulogic
    );
  end component;
  attribute syn_black_box of fdre: component is true;
  attribute fpga_dont_touch of fdre: component is "true";
  component fdse
    port (
      q: out std_ulogic;
      d: in  std_ulogic;
      c: in  std_ulogic;
      ce: in  std_ulogic;
      s: in  std_ulogic
    );
  end component;
  attribute syn_black_box of fdse: component is true;
  attribute fpga_dont_touch of fdse: component is "true";
  constant init_const: std_logic_vector(width - 1 downto 0)
    := build_init_const(width, init_index, init_value);
begin
  fd_prim_array: for index in 0 to width - 1 generate
    bit_is_0: if (init_const(index) = '0') generate
      fdre_comp: fdre
        port map (
          c => clk,
          d => i(index),
          q => o(index),
          ce => ce,
          r => clr
        );
    end generate;
    bit_is_1: if (init_const(index) = '1') generate
      fdse_comp: fdse
        port map (
          c => clk,
          d => i(index),
          q => o(index),
          ce => ce,
          s => clr
        );
    end generate;
  end generate;
end architecture structural;
-- synopsys translate_off
library unisim;
use unisim.vcomponents.all;
-- synopsys translate_on
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity synth_reg_w_init is
  generic (
    width: integer := 8;
    init_index: integer := 0;
    init_value: bit_vector := b"0000";
    latency: integer := 1
  );
  port (
    i: in std_logic_vector(width - 1 downto 0);
    ce: in std_logic;
    clr: in std_logic;
    clk: in std_logic;
    o: out std_logic_vector(width - 1 downto 0)
  );
end synth_reg_w_init;
architecture structural of synth_reg_w_init is
  component single_reg_w_init
    generic (
      width: integer := 8;
      init_index: integer := 0;
      init_value: bit_vector := b"0000"
    );
    port (
      i: in std_logic_vector(width - 1 downto 0);
      ce: in std_logic;
      clr: in std_logic;
      clk: in std_logic;
      o: out std_logic_vector(width - 1 downto 0)
    );
  end component;
  signal dly_i: std_logic_vector((latency + 1) * width - 1 downto 0);
  signal dly_clr: std_logic;
begin
  latency_eq_0: if (latency = 0) generate
    o <= i;
  end generate;
  latency_gt_0: if (latency >= 1) generate
    dly_i((latency + 1) * width - 1 downto latency * width) <= i
      after 200 ps;
    dly_clr <= clr after 200 ps;
    fd_array: for index in latency downto 1 generate
       reg_comp: single_reg_w_init
          generic map (
            width => width,
            init_index => init_index,
            init_value => init_value
          )
          port map (
            clk => clk,
            i => dly_i((index + 1) * width - 1 downto index * width),
            o => dly_i(index * width - 1 downto (index - 1) * width),
            ce => ce,
            clr => dly_clr
          );
    end generate;
    o <= dly_i(width - 1 downto 0);
  end generate;
end structural;

-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity axi_sgiface is
    generic (
        -- AXI specific.
        -- TODO: need to figure out a way to pass these generics from outside
        C_S_AXI_SUPPORT_BURST   : integer := 0;
        -- TODO: fix the internal ID width to 8
        C_S_AXI_ID_WIDTH        : integer := 8;
        C_S_AXI_DATA_WIDTH      : integer := 32;
        C_S_AXI_ADDR_WIDTH      : integer := 32;
        C_S_AXI_TOTAL_ADDR_LEN  : integer := 12;
        C_S_AXI_LINEAR_ADDR_LEN : integer := 8;
        C_S_AXI_BANK_ADDR_LEN   : integer := 2;
        C_S_AXI_AWLEN_WIDTH     : integer := 8;
        C_S_AXI_ARLEN_WIDTH     : integer := 8
    );
    port (
        -- General.
        AXI_AClk      : in  std_logic;
        AXI_AResetN    : in  std_logic;
        -- not used
        AXI_Ce        : in  std_logic;
  
        -- AXI Port.
        S_AXI_AWADDR  : in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
        S_AXI_AWID    : in  std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);
        S_AXI_AWLEN   : in  std_logic_vector(C_S_AXI_AWLEN_WIDTH-1 downto 0);
        S_AXI_AWSIZE  : in  std_logic_vector(2 downto 0);
        S_AXI_AWBURST : in  std_logic_vector(1 downto 0);
        S_AXI_AWLOCK  : in  std_logic_vector(1 downto 0);
        S_AXI_AWCACHE : in  std_logic_vector(3 downto 0);
        S_AXI_AWPROT  : in  std_logic_vector(2 downto 0);
        S_AXI_AWVALID : in  std_logic;
        S_AXI_AWREADY : out std_logic;
        
        S_AXI_WLAST   : in  std_logic;
        S_AXI_WDATA   : in  std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
        S_AXI_WSTRB   : in  std_logic_vector((C_S_AXI_DATA_WIDTH/8)-1 downto 0);
        S_AXI_WVALID  : in  std_logic;
        S_AXI_WREADY  : out std_logic;
        
        S_AXI_BRESP   : out std_logic_vector(1 downto 0);
        S_AXI_BID     : out std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);
        S_AXI_BVALID  : out std_logic;
        S_AXI_BREADY  : in  std_logic;
        
        S_AXI_ARADDR  : in  std_logic_vector(C_S_AXI_ADDR_WIDTH-1 downto 0);
        S_AXI_ARID    : in  std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);
        S_AXI_ARLEN   : in  std_logic_vector(C_S_AXI_ARLEN_WIDTH-1 downto 0);
        S_AXI_ARSIZE  : in  std_logic_vector(2 downto 0);
        S_AXI_ARBURST : in  std_logic_vector(1 downto 0);
        S_AXI_ARLOCK  : in  std_logic_vector(1 downto 0);
        S_AXI_ARCACHE : in  std_logic_vector(3 downto 0);
        S_AXI_ARPROT  : in  std_logic_vector(2 downto 0);
        S_AXI_ARVALID : in  std_logic;
        S_AXI_ARREADY : out std_logic;
        
        -- 'From Register'
        -- 'SYSTEM_TIME_USEC_MSB'
        sm_SYSTEM_TIME_USEC_MSB_dout : in std_logic_vector(32-1 downto 0);
        -- 'SYSTEM_TIME_USEC_LSB'
        sm_SYSTEM_TIME_USEC_LSB_dout : in std_logic_vector(32-1 downto 0);
        -- 'MAC_TIME_USEC_MSB'
        sm_MAC_TIME_USEC_MSB_dout : in std_logic_vector(32-1 downto 0);
        -- 'MAC_TIME_USEC_LSB'
        sm_MAC_TIME_USEC_LSB_dout : in std_logic_vector(32-1 downto 0);
        -- 'To Register'
        -- 'NEW_MAC_TIME_MSB'
        sm_NEW_MAC_TIME_MSB_dout : in std_logic_vector(32-1 downto 0);
        sm_NEW_MAC_TIME_MSB_din  : out std_logic_vector(32-1 downto 0);
        sm_NEW_MAC_TIME_MSB_en   : out std_logic;
        -- 'NEW_MAC_TIME_LSB'
        sm_NEW_MAC_TIME_LSB_dout : in std_logic_vector(32-1 downto 0);
        sm_NEW_MAC_TIME_LSB_din  : out std_logic_vector(32-1 downto 0);
        sm_NEW_MAC_TIME_LSB_en   : out std_logic;
        -- 'Control'
        sm_Control_dout : in std_logic_vector(32-1 downto 0);
        sm_Control_din  : out std_logic_vector(32-1 downto 0);
        sm_Control_en   : out std_logic;
        -- 'From FIFO'
        -- 'To FIFO'
        -- 'Shared Memory'

        S_AXI_RLAST   : out std_logic;
        S_AXI_RID     : out std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);
        S_AXI_RDATA   : out std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
        S_AXI_RRESP   : out std_logic_vector(1 downto 0);
        S_AXI_RVALID  : out std_logic;
        S_AXI_RREADY  : in  std_logic
    );
end entity axi_sgiface;

architecture IMP of axi_sgiface is

-- Internal signals for write channel.
signal S_AXI_BVALID_i       : std_logic;
signal S_AXI_BID_i          : std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);
signal S_AXI_WREADY_i       : std_logic;
  
-- Internal signals for read channels.
signal S_AXI_ARLEN_i        : std_logic_vector(C_S_AXI_ARLEN_WIDTH-1 downto 0);
signal S_AXI_RLAST_i        : std_logic;
signal S_AXI_RREADY_i       : std_logic;
signal S_AXI_RVALID_i       : std_logic;
signal S_AXI_RDATA_i        : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal S_AXI_RID_i          : std_logic_vector(C_S_AXI_ID_WIDTH-1 downto 0);

-- for read channel
signal read_bank_addr_i     : std_logic_vector(C_S_AXI_BANK_ADDR_LEN-1 downto 0);
signal read_linear_addr_i   : std_logic_vector(C_S_AXI_LINEAR_ADDR_LEN-1 downto 0);
-- for write channel
signal write_bank_addr_i    : std_logic_vector(C_S_AXI_BANK_ADDR_LEN-1 downto 0);
signal write_linear_addr_i  : std_logic_vector(C_S_AXI_LINEAR_ADDR_LEN-1 downto 0);

signal reg_bank_out_i       : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal fifo_bank_out_i      : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal shmem_bank_out_i     : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
    
-- 'From Register'
-- 'SYSTEM_TIME_USEC_MSB'
signal sm_SYSTEM_TIME_USEC_MSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'SYSTEM_TIME_USEC_LSB'
signal sm_SYSTEM_TIME_USEC_LSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'MAC_TIME_USEC_MSB'
signal sm_MAC_TIME_USEC_MSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'MAC_TIME_USEC_LSB'
signal sm_MAC_TIME_USEC_LSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'To Register'
-- 'NEW_MAC_TIME_MSB'
signal sm_NEW_MAC_TIME_MSB_din_i   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal sm_NEW_MAC_TIME_MSB_en_i    : std_logic;
signal sm_NEW_MAC_TIME_MSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'NEW_MAC_TIME_LSB'
signal sm_NEW_MAC_TIME_LSB_din_i   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal sm_NEW_MAC_TIME_LSB_en_i    : std_logic;
signal sm_NEW_MAC_TIME_LSB_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'Control'
signal sm_Control_din_i   : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
signal sm_Control_en_i    : std_logic;
signal sm_Control_dout_i  : std_logic_vector(C_S_AXI_DATA_WIDTH-1 downto 0);
-- 'From FIFO'
-- 'To FIFO'
-- 'Shared Memory'

type t_read_state is (IDLE, READ_PREP, READ_DATA);
signal read_state : t_read_state;

type t_write_state is (IDLE, WRITE_DATA, WRITE_RESPONSE);
signal write_state : t_write_state;

type t_memmap_state is (READ, WRITE);
signal memmap_state : t_memmap_state;

constant C_READ_PREP_DELAY : std_logic_vector(1 downto 0) := "11";

signal read_prep_counter : std_logic_vector(1 downto 0);
signal read_addr_counter : std_logic_vector(C_S_AXI_ARLEN_WIDTH-1 downto 0);
signal read_data_counter : std_logic_vector(C_S_AXI_ARLEN_WIDTH-1 downto 0);

-- enable of shared BRAMs
signal s_shram_en : std_logic;

signal write_addr_valid : std_logic;
signal write_ready : std_logic;

-- 're' of From/To FIFOs
signal s_fifo_re : std_logic;
-- 'we' of To FIFOs
signal s_fifo_we : std_logic;

begin

-- enable for 'Shared Memory' blocks

-- conversion to match with the data bus width
-- 'From Register'
-- 'SYSTEM_TIME_USEC_MSB'
gen_sm_SYSTEM_TIME_USEC_MSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_SYSTEM_TIME_USEC_MSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_SYSTEM_TIME_USEC_MSB_dout_i;
sm_SYSTEM_TIME_USEC_MSB_dout_i(32-1 downto 0) <= sm_SYSTEM_TIME_USEC_MSB_dout;
-- 'SYSTEM_TIME_USEC_LSB'
gen_sm_SYSTEM_TIME_USEC_LSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_SYSTEM_TIME_USEC_LSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_SYSTEM_TIME_USEC_LSB_dout_i;
sm_SYSTEM_TIME_USEC_LSB_dout_i(32-1 downto 0) <= sm_SYSTEM_TIME_USEC_LSB_dout;
-- 'MAC_TIME_USEC_MSB'
gen_sm_MAC_TIME_USEC_MSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_MAC_TIME_USEC_MSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_MAC_TIME_USEC_MSB_dout_i;
sm_MAC_TIME_USEC_MSB_dout_i(32-1 downto 0) <= sm_MAC_TIME_USEC_MSB_dout;
-- 'MAC_TIME_USEC_LSB'
gen_sm_MAC_TIME_USEC_LSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_MAC_TIME_USEC_LSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_MAC_TIME_USEC_LSB_dout_i;
sm_MAC_TIME_USEC_LSB_dout_i(32-1 downto 0) <= sm_MAC_TIME_USEC_LSB_dout;
-- 'To Register'
-- 'NEW_MAC_TIME_MSB'
sm_NEW_MAC_TIME_MSB_din     <= sm_NEW_MAC_TIME_MSB_din_i(32-1 downto 0);
sm_NEW_MAC_TIME_MSB_en      <= sm_NEW_MAC_TIME_MSB_en_i;
gen_sm_NEW_MAC_TIME_MSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_NEW_MAC_TIME_MSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_NEW_MAC_TIME_MSB_dout_i;
sm_NEW_MAC_TIME_MSB_dout_i(32-1 downto 0) <= sm_NEW_MAC_TIME_MSB_dout;
-- 'NEW_MAC_TIME_LSB'
sm_NEW_MAC_TIME_LSB_din     <= sm_NEW_MAC_TIME_LSB_din_i(32-1 downto 0);
sm_NEW_MAC_TIME_LSB_en      <= sm_NEW_MAC_TIME_LSB_en_i;
gen_sm_NEW_MAC_TIME_LSB_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_NEW_MAC_TIME_LSB_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_NEW_MAC_TIME_LSB_dout_i;
sm_NEW_MAC_TIME_LSB_dout_i(32-1 downto 0) <= sm_NEW_MAC_TIME_LSB_dout;
-- 'Control'
sm_Control_din     <= sm_Control_din_i(32-1 downto 0);
sm_Control_en      <= sm_Control_en_i;
gen_sm_Control_dout_i: if (32 < C_S_AXI_DATA_WIDTH) generate
    sm_Control_dout_i(C_S_AXI_DATA_WIDTH-1 downto 32) <= (others => '0');
end generate gen_sm_Control_dout_i;
sm_Control_dout_i(32-1 downto 0) <= sm_Control_dout;
-- 'From FIFO'
-- 'To FIFO'
-- 'Shared Memory'

ReadWriteSelect: process(memmap_state) is begin
    if (memmap_state = READ) then
    else
    end if;
end process ReadWriteSelect;

-----------------------------------------------------------------------------
-- address for 'Shared Memory'
-----------------------------------------------------------------------------
SharedMemory_Addr_ResetN : process(AXI_AClk) is begin
    if (AXI_AClk'event and AXI_AClk = '1') then
        if (AXI_AResetN = '0') then
            memmap_state <= READ;
        else
            if (S_AXI_AWVALID = '1') then
                -- write operation
                memmap_state <= WRITE;
            elsif (S_AXI_ARVALID = '1') then
                -- read operation
                memmap_state <= READ;
            end if;
        end if;
    end if;
end process SharedMemory_Addr_ResetN;

-----------------------------------------------------------------------------
-- WRITE Command Control
-----------------------------------------------------------------------------
S_AXI_BID     <= S_AXI_BID_i;
S_AXI_BVALID  <= S_AXI_BVALID_i;
S_AXI_WREADY  <= S_AXI_WREADY_i;
-- No error checking
S_AXI_BRESP  <= (others=>'0');

PROC_AWREADY_ACK: process(read_state, write_state, S_AXI_ARVALID, S_AXI_AWVALID) is begin
    if (write_state = IDLE and S_AXI_AWVALID = '1' and read_state = IDLE) then
        S_AXI_AWREADY <= S_AXI_AWVALID;
    else
        S_AXI_AWREADY <= '0';
    end if;
end process PROC_AWREADY_ACK;

Cmd_Decode_Write: process(AXI_AClk) is begin
    if (AXI_AClk'event and AXI_AClk = '1') then
        if (AXI_AResetN = '0') then
            write_addr_valid    <= '0';
            write_ready         <= '0';
            s_fifo_we           <= '0';
            S_AXI_BVALID_i      <= '0';
            S_AXI_BID_i         <= (others => '0');
            write_bank_addr_i   <= (others => '0');
            write_linear_addr_i <= (others => '0');
        else
            if (write_state = IDLE) then
                if (S_AXI_AWVALID = '1' and read_state = IDLE) then
                    -- reflect awid
                    S_AXI_BID_i <= S_AXI_AWID;

                    -- latch bank and linear addresses
                    write_bank_addr_i   <= S_AXI_AWADDR(C_S_AXI_TOTAL_ADDR_LEN-1 downto C_S_AXI_LINEAR_ADDR_LEN+2);
                    write_linear_addr_i <= S_AXI_AWADDR(C_S_AXI_LINEAR_ADDR_LEN+1 downto 2);
                    write_addr_valid <= '1';
                    s_fifo_we <= '1';

                    -- write state transition
                    write_state <= WRITE_DATA;
                end if;
            elsif (write_state = WRITE_DATA) then
                write_ready <= '1';
                s_fifo_we <= '0';
                write_addr_valid <= S_AXI_WVALID;
                
                if (S_AXI_WVALID = '1' and write_ready = '1') then
                    write_linear_addr_i <= Std_Logic_Vector(unsigned(write_linear_addr_i) + 1);
                end if;

                if (S_AXI_WLAST = '1' and write_ready = '1') then
                    -- start responding through B channel upon the last write data sample
                    S_AXI_BVALID_i <= '1';
                    -- write data is over
                    write_addr_valid <= '0';
                    write_ready <= '0';
                    -- write state transition
                    write_state <= WRITE_RESPONSE;
                end if;
            elsif (write_state = WRITE_RESPONSE) then

                if (S_AXI_BREADY = '1') then
                    -- write respond is over
                    S_AXI_BVALID_i <= '0';
                    S_AXI_BID_i <= (others => '0');

                    -- write state transition
                    write_state <= IDLE;
                end if;
            end if;
        end if;
    end if;
end process Cmd_Decode_Write;

Write_Linear_Addr_Decode : process(AXI_AClk) is 

begin
    if (AXI_AClk'event and AXI_AClk = '1') then
        if (AXI_AResetN = '0') then
            -- 'To Register'
            -- NEW_MAC_TIME_MSB din/en
            sm_NEW_MAC_TIME_MSB_din_i <= (others => '0');
            sm_NEW_MAC_TIME_MSB_en_i <= '0';
            -- NEW_MAC_TIME_LSB din/en
            sm_NEW_MAC_TIME_LSB_din_i <= (others => '0');
            sm_NEW_MAC_TIME_LSB_en_i <= '0';
            -- Control din/en
            sm_Control_din_i <= (others => '0');
            sm_Control_en_i <= '0';
            -- 'To FIFO'
            -- 'Shared Memory'
        else
            -- default assignments

            -- 'To Register'
            if (unsigned(write_bank_addr_i) = 2) then
                if (unsigned(write_linear_addr_i) = 0) then
                    -- NEW_MAC_TIME_MSB din/en
                    sm_NEW_MAC_TIME_MSB_din_i <= S_AXI_WDATA;
                    sm_NEW_MAC_TIME_MSB_en_i  <= write_addr_valid;
                elsif (unsigned(write_linear_addr_i) = 1) then
                    -- NEW_MAC_TIME_LSB din/en
                    sm_NEW_MAC_TIME_LSB_din_i <= S_AXI_WDATA;
                    sm_NEW_MAC_TIME_LSB_en_i  <= write_addr_valid;
                elsif (unsigned(write_linear_addr_i) = 2) then
                    -- Control din/en
                    sm_Control_din_i <= S_AXI_WDATA;
                    sm_Control_en_i  <= write_addr_valid;
                end if;
            end if;        
        
        
        end if;
    end if;
end process Write_Linear_Addr_Decode;
 
-----------------------------------------------------------------------------
-- READ Control
-----------------------------------------------------------------------------

S_AXI_RDATA  <= S_AXI_RDATA_i;
S_AXI_RVALID  <= S_AXI_RVALID_i;
S_AXI_RLAST   <= S_AXI_RLAST_i;
S_AXI_RID     <= S_AXI_RID_i;
-- TODO: no error checking
S_AXI_RRESP <= (others=>'0');

PROC_ARREADY_ACK: process(read_state, S_AXI_ARVALID, write_state, S_AXI_AWVALID) is begin
    -- Note: WRITE has higher priority than READ
    if (read_state = IDLE and S_AXI_ARVALID = '1' and write_state = IDLE and S_AXI_AWVALID /= '1') then
        S_AXI_ARREADY <= S_AXI_ARVALID;
    else
        S_AXI_ARREADY <= '0';
    end if;
end process PROC_ARREADY_ACK;

S_AXI_WREADY_i <= write_ready;

Process_Sideband: process(write_state, read_state) is begin
    if (read_state = READ_PREP) then
        s_shram_en <= '1';
    elsif (read_state = READ_DATA) then
        s_shram_en <= S_AXI_RREADY;
    elsif (write_state = WRITE_DATA) then
        s_shram_en <= S_AXI_WVALID;
    else
        s_shram_en <= '0';
    end if;
end process Process_Sideband;

Cmd_Decode_Read: process(AXI_AClk) is begin
    if (AXI_AClk'event and AXI_AClk = '1') then
        if (AXI_AResetN = '0') then
            S_AXI_RVALID_i <= '0';
            read_bank_addr_i    <= (others => '0');
            read_linear_addr_i  <= (others => '0');
            S_AXI_ARLEN_i       <= (others => '0');
            S_AXI_RLAST_i       <= '0';
            S_AXI_RID_i         <= (others => '0');
            read_state          <= IDLE;
            read_prep_counter   <= (others => '0');
            read_addr_counter   <= (others => '0');
            read_data_counter   <= (others => '0');
        else
            -- default assignments
            s_fifo_re <= '0';

            if (read_state = IDLE) then
                -- Note WRITE has higher priority than READ
                if (S_AXI_ARVALID = '1' and write_state = IDLE and S_AXI_AWVALID /= '1') then
                    -- extract bank and linear addresses
                    read_bank_addr_i    <= S_AXI_ARADDR(C_S_AXI_TOTAL_ADDR_LEN-1 downto C_S_AXI_LINEAR_ADDR_LEN+2);
                    read_linear_addr_i  <= S_AXI_ARADDR(C_S_AXI_LINEAR_ADDR_LEN+1 downto 2);
                    s_fifo_re <= '1';

                    -- reflect arid
                    S_AXI_RID_i <= S_AXI_ARID;

                    -- load read liner address and data counter
                    read_addr_counter <= S_AXI_ARLEN;
                    read_data_counter <= S_AXI_ARLEN;

                    -- load read preparation counter
                    read_prep_counter <= C_READ_PREP_DELAY;
                    -- read state transition
                    read_state <= READ_PREP;
                end if;
            elsif (read_state = READ_PREP) then
                if (unsigned(read_prep_counter) = 0) then
                    if (unsigned(read_data_counter) = 0) then
                        -- tag the last data generated by the slave
                        S_AXI_RLAST_i <= '1';
                    end if;
                    -- valid data appears
                    S_AXI_RVALID_i <= '1';
                    -- read state transition
                    read_state <= READ_DATA;
                else
                    -- decrease read preparation counter
                    read_prep_counter <= Std_Logic_Vector(unsigned(read_prep_counter) - 1);
                end if;

                if (unsigned(read_prep_counter) /= 3 and unsigned(read_addr_counter) /= 0) then
                    -- decrease address counter
                    read_addr_counter <= Std_Logic_Vector(unsigned(read_addr_counter) - 1);
                    -- increase linear address (no band crossing)
                    read_linear_addr_i <= Std_Logic_Vector(unsigned(read_linear_addr_i) + 1);
                end if;
            elsif (read_state = READ_DATA) then
                if (S_AXI_RREADY = '1') then
                    if (unsigned(read_data_counter) = 1) then
                        -- tag the last data generated by the slave
                        S_AXI_RLAST_i <= '1';
                    end if;

                    if (unsigned(read_data_counter) = 0) then
                        -- arid
                        S_AXI_RID_i <= (others => '0');
                        -- rlast
                        S_AXI_RLAST_i <= '0';
                        -- no more valid data
                        S_AXI_RVALID_i <= '0';
                        -- read state transition
                        read_state <= IDLE;
                    else
                        -- decrease read preparation counter
                        read_data_counter <= Std_Logic_Vector(unsigned(read_data_counter) - 1);

                        if (unsigned(read_addr_counter) /= 0) then
                            -- decrease address counter
                            read_addr_counter <= Std_Logic_Vector(unsigned(read_addr_counter) - 1);
                            -- increase linear address (no band crossing)
                            read_linear_addr_i <= Std_Logic_Vector(unsigned(read_linear_addr_i) + 1);
                        end if;
                    end if;
                end if;
            end if;

        end if;
    end if;
end process Cmd_Decode_Read;

Read_Linear_Addr_Decode : process(AXI_AClk) is begin
    if (AXI_AClk'event and AXI_AClk = '1') then
        if (AXI_AResetN = '0') then
            reg_bank_out_i   <= (others => '0');
            fifo_bank_out_i  <= (others => '0');
            shmem_bank_out_i <= (others => '0');
            S_AXI_RDATA_i    <= (others => '0');
        else
            if (unsigned(read_bank_addr_i) = 2) then
                -- 'From Register'
                if (unsigned(read_linear_addr_i) = 3) then
                    -- 'SYSTEM_TIME_USEC_MSB' dout
                    reg_bank_out_i <= sm_SYSTEM_TIME_USEC_MSB_dout_i;
                elsif (unsigned(read_linear_addr_i) = 4) then
                    -- 'SYSTEM_TIME_USEC_LSB' dout
                    reg_bank_out_i <= sm_SYSTEM_TIME_USEC_LSB_dout_i;
                elsif (unsigned(read_linear_addr_i) = 5) then
                    -- 'MAC_TIME_USEC_MSB' dout
                    reg_bank_out_i <= sm_MAC_TIME_USEC_MSB_dout_i;
                elsif (unsigned(read_linear_addr_i) = 6) then
                    -- 'MAC_TIME_USEC_LSB' dout
                    reg_bank_out_i <= sm_MAC_TIME_USEC_LSB_dout_i;
                end if;
                -- 'To Register' (with register readback)
                if (unsigned(read_linear_addr_i) = 0) then
                    -- 'NEW_MAC_TIME_MSB' dout
                    reg_bank_out_i <= sm_NEW_MAC_TIME_MSB_dout_i;
                elsif (unsigned(read_linear_addr_i) = 1) then
                    -- 'NEW_MAC_TIME_LSB' dout
                    reg_bank_out_i <= sm_NEW_MAC_TIME_LSB_dout_i;
                elsif (unsigned(read_linear_addr_i) = 2) then
                    -- 'Control' dout
                    reg_bank_out_i <= sm_Control_dout_i;
                end if;

                S_AXI_RDATA_i <= reg_bank_out_i;
            elsif (unsigned(read_bank_addr_i) = 1) then
                -- 'From FIFO'
                -- 'To FIFO'

                S_AXI_RDATA_i <= fifo_bank_out_i;
            elsif (unsigned(read_bank_addr_i) = 0 and s_shram_en = '1') then
                -- 'Shared Memory'

                S_AXI_RDATA_i <= shmem_bank_out_i;
            end if;
        end if;
    end if;
end process Read_Linear_Addr_Decode;

end architecture IMP;


-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity xldelay is
   generic(width        : integer := -1;
           latency      : integer := -1;
           reg_retiming : integer :=  0;
           reset        : integer :=  0);
   port(d       : in std_logic_vector (width-1 downto 0);
        ce      : in std_logic;
        clk     : in std_logic;
        en      : in std_logic;
        rst     : in std_logic;
        q       : out std_logic_vector (width-1 downto 0));
end xldelay;
architecture behavior of xldelay is
   component synth_reg
      generic (width       : integer;
               latency     : integer);
      port (i       : in std_logic_vector(width-1 downto 0);
            ce      : in std_logic;
            clr     : in std_logic;
            clk     : in std_logic;
            o       : out std_logic_vector(width-1 downto 0));
   end component;
   component synth_reg_reg
      generic (width       : integer;
               latency     : integer);
      port (i       : in std_logic_vector(width-1 downto 0);
            ce      : in std_logic;
            clr     : in std_logic;
            clk     : in std_logic;
            o       : out std_logic_vector(width-1 downto 0));
   end component;
   signal internal_ce  : std_logic;
begin
   internal_ce  <= ce and en;
   srl_delay: if ((reg_retiming = 0) and (reset = 0)) or (latency < 1) generate
     synth_reg_srl_inst : synth_reg
       generic map (
         width   => width,
         latency => latency)
       port map (
         i   => d,
         ce  => internal_ce,
         clr => '0',
         clk => clk,
         o   => q);
   end generate srl_delay;
   reg_delay: if ((reg_retiming = 1) or (reset = 1)) and (latency >= 1) generate
     synth_reg_reg_inst : synth_reg_reg
       generic map (
         width   => width,
         latency => latency)
       port map (
         i   => d,
         ce  => internal_ce,
         clr => rst,
         clk => clk,
         o   => q);
   end generate reg_delay;
end architecture behavior;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity inverter_e5b38cca3b is
  port (
    ip : in std_logic_vector((1 - 1) downto 0);
    op : out std_logic_vector((1 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end inverter_e5b38cca3b;


architecture behavior of inverter_e5b38cca3b is
  signal ip_1_26: boolean;
  type array_type_op_mem_22_20 is array (0 to (1 - 1)) of boolean;
  signal op_mem_22_20: array_type_op_mem_22_20 := (
    0 => false);
  signal op_mem_22_20_front_din: boolean;
  signal op_mem_22_20_back: boolean;
  signal op_mem_22_20_push_front_pop_back_en: std_logic;
  signal internal_ip_12_1_bitnot: boolean;
begin
  ip_1_26 <= ((ip) = "1");
  op_mem_22_20_back <= op_mem_22_20(0);
  proc_op_mem_22_20: process (clk)
  is
    variable i: integer;
  begin
    if (clk'event and (clk = '1')) then
      if ((ce = '1') and (op_mem_22_20_push_front_pop_back_en = '1')) then
        op_mem_22_20(0) <= op_mem_22_20_front_din;
      end if;
    end if;
  end process proc_op_mem_22_20;
  internal_ip_12_1_bitnot <= ((not boolean_to_vector(ip_1_26)) = "1");
  op_mem_22_20_push_front_pop_back_en <= '0';
  op <= boolean_to_vector(internal_ip_12_1_bitnot);
end behavior;

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity logical_80f90b97d0 is
  port (
    d0 : in std_logic_vector((1 - 1) downto 0);
    d1 : in std_logic_vector((1 - 1) downto 0);
    y : out std_logic_vector((1 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end logical_80f90b97d0;


architecture behavior of logical_80f90b97d0 is
  signal d0_1_24: std_logic;
  signal d1_1_27: std_logic;
  signal fully_2_1_bit: std_logic;
begin
  d0_1_24 <= d0(0);
  d1_1_27 <= d1(0);
  fully_2_1_bit <= d0_1_24 and d1_1_27;
  y <= std_logic_to_vector(fully_2_1_bit);
end behavior;


-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_arith.all;
use work.conv_pkg.all;
entity xlslice is
    generic (
        new_msb      : integer := 9;
        new_lsb      : integer := 1;
        x_width      : integer := 16;
        y_width      : integer := 8);
    port (
        x : in std_logic_vector (x_width-1 downto 0);
        y : out std_logic_vector (y_width-1 downto 0));
end xlslice;
architecture behavior of xlslice is
begin
    y <= x(new_msb downto new_lsb);
end  behavior;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity constant_6293007044 is
  port (
    op : out std_logic_vector((1 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end constant_6293007044;


architecture behavior of constant_6293007044 is
begin
  op <= "1";
end behavior;


-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity xlregister is
   generic (d_width          : integer := 5;
            init_value       : bit_vector := b"00");
   port (d   : in std_logic_vector (d_width-1 downto 0);
         rst : in std_logic_vector(0 downto 0) := "0";
         en  : in std_logic_vector(0 downto 0) := "1";
         ce  : in std_logic;
         clk : in std_logic;
         q   : out std_logic_vector (d_width-1 downto 0));
end xlregister;
architecture behavior of xlregister is
   component synth_reg_w_init
      generic (width      : integer;
               init_index : integer;
               init_value : bit_vector;
               latency    : integer);
      port (i   : in std_logic_vector(width-1 downto 0);
            ce  : in std_logic;
            clr : in std_logic;
            clk : in std_logic;
            o   : out std_logic_vector(width-1 downto 0));
   end component;
   -- synopsys translate_off
   signal real_d, real_q           : real;
   -- synopsys translate_on
   signal internal_clr             : std_logic;
   signal internal_ce              : std_logic;
begin
   internal_clr <= rst(0) and ce;
   internal_ce  <= en(0) and ce;
   synth_reg_inst : synth_reg_w_init
      generic map (width      => d_width,
                   init_index => 2,
                   init_value => init_value,
                   latency    => 1)
      port map (i   => d,
                ce  => internal_ce,
                clr => internal_clr,
                clk => clk,
                o   => q);
end architecture behavior;

-------------------------------------------------------------------
-- System Generator version 14.3 VHDL source file.
--
-- Copyright(C) 2012 by Xilinx, Inc.  All rights reserved.  This
-- text/file contains proprietary, confidential information of Xilinx,
-- Inc., is distributed under license from Xilinx, Inc., and may be used,
-- copied and/or disclosed only pursuant to the terms of a valid license
-- agreement with Xilinx, Inc.  Xilinx hereby grants you a license to use
-- this text/file solely for design, simulation, implementation and
-- creation of design files limited to Xilinx devices or technologies.
-- Use with non-Xilinx devices or technologies is expressly prohibited
-- and immediately terminates your license unless covered by a separate
-- agreement.
--
-- Xilinx is providing this design, code, or information "as is" solely
-- for use in developing programs and solutions for Xilinx devices.  By
-- providing this design, code, or information as one possible
-- implementation of this feature, application or standard, Xilinx is
-- making no representation that this implementation is free from any
-- claims of infringement.  You are responsible for obtaining any rights
-- you may require for your implementation.  Xilinx expressly disclaims
-- any warranty whatsoever with respect to the adequacy of the
-- implementation, including but not limited to warranties of
-- merchantability or fitness for a particular purpose.
--
-- Xilinx products are not intended for use in life support appliances,
-- devices, or systems.  Use in such applications is expressly prohibited.
--
-- Any modifications that are made to the source code are done at the user's
-- sole risk and will be unsupported.
--
-- This copyright and support notice must be retained as part of this
-- text at all times.  (c) Copyright 1995-2012 Xilinx, Inc.  All rights
-- reserved.
-------------------------------------------------------------------
-- synopsys translate_off
library XilinxCoreLib;
-- synopsys translate_on
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;
entity xlcounter_free_wlan_mac_time_hw is
  generic (
    core_name0: string := "";
    op_width: integer := 5;
    op_arith: integer := xlSigned
  );
  port (
    ce: in std_logic;
    clr: in std_logic;
    clk: in std_logic;
    op: out std_logic_vector(op_width - 1 downto 0);
    up: in std_logic_vector(0 downto 0) := (others => '0');
    load: in std_logic_vector(0 downto 0) := (others => '0');
    din: in std_logic_vector(op_width - 1 downto 0) := (others => '0');
    en: in std_logic_vector(0 downto 0);
    rst: in std_logic_vector(0 downto 0)
  );
end xlcounter_free_wlan_mac_time_hw ;
architecture behavior of xlcounter_free_wlan_mac_time_hw is
  component cntr_11_0_86806e294f737f4c
    port (
      clk: in std_logic;
      ce: in std_logic;
      SINIT: in std_logic;
      q: out std_logic_vector(op_width - 1 downto 0)
    );
  end component;

  attribute syn_black_box of cntr_11_0_86806e294f737f4c:
    component is true;
  attribute fpga_dont_touch of cntr_11_0_86806e294f737f4c:
    component is "true";
  attribute box_type of cntr_11_0_86806e294f737f4c:
    component  is "black_box";
  component cntr_11_0_5e62871cb125c52e
    port (
      clk: in std_logic;
      ce: in std_logic;
      SINIT: in std_logic;
      load: in std_logic;
      l: in std_logic_vector(op_width - 1 downto 0);
      q: out std_logic_vector(op_width - 1 downto 0)
    );
  end component;

  attribute syn_black_box of cntr_11_0_5e62871cb125c52e:
    component is true;
  attribute fpga_dont_touch of cntr_11_0_5e62871cb125c52e:
    component is "true";
  attribute box_type of cntr_11_0_5e62871cb125c52e:
    component  is "black_box";
  component cntr_11_0_78c1092db561b4bc
    port (
      clk: in std_logic;
      ce: in std_logic;
      SINIT: in std_logic;
      q: out std_logic_vector(op_width - 1 downto 0)
    );
  end component;

  attribute syn_black_box of cntr_11_0_78c1092db561b4bc:
    component is true;
  attribute fpga_dont_touch of cntr_11_0_78c1092db561b4bc:
    component is "true";
  attribute box_type of cntr_11_0_78c1092db561b4bc:
    component  is "black_box";
-- synopsys translate_off
  constant zeroVec: std_logic_vector(op_width - 1 downto 0) := (others => '0');
  constant oneVec: std_logic_vector(op_width - 1 downto 0) := (others => '1');
  constant zeroStr: string(1 to op_width) :=
    std_logic_vector_to_bin_string(zeroVec);
  constant oneStr: string(1 to op_width) :=
    std_logic_vector_to_bin_string(oneVec);
-- synopsys translate_on
  signal core_sinit: std_logic;
  signal core_ce: std_logic;
  signal op_net: std_logic_vector(op_width - 1 downto 0);
begin
  core_ce <= ce and en(0);
  core_sinit <= (clr or rst(0)) and ce;
  op <= op_net;
  comp0: if ((core_name0 = "cntr_11_0_86806e294f737f4c")) generate
    core_instance0: cntr_11_0_86806e294f737f4c
      port map (
        clk => clk,
        ce => core_ce,
        SINIT => core_sinit,
        q => op_net
      );
  end generate;
  comp1: if ((core_name0 = "cntr_11_0_5e62871cb125c52e")) generate
    core_instance1: cntr_11_0_5e62871cb125c52e
      port map (
        clk => clk,
        ce => core_ce,
        SINIT => core_sinit,
        load => load(0),
        l => din,
        q => op_net
      );
  end generate;
  comp2: if ((core_name0 = "cntr_11_0_78c1092db561b4bc")) generate
    core_instance2: cntr_11_0_78c1092db561b4bc
      port map (
        clk => clk,
        ce => core_ce,
        SINIT => core_sinit,
        q => op_net
      );
  end generate;
end behavior;
library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity constant_5f4d695836 is
  port (
    op : out std_logic_vector((8 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end constant_5f4d695836;


architecture behavior of constant_5f4d695836 is
begin
  op <= "10011111";
end behavior;

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity relational_54048c8b02 is
  port (
    a : in std_logic_vector((8 - 1) downto 0);
    b : in std_logic_vector((8 - 1) downto 0);
    op : out std_logic_vector((1 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end relational_54048c8b02;


architecture behavior of relational_54048c8b02 is
  signal a_1_31: unsigned((8 - 1) downto 0);
  signal b_1_34: unsigned((8 - 1) downto 0);
  signal result_12_3_rel: boolean;
begin
  a_1_31 <= std_logic_vector_to_unsigned(a);
  b_1_34 <= std_logic_vector_to_unsigned(b);
  result_12_3_rel <= a_1_31 = b_1_34;
  op <= boolean_to_vector(result_12_3_rel);
end behavior;

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity concat_62c4475a80 is
  port (
    in0 : in std_logic_vector((32 - 1) downto 0);
    in1 : in std_logic_vector((32 - 1) downto 0);
    y : out std_logic_vector((64 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end concat_62c4475a80;


architecture behavior of concat_62c4475a80 is
  signal in0_1_23: unsigned((32 - 1) downto 0);
  signal in1_1_27: unsigned((32 - 1) downto 0);
  signal y_2_1_concat: unsigned((64 - 1) downto 0);
begin
  in0_1_23 <= std_logic_vector_to_unsigned(in0);
  in1_1_27 <= std_logic_vector_to_unsigned(in1);
  y_2_1_concat <= std_logic_vector_to_unsigned(unsigned_to_std_logic_vector(in0_1_23) & unsigned_to_std_logic_vector(in1_1_27));
  y <= unsigned_to_std_logic_vector(y_2_1_concat);
end behavior;

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;
use work.conv_pkg.all;

entity logical_aacf6e1b0e is
  port (
    d0 : in std_logic_vector((1 - 1) downto 0);
    d1 : in std_logic_vector((1 - 1) downto 0);
    y : out std_logic_vector((1 - 1) downto 0);
    clk : in std_logic;
    ce : in std_logic;
    clr : in std_logic);
end logical_aacf6e1b0e;


architecture behavior of logical_aacf6e1b0e is
  signal d0_1_24: std_logic;
  signal d1_1_27: std_logic;
  signal fully_2_1_bit: std_logic;
begin
  d0_1_24 <= d0(0);
  d1_1_27 <= d1(0);
  fully_2_1_bit <= d0_1_24 or d1_1_27;
  y <= std_logic_to_vector(fully_2_1_bit);
end behavior;

library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw/EDK Processor"

entity edk_processor_entity_41d480e4f4 is
  port (
    axi_aresetn: in std_logic; 
    from_register: in std_logic_vector(31 downto 0); 
    from_register1: in std_logic_vector(31 downto 0); 
    from_register2: in std_logic_vector(31 downto 0); 
    from_register3: in std_logic_vector(31 downto 0); 
    plb_ce_1: in std_logic; 
    plb_clk_1: in std_logic; 
    s_axi_araddr: in std_logic_vector(31 downto 0); 
    s_axi_arburst: in std_logic_vector(1 downto 0); 
    s_axi_arcache: in std_logic_vector(3 downto 0); 
    s_axi_arid: in std_logic_vector(7 downto 0); 
    s_axi_arlen: in std_logic_vector(7 downto 0); 
    s_axi_arlock: in std_logic_vector(1 downto 0); 
    s_axi_arprot: in std_logic_vector(2 downto 0); 
    s_axi_arsize: in std_logic_vector(2 downto 0); 
    s_axi_arvalid: in std_logic; 
    s_axi_awaddr: in std_logic_vector(31 downto 0); 
    s_axi_awburst: in std_logic_vector(1 downto 0); 
    s_axi_awcache: in std_logic_vector(3 downto 0); 
    s_axi_awid: in std_logic_vector(7 downto 0); 
    s_axi_awlen: in std_logic_vector(7 downto 0); 
    s_axi_awlock: in std_logic_vector(1 downto 0); 
    s_axi_awprot: in std_logic_vector(2 downto 0); 
    s_axi_awsize: in std_logic_vector(2 downto 0); 
    s_axi_awvalid: in std_logic; 
    s_axi_bready: in std_logic; 
    s_axi_rready: in std_logic; 
    s_axi_wdata: in std_logic_vector(31 downto 0); 
    s_axi_wlast: in std_logic; 
    s_axi_wstrb: in std_logic_vector(3 downto 0); 
    s_axi_wvalid: in std_logic; 
    to_register: in std_logic_vector(31 downto 0); 
    to_register1: in std_logic_vector(31 downto 0); 
    to_register2: in std_logic_vector(31 downto 0); 
    memmap_x0: out std_logic; 
    memmap_x1: out std_logic; 
    memmap_x10: out std_logic; 
    memmap_x11: out std_logic_vector(31 downto 0); 
    memmap_x12: out std_logic; 
    memmap_x13: out std_logic_vector(31 downto 0); 
    memmap_x14: out std_logic; 
    memmap_x15: out std_logic_vector(31 downto 0); 
    memmap_x16: out std_logic; 
    memmap_x2: out std_logic_vector(7 downto 0); 
    memmap_x3: out std_logic_vector(1 downto 0); 
    memmap_x4: out std_logic; 
    memmap_x5: out std_logic_vector(31 downto 0); 
    memmap_x6: out std_logic_vector(7 downto 0); 
    memmap_x7: out std_logic; 
    memmap_x8: out std_logic_vector(1 downto 0); 
    memmap_x9: out std_logic
  );
end edk_processor_entity_41d480e4f4;

architecture structural of edk_processor_entity_41d480e4f4 is
  signal axi_aresetn_net_x0: std_logic;
  signal from_register1_data_out_net_x0: std_logic_vector(31 downto 0);
  signal from_register2_data_out_net_x0: std_logic_vector(31 downto 0);
  signal from_register3_data_out_net_x0: std_logic_vector(31 downto 0);
  signal from_register_data_out_net_x0: std_logic_vector(31 downto 0);
  signal memmap_s_axi_arready_net_x0: std_logic;
  signal memmap_s_axi_awready_net_x0: std_logic;
  signal memmap_s_axi_bid_net_x0: std_logic_vector(7 downto 0);
  signal memmap_s_axi_bresp_net_x0: std_logic_vector(1 downto 0);
  signal memmap_s_axi_bvalid_net_x0: std_logic;
  signal memmap_s_axi_rdata_net_x0: std_logic_vector(31 downto 0);
  signal memmap_s_axi_rid_net_x0: std_logic_vector(7 downto 0);
  signal memmap_s_axi_rlast_net_x0: std_logic;
  signal memmap_s_axi_rresp_net_x0: std_logic_vector(1 downto 0);
  signal memmap_s_axi_rvalid_net_x0: std_logic;
  signal memmap_s_axi_wready_net_x0: std_logic;
  signal memmap_sm_control_din_net_x0: std_logic_vector(31 downto 0);
  signal memmap_sm_control_en_net_x0: std_logic;
  signal memmap_sm_new_mac_time_lsb_din_net_x0: std_logic_vector(31 downto 0);
  signal memmap_sm_new_mac_time_lsb_en_net_x0: std_logic;
  signal memmap_sm_new_mac_time_msb_din_net_x0: std_logic_vector(31 downto 0);
  signal memmap_sm_new_mac_time_msb_en_net_x0: std_logic;
  signal plb_ce_1_sg_x0: std_logic;
  signal plb_clk_1_sg_x0: std_logic;
  signal s_axi_araddr_net_x0: std_logic_vector(31 downto 0);
  signal s_axi_arburst_net_x0: std_logic_vector(1 downto 0);
  signal s_axi_arcache_net_x0: std_logic_vector(3 downto 0);
  signal s_axi_arid_net_x0: std_logic_vector(7 downto 0);
  signal s_axi_arlen_net_x0: std_logic_vector(7 downto 0);
  signal s_axi_arlock_net_x0: std_logic_vector(1 downto 0);
  signal s_axi_arprot_net_x0: std_logic_vector(2 downto 0);
  signal s_axi_arsize_net_x0: std_logic_vector(2 downto 0);
  signal s_axi_arvalid_net_x0: std_logic;
  signal s_axi_awaddr_net_x0: std_logic_vector(31 downto 0);
  signal s_axi_awburst_net_x0: std_logic_vector(1 downto 0);
  signal s_axi_awcache_net_x0: std_logic_vector(3 downto 0);
  signal s_axi_awid_net_x0: std_logic_vector(7 downto 0);
  signal s_axi_awlen_net_x0: std_logic_vector(7 downto 0);
  signal s_axi_awlock_net_x0: std_logic_vector(1 downto 0);
  signal s_axi_awprot_net_x0: std_logic_vector(2 downto 0);
  signal s_axi_awsize_net_x0: std_logic_vector(2 downto 0);
  signal s_axi_awvalid_net_x0: std_logic;
  signal s_axi_bready_net_x0: std_logic;
  signal s_axi_rready_net_x0: std_logic;
  signal s_axi_wdata_net_x0: std_logic_vector(31 downto 0);
  signal s_axi_wlast_net_x0: std_logic;
  signal s_axi_wstrb_net_x0: std_logic_vector(3 downto 0);
  signal s_axi_wvalid_net_x0: std_logic;
  signal to_register1_dout_net_x0: std_logic_vector(31 downto 0);
  signal to_register2_dout_net_x0: std_logic_vector(31 downto 0);
  signal to_register_dout_net_x0: std_logic_vector(31 downto 0);

begin
  axi_aresetn_net_x0 <= axi_aresetn;
  from_register_data_out_net_x0 <= from_register;
  from_register1_data_out_net_x0 <= from_register1;
  from_register2_data_out_net_x0 <= from_register2;
  from_register3_data_out_net_x0 <= from_register3;
  plb_ce_1_sg_x0 <= plb_ce_1;
  plb_clk_1_sg_x0 <= plb_clk_1;
  s_axi_araddr_net_x0 <= s_axi_araddr;
  s_axi_arburst_net_x0 <= s_axi_arburst;
  s_axi_arcache_net_x0 <= s_axi_arcache;
  s_axi_arid_net_x0 <= s_axi_arid;
  s_axi_arlen_net_x0 <= s_axi_arlen;
  s_axi_arlock_net_x0 <= s_axi_arlock;
  s_axi_arprot_net_x0 <= s_axi_arprot;
  s_axi_arsize_net_x0 <= s_axi_arsize;
  s_axi_arvalid_net_x0 <= s_axi_arvalid;
  s_axi_awaddr_net_x0 <= s_axi_awaddr;
  s_axi_awburst_net_x0 <= s_axi_awburst;
  s_axi_awcache_net_x0 <= s_axi_awcache;
  s_axi_awid_net_x0 <= s_axi_awid;
  s_axi_awlen_net_x0 <= s_axi_awlen;
  s_axi_awlock_net_x0 <= s_axi_awlock;
  s_axi_awprot_net_x0 <= s_axi_awprot;
  s_axi_awsize_net_x0 <= s_axi_awsize;
  s_axi_awvalid_net_x0 <= s_axi_awvalid;
  s_axi_bready_net_x0 <= s_axi_bready;
  s_axi_rready_net_x0 <= s_axi_rready;
  s_axi_wdata_net_x0 <= s_axi_wdata;
  s_axi_wlast_net_x0 <= s_axi_wlast;
  s_axi_wstrb_net_x0 <= s_axi_wstrb;
  s_axi_wvalid_net_x0 <= s_axi_wvalid;
  to_register_dout_net_x0 <= to_register;
  to_register1_dout_net_x0 <= to_register1;
  to_register2_dout_net_x0 <= to_register2;
  memmap_x0 <= memmap_s_axi_arready_net_x0;
  memmap_x1 <= memmap_s_axi_awready_net_x0;
  memmap_x10 <= memmap_s_axi_wready_net_x0;
  memmap_x11 <= memmap_sm_new_mac_time_msb_din_net_x0;
  memmap_x12 <= memmap_sm_new_mac_time_msb_en_net_x0;
  memmap_x13 <= memmap_sm_new_mac_time_lsb_din_net_x0;
  memmap_x14 <= memmap_sm_new_mac_time_lsb_en_net_x0;
  memmap_x15 <= memmap_sm_control_din_net_x0;
  memmap_x16 <= memmap_sm_control_en_net_x0;
  memmap_x2 <= memmap_s_axi_bid_net_x0;
  memmap_x3 <= memmap_s_axi_bresp_net_x0;
  memmap_x4 <= memmap_s_axi_bvalid_net_x0;
  memmap_x5 <= memmap_s_axi_rdata_net_x0;
  memmap_x6 <= memmap_s_axi_rid_net_x0;
  memmap_x7 <= memmap_s_axi_rlast_net_x0;
  memmap_x8 <= memmap_s_axi_rresp_net_x0;
  memmap_x9 <= memmap_s_axi_rvalid_net_x0;

  memmap: entity work.axi_sgiface
    port map (
      axi_aclk => plb_clk_1_sg_x0,
      axi_aresetn => axi_aresetn_net_x0,
      axi_ce => plb_ce_1_sg_x0,
      s_axi_araddr => s_axi_araddr_net_x0,
      s_axi_arburst => s_axi_arburst_net_x0,
      s_axi_arcache => s_axi_arcache_net_x0,
      s_axi_arid => s_axi_arid_net_x0,
      s_axi_arlen => s_axi_arlen_net_x0,
      s_axi_arlock => s_axi_arlock_net_x0,
      s_axi_arprot => s_axi_arprot_net_x0,
      s_axi_arsize => s_axi_arsize_net_x0,
      s_axi_arvalid => s_axi_arvalid_net_x0,
      s_axi_awaddr => s_axi_awaddr_net_x0,
      s_axi_awburst => s_axi_awburst_net_x0,
      s_axi_awcache => s_axi_awcache_net_x0,
      s_axi_awid => s_axi_awid_net_x0,
      s_axi_awlen => s_axi_awlen_net_x0,
      s_axi_awlock => s_axi_awlock_net_x0,
      s_axi_awprot => s_axi_awprot_net_x0,
      s_axi_awsize => s_axi_awsize_net_x0,
      s_axi_awvalid => s_axi_awvalid_net_x0,
      s_axi_bready => s_axi_bready_net_x0,
      s_axi_rready => s_axi_rready_net_x0,
      s_axi_wdata => s_axi_wdata_net_x0,
      s_axi_wlast => s_axi_wlast_net_x0,
      s_axi_wstrb => s_axi_wstrb_net_x0,
      s_axi_wvalid => s_axi_wvalid_net_x0,
      sm_control_dout => to_register2_dout_net_x0,
      sm_mac_time_usec_lsb_dout => from_register3_data_out_net_x0,
      sm_mac_time_usec_msb_dout => from_register2_data_out_net_x0,
      sm_new_mac_time_lsb_dout => to_register1_dout_net_x0,
      sm_new_mac_time_msb_dout => to_register_dout_net_x0,
      sm_system_time_usec_lsb_dout => from_register1_data_out_net_x0,
      sm_system_time_usec_msb_dout => from_register_data_out_net_x0,
      s_axi_arready => memmap_s_axi_arready_net_x0,
      s_axi_awready => memmap_s_axi_awready_net_x0,
      s_axi_bid => memmap_s_axi_bid_net_x0,
      s_axi_bresp => memmap_s_axi_bresp_net_x0,
      s_axi_bvalid => memmap_s_axi_bvalid_net_x0,
      s_axi_rdata => memmap_s_axi_rdata_net_x0,
      s_axi_rid => memmap_s_axi_rid_net_x0,
      s_axi_rlast => memmap_s_axi_rlast_net_x0,
      s_axi_rresp => memmap_s_axi_rresp_net_x0,
      s_axi_rvalid => memmap_s_axi_rvalid_net_x0,
      s_axi_wready => memmap_s_axi_wready_net_x0,
      sm_control_din => memmap_sm_control_din_net_x0,
      sm_control_en => memmap_sm_control_en_net_x0,
      sm_new_mac_time_lsb_din => memmap_sm_new_mac_time_lsb_din_net_x0,
      sm_new_mac_time_lsb_en => memmap_sm_new_mac_time_lsb_en_net_x0,
      sm_new_mac_time_msb_din => memmap_sm_new_mac_time_msb_din_net_x0,
      sm_new_mac_time_msb_en => memmap_sm_new_mac_time_msb_en_net_x0
    );

end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw/Posedge"

entity posedge_entity_19bcf1b43c is
  port (
    ce_1: in std_logic; 
    clk_1: in std_logic; 
    d: in std_logic; 
    q: out std_logic
  );
end posedge_entity_19bcf1b43c;

architecture structural of posedge_entity_19bcf1b43c is
  signal b_1_y_net_x0: std_logic;
  signal ce_1_sg_x0: std_logic;
  signal clk_1_sg_x0: std_logic;
  signal delay_q_net: std_logic;
  signal inverter_op_net: std_logic;
  signal logical1_y_net_x0: std_logic;

begin
  ce_1_sg_x0 <= ce_1;
  clk_1_sg_x0 <= clk_1;
  b_1_y_net_x0 <= d;
  q <= logical1_y_net_x0;

  delay: entity work.xldelay
    generic map (
      latency => 1,
      reg_retiming => 0,
      reset => 0,
      width => 1
    )
    port map (
      ce => ce_1_sg_x0,
      clk => clk_1_sg_x0,
      d(0) => b_1_y_net_x0,
      en => '1',
      rst => '1',
      q(0) => delay_q_net
    );

  inverter: entity work.inverter_e5b38cca3b
    port map (
      ce => ce_1_sg_x0,
      clk => clk_1_sg_x0,
      clr => '0',
      ip(0) => delay_q_net,
      op(0) => inverter_op_net
    );

  logical1: entity work.logical_80f90b97d0
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      d0(0) => inverter_op_net,
      d1(0) => b_1_y_net_x0,
      y(0) => logical1_y_net_x0
    );

end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw/Registers/Ctrl Bits"

entity ctrl_bits_entity_469f4cf78c is
  port (
    x32b: in std_logic_vector(31 downto 0); 
    reg_load_mac_time: out std_logic; 
    reg_reset_system_time: out std_logic
  );
end ctrl_bits_entity_469f4cf78c;

architecture structural of ctrl_bits_entity_469f4cf78c is
  signal b_0_y_net: std_logic;
  signal b_1_y_net_x1: std_logic;
  signal register36_q_net_x0: std_logic_vector(31 downto 0);

begin
  register36_q_net_x0 <= x32b;
  reg_load_mac_time <= b_1_y_net_x1;
  reg_reset_system_time <= b_0_y_net;

  b_0: entity work.xlslice
    generic map (
      new_lsb => 0,
      new_msb => 0,
      x_width => 32,
      y_width => 1
    )
    port map (
      x => register36_q_net_x0,
      y(0) => b_0_y_net
    );

  b_1: entity work.xlslice
    generic map (
      new_lsb => 1,
      new_msb => 1,
      x_width => 32,
      y_width => 1
    )
    port map (
      x => register36_q_net_x0,
      y(0) => b_1_y_net_x1
    );

end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw/Registers"

entity registers_entity_9b36cfcda0 is
  port (
    ce_1: in std_logic; 
    clk_1: in std_logic; 
    from_register1: in std_logic_vector(31 downto 0); 
    from_register2: in std_logic_vector(31 downto 0); 
    from_register3: in std_logic_vector(31 downto 0); 
    mac_time_usec: in std_logic_vector(63 downto 0); 
    system_time_usec: in std_logic_vector(63 downto 0); 
    constant1_x0: out std_logic; 
    constant2_x0: out std_logic; 
    constant5_x0: out std_logic; 
    constant6_x0: out std_logic; 
    ctrl_bits: out std_logic; 
    ctrl_bits_x0: out std_logic; 
    reg_new_mac_time_lsb: out std_logic_vector(31 downto 0); 
    reg_new_mac_time_msb: out std_logic_vector(31 downto 0); 
    x32lsb1_x0: out std_logic_vector(31 downto 0); 
    x32lsb_x0: out std_logic_vector(31 downto 0); 
    x32msb1_x0: out std_logic_vector(31 downto 0); 
    x32msb_x0: out std_logic_vector(31 downto 0)
  );
end registers_entity_9b36cfcda0;

architecture structural of registers_entity_9b36cfcda0 is
  signal b_0_y_net_x0: std_logic;
  signal b_1_y_net_x2: std_logic;
  signal ce_1_sg_x1: std_logic;
  signal clk_1_sg_x1: std_logic;
  signal constant1_op_net_x0: std_logic;
  signal constant2_op_net_x0: std_logic;
  signal constant5_op_net_x0: std_logic;
  signal constant6_op_net_x0: std_logic;
  signal from_register1_data_out_net_x0: std_logic_vector(31 downto 0);
  signal from_register2_data_out_net_x0: std_logic_vector(31 downto 0);
  signal from_register3_data_out_net_x0: std_logic_vector(31 downto 0);
  signal register1_q_net: std_logic_vector(63 downto 0);
  signal register27_q_net_x0: std_logic_vector(31 downto 0);
  signal register28_q_net_x0: std_logic_vector(31 downto 0);
  signal register2_q_net: std_logic_vector(63 downto 0);
  signal register36_q_net_x0: std_logic_vector(31 downto 0);
  signal register50_q_net_x0: std_logic_vector(63 downto 0);
  signal register8_q_net_x0: std_logic_vector(63 downto 0);
  signal x32lsb1_y_net_x0: std_logic_vector(31 downto 0);
  signal x32lsb_y_net_x0: std_logic_vector(31 downto 0);
  signal x32msb1_y_net_x0: std_logic_vector(31 downto 0);
  signal x32msb_y_net_x0: std_logic_vector(31 downto 0);

begin
  ce_1_sg_x1 <= ce_1;
  clk_1_sg_x1 <= clk_1;
  from_register1_data_out_net_x0 <= from_register1;
  from_register2_data_out_net_x0 <= from_register2;
  from_register3_data_out_net_x0 <= from_register3;
  register8_q_net_x0 <= mac_time_usec;
  register50_q_net_x0 <= system_time_usec;
  constant1_x0 <= constant1_op_net_x0;
  constant2_x0 <= constant2_op_net_x0;
  constant5_x0 <= constant5_op_net_x0;
  constant6_x0 <= constant6_op_net_x0;
  ctrl_bits <= b_0_y_net_x0;
  ctrl_bits_x0 <= b_1_y_net_x2;
  reg_new_mac_time_lsb <= register28_q_net_x0;
  reg_new_mac_time_msb <= register27_q_net_x0;
  x32lsb1_x0 <= x32lsb1_y_net_x0;
  x32lsb_x0 <= x32lsb_y_net_x0;
  x32msb1_x0 <= x32msb1_y_net_x0;
  x32msb_x0 <= x32msb_y_net_x0;

  constant1: entity work.constant_6293007044
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      op(0) => constant1_op_net_x0
    );

  constant2: entity work.constant_6293007044
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      op(0) => constant2_op_net_x0
    );

  constant5: entity work.constant_6293007044
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      op(0) => constant5_op_net_x0
    );

  constant6: entity work.constant_6293007044
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      op(0) => constant6_op_net_x0
    );

  ctrl_bits_469f4cf78c: entity work.ctrl_bits_entity_469f4cf78c
    port map (
      x32b => register36_q_net_x0,
      reg_load_mac_time => b_1_y_net_x2,
      reg_reset_system_time => b_0_y_net_x0
    );

  register1: entity work.xlregister
    generic map (
      d_width => 64,
      init_value => b"0000000000000000000000000000000000000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x1,
      clk => clk_1_sg_x1,
      d => register8_q_net_x0,
      en => "1",
      rst => "0",
      q => register1_q_net
    );

  register2: entity work.xlregister
    generic map (
      d_width => 64,
      init_value => b"0000000000000000000000000000000000000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x1,
      clk => clk_1_sg_x1,
      d => register50_q_net_x0,
      en => "1",
      rst => "0",
      q => register2_q_net
    );

  register27: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x1,
      clk => clk_1_sg_x1,
      d => from_register3_data_out_net_x0,
      en => "1",
      rst => "0",
      q => register27_q_net_x0
    );

  register28: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x1,
      clk => clk_1_sg_x1,
      d => from_register2_data_out_net_x0,
      en => "1",
      rst => "0",
      q => register28_q_net_x0
    );

  register36: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x1,
      clk => clk_1_sg_x1,
      d => from_register1_data_out_net_x0,
      en => "1",
      rst => "0",
      q => register36_q_net_x0
    );

  x32lsb: entity work.xlslice
    generic map (
      new_lsb => 0,
      new_msb => 31,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register1_q_net,
      y => x32lsb_y_net_x0
    );

  x32lsb1: entity work.xlslice
    generic map (
      new_lsb => 0,
      new_msb => 31,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register2_q_net,
      y => x32lsb1_y_net_x0
    );

  x32msb: entity work.xlslice
    generic map (
      new_lsb => 32,
      new_msb => 63,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register1_q_net,
      y => x32msb_y_net_x0
    );

  x32msb1: entity work.xlslice
    generic map (
      new_lsb => 32,
      new_msb => 63,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register2_q_net,
      y => x32msb1_y_net_x0
    );

end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw/usec Pulse"

entity usec_pulse_entity_1defadf13e is
  port (
    ce_1: in std_logic; 
    clk_1: in std_logic; 
    usec: out std_logic; 
    usec_fractional_part: out std_logic_vector(7 downto 0)
  );
end usec_pulse_entity_1defadf13e;

architecture structural of usec_pulse_entity_1defadf13e is
  signal ce_1_sg_x2: std_logic;
  signal clk_1_sg_x2: std_logic;
  signal clk_usec_op_net: std_logic_vector(7 downto 0);
  signal constant1_op_net: std_logic_vector(7 downto 0);
  signal relational_op_net_x0: std_logic;

begin
  ce_1_sg_x2 <= ce_1;
  clk_1_sg_x2 <= clk_1;
  usec <= relational_op_net_x0;
  usec_fractional_part <= clk_usec_op_net;

  clk_usec: entity work.xlcounter_free_wlan_mac_time_hw
    generic map (
      core_name0 => "cntr_11_0_86806e294f737f4c",
      op_arith => xlUnsigned,
      op_width => 8
    )
    port map (
      ce => ce_1_sg_x2,
      clk => clk_1_sg_x2,
      clr => '0',
      en => "1",
      rst(0) => relational_op_net_x0,
      op => clk_usec_op_net
    );

  constant1: entity work.constant_5f4d695836
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      op => constant1_op_net
    );

  relational: entity work.relational_54048c8b02
    port map (
      a => clk_usec_op_net,
      b => constant1_op_net,
      ce => '0',
      clk => '0',
      clr => '0',
      op(0) => relational_op_net_x0
    );

end structural;
library IEEE;
use IEEE.std_logic_1164.all;
use work.conv_pkg.all;

-- Generated from Simulink block "wlan_mac_time_hw"

entity wlan_mac_time_hw is
  port (
    axi_aresetn: in std_logic; 
    ce_1: in std_logic; 
    clk_1: in std_logic; 
    data_out: in std_logic_vector(31 downto 0); 
    data_out_x0: in std_logic_vector(31 downto 0); 
    data_out_x1: in std_logic_vector(31 downto 0); 
    data_out_x2: in std_logic_vector(31 downto 0); 
    data_out_x3: in std_logic_vector(31 downto 0); 
    data_out_x4: in std_logic_vector(31 downto 0); 
    data_out_x5: in std_logic_vector(31 downto 0); 
    dout: in std_logic_vector(31 downto 0); 
    dout_x0: in std_logic_vector(31 downto 0); 
    dout_x1: in std_logic_vector(31 downto 0); 
    plb_ce_1: in std_logic; 
    plb_clk_1: in std_logic; 
    s_axi_araddr: in std_logic_vector(31 downto 0); 
    s_axi_arburst: in std_logic_vector(1 downto 0); 
    s_axi_arcache: in std_logic_vector(3 downto 0); 
    s_axi_arid: in std_logic_vector(7 downto 0); 
    s_axi_arlen: in std_logic_vector(7 downto 0); 
    s_axi_arlock: in std_logic_vector(1 downto 0); 
    s_axi_arprot: in std_logic_vector(2 downto 0); 
    s_axi_arsize: in std_logic_vector(2 downto 0); 
    s_axi_arvalid: in std_logic; 
    s_axi_awaddr: in std_logic_vector(31 downto 0); 
    s_axi_awburst: in std_logic_vector(1 downto 0); 
    s_axi_awcache: in std_logic_vector(3 downto 0); 
    s_axi_awid: in std_logic_vector(7 downto 0); 
    s_axi_awlen: in std_logic_vector(7 downto 0); 
    s_axi_awlock: in std_logic_vector(1 downto 0); 
    s_axi_awprot: in std_logic_vector(2 downto 0); 
    s_axi_awsize: in std_logic_vector(2 downto 0); 
    s_axi_awvalid: in std_logic; 
    s_axi_bready: in std_logic; 
    s_axi_rready: in std_logic; 
    s_axi_wdata: in std_logic_vector(31 downto 0); 
    s_axi_wlast: in std_logic; 
    s_axi_wstrb: in std_logic_vector(3 downto 0); 
    s_axi_wvalid: in std_logic; 
    data_in: out std_logic_vector(31 downto 0); 
    data_in_x0: out std_logic_vector(31 downto 0); 
    data_in_x1: out std_logic_vector(31 downto 0); 
    data_in_x2: out std_logic_vector(31 downto 0); 
    data_in_x3: out std_logic_vector(31 downto 0); 
    data_in_x4: out std_logic_vector(31 downto 0); 
    data_in_x5: out std_logic_vector(31 downto 0); 
    en: out std_logic; 
    en_x0: out std_logic; 
    en_x1: out std_logic; 
    en_x2: out std_logic; 
    en_x3: out std_logic; 
    en_x4: out std_logic; 
    en_x5: out std_logic; 
    mac_time_lsb: out std_logic_vector(31 downto 0); 
    mac_time_msb: out std_logic_vector(31 downto 0); 
    s_axi_arready: out std_logic; 
    s_axi_awready: out std_logic; 
    s_axi_bid: out std_logic_vector(7 downto 0); 
    s_axi_bresp: out std_logic_vector(1 downto 0); 
    s_axi_bvalid: out std_logic; 
    s_axi_rdata: out std_logic_vector(31 downto 0); 
    s_axi_rid: out std_logic_vector(7 downto 0); 
    s_axi_rlast: out std_logic; 
    s_axi_rresp: out std_logic_vector(1 downto 0); 
    s_axi_rvalid: out std_logic; 
    s_axi_wready: out std_logic; 
    system_time_lsb: out std_logic_vector(31 downto 0); 
    system_time_msb: out std_logic_vector(31 downto 0); 
    time_usec_frac: out std_logic_vector(7 downto 0); 
    usec_pulse: out std_logic
  );
end wlan_mac_time_hw;

architecture structural of wlan_mac_time_hw is
  attribute core_generation_info: string;
  attribute core_generation_info of structural : architecture is "wlan_mac_time_hw,sysgen_core,{clock_period=10.00000000,clocking=Clock_Enables,sample_periods=1.00000000000 1.00000000000,testbench=0,total_blocks=185,xilinx_arithmetic_relational_operator_block=1,xilinx_bit_slice_extractor_block=16,xilinx_bus_concatenator_block=1,xilinx_constant_block_block=5,xilinx_counter_block=3,xilinx_delay_block=1,xilinx_disregard_subsystem_for_generation_block=1,xilinx_edk_core_block=1,xilinx_edk_processor_block=1,xilinx_gateway_in_block=26,xilinx_gateway_out_block=17,xilinx_inverter_block=1,xilinx_logical_block_block=2,xilinx_register_block=20,xilinx_shared_memory_based_from_register_block=7,xilinx_shared_memory_based_to_register_block=7,xilinx_system_generator_block=1,}";

  signal axi_aresetn_net: std_logic;
  signal b_0_y_net_x0: std_logic;
  signal b_1_y_net_x2: std_logic;
  signal ce_1_sg_x3: std_logic;
  signal clk_1_sg_x3: std_logic;
  signal clk_usec_op_net: std_logic_vector(7 downto 0);
  signal concat_y_net: std_logic_vector(63 downto 0);
  signal data_in_net: std_logic_vector(31 downto 0);
  signal data_in_x0_net: std_logic_vector(31 downto 0);
  signal data_in_x1_net: std_logic_vector(31 downto 0);
  signal data_in_x2_net: std_logic_vector(31 downto 0);
  signal data_in_x3_net: std_logic_vector(31 downto 0);
  signal data_in_x4_net: std_logic_vector(31 downto 0);
  signal data_in_x5_net: std_logic_vector(31 downto 0);
  signal data_out_net: std_logic_vector(31 downto 0);
  signal data_out_x0_net: std_logic_vector(31 downto 0);
  signal data_out_x1_net: std_logic_vector(31 downto 0);
  signal data_out_x2_net: std_logic_vector(31 downto 0);
  signal data_out_x3_net: std_logic_vector(31 downto 0);
  signal data_out_x4_net: std_logic_vector(31 downto 0);
  signal data_out_x5_net: std_logic_vector(31 downto 0);
  signal dout_net: std_logic_vector(31 downto 0);
  signal dout_x0_net: std_logic_vector(31 downto 0);
  signal dout_x1_net: std_logic_vector(31 downto 0);
  signal en_net: std_logic;
  signal en_x0_net: std_logic;
  signal en_x1_net: std_logic;
  signal en_x2_net: std_logic;
  signal en_x3_net: std_logic;
  signal en_x4_net: std_logic;
  signal en_x5_net: std_logic;
  signal logical1_y_net_x0: std_logic;
  signal logical_y_net: std_logic;
  signal mac_time_64_bit_counter_op_net: std_logic_vector(63 downto 0);
  signal mac_time_lsb_net: std_logic_vector(31 downto 0);
  signal mac_time_msb_net: std_logic_vector(31 downto 0);
  signal plb_ce_1_sg_x1: std_logic;
  signal plb_clk_1_sg_x1: std_logic;
  signal register11_q_net: std_logic_vector(7 downto 0);
  signal register13_q_net: std_logic_vector(7 downto 0);
  signal register1_q_net: std_logic_vector(31 downto 0);
  signal register27_q_net_x0: std_logic_vector(31 downto 0);
  signal register28_q_net_x0: std_logic_vector(31 downto 0);
  signal register2_q_net: std_logic_vector(31 downto 0);
  signal register50_q_net_x0: std_logic_vector(63 downto 0);
  signal register5_q_net: std_logic_vector(31 downto 0);
  signal register6_q_net: std_logic_vector(31 downto 0);
  signal register8_q_net_x0: std_logic_vector(63 downto 0);
  signal register9_q_net: std_logic;
  signal relational_op_net_x0: std_logic;
  signal s_axi_araddr_net: std_logic_vector(31 downto 0);
  signal s_axi_arburst_net: std_logic_vector(1 downto 0);
  signal s_axi_arcache_net: std_logic_vector(3 downto 0);
  signal s_axi_arid_net: std_logic_vector(7 downto 0);
  signal s_axi_arlen_net: std_logic_vector(7 downto 0);
  signal s_axi_arlock_net: std_logic_vector(1 downto 0);
  signal s_axi_arprot_net: std_logic_vector(2 downto 0);
  signal s_axi_arready_net: std_logic;
  signal s_axi_arsize_net: std_logic_vector(2 downto 0);
  signal s_axi_arvalid_net: std_logic;
  signal s_axi_awaddr_net: std_logic_vector(31 downto 0);
  signal s_axi_awburst_net: std_logic_vector(1 downto 0);
  signal s_axi_awcache_net: std_logic_vector(3 downto 0);
  signal s_axi_awid_net: std_logic_vector(7 downto 0);
  signal s_axi_awlen_net: std_logic_vector(7 downto 0);
  signal s_axi_awlock_net: std_logic_vector(1 downto 0);
  signal s_axi_awprot_net: std_logic_vector(2 downto 0);
  signal s_axi_awready_net: std_logic;
  signal s_axi_awsize_net: std_logic_vector(2 downto 0);
  signal s_axi_awvalid_net: std_logic;
  signal s_axi_bid_net: std_logic_vector(7 downto 0);
  signal s_axi_bready_net: std_logic;
  signal s_axi_bresp_net: std_logic_vector(1 downto 0);
  signal s_axi_bvalid_net: std_logic;
  signal s_axi_rdata_net: std_logic_vector(31 downto 0);
  signal s_axi_rid_net: std_logic_vector(7 downto 0);
  signal s_axi_rlast_net: std_logic;
  signal s_axi_rready_net: std_logic;
  signal s_axi_rresp_net: std_logic_vector(1 downto 0);
  signal s_axi_rvalid_net: std_logic;
  signal s_axi_wdata_net: std_logic_vector(31 downto 0);
  signal s_axi_wlast_net: std_logic;
  signal s_axi_wready_net: std_logic;
  signal s_axi_wstrb_net: std_logic_vector(3 downto 0);
  signal s_axi_wvalid_net: std_logic;
  signal system_time_64_bit_counter_op_net: std_logic_vector(63 downto 0);
  signal system_time_lsb_net: std_logic_vector(31 downto 0);
  signal system_time_msb_net: std_logic_vector(31 downto 0);
  signal time_usec_frac_net: std_logic_vector(7 downto 0);
  signal usec_pulse_net: std_logic;
  signal x32lsb1_y_net: std_logic_vector(31 downto 0);
  signal x32lsb_y_net: std_logic_vector(31 downto 0);
  signal x32msb1_y_net: std_logic_vector(31 downto 0);
  signal x32msb_y_net: std_logic_vector(31 downto 0);

begin
  axi_aresetn_net <= axi_aresetn;
  ce_1_sg_x3 <= ce_1;
  clk_1_sg_x3 <= clk_1;
  data_out_net <= data_out;
  data_out_x0_net <= data_out_x0;
  data_out_x1_net <= data_out_x1;
  data_out_x2_net <= data_out_x2;
  data_out_x3_net <= data_out_x3;
  data_out_x4_net <= data_out_x4;
  data_out_x5_net <= data_out_x5;
  dout_net <= dout;
  dout_x0_net <= dout_x0;
  dout_x1_net <= dout_x1;
  plb_ce_1_sg_x1 <= plb_ce_1;
  plb_clk_1_sg_x1 <= plb_clk_1;
  s_axi_araddr_net <= s_axi_araddr;
  s_axi_arburst_net <= s_axi_arburst;
  s_axi_arcache_net <= s_axi_arcache;
  s_axi_arid_net <= s_axi_arid;
  s_axi_arlen_net <= s_axi_arlen;
  s_axi_arlock_net <= s_axi_arlock;
  s_axi_arprot_net <= s_axi_arprot;
  s_axi_arsize_net <= s_axi_arsize;
  s_axi_arvalid_net <= s_axi_arvalid;
  s_axi_awaddr_net <= s_axi_awaddr;
  s_axi_awburst_net <= s_axi_awburst;
  s_axi_awcache_net <= s_axi_awcache;
  s_axi_awid_net <= s_axi_awid;
  s_axi_awlen_net <= s_axi_awlen;
  s_axi_awlock_net <= s_axi_awlock;
  s_axi_awprot_net <= s_axi_awprot;
  s_axi_awsize_net <= s_axi_awsize;
  s_axi_awvalid_net <= s_axi_awvalid;
  s_axi_bready_net <= s_axi_bready;
  s_axi_rready_net <= s_axi_rready;
  s_axi_wdata_net <= s_axi_wdata;
  s_axi_wlast_net <= s_axi_wlast;
  s_axi_wstrb_net <= s_axi_wstrb;
  s_axi_wvalid_net <= s_axi_wvalid;
  data_in <= data_in_net;
  data_in_x0 <= data_in_x0_net;
  data_in_x1 <= data_in_x1_net;
  data_in_x2 <= data_in_x2_net;
  data_in_x3 <= data_in_x3_net;
  data_in_x4 <= data_in_x4_net;
  data_in_x5 <= data_in_x5_net;
  en <= en_net;
  en_x0 <= en_x0_net;
  en_x1 <= en_x1_net;
  en_x2 <= en_x2_net;
  en_x3 <= en_x3_net;
  en_x4 <= en_x4_net;
  en_x5 <= en_x5_net;
  mac_time_lsb <= mac_time_lsb_net;
  mac_time_msb <= mac_time_msb_net;
  s_axi_arready <= s_axi_arready_net;
  s_axi_awready <= s_axi_awready_net;
  s_axi_bid <= s_axi_bid_net;
  s_axi_bresp <= s_axi_bresp_net;
  s_axi_bvalid <= s_axi_bvalid_net;
  s_axi_rdata <= s_axi_rdata_net;
  s_axi_rid <= s_axi_rid_net;
  s_axi_rlast <= s_axi_rlast_net;
  s_axi_rresp <= s_axi_rresp_net;
  s_axi_rvalid <= s_axi_rvalid_net;
  s_axi_wready <= s_axi_wready_net;
  system_time_lsb <= system_time_lsb_net;
  system_time_msb <= system_time_msb_net;
  time_usec_frac <= time_usec_frac_net;
  usec_pulse <= usec_pulse_net;

  concat: entity work.concat_62c4475a80
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      in0 => register27_q_net_x0,
      in1 => register28_q_net_x0,
      y => concat_y_net
    );

  edk_processor_41d480e4f4: entity work.edk_processor_entity_41d480e4f4
    port map (
      axi_aresetn => axi_aresetn_net,
      from_register => data_out_net,
      from_register1 => data_out_x0_net,
      from_register2 => data_out_x1_net,
      from_register3 => data_out_x2_net,
      plb_ce_1 => plb_ce_1_sg_x1,
      plb_clk_1 => plb_clk_1_sg_x1,
      s_axi_araddr => s_axi_araddr_net,
      s_axi_arburst => s_axi_arburst_net,
      s_axi_arcache => s_axi_arcache_net,
      s_axi_arid => s_axi_arid_net,
      s_axi_arlen => s_axi_arlen_net,
      s_axi_arlock => s_axi_arlock_net,
      s_axi_arprot => s_axi_arprot_net,
      s_axi_arsize => s_axi_arsize_net,
      s_axi_arvalid => s_axi_arvalid_net,
      s_axi_awaddr => s_axi_awaddr_net,
      s_axi_awburst => s_axi_awburst_net,
      s_axi_awcache => s_axi_awcache_net,
      s_axi_awid => s_axi_awid_net,
      s_axi_awlen => s_axi_awlen_net,
      s_axi_awlock => s_axi_awlock_net,
      s_axi_awprot => s_axi_awprot_net,
      s_axi_awsize => s_axi_awsize_net,
      s_axi_awvalid => s_axi_awvalid_net,
      s_axi_bready => s_axi_bready_net,
      s_axi_rready => s_axi_rready_net,
      s_axi_wdata => s_axi_wdata_net,
      s_axi_wlast => s_axi_wlast_net,
      s_axi_wstrb => s_axi_wstrb_net,
      s_axi_wvalid => s_axi_wvalid_net,
      to_register => dout_net,
      to_register1 => dout_x0_net,
      to_register2 => dout_x1_net,
      memmap_x0 => s_axi_arready_net,
      memmap_x1 => s_axi_awready_net,
      memmap_x10 => s_axi_wready_net,
      memmap_x11 => data_in_net,
      memmap_x12 => en_net,
      memmap_x13 => data_in_x0_net,
      memmap_x14 => en_x0_net,
      memmap_x15 => data_in_x1_net,
      memmap_x16 => en_x1_net,
      memmap_x2 => s_axi_bid_net,
      memmap_x3 => s_axi_bresp_net,
      memmap_x4 => s_axi_bvalid_net,
      memmap_x5 => s_axi_rdata_net,
      memmap_x6 => s_axi_rid_net,
      memmap_x7 => s_axi_rlast_net,
      memmap_x8 => s_axi_rresp_net,
      memmap_x9 => s_axi_rvalid_net
    );

  logical: entity work.logical_aacf6e1b0e
    port map (
      ce => '0',
      clk => '0',
      clr => '0',
      d0(0) => logical1_y_net_x0,
      d1(0) => relational_op_net_x0,
      y(0) => logical_y_net
    );

  mac_time_64_bit_counter: entity work.xlcounter_free_wlan_mac_time_hw
    generic map (
      core_name0 => "cntr_11_0_5e62871cb125c52e",
      op_arith => xlUnsigned,
      op_width => 64
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      clr => '0',
      din => concat_y_net,
      en(0) => logical_y_net,
      load(0) => logical1_y_net_x0,
      rst => "0",
      op => mac_time_64_bit_counter_op_net
    );

  posedge_19bcf1b43c: entity work.posedge_entity_19bcf1b43c
    port map (
      ce_1 => ce_1_sg_x3,
      clk_1 => clk_1_sg_x3,
      d => b_1_y_net_x2,
      q => logical1_y_net_x0
    );

  register1: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => x32msb_y_net,
      en => "1",
      rst => "0",
      q => register1_q_net
    );

  register10: entity work.xlregister
    generic map (
      d_width => 1,
      init_value => b"0"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d(0) => register9_q_net,
      en => "1",
      rst => "0",
      q(0) => usec_pulse_net
    );

  register11: entity work.xlregister
    generic map (
      d_width => 8,
      init_value => b"00000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register13_q_net,
      en => "1",
      rst => "0",
      q => register11_q_net
    );

  register12: entity work.xlregister
    generic map (
      d_width => 8,
      init_value => b"00000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register11_q_net,
      en => "1",
      rst => "0",
      q => time_usec_frac_net
    );

  register13: entity work.xlregister
    generic map (
      d_width => 8,
      init_value => b"00000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => clk_usec_op_net,
      en => "1",
      rst => "0",
      q => register13_q_net
    );

  register2: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => x32lsb_y_net,
      en => "1",
      rst => "0",
      q => register2_q_net
    );

  register3: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register2_q_net,
      en => "1",
      rst => "0",
      q => mac_time_lsb_net
    );

  register4: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register5_q_net,
      en => "1",
      rst => "0",
      q => system_time_msb_net
    );

  register5: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => x32msb1_y_net,
      en => "1",
      rst => "0",
      q => register5_q_net
    );

  register50: entity work.xlregister
    generic map (
      d_width => 64,
      init_value => b"0000000000000000000000000000000000000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => system_time_64_bit_counter_op_net,
      en => "1",
      rst => "0",
      q => register50_q_net_x0
    );

  register6: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => x32lsb1_y_net,
      en => "1",
      rst => "0",
      q => register6_q_net
    );

  register7: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register6_q_net,
      en => "1",
      rst => "0",
      q => system_time_lsb_net
    );

  register8: entity work.xlregister
    generic map (
      d_width => 64,
      init_value => b"0000000000000000000000000000000000000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => mac_time_64_bit_counter_op_net,
      en => "1",
      rst => "0",
      q => register8_q_net_x0
    );

  register9: entity work.xlregister
    generic map (
      d_width => 1,
      init_value => b"0"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d(0) => relational_op_net_x0,
      en => "1",
      rst => "0",
      q(0) => register9_q_net
    );

  register_x0: entity work.xlregister
    generic map (
      d_width => 32,
      init_value => b"00000000000000000000000000000000"
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      d => register1_q_net,
      en => "1",
      rst => "0",
      q => mac_time_msb_net
    );

  registers_9b36cfcda0: entity work.registers_entity_9b36cfcda0
    port map (
      ce_1 => ce_1_sg_x3,
      clk_1 => clk_1_sg_x3,
      from_register1 => data_out_x3_net,
      from_register2 => data_out_x4_net,
      from_register3 => data_out_x5_net,
      mac_time_usec => register8_q_net_x0,
      system_time_usec => register50_q_net_x0,
      constant1_x0 => en_x4_net,
      constant2_x0 => en_x5_net,
      constant5_x0 => en_x2_net,
      constant6_x0 => en_x3_net,
      ctrl_bits => b_0_y_net_x0,
      ctrl_bits_x0 => b_1_y_net_x2,
      reg_new_mac_time_lsb => register28_q_net_x0,
      reg_new_mac_time_msb => register27_q_net_x0,
      x32lsb1_x0 => data_in_x4_net,
      x32lsb_x0 => data_in_x2_net,
      x32msb1_x0 => data_in_x5_net,
      x32msb_x0 => data_in_x3_net
    );

  system_time_64_bit_counter: entity work.xlcounter_free_wlan_mac_time_hw
    generic map (
      core_name0 => "cntr_11_0_78c1092db561b4bc",
      op_arith => xlUnsigned,
      op_width => 64
    )
    port map (
      ce => ce_1_sg_x3,
      clk => clk_1_sg_x3,
      clr => '0',
      en(0) => relational_op_net_x0,
      rst(0) => b_0_y_net_x0,
      op => system_time_64_bit_counter_op_net
    );

  usec_pulse_1defadf13e: entity work.usec_pulse_entity_1defadf13e
    port map (
      ce_1 => ce_1_sg_x3,
      clk_1 => clk_1_sg_x3,
      usec => relational_op_net_x0,
      usec_fractional_part => clk_usec_op_net
    );

  x32lsb: entity work.xlslice
    generic map (
      new_lsb => 0,
      new_msb => 31,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register8_q_net_x0,
      y => x32lsb_y_net
    );

  x32lsb1: entity work.xlslice
    generic map (
      new_lsb => 0,
      new_msb => 31,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register50_q_net_x0,
      y => x32lsb1_y_net
    );

  x32msb: entity work.xlslice
    generic map (
      new_lsb => 32,
      new_msb => 63,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register8_q_net_x0,
      y => x32msb_y_net
    );

  x32msb1: entity work.xlslice
    generic map (
      new_lsb => 32,
      new_msb => 63,
      x_width => 64,
      y_width => 32
    )
    port map (
      x => register50_q_net_x0,
      y => x32msb1_y_net
    );

end structural;
