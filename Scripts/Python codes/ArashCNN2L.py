import math
import numpy as np
#import h5py
import matplotlib.pyplot as plt
import scipy
# from PIL import Image
# from scipy import ndimage
import tensorflow as tf
from tensorflow.python.framework import ops
from cnn_utils import *
from readMat2L import CirRecDataset,next_batch

# number of training examples = 1080
# number of test examples = 120
# X_train shape: (1080, 64, 64, 3)
# Y_train shape: (1080, 6)
# X_test shape: (120, 64, 64, 3)
# Y_test shape: (120, 6)

def create_placeholders(n_H0, n_W0, n_C0, n_y):
    # """
    # Creates the placeholders for the tensorflow session.
    
    # Arguments:
    # n_H0 -- scalar, height of an input image
    # n_W0 -- scalar, width of an input image
    # n_C0 -- scalar, number of channels of the input
    # n_y -- scalar, number of classes
        
    # Returns:
    # X -- placeholder for the data input, of shape [None, n_H0, n_W0, n_C0] and dtype "float"
    # Y -- placeholder for the input labels, of shape [None, n_y] and dtype "float"
    # """
	# 
    ### START CODE HERE ### (2 lines)
	X = tf.placeholder(tf.float32, [None, n_H0, n_W0, n_C0])
	#X=tf.placeholder(tf.float32, [None, n_H0, n_W0])
	Y=tf.placeholder(tf.float32, [None, n_y])
	### END CODE HERE ###
	return X, Y
## Test placeholder
# X, Y = create_placeholders(64, 64, 3, 6)
# print ("X = " + str(X))
# print ("Y = " + str(Y))
#######################################################
def initialize_parameters(parameters_Val):
    # """
    # Initializes weight parameters to build a neural network with tensorflow. The shapes are:
                        # W1 : [f1, f1, n_C0, n_C1]
                        # W2 : [f2, f2, n_C1, n_C2]
    # Returns:
    # parameters -- a dictionary of tensors containing W1, W2
    # """
	tf.set_random_seed(1)                              # so that your "random" numbers match ours
	n_C1 = parameters_Val["n_C1"]
	n_C0 = parameters_Val["n_C0"]
	f1 = parameters_Val["f1"]   
	# n_C is number of filters in each layers
	# size of each filter W: f, f, n_C_prev, n_C
	# n_C0 is number of input channels, 3D and more. for example for RGB image is 3.
    ### START CODE HERE ### 
	W1 = tf.get_variable("W1", [f1,f1,n_C0,n_C1], initializer = tf.contrib.layers.xavier_initializer(seed = 0)) 
    #W2 = tf.get_variable("W2", [2,2,8,16], initializer = tf.contrib.layers.xavier_initializer(seed = 0)) 
    ### END CODE HERE ###
	parameters = {"W1": W1}
	return parameters
### test initialize_parameters
# tf.reset_default_graph()
# with tf.Session() as sess_test:
    # parameters = initialize_parameters()
    # init = tf.global_variables_initializer()
    # sess_test.run(init)
    # print("W1 = " + str(parameters["W1"].eval()[1,1,1]))
    # print("W2 = " + str(parameters["W2"].eval()[1,1,1]))
