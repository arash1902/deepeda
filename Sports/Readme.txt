The core function for the SPORTCNN is the writeAlan(Gia_MapLut_SPORTS* pLuts, int lutNum, map<int, int>& out2Lut, ofstream& output). The writeAlan
function is the main function that use the mapping logic to plot the LUT diagram out. It uses the Dar_LibReturnClass function from the Dar.h to review the
code of the fanin and fanout. 
We delete the other original irrelevent functions because we only need to plot the map out, but not grouping it as the way in the original file. In the future,
we will apply the YOLO algorithm to do the partition process of the circuit map.
Besides the writeAlan function, I also keep the function of getMapLut, writeTruth, writeLutConnect2, writeLutConnect, writeSingleTruth, assignID2EV, 
BFSGrouping, buildLutGraph, writeLutTable, and writeLut. 

There are a list of important class, function and variables that support the functionality of the writeAlan function:

In sportCNN.h 

class dEV bag:

The class dEV initialize three variables that are the lut, count and ones. The lut is the id of different gates, including add, multiplier, and splitter. The count
is the times that different lut appear. The ones is where we should put the "1" in the long id map code of Lut.  At the same time, it also include the 
functions that could add the count, get the count, get lut code and get the ones position.


class EVGroup:
It is the class using for the grouping. It will construct the LUTs list vector and do the seperation of first half and second half to do the new grouping through
the void split() function.  

class dEV:
It is the class that construct and modify the vectors of fanin and fanout. In this class, user could insert the fanins and fanouts from both the head and end 
of the vector. The mode single key will split by "&" and the mode double key will split by the "||". Then it could output the vector. 

struct Gia_MapLut_Sport:
It stores all the important parameters that suppot the construction of the LUTs Map.


In sportCNN.cpp

function getMapLut:
"getMapLut" could assign map cell ID to each nodes and gates and finally return the "pLuts"that is the lut node code in the scale intialize by the
sportCNN(Abc_Ntk_t *currNtk). The Abc_Ntk_t *currNtk initialize the numbers of aigNum, the lutNum, lutGroupnum, and Truth number in the plotting map.
It also fetch the detailed information of the type of the lut and its output ID. 

function assignID2EV:
It is the function that will insert the related npnID to the fanins and fanouts. Then, it will combine fanins and fanouts to the new EV and put it to the dEV.
The dEV will be introduced in the SPORTCNN.h part

function BFSGrouping:
It is a grouping function that help to write the LUT out. We will use the YOLO algorithm to seperate the map graph and introduce the new grouping way,
so the function here is more like a reference. 

function writeGroups:
As the name of the function explained, this function write the group out. It first intialize the Luts, and group bags vectors. Then it iterate through the groups
to get the IDs, counts and informations. Finally the function will give the output sorting by the size of the bags. 

function EvenlyGrouping:
Same as the BFS Grouping, it is another way of grouping that seperate the luts with evenly numbers out of the total Lut numbers. 

function buildLutGraph:
It  will link the fanoutIDs with the faninLut. In this way it would link the each Lut together to build the graph. 

function writeLUT:
It is a function that run and combine the functions list previously. The writeLUT function will first use the getmapLut function to get the pLuts list. 
Then it intialize the vectors of "input of Luts" and "output of Luts". After that, it uses the buildLutGraph function to build the graph out and groups
each section and finally print that out. 

function getOneWindow:
It is a searching function to find a specific targetLut to get the location of it in the group and the other luts in the range. 

function getSubCircuits:
Through the function of getOneWindow, we could get the related groups of the getOneWindow you find to construct the a sub circuit map.

function getSubCircuits2:
The difference between function getSubCircuits and getSubCircuits2 is the plotting range between getSubCircuits and getSubCircuits2 is changed. 
One is "int size = (_lutGroupNum*10 > lutNum/num*5)? _lutGroupNum*10: lutNum/num*6", whereas the other is  "8*_lutGroupNum"

function writeCNNPartition:
This is the original partition way. We will substitute it with a new partition way called YOLO.

In sport Cmd.cpp

kept only function Sport_CommandCNN() and function to initialize this

Sport_CommandCNN() is the command function that finally execute the sportCNN functions with the CNNpartiton().


