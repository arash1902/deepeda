import random  # import random library for random number generating
import os
import numpy as np
def CirRecDataset():
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/div_bin/'
	datalist = [ ]
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath).read()
		readcontents = [item.split() for item in contents.split('\n')[:-1]]
		flat_contents = [item for sublist in readcontents for item in sublist]
		datalist.append(flat_contents)
	labels0 = np.zeros((len(datalist)))
	div_size = len(datalist)
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/newmul_bin/'
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath).read()
		readcontents = [item.split() for item in contents.split('\n')[:-1]]
		flat_contents = [item for sublist in readcontents for item in sublist]
		datalist.append(flat_contents)
	labels1 = np.ones(((len(datalist) - div_size)))
	mult_size = len(datalist) - div_size
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/mod_bin/'
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath).read()
		readcontents = [item.split() for item in contents.split('\n')[:-1]]
		flat_contents = [item for sublist in readcontents for item in sublist]
		datalist.append(flat_contents)
	labels2 = np.ones(((len(datalist) - div_size - mult_size))) + 1
	mod_size= len(datalist) - div_size - mult_size
	stringArray = np.array(datalist)
	mat = np.fromstring(stringArray,'u1') - ord('0')
	data = mat.reshape((len(datalist),(mat.shape[0]/len(datalist)) ))	
	labels = np.concatenate((labels0, labels1, labels2), axis=0)
	# print(div_size) 870
	# print(mult_size) 872
	# print(mod_size) 870
	return data, labels

## Return a total of "num" random samples and labels.	 
def next_batch(num, data, labels):
    idx = np.arange(0 , len(data))
    np.random.shuffle(idx)
    idx = idx[:num]
    data_shuffle = [data[ i] for i in idx]
    labels_shuffle = [labels[ i] for i in idx]
    return np.asarray(data_shuffle), np.asarray(labels_shuffle)
	
## Return a index-specific samples and labels.	
def selection(idx, data, labels):
    data_shuffle = [data[ i] for i in idx]
    labels_shuffle = [labels[ i] for i in idx]
    return np.asarray(data_shuffle), np.asarray(labels_shuffle)
	
