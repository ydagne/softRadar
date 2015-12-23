#
#  Copyright 2015 Yihenew Beyene
#  
#  This file is part of SoftRadar.
#  
#      SoftRadar is free software: you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation, either version 3 of the License, or
#      (at your option) any later version.
#  
#      SoftRadar is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#  
#      You should have received a copy of the GNU General Public License
#      along with SoftRadar.  If not, see <http://www.gnu.org/licenses/>.


#!/usr/bin/env python

# Execute this script in the same directory as where you run the device manager

import socket
import struct
import time
import math


def testCapture():
	
	burstlen = 10500    # This should match with configured value
	pulsewidth = 4
	prt = (50,100,200,75,500)
	pi_ = math.pi
	phase = (pi_/4.0, 3.0*pi_/4.0, -pi_/4.0, -3.0*pi_/4.0, 3.0*pi_/4.0, -pi_/4.0)
	ampl = 0.4
	waveform = ()
	
	idx = 0
	for prtv in prt:
	
		# BPSK modulation  phi = pi/4
		for i in range(pulsewidth):
		
			# real part, imaginary part
			waveform += (ampl*math.cos(phase[idx]),ampl*math.sin(phase[idx])) 
			
		idx += 1;
			
		for i in range(prtv-pulsewidth):
		
			waveform += (0.0,0.0)  # real part, imaginary part
			

	# Last part
	for i in range(pulsewidth):
	
		# real part, imaginary part
		waveform += (ampl*math.cos(phase[idx]),ampl*math.sin(phase[idx]))
		
	for i in range(2*burstlen-len(waveform)):
	
		waveform += (0.0,)  
			
			
	waveformString = "".join( (struct.pack('f',v) for v in waveform))
	
	# write waveform to file
	fileName  = "waveform.wm"
	wmFile = open (fileName, "wb")
	wmFile.write(waveformString)
	wmFile.close()

	s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	
	# Loading waveform file
	s.sendto("CMD LOAD " + fileName, ("localhost", 5700) )
	
	# Starting streamer
	s.sendto("CMD START", ("localhost", 5700))
	
	# Wait for few seconds
	time.sleep(5)
	
	# Stop streamer
	s.sendto("CMD STOP", ("localhost", 5700))
	time.sleep(1)
	
	# Terminate 
	s.sendto("CMD TERMINATE", ("localhost", 5700))
	
	


if __name__ == '__main__':
    try:
        testCapture()
    except KeyboardInterrupt:
        pass
