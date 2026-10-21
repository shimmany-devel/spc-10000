# Vivado Bitstream Generation and Hardware Export (XSA) Script

open_project ./vivado_project/spc10000_zcu102.xpr

# Synthesis
launch_runs synth_1 -jobs 8
wait_on_run synth_1

# Implementation
launch_runs impl_1 -to_step write_bitstream -jobs 8
wait_on_run impl_1

# Check status
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation / Bitstream generation failed!"
    exit 1
}

# Export Hardware (.xsa)
write_hw_platform -fixed -include_bit -force -file ./spc10000_zcu102.xsa
puts "=== Bitstream and XSA generated successfully ==="
