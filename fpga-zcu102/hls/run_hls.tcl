# Vitis HLS Script to synthesize C++ Z8000 core into Verilog IP

open_project z8000_hls_prj
set_top z8000_hls_step
add_files z8000_hls.cpp

open_solution "solution1" -flow_target vivado
set_part {xczu9eg-ffvb1156-2-e}
create_clock -period 10 -name default

csynth_design
export_design -format ip_catalog -rtl verilog
exit
