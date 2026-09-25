XILINX_VIVADO=/tools/Xilinx/Vivado/2024.1

#${XILINX_VIVADO}/bin/vivado -mode batch -source ${1}

${XILINX_VIVADO}/bin/vivado -mode batch -notrace -source ${1}
