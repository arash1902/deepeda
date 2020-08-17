# Deep learning-driven EDA

Deep learning-driven digital circuit recognition  


## Publication

+ Arash Fayyazi, Soheil Shababi, Pierluigi Nuzzo, Shahin Nazarian, and Massoud Pedram. [Deep Learning-Based Circuit Recognition Using Sparse Mapping and Level-Dependent Decaying Sum Circuit Representations](https://ieeexplore.ieee.org/abstract/document/8715251). In IEEE DATE, 2019.



## Developers
***********
Arash Fayyazi <fayyazi@usc.edu>

Pierluigi Nuzzo <Nuzzo@usc.edu>

Shahin Nazarian <shahin.nazarian@usc.edu>

Massoud Pedram <pedram@usc.edu>


## Questions or Bugs?
***********
You may send email to  <fayyazi@usc.edu> for any questions you may have or bugs that you find.



%%%%%%%%%%%%%% ABC %%%%%%%%%%%%%%%%%%%%
To compile ABC as a binary, download and unzip the code, then type
>>> make
for more information, visit https://github.com/berkeley-abc/abc

%%%%%%%%%%%%%% framework in ABC %%%%%%%%%%%%%%%%%%%%
To compile the proposed framework:
first go under abc/src/, create a folder called "ext"

under abc/src/ext, copy benchmarks and Sports directory.

under abc/, execute

>>>make -j

Then in ABC folder, execute
>>>./abc 
this command runs the binary in the command-line mode:
then execute,

>>>&r mul_4x4_a.aig; &if -K 4; sportCNN -F mul_4x4_a -M 1

if you want to generate CNN input data from all benchmarks, use following commands:

>>>python ReadDataArash_baseline.py

or

>>>python ReadDataArash_2L.py

or

>>>python ReadDataArash_CellLib.py

based on desire approach.
%%%%%%%%%%%%%% CNN run and test %%%%%%%%%%%%%%%%%%%%

Please first correct all paths based on where you put ABC and these files.

for training run following command:

>>>python ArashCNN.py

or

>>>python ArashCNN_2L.py

or 

>>>python ArashCNN_CellLibTest.py

Then for testing a input:

>>>python TestRun.py input_file label

example:

>>>python TestRun.py div_16d16_a 0

result will be:

CNN predict: division
real operation is: division