##########################################################################
def forward_propagation(X, parameters, keep_prob):
    # """
    # Implements the forward propagation for the model:
    # CONV2D -> RELU -> MAXPOOL -> CONV2D -> RELU -> MAXPOOL -> FLATTEN -> FULLYCONNECTED
    
    # Arguments:
    # X -- input dataset placeholder, of shape (input size, number of examples)
    # parameters -- python dictionary containing your parameters "W1", "W2"
                  # the shapes are given in initialize_parameters

    # Returns:
    # Z3 -- the output of the last LINEAR unit
    # """
    
    # Retrieve the parameters from the dictionary "parameters" 
	W1 = parameters['W1']
	keep_prob1 = keep_prob['keep_prob1']
	keep_prob2 = keep_prob['keep_prob2']
    #W2 = parameters['W2']
    ### START CODE HERE ###
    # CONV2D: stride of 1, padding 'SAME'
	Z1 = tf.nn.conv2d(X,W1, strides = [1,1,1,1], padding = 'SAME')
    # RELU
	A1 = tf.nn.relu(Z1)
    # MAXPOOL: window 2x2, stride 1, padding 'SAME'
	P1 = tf.nn.max_pool(A1, ksize = [1,2,2,1], strides = [1,1,1,1], padding = 'SAME')
    # CONV2D: filters W2, stride 1, padding 'SAME'
    #Z2 = tf.nn.conv2d(P1,W2, strides = [1,1,1,1], padding = 'SAME')
    # RELU
    #A2 = tf.nn.relu(Z2)
    # MAXPOOL: window 4x4, stride 4, padding 'SAME'
    #P2 = tf.nn.max_pool(A2, ksize = [1,4,4,1], strides = [1,4,4,1], padding = 'SAME')
	# dropout layer
	D1 = tf.nn.dropout(P1, keep_prob1)
	# FLATTEN
	D1 = tf.contrib.layers.flatten(D1)
	# FULLY-CONNECTED
	Z2 = tf.contrib.layers.fully_connected(D1, 32, activation_fn=tf.nn.relu)
	# dropout layer
	D2 = tf.nn.dropout(Z2, keep_prob2)  
    # FULLY-CONNECTED without non-linear activation function (not call softmax).
    # 3 neurons in output layer. Hint: one of the arguments should be "activation_fn=None" 
	Z3 = tf.contrib.layers.fully_connected(D2, 3, activation_fn=None)
    ### END CODE HERE ###
	return Z3
### TEST forward_propagation
# tf.reset_default_graph()

# with tf.Session() as sess:
    # np.random.seed(1)
    # X, Y = create_placeholders(64, 64, 3, 6)
    # parameters = initialize_parameters()
    # Z3 = forward_propagation(X, parameters)
    # init = tf.global_variables_initializer()
    # sess.run(init)
    # a = sess.run(Z3, {X: np.random.randn(2,64,64,3), Y: np.random.randn(2,6)})
    # print("Z3 = " + str(a))
####################################################################
def compute_cost(Z3, Y):
    # """
    # Computes the cost
    
    # Arguments:
    # Z3 -- output of forward propagation (output of the last LINEAR unit), of shape (6, number of examples)
    # Y -- "true" labels vector placeholder, same shape as Z3
    
    # Returns:
    # cost - Tensor of the cost function
    # """
    
    ### START CODE HERE ### (1 line of code)
    cost = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(logits = Z3, labels = Y))
    ### END CODE HERE ###
    
    return cost
### test compute_cost
# tf.reset_default_graph()

# with tf.Session() as sess:
    # np.random.seed(1)
    # X, Y = create_placeholders(64, 64, 3, 6)
    # parameters = initialize_parameters()
    # Z3 = forward_propagation(X, parameters)
    # cost = compute_cost(Z3, Y)
    # init = tf.global_variables_initializer()
    # sess.run(init)
    # a = sess.run(cost, {X: np.random.randn(4,64,64,3), Y: np.random.randn(4,6)})
    # print("cost = " + str(a))
######################################################################


