# Vivado Bitstream Generation and Hardware Export (XSA) Script

open_project ./vivado_project/spc10000_zcu102.xpr

# Reset previous failed runs
reset_run synth_1 -quiet
reset_run impl_1 -quiet

# Synthesis
puts "=== Launching Synthesis ==="
launch_runs synth_1 -jobs 16
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: Synthesis failed!"
    exit 1
}
puts "=== Synthesis completed successfully ==="

# Implementation
puts "=== Launching Implementation and Bitstream ==="
launch_runs impl_1 -to_step write_bitstream -jobs 16
wait_on_run impl_1

# Check status
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: Implementation / Bitstream generation failed!"
    exit 1
}
puts "=== Implementation and Bitstream completed successfully ==="

# Export Hardware (.xsa)
write_hw_platform -fixed -include_bit -force -file ./spc10000_zcu102.xsa
puts "=== Bitstream and XSA generated successfully ==="
