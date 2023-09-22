
import sys, random

with open (sys.argv[1],'w') as fl:
	fl.writelines(sys.argv[2]+' '+sys.argv[3]+'\n')
	for i in xrange(int(sys.argv[2])):
		for j in xrange(int(sys.argv[3])):
			fl.write(str(random.randint(0,999))+' ')
		fl.write('\n')
	fl.write('\n')
	fl.writelines(sys.argv[3]+' '+sys.argv[4]+'\n')
	for i in xrange(int(sys.argv[3])):
		for j in xrange(int(sys.argv[4])):
			fl.write(str(random.randint(0,999))+' ')
			
	
	
	
