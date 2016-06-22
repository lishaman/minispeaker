#!/usr/bin/python
import math
import sys

apll = int(sys.argv[1])
mpll = int(sys.argv[2])
M = N = SAVEN = SAVEM = 1.0
exprate = [8000.0,11025.0,12000.0,16000.0,22050.0,24000.0,32000.0,44100.0,48000.0,88200.0,96000.0]


def cacule_div(rate,pll):
 global M,N,min,SAVEM,SAVEN
 min = 65535.0;
 M = pll%rate
 N = pll/rate
 if(M == 0):
	SAVEM=1
	SAVEN = N
	return
 else:
	for M1 in range(1,0x1ff):
		M = 0x1ff - M1
		N = int(pll*M/rate)
		if (N > (65535<<2)):
			continue
		tmpval = abs(float(pll)*M/N - rate)
		if(tmpval < min):
			min = tmpval
			SAVEM = M
			SAVEN = N
 return


print "int audio_div_apll[] = {"
for rate in exprate:
    cacule_div(rate,apll)
    print int(rate),',',int(SAVEM),',',int(SAVEN),','
print '};'

print "int audio_div_mpll[] = {"
for rate in exprate:
    cacule_div(rate,mpll)
    print int(rate),',',int(SAVEM),',',int(SAVEN),','
print '};'
