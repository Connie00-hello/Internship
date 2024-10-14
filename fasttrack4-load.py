import os
import sys
from sys import stderr
#import os.path
import subprocess
import threading
import time
from optparse import OptionParser

"""
	Argument list from fasttrack.c
		printf("usage: fasttrack [1] [2] [3] [4] [5] [6] [[7] [8] [9] [10] [11]..]\n");
		printf("argument [1]: number of assets (int)\n");
		printf("argument [2]: asset start number (int)\n");
		printf("argument [3]: sleep between readings in sec (int)\n");
		printf("argument [4]: the number of reading to take before quitting (int)\n");
		printf("argument [5]: the probability of an asset failure (float 0-1)\n");
		printf("argument [6]: the number of components in the asset (int)\n");
		printf("argument [7]: the type of component (char)\n");
		printf("argument [8]: the normal expected reading of the component (float)\n");
		printf("argument [9]: the expected variance of the component readings (float)\n");
		printf("argument [10]: the number of iterations in fail mode before asset fails (int)\n");
		printf("argument [11]: the target reading for a component in fail mode (float)\n");
		printf("repeat arguments [7], [8], [9], [10] and [11] the number of times specified in argument [6]\n");
"""
# sensor number : ( sensor type, normal expected reading of the sensor, expected variance of the sensor readings, iterations to failure target, target reading for a sensor in fail mode)
sensors = { 
	1: ('DHT22', '45.00', '1.00', '100', '80.00'),
	2: ('WA1017',  '0.90', '0.01', '10', '0.71'),
	3: ('EE671',  '0.98', '0.01', '500', '0.92'),
	4: ('MLH01', '121.3', '4.00', '50', '213.5'),
	5: ('NXP3320', '0.01', '0.001', '1000', '0.45')
	}
sensorcnt = len(sensors)
sensorparams = len(sensors[1])
sensorlist = ''
for i in sensors:
	for j in range(0, sensorparams):
		sensorlist = sensorlist + ' ' + sensors[i][j]


def parse_args():
	parser = OptionParser(usage="fasttrack4.py [options]")

	parser.add_option("-a", "--assets", type="int", default=1, help="Number of assets to launch")
	parser.add_option("-i", "--id", type="int", default=10000, help="ID number of assets to start with")
	parser.add_option("-s", "--sleep", type="int", default=1, help="Total sleep between readings")
	parser.add_option("-r", "--readings", type="int", default=10, help="Total number of readings before quitting")
	parser.add_option("-p", "--probability", type="float", default=0.01, help="Probability of an asset failure")
	parser.add_option("-m", "--maintenance", type="float", default=0.5, help="Probability factor of a maintenance event")
	parser.add_option("-f", "--file", type="string", default="sensors.out", help="File to read")
	parser.add_option("-o", "--outprefix", type="string", default="sensors", help="Outfile prefix to write")
	parser.add_option("-x", "--retries", type="int", default=10, help="Number of retries to wait for more input before quitting")
	parser.add_option("-l", "--lines", type="int", default=10000, help="Lines to process at a time")

	(opts, args) = parser.parse_args()

	if opts.assets < 1:
		print >> stderr, "Must launch at least 1 asset, you put: %s" % opts.assets
		sys.exit(1)
	if opts.id < 0:
		print >> stderr, "Must have an id of at least 0, you put: %s" % opts.assets
		sys.exit(1)
	if opts.sleep < 0:
		print >> stderr, "Sleep must be >= 0 seconds, you put: %s" % opts.sleep
		sys.exit(1)
	if opts.readings < 1:
		print >> stderr, "Must specify at least 1 reading, you put: %s" % opts.readings
		sys.exit(1)
	if opts.probability < 0 or opts.probability > 1:
		print >> stderr, "Probability must be a decimal between 0 and 1, you put: %s" % opts.probability
		sys.exit(1)
	if opts.lines <= 0:
		print >> stderr, "Lines must be > 0 , you put: %s" % opts.lines
		sys.exit(1)

	return opts

def startAssets(assets, id, sleep, readings, probability, maintenance, file):
	global sensorcnt
	global sensorlist
#	print("./fasttrack4 %d %d %d %d %f %f %d %s > %s" % (assets, id, sleep, readings, probability, maintenance, sensorcnt, sensorlist, file))
	subprocess.call("c:\kafka\sensors\fasttrack4 %d %d %d %d %f %f %d %s > %s" % (assets, id, sleep, readings, probability, maintenance, sensorcnt, sensorlist, file), shell=True)
	return

def launchAssetThread(assets, id, sleep, readings, probability, maintenance, file):
	t0 = threading.Thread(target=startAssets, args=(assets, id, sleep, readings, probability, maintenance, file, ))
	t0.start()
	return

def main():

	global opts
	opts = parse_args()

 	launchAssetThread(opts.assets, opts.id, opts.sleep, opts.readings, opts.probability, opts.maintenance, opts.file)
	time.sleep(opts.sleep)

	NUM_OF_LINES = opts.lines
	retry = 0
	i = 0
	batch = 0
	filename = opts.file
	fin = open(filename, "r")
	outfile = "%s%s.part" % (opts.outprefix, str(0).zfill(6))
	fout = open(outfile, "w", 1)
	while retry < opts.retries:
		if retry >= opts.retries:
			break
		where = fin.tell()
		line = fin.readline()
		if not line.strip():
			retry += 1
 			print ("Nothing... retry #%d" % retry)
			time.sleep(opts.sleep)
			continue
		fout.write(line)
# 		print(line)
		i += 1
		retry = 0
		if (i + 1) % NUM_OF_LINES == 0:
			fout.close()
			outfile = "%s%s.part" % (opts.outprefix, str(i / NUM_OF_LINES + 1).zfill(6))
			fout = open(outfile, "w", 1)

	fout.close() 

if  __name__ =='__main__':main()