def model(X_train, Y_train, X_test, Y_test, X_validation, Y_validation, learning_rate = 0.009,
          num_epochs = 100, minibatch_size = 32, print_cost = True):
    # """
    # Implements a three-layer ConvNet in Tensorflow:
    # CONV2D -> RELU -> MAXPOOL -> CONV2D -> RELU -> MAXPOOL -> FLATTEN -> FULLYCONNECTED
    
    # Arguments:
    # X_train -- training set, of shape (None, 64, 64, 3)
    # Y_train -- test set, of shape (None, n_y = 6)
    # X_test -- training set, of shape (None, 64, 64, 3)
    # Y_test -- test set, of shape (None, n_y = 6)
    # learning_rate -- learning rate of the optimization
    # num_epochs -- number of epochs of the optimization loop
    # minibatch_size -- size of a minibatch
    # print_cost -- True to print the cost every 100 epochs
    
    # Returns:
    # train_accuracy -- real number, accuracy on the train set (X_train)
    # test_accuracy -- real number, testing accuracy on the test set (X_test)
    # parameters -- parameters learnt by the model. They can then be used to predict.
    # """
    
	ops.reset_default_graph()                         # to be able to rerun the model without overwriting tf variables
	tf.set_random_seed(1)                             # to keep results consistent (tensorflow seed)
	seed = 3                                          # to keep results consistent (numpy seed)
    #(m, n_H0, n_W0, n_C0) = X_train.shape			  # if input has channels, 3D or more
	(m, n_H0, n_W0,n_C0) = X_train.shape	
	n_y = Y_train.shape[1]                            
	costs = []                                        # To keep track of the cost
    
    # Create Placeholders of the correct shape
    ### START CODE HERE ### (1 line)
	X, Y = create_placeholders(n_H0, n_W0, n_C0, n_y) # if 3D and more
	#X, Y = create_placeholders(n_H0, n_W0, n_y)
	keep_prob1 = tf.placeholder(tf.float32)
	keep_prob2 = tf.placeholder(tf.float32)
    ### END CODE HERE ###

    # Initialize parameters
    ### START CODE HERE ### (1 line)
	parameters_Val = {"f1" : 8, "n_C1": 64, "n_C0": n_C0}
	keep_prob = {"keep_prob1" : keep_prob1, "keep_prob2": keep_prob2}
	parameters = initialize_parameters(parameters_Val)
	### END CODE HERE ###
    
    # Forward propagation: Build the forward propagation in the tensorflow graph
    ### START CODE HERE ### (1 line)
	Z3 = forward_propagation(X, parameters,keep_prob)
    ### END CODE HERE ###
    
    # Cost function: Add cost function to tensorflow graph
    ### START CODE HERE ### (1 line)
	cost = compute_cost(Z3, Y)
    ### END CODE HERE ###
    
    # Backpropagation: Define the tensorflow optimizer. Use an AdamOptimizer that minimizes the cost.
    ### START CODE HERE ### (1 line)
	optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate).minimize(cost)
	#optimizer = tf.train.AdadeltaOptimizer(learning_rate=1., rho=0.95, epsilon=1e-6).minimize(cost)
    ### END CODE HERE ###
    
    # Initialize all the variables globally
	init = tf.global_variables_initializer()
	saver = tf.train.Saver()
	restore_checkpoint = True
	best_loss_val = np.infty
	checkpoint_path = "./my_conv_network"
    # Start the session to compute the tensorflow graph
	with tf.Session() as sess:
        
        # Run the initialization
		sess.run(init)
        # Do the training loop
		for epoch in range(num_epochs):

			minibatch_cost = 0.
			num_minibatches = int(m / minibatch_size) # number of minibatches of size minibatch_size in the train set
			seed = seed + 1
			minibatches = random_mini_batches(X_train, Y_train, minibatch_size, seed)

			for minibatch in minibatches:

				# Select a minibatch
				(minibatch_X, minibatch_Y) = minibatch
				# IMPORTANT: The line that runs the graph on a minibatch.
				# Run the session to execute the optimizer and the cost, the feedict should contain a minibatch for (X,Y).
				### START CODE HERE ### (1 line)
				_ , temp_cost = sess.run([optimizer, cost], feed_dict={X:minibatch_X, Y:minibatch_Y, keep_prob1 : 0.25, keep_prob2 : 0.5})
				### END CODE HERE ###

				minibatch_cost += temp_cost / num_minibatches


			# Print the cost every epoch
			if print_cost == True and epoch % 5 == 0:
				print ("Cost after epoch %i: %f" % (epoch, minibatch_cost))
			if print_cost == True and epoch % 1 == 0:
				costs.append(minibatch_cost)
			# And save the model if it improved:
			if minibatch_cost < best_loss_val:
				save_path = saver.save(sess, checkpoint_path)
				best_loss_val = minibatch_cost
                
		# plot the cost
		# plt.plot(np.squeeze(costs))
		# plt.ylabel('cost')
		# plt.xlabel('iterations (per tens)')
		# plt.title("Learning rate =" + str(learning_rate))
		# plt.show()
		predict_op = tf.argmax(Z3, 1)
		correct_prediction = tf.equal(predict_op, tf.argmax(Y, 1))
        
        # Calculate accuracy on the test set
		accuracy = tf.reduce_mean(tf.cast(correct_prediction, "float"))
		print(accuracy)
	with tf.Session() as sess:
		saver.restore(sess, checkpoint_path)
		train_accuracy = accuracy.eval({X: X_validation, Y: Y_validation, keep_prob1 : 1.0, keep_prob2 : 1.0})
		test_accuracy = accuracy.eval({X: X_test, Y: Y_test, keep_prob1 : 1.0, keep_prob2 : 1.0})
		print("Train Accuracy:", train_accuracy)
		print("Test Accuracy:", test_accuracy)		
		return train_accuracy, test_accuracy, parameters
        # Calculate the correct predictions
	# with tf.Session() as sess:
	# saver.restore(sess, checkpoint_path)

		
