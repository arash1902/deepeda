import random  # import random library for random number generating
import os
import numpy as np
import csv
def CirRecDataset():
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/div_bin2L/'
	datalist = [ ]
	div_size = 0
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath, 'r')
		reader = csv.reader(contents)
		div_size += 1
		for line in reader:
			for i in line: 
				datalist.append(float(i))
		contents.close()
	rows= len(open(filepath).readlines())
	labels0 = np.zeros((div_size))
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/mul_bin2L/'
	mult_size = 0
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath, 'r')
		reader = csv.reader(contents)
		mult_size += 1
		for line in reader:
			for i in line: 
				datalist.append(float(i))
		cols = len(line)
		contents.close()
		#rows = sum(1 for row in reader)  # fileObject is your csv.reader
	labels1 = np.ones((mult_size))
	path = '/home/home2/students/arash/ABC/src/ext/benchmarks/mod_bin2L/'
	mod_size = 0
	for filename in os.listdir(path):
		filepath = os.path.join(path, filename)
		contents = open(filepath, 'r')
		reader = csv.reader(contents)
		mod_size += 1
		for line in reader:
			for i in line: 
				datalist.append(float(i))
		contents.close()
	labels2 = np.ones((mod_size)) + 1
	mat = np.asarray(datalist)
	print(div_size)
	print(mult_size)
	print(mod_size)
	print(rows)
	data = mat.reshape((mat.shape[0]/(cols*rows)),cols*rows )	
	print(mat.shape)
	labels = np.concatenate((labels0, labels1, labels2), axis=0) 
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
