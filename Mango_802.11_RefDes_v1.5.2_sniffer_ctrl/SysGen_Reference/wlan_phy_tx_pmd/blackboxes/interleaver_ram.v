module interleaver_ram(
  clk,
  ce,
  wea,
  addra,
  dina,
  douta,
  web,
  addrb,
  dinb,
  doutb
);

//Sysgen blackboxes must have clk and ce ports
// VHDL wrapper uses std_logic for these signals
input clk;
input ce;

// Sysgen instantiates this module in a VHDL wrapper. The wrapper uses std_logic_vector(0:0) signals
//  to connect to this module's ports. It's *very* important the scalar I/O below have dimensions [0:0].
//  Without these XST bizarrely decides no connection is made and optimizes out the interleaver RAM.
// Using Sysgen's "port.useHDLVector(false)" in the config.m would probably achieve the same thing.

input  [0:0] dina;
input  [0:0] wea;
input  [8:0] addra;
output [7:0] douta;

input  [0:0] dinb;
input  [0:0] web;
input  [8:0] addrb;
output [7:0] doutb;

//Map sysgen clk to BRAM clks
// This module does not use Sysgen's clock enbale (ce) signal
// The Sysgen model must run this block at the system sample rate
wire clka, clkb;
assign clka = clk;
assign clkb = clk;

// dp_ram_wr_1b_rd_8b_512b is packaged as an ngc netlist cretaed with Coregen's Block Memory Generator
//  The block memory is configured as:
//   True dual port memory mode (required for different read/write widths)
//   Write width A/B = 1 bit
//   Write depth = 512 (512 bits total)
//   Read width A/B = 8 bits
//   Read depth = 64 (512 / 8)
//   Always enbaled (no ena/enb ports)

(* box_type = "black_box" *)
dp_ram_wr_1b_rd_8b_512b ram_inst (
  .clka(clka), // input clka
  .wea(wea), // input [0 : 0] wea
  .addra(addra), // input [8 : 0] addra
  .dina(dina), // input [0 : 0] dina
  .douta(douta), // output [7 : 0] douta
  .clkb(clkb), // input clkb
  .web(web), // input [0 : 0] web
  .addrb(addrb), // input [8 : 0] addrb
  .dinb(dinb), // input [0 : 0] dinb
  .doutb(doutb) // output [7 : 0] doutb
);

endmodule

//Define module for RAM blackbox - ngdbuild will substitute dp_ram_wr_1b_rd_8b_512b.ngc during implementation
// Code inside translate_off/translate_on only used for simulation
// Implementation will use NGC netlist for RAM block
module dp_ram_wr_1b_rd_8b_512b(
  clka,
  wea,
  addra,
  dina,
  douta,
  clkb,
  web,
  addrb,
  dinb,
  doutb
);

input clka;
input [0 : 0] wea;
input [8 : 0] addra;
input [0 : 0] dina;
output [7 : 0] douta;
input clkb;
input [0 : 0] web;
input [8 : 0] addrb;
input [0 : 0] dinb;
output [7 : 0] doutb;

