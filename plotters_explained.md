06/08/26
To generate required output.csv file, go to daqst3 and run:
./zcu106 -d /dev/ttyUSB3 -m 0 
./zcu106 -d /dev/ttyUSB3 -n 10 -m 2 | ./ana_zcu
output.csv will be generated.

 Then run convert_event_format.py to convert that file into the required format for eicroc1_plotter.cxx. Root must be 
installed to make the plots. 32x32_sample_output.csv is an example of the expected input file format.

For the 32x32 array, eicroc1_plotter.cxx generates:
- ADC mean distribution for 16 sample pixels
- ADC mean map
- hit pixels plot
- peak ADC trigger pixel (if applicable)
- s-curves (I am still debugging this one)
