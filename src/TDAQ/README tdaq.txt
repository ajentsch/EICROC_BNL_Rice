1) Our current Zynq is 10.xxx.2 and I set it as peta1 in the hosts file, for convenience

2) All binaries exist in /RTS/bin, both on daqst3 and peta1, and should be run from there.

3) All default configuration files (.py and .json) live in /RTS/conf

4) New utility tdaq which runs on daqst3, automatically connects to peta1
and starts a run as if it was typed locally with some caveats. Data
is transmitted over ethernet from peta1 to daqst3. Which means that
a) you don't need to ssh into peta1, b) you don't need to store data on peta1's
SD card -- it all goes over ethernet to daqst3.

Usage:

tdaq [-H host:port] -- "params to eicroc"

If no -H, defaults to peta1:19000 so you generally should not use -H yet.

So they way you run it is on daqst3, for example:

/RTS/bin/tdaq -- -B 1 -F 0 -A 0 -m 3 -n 1000 >dta

NOTE the double dash! All parameters after that are passed as-is to eicroc running on peta1 under inetd.

LOGs show up on stderr (as always) and the data shows up on stdout (as always).