// synthesis translate_off
//  BLK_MEM_GEN_V7_3 simulation model supplied by ISE - implementation will use coregen netlist
  BLK_MEM_GEN_V7_3 #(
    .C_ADDRA_WIDTH(9),
    .C_ADDRB_WIDTH(9),
    .C_ALGORITHM(1),
    .C_AXI_ID_WIDTH(4),
    .C_AXI_SLAVE_TYPE(0),
    .C_AXI_TYPE(1),
    .C_BYTE_SIZE(9),
    .C_COMMON_CLK(1),
    .C_DEFAULT_DATA("0"),
    .C_DISABLE_WARN_BHV_COLL(0),
    .C_DISABLE_WARN_BHV_RANGE(0),
    .C_ENABLE_32BIT_ADDRESS(0),
    .C_FAMILY("virtex6"),
    .C_HAS_AXI_ID(0),
    .C_HAS_ENA(0),
    .C_HAS_ENB(0),
    .C_HAS_INJECTERR(0),
    .C_HAS_MEM_OUTPUT_REGS_A(0),
    .C_HAS_MEM_OUTPUT_REGS_B(0),
    .C_HAS_MUX_OUTPUT_REGS_A(0),
    .C_HAS_MUX_OUTPUT_REGS_B(0),
    .C_HAS_REGCEA(0),
    .C_HAS_REGCEB(0),
    .C_HAS_RSTA(0),
    .C_HAS_RSTB(0),
    .C_HAS_SOFTECC_INPUT_REGS_A(0),
    .C_HAS_SOFTECC_OUTPUT_REGS_B(0),
    .C_INIT_FILE("BlankString"),
    .C_INIT_FILE_NAME("no_coe_file_loaded"),
    .C_INITA_VAL("0"),
    .C_INITB_VAL("0"),
    .C_INTERFACE_TYPE(0),
    .C_LOAD_INIT_FILE(0),
    .C_MEM_TYPE(2),
    .C_MUX_PIPELINE_STAGES(0),
    .C_PRIM_TYPE(1),
    .C_READ_DEPTH_A(64),
    .C_READ_DEPTH_B(64),
    .C_READ_WIDTH_A(8),
    .C_READ_WIDTH_B(8),
    .C_RST_PRIORITY_A("CE"),
    .C_RST_PRIORITY_B("CE"),
    .C_RST_TYPE("SYNC"),
    .C_RSTRAM_A(0),
    .C_RSTRAM_B(0),
    .C_SIM_COLLISION_CHECK("ALL"),
    .C_USE_BRAM_BLOCK(0),
    .C_USE_BYTE_WEA(0),
    .C_USE_BYTE_WEB(0),
    .C_USE_DEFAULT_DATA(0),
    .C_USE_ECC(0),
    .C_USE_SOFTECC(0),
    .C_WEA_WIDTH(1),
    .C_WEB_WIDTH(1),
    .C_WRITE_DEPTH_A(512),
    .C_WRITE_DEPTH_B(512),
    .C_WRITE_MODE_A("WRITE_FIRST"),
    .C_WRITE_MODE_B("WRITE_FIRST"),
    .C_WRITE_WIDTH_A(1),
    .C_WRITE_WIDTH_B(1),
    .C_XDEVICEFAMILY("virtex6")
  )
  inst (
    .CLKA(clka),
    .WEA(wea),
    .ADDRA(addra),
    .DINA(dina),
    .DOUTA(douta),
    .CLKB(clkb),
    .WEB(web),
    .ADDRB(addrb),
    .DINB(dinb),
    .DOUTB(doutb),
    .RSTA(),
    .ENA(),
    .REGCEA(),
    .RSTB(),
    .ENB(),
    .REGCEB(),
    .INJECTSBITERR(),
    .INJECTDBITERR(),
    .SBITERR(),
    .DBITERR(),
    .RDADDRECC(),
    .S_ACLK(),
    .S_ARESETN(),
    .S_AXI_AWID(),
    .S_AXI_AWADDR(),
    .S_AXI_AWLEN(),
    .S_AXI_AWSIZE(),
    .S_AXI_AWBURST(),
    .S_AXI_AWVALID(),
    .S_AXI_AWREADY(),
    .S_AXI_WDATA(),
    .S_AXI_WSTRB(),
    .S_AXI_WLAST(),
    .S_AXI_WVALID(),
    .S_AXI_WREADY(),
    .S_AXI_BID(),
    .S_AXI_BRESP(),
    .S_AXI_BVALID(),
    .S_AXI_BREADY(),
    .S_AXI_ARID(),
    .S_AXI_ARADDR(),
    .S_AXI_ARLEN(),
    .S_AXI_ARSIZE(),
    .S_AXI_ARBURST(),
    .S_AXI_ARVALID(),
    .S_AXI_ARREADY(),
    .S_AXI_RID(),
    .S_AXI_RDATA(),
    .S_AXI_RRESP(),
    .S_AXI_RLAST(),
    .S_AXI_RVALID(),
    .S_AXI_RREADY(),
    .S_AXI_INJECTSBITERR(),
    .S_AXI_INJECTDBITERR(),
    .S_AXI_SBITERR(),
    .S_AXI_DBITERR(),
    .S_AXI_RDADDRECC()
  );

// synthesis translate_on
endmodule

