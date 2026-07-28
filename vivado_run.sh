XILINX_VIVADO=/c/Xilinx/Vivado/2022.2

#${XILINX_VIVADO}/bin/vivado -mode batch -source ${1}

${XILINX_VIVADO}/bin/vivado -mode batch -notrace -source ${1}
