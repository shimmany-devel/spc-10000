# Makefile for ZCU102 SPC-10000 Build Flow

VIVADO ?= vivado
VIVADO_FLAGS = -mode batch -notrace

all: project bitstream

project:
	$(VIVADO) $(VIVADO_FLAGS) -source scripts/create_project.tcl

bitstream:
	$(VIVADO) $(VIVADO_FLAGS) -source scripts/build_bitstream.tcl

clean:
	rm -rf vivado_project *.log *.jou *.xsa NA/

.PHONY: all project bitstream clean
