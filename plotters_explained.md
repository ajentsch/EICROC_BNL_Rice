Last updated 06/17/26

To generate required output.csv file, go to daqst3 and run:

./zcu106 -d /dev/ttyUSB3 -m 0
 
./zcu106 -d /dev/ttyUSB3 -n 10 -m 2 | ./ana_zcu

output.csv will be generated.

 Then run convert_event_format.py to convert that file into the required format for eicroc1_plotter.cxx. Root must be 
installed to make the plots. 32x32_sample_output.csv is an example of the expected input file format.

For the 32x32 array, eicroc1_plotter.cxx generates:
- ADC mean distribution for 16 sample pixels
- ADC mean map (2D)
- noise spectra, normalized, per pixel and per timebin for 2 selected pixels of interest
- global noise spectra, normalized, per pixel across all timebins and events
- ADC mean distributions per timebin, scatter plots. option to display these with or without error bars from RMS
- scatter plot for 16 channels of interest

S-curve code has been adjusted for EICROC1 but I have not yet tested them with threshold scans, planning to do that next Monday 06/20/26, along with the pedestal measurements. Started writing code for this as well but have not pushed it yet since it's still unfinished.
