###############################################################################
## Tool paths
###############################################################################
VERILATOR_SRC ?= /usr/share/verilator/include
SYSTEMC_HOME  ?= /usr/local/systemc-2.3.1

export VERILATOR_SRC
export SYSTEMC_HOME

###############################################################################
## Makefile
###############################################################################
all: run

build:
	make -f makefile.generate_verilated
	make -f makefile.build_verilated
	make -f makefile.build_sysc_tb

clean:
	make -f makefile.generate_verilated $@
	make -f makefile.build_verilated $@
	make -f makefile.build_sysc_tb $@
	-rm *.vcd

run: build
	./build/test.x

view:
	gtkwave verilator.vcd gtksettings.sav