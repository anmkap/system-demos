import os
from sys import argv


for x in xrange(int(argv[1])):
	os.system("./sub " + str(x+2) + " " + argv[2] + " &")
