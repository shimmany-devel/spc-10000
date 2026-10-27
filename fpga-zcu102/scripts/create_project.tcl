# Vivado Tcl script to create ZCU102 project for SPC-10000

set project_name "spc10000_zcu102"
set project_dir "./vivado_project"
set part "xczu9eg-ffvb1156-2-e"
set board_part "xilinx.com:zcu102:part0:3.4"

# Create project
create_project $project_name $project_dir -part $part -force
set_property board_part $board_part [current_project]

# Set target language to Verilog/SystemVerilog
set_property target_language Verilog [current_project]

# Add RTL sources
add_files -norecurse {
    ./rtl/zcu102_spc10000_top.sv
    ./rtl/top_spc10000.sv
    ./rtl/bus/axi_lite_regs.sv
    ./rtl/bus/mem_controller.sv
    ./rtl/cpu_z8000/z8000_core.sv
    ./rtl/cpu_z80/z80_top.sv
    ./rtl/video/mc6847_core.sv
    ./rtl/video/axis_video_out.sv
    ./rtl/sound/psg_ay8910.sv
}

# Add Constraints
add_files -fileset constrs_1 -norecurse {
    ./constraints/zcu102_spc10000.xdc
}

# Create Block Design with Zynq UltraScale+ MPSoC
create_bd_design "system"

# 1. Add Zynq UltraScale+ Processing System
set ps_cell [create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:3.5 zynq_ultra_ps_e_0]
apply_bd_automation -rule xilinx.com:bd_rule:zynq_ultra_ps_e -config {apply_board_preset "1" } [get_bd_cells zynq_ultra_ps_e_0]

# Configure PS: Enable HPM0 FPD, disable HPM1, set PL0 Clock to 100MHz
set_property -dict [list \
    CONFIG.PSU__USE__M_AXI_GP0 {1} \
    CONFIG.PSU__USE__M_AXI_GP1 {0} \
    CONFIG.PSU__CRL_APB__PL0_REF_CTRL__FREQMHZ {100} \
] [get_bd_cells zynq_ultra_ps_e_0]

# 2. Add Processor System Reset
set rst_cell [create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0]

# 3. Add AXI SmartConnect (1 Slave In, 2 Masters Out)
set smc_cell [create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 smartconnect_0]
set_property -dict [list CONFIG.NUM_SI {1} CONFIG.NUM_MI {2}] [get_bd_cells smartconnect_0]

# 4. Add AXI BRAM Controller (M00_AXI)
set bram_ctrl [create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_0]
set_property -dict [list \
    CONFIG.DATA_WIDTH {32} \
    CONFIG.ECC_TYPE {0} \
    CONFIG.SINGLE_PORT_BRAM {1} \
] [get_bd_cells axi_bram_ctrl_0]

# 5. Connect Clocks & Resets
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] \
               [get_bd_pins zynq_ultra_ps_e_0/maxihpm0_fpd_aclk] \
               [get_bd_pins proc_sys_reset_0/slowest_sync_clk] \
               [get_bd_pins smartconnect_0/aclk] \
               [get_bd_pins axi_bram_ctrl_0/s_axi_aclk]

connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_resetn0] [get_bd_pins proc_sys_reset_0/ext_reset_in]

connect_bd_net [get_bd_pins proc_sys_reset_0/interconnect_aresetn] [get_bd_pins smartconnect_0/aresetn]
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_pins axi_bram_ctrl_0/s_axi_aresetn]

# 6. Connect AXI Interfaces
# PS -> SmartConnect
connect_bd_intf_net [get_bd_intf_pins zynq_ultra_ps_e_0/M_AXI_HPM0_FPD] [get_bd_intf_pins smartconnect_0/S00_AXI]

# SmartConnect M00 -> BRAM Controller
connect_bd_intf_net [get_bd_intf_pins smartconnect_0/M00_AXI] [get_bd_intf_pins axi_bram_ctrl_0/S_AXI]

# SmartConnect M01 -> External M_AXI_REGS
make_bd_intf_pins_external -name "M_AXI_REGS" [get_bd_intf_pins smartconnect_0/M01_AXI]

# BRAM Controller BRAM_PORTA -> External BRAM_PORTA
make_bd_intf_pins_external -name "BRAM_PORTA" [get_bd_intf_pins axi_bram_ctrl_0/BRAM_PORTA]
set_property CONFIG.READ_WRITE_MODE "READ_WRITE" [get_bd_intf_ports BRAM_PORTA]

# External Clock and Reset for PL
create_bd_port -dir O -type clk pl_clk100
connect_bd_net [get_bd_pins zynq_ultra_ps_e_0/pl_clk0] [get_bd_ports pl_clk100]

create_bd_port -dir O -type rst pl_resetn
connect_bd_net [get_bd_pins proc_sys_reset_0/peripheral_aresetn] [get_bd_ports pl_resetn]

set_property CONFIG.ASSOCIATED_BUSIF {M_AXI_REGS} [get_bd_ports pl_clk100]
set_property CONFIG.ASSOCIATED_RESET {pl_resetn} [get_bd_ports pl_clk100]

# 7. Address Mapping
assign_bd_address

# Move M_AXI_REGS first to avoid overlap with 128K BRAM window
set_property offset 0xA0020000 [get_bd_addr_segs {zynq_ultra_ps_e_0/Data/SEG_M_AXI_REGS_Reg}]
set_property range 64K [get_bd_addr_segs {zynq_ultra_ps_e_0/Data/SEG_M_AXI_REGS_Reg}]

# Expand BRAM Controller to 128K
set_property range 128K [get_bd_addr_segs {zynq_ultra_ps_e_0/Data/SEG_axi_bram_ctrl_0_Mem0}]
set_property offset 0xA0000000 [get_bd_addr_segs {zynq_ultra_ps_e_0/Data/SEG_axi_bram_ctrl_0_Mem0}]

# Validate BD
validate_bd_design
save_bd_design

# Generate Wrapper
make_wrapper -files [get_files $project_dir/$project_name.srcs/sources_1/bd/system/system.bd] -top
add_files -norecurse $project_dir/$project_name.gen/sources_1/bd/system/hdl/system_wrapper.v

set_property top zcu102_spc10000_top [current_fileset]
update_compile_order -fileset sources_1
puts "=== SPC-10000 Project Created Successfully ==="
