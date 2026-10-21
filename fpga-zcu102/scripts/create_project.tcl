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

# Add Zynq UltraScale+ Processing System
create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:3.5 zynq_ultra_ps_e_0
apply_bd_automation -rule xilinx.com:bd_rule:zynq_ultra_ps_e -config {apply_board_preset "1" } [get_bd_cells zynq_ultra_ps_e_0]

# Configure PS to enable AXI Master (HPM0 FPD) and PL Clock (100MHz)
set_property -dict [list \
    CONFIG.PSU__USE__M_AXI_GP0 {1} \
    CONFIG.PSU__CRL_APB__PL0_REF_CTRL__FREQMHZ {100} \
] [get_bd_cells zynq_ultra_ps_e_0]

# Add AXI BRAM Controller for direct memory access to PL
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.1 axi_bram_ctrl_0
set_property -dict [list CONFIG.DATA_WIDTH {32} CONFIG.ECC_TYPE {0}] [get_bd_cells axi_bram_ctrl_0]

# Generate Wrapper
make_wrapper -files [get_files $project_dir/$project_name.srcs/sources_1/bd/system/system.bd] -top
add_files -norecurse $project_dir/$project_name.gen/sources_1/bd/system/hdl/system_wrapper.v

update_compile_order -fileset sources_1
puts "=== SPC-10000 Project Created Successfully ==="