### run it
def main():
# %matplotlib inline
	np.random.seed(1)
	all_data2, all_labels2 = CirRecDataset()
	# print(div_size) 870
	# print(mult_size) 872
	# print(mod_size) 872
	# print(newmul_size) 342
	div_data = all_data2[0:200]
	mul_data = all_data2[875:1075]
	mod_data = all_data2[1215:1415]
	div_labels = all_labels2[0:200]
	mul_labels = all_labels2[875:1075]
	mod_labels = all_labels2[1215:1415]
	all_data= np.concatenate((div_data, mul_data, mod_data), axis=0)
	all_labels = np.concatenate((div_labels, mul_labels, mod_labels), axis=0)
	### a[[0,1,3], :][:, [0,2]]  # y[np.array([0,2,4]),1:3]
	all_data, all_labels = next_batch(all_data.shape[0], all_data, all_labels)
	validation_size =  50
	test_size = 50
	train_size = all_data.shape[0] - validation_size - test_size
	X_test_orig = all_data[:test_size]
	Y_test_orig = all_labels[:test_size]
	train_data = all_data[test_size:]
	train_labels = all_labels[test_size:]
	X_validation_orig = train_data[:validation_size]
	Y_validation_orig = train_labels[:validation_size]
	X_train_orig = train_data[validation_size:]
	Y_train_orig = train_labels[validation_size:]
	# X_train = X_train_orig/255.
	# X_test = X_test_orig/255.
	Input_shape = [None, 120, 222, 1]; 
	X_train = X_train_orig.reshape([-1, Input_shape[1], Input_shape[2],1])
	X_test = X_test_orig.reshape([-1, Input_shape[1], Input_shape[2],1])
	X_validation =  X_validation_orig.reshape([-1, Input_shape[1], Input_shape[2],1])
	Y_test_orig = Y_test_orig.reshape([1,Y_test_orig.shape[0]])
	Y_train_orig = Y_train_orig.reshape([1,Y_train_orig.shape[0]])
	Y_validation_orig = Y_validation_orig.reshape([1,Y_validation_orig.shape[0]])
	print ("number of training examples = " + str(X_train.shape[0]))
	print ("number of test examples = " + str(X_test.shape[0]))
	print ("X_train shape: " + str(X_train.shape))
	print ("Y_train shape: " + str(Y_train_orig.shape))
	print ("X_test shape: " + str(X_test.shape))
	print ("Y_test shape: " + str(Y_test_orig.shape))
	#print(Y_test_orig)
	Y_train_orig = Y_train_orig.astype(int)
	Y_test_orig = Y_test_orig.astype(int)
	Y_validation_orig = Y_validation_orig.astype(int)
	Y_train = convert_to_one_hot(Y_train_orig, 3).T
	Y_test = convert_to_one_hot(Y_test_orig, 3).T
	Y_validation = convert_to_one_hot(Y_validation_orig, 3).T
	conv_layers = {}
	_, _, parameters = model(X_train, Y_train, X_test, Y_test, X_validation, Y_validation)
if __name__ == "__main__": main()
##########################################################


