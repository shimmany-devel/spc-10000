# ZCU102 Constraints for SPC-10000 System
# Device: xczu9eg-ffvb1156-2-e

# Clocks
# PS provides pl_clk0 at 100MHz via Block Design
# If differential system clock from board is used directly:
# set_property PACKAGE_PIN AL8 [get_ports sys_clk_p]
# set_property PACKAGE_PIN AL7 [get_ports sys_clk_n]
# set_property IOSTANDARD DIFF_SSTL12 [get_ports sys_clk_p]

# User LEDs (GPIO LEDs)
set_property PACKAGE_PIN AG14 [get_ports {led[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[0]}]
set_property PACKAGE_PIN AF13 [get_ports {led[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[1]}]
set_property PACKAGE_PIN AE13 [get_ports {led[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[2]}]
set_property PACKAGE_PIN AJ14 [get_ports {led[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[3]}]
set_property PACKAGE_PIN AJ15 [get_ports {led[4]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[4]}]
set_property PACKAGE_PIN AH13 [get_ports {led[5]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[5]}]
set_property PACKAGE_PIN AH14 [get_ports {led[6]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[6]}]
set_property PACKAGE_PIN AL12 [get_ports {led[7]}]
set_property IOSTANDARD LVCMOS33 [get_ports {led[7]}]

# User DIP Switches (SW12 - Mode/Clock Selection)
# [0]: 0=SPC-1000, 1=SPC-10000 mode
# [1:2]: Clock Speed (00: 4/10MHz, 01: 20MHz, 10: 50MHz, 11: 100MHz)
# [3]: CPU Reset manual override
set_property PACKAGE_PIN AN14 [get_ports {sw_mode[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw_mode[0]}]
set_property PACKAGE_PIN AP14 [get_ports {sw_mode[1]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw_mode[1]}]
set_property PACKAGE_PIN AM14 [get_ports {sw_mode[2]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw_mode[2]}]
set_property PACKAGE_PIN AN13 [get_ports {sw_mode[3]}]
set_property IOSTANDARD LVCMOS33 [get_ports {sw_mode[3]}]

# PMOD0 Audio / Diagnostic PWM Out (Pin 1 & 2)
set_property PACKAGE_PIN A20 [get_ports audio_l_pwm]
set_property IOSTANDARD LVCMOS33 [get_ports audio_l_pwm]
set_property PACKAGE_PIN B20 [get_ports audio_r_pwm]
set_property IOSTANDARD LVCMOS33 [get_ports audio_r_pwm]
