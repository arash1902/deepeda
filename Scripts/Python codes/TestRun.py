### Copyright (c) 2017 Arash Fayyazi - All Rights Reserved
#############################################################
#!/usr/bin/python
import math
import sys
import numpy as np
from ArashCNN_CellLibTest import run_CNNCL
def random_gen(len_data,num):
	idx1 = np.arange(0 , len_data[0])
	idx2 = np.arange(len_data[0] , (len_data[0]+len_data[1]))
	idx3 = np.arange((len_data[0]+len_data[1]) , (len_data[0]+len_data[1]+len_data[1]))
	np.random.shuffle(idx1)
	idx1 = idx1[:num]
	np.random.shuffle(idx2)
	idx2 = idx2[:num]
	np.random.shuffle(idx3)
	idx3 = idx3[:num]
	idx = np.concatenate((idx1, idx2, idx3), axis=0)
	return idx
def testfile(filename):
	datalist = [ ]
	contents = open(filename).read()
	readcontents = [item.split() for item in contents.split('\n')[:-1]]
	flat_contents = [item for sublist in readcontents for item in sublist]
	datalist.append(flat_contents)
	stringArray = np.array(datalist)
	mat = np.fromstring(stringArray,'u1') - ord('0')
	data = mat.reshape((len(datalist),(mat.shape[0]/len(datalist)) ))	
	return data
### run it
def main():
	filename = sys.argv[1]
	label = sys.argv[2]
	data= testfile(filename)
	run_CNNCL(data,label)
if __name__ == "__main__": main()
##########################################################


