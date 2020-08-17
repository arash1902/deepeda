#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "proof/fraig/fraig.h"
#include "opt/fxu/fxu.h"
#include "opt/cut/cut.h"
#include "map/fpga/fpga.h"
#include "map/if/if.h"
#include "opt/sim/sim.h"
#include "opt/res/res.h"
#include "opt/lpk/lpk.h"
#include "aig/gia/giaAig.h"
#include "opt/dar/dar.h"
#include "opt/mfs/mfs.h"
#include "proof/fra/fra.h"
#include "aig/saig/saig.h"
#include "proof/int/int.h"
#include "proof/dch/dch.h"
#include "proof/ssw/ssw.h"
#include "opt/cgt/cgt.h"
#include "bool/kit/kit.h"
#include "map/amap/amap.h"
#include "opt/ret/retInt.h"
#include "sat/cnf/cnf.h"
#include "proof/cec/cec.h"
#include "proof/pdr/pdr.h"
#include "misc/tim/tim.h"
#include "bdd/llb/llb.h"
#include "bdd/bbr/bbr.h"
#include "map/cov/cov.h"
#include "base/cmd/cmd.h"
#include "proof/abs/abs.h"
#include "sat/bmc/bmc.h"
#include "proof/ssc/ssc.h"
#include "opt/sfm/sfm.h"
#include "bool/rpo/rpo.h"
#include "map/mpm/mpm.h"
#include "aig/gia/gia.h"
#include "ext/Sport/SportCNN.h"
#include<iostream>
#include<fstream>
#include <algorithm>
#include <cmath>  

// Soheil Added:
#include "map/mio/mioInt.h"

//


using namespace std;
extern "C" unsigned * Gia_ManConvertAigToTruth( Gia_Man_t * p, Gia_Obj_t * pRoot, Vec_Int_t * vLeaves, Vec_Int_t * vTruth, Vec_Int_t * vVisited );
extern "C" int Dar_LibReturnClass( unsigned uTruth );

ABC_NAMESPACE_IMPL_START

void sportUsageCNN(){
  Abc_Print( -2, "usage: sportCNN [-F <fileName>][-P <folderName>] [-M <id>] \n" );
  Abc_Print( -2, "\t -h    : print the command usage\n");
  Abc_Print( -2, "\t -F <filename>   : write out matrices to describe the current circuit to <filename>, run after &if -K 4\n");
  Abc_Print( -2, "\t -P <fileFolder> : for locate operator, partition the current circuit, write out matrices for sub-circuits into <folderName>, come with *.group and *.data \n");
  Abc_Print( -2, "\t -M <mode>   : indicate the output mode 1-5, for DFS order, run &dfs first \n");
  Abc_Print( -2, "\t mode 1: singleEV+original order\n");
  Abc_Print( -2, "\t mode 2: doubleEV+original order\n");
  Abc_Print( -2, "\t mode 3: singleEV+BFS order\n");
  Abc_Print( -2, "\t mode 4: doubleEV+BFS order\n");
  Abc_Print( -2, "\t mode 5: naive format, run after &if -K 3\n");





}
sportCNN::sportCNN(Abc_Ntk_t* currNtk){

  _currNtk = currNtk;
  _aigNum = 20000;
  _lutNum = 4000;
  _lutGroupNum = 40;
  _truthNum = 10; // top 10?
}

Gia_MapLut_Sport* sportCNN::getMapLut( int& lutNum){
 
  
  Abc_Obj_t * pObj; // istead of Gia_Obj_t * pObj;
  int i, k, iFan, iLut = 0;
  //Soheil added:
  Mio_Gate_t * mappedCell; // is used for assigning mapping Cell id of each node of current network
  //
  int nLuts = _currNtk->vObjs->nSize; // instead of  nLuts = 1 + _currNtk->Vobjs->nSize              
  Gia_MapLut_Sport* pLuts = ABC_CALLOC( Gia_MapLut_Sport, nLuts );
//  pLuts[iLut].Out = 0;  ?? 
//  iLut++;  ??  
 
  //inputs
  Abc_NtkForEachPi(_currNtk, pObj, i) { //instead of Gia_ManForEachCi( _cir, pObj, i )
    pLuts[iLut].Type = 1;	 
 	//mappedCell = (Mio_Gate_t *) pObj->pData;// mapped cell of current object in current network		
        
	//dpLuts[iLut].cellId = mappedCell->Cell; // mapped cell id of current object
	
    pLuts[iLut].Out = pObj->Id;      
	iLut++;  
    }
	
 
    // nodes  
    Abc_NtkForEachNode(_currNtk, pObj, i)        
        {
		pLuts[iLut].Type = 3;         
		Abc_ObjForEachFaninId(pObj, iFan, k) //instead of Gia_LutForEachFanin(_cir,i,iFan,k )	    
			pLuts[iLut].pFans[k] = iFan;

		pLuts[iLut].nFans = k;
		assert(k == pObj->vFanins.nSize); // soheil assert!            
		pLuts[iLut].Out = pObj->Id;
		mappedCell = (Mio_Gate_t *) pObj->pData;// mapped cell of current object in current network		
		pLuts[iLut].cellId = mappedCell->Cell; // mapped cell id of current object
		iLut++;
        }
    // outputs
    Abc_NtkForEachPo(_currNtk, pObj, i) //Gia_ManForEachCo( _cir, pObj, i )
    {
        pLuts[iLut].Type = 2;
        pLuts[iLut].pFans[0] = pObj->vFanins.pArray[0]; // instead of Gia_ObjFanin0(pObj)->Value;  
	//soheil: because each PO has only one fanin 
        pLuts[iLut].nFans = 1;
       	pLuts[iLut].Out = pObj->Id;
 	//mappedCell = (Mio_Gate_t *) pObj->pData;// mapped cell of current object in current network		
	//pLuts[iLut].cellId = mappedCell->Cell; // mapped cell id of current object
        iLut++;
    }
//    assert( iLut == nLuts );
  
    printf("iLut=%d, nLuts=%d", iLut,nLuts);

    lutNum = nLuts;  

  return pLuts;

}
void sportCNN::assignId2EV( Gia_MapLut_Sport* pLuts, vector<dEV* >& id2dEV, int lutNum, map<int, int>& out2Lut, map<int, set<int>* >& id2FanoutIDs){
  for(int idx = 0; idx < lutNum; ++idx){
    set<int> fanins;
    set<int> fanouts;
    int npnID = pLuts[idx].cellId;
    fanins.insert(npnID);
    fanouts.insert(npnID);

    for(int j = 0; j < pLuts[idx].nFans; j++){
      int faninLut = out2Lut[pLuts[idx].pFans[j]];
      npnID = pLuts[faninLut].cellId;
      fanins.insert(npnID); 
    }

    if(id2FanoutIDs.find(idx)!= id2FanoutIDs.end()){
      set<int>* fanoutIDs = id2FanoutIDs[idx];
      set<int>::iterator site = fanoutIDs->begin();
      for(; site != fanoutIDs->end(); ++site){
        npnID = pLuts[(*site)].cellId;
        fanouts.insert(npnID);
      }
    }
    dEV* newEV = new dEV(fanins, fanouts);
    id2dEV.push_back(newEV);
  }
}
void sportCNN::BFSGrouping( Gia_MapLut_Sport* pLuts, int lutNum, map<int, int>& out2Lut, map<int, set<int>* >& id2FanoutIDs, vector<EVGroup*>& groups ){
  queue<int> todoList;
  vector<int> newOrder;
  set<int> visitedNode;
  int piNum = Gia_ManPiNum(_cir);
  for(int i = 0; i < piNum+1; ++i )	
    todoList.push(i);
 
  while(!todoList.empty()){
    int vertex = todoList.front();
    todoList.pop();
    if(visitedNode.find(vertex)!= visitedNode.end()) // ideally never
      continue;
    newOrder.push_back(vertex);
    visitedNode.insert(vertex);
    if(id2FanoutIDs.find(vertex)!= id2FanoutIDs.end()){
      set<int>* fanouts = id2FanoutIDs[vertex];
      set<int>::iterator site = fanouts->begin();
      for(; site!= fanouts->end(); ++site){
        if(visitedNode.find((*site))!= visitedNode.end())// never
          continue;
        bool fully = true;
        for(int k = 0; k < pLuts[(*site)].nFans; k++){
          int faninLut = out2Lut[(pLuts[(*site)].pFans[k])];
          if(visitedNode.find(faninLut) == visitedNode.end()){
            fully = false;
            break;
          }
        }
        if(fully)
          todoList.push((*site));
      }
    }
  }
  if(newOrder.size() != lutNum)
    cerr<<"Error! mismatch!\n";

  int groupSize = lutNum/_lutGroupNum;
  if(groupSize < 1){
    cerr<<"WARN: number of LUTs is too small.\n";
    return;
  }
//  vector<EVGroup* > groups;
  int remain = lutNum%_lutGroupNum;
  int idx = 0;
  for(int i = 0; i < _lutGroupNum; ++i){
    // inside each group perform pooling
    set<int> LUTs;
    int upper = idx+groupSize;
    if(i < remain )
      upper += 1;
    for(; idx < upper; ++idx)
      LUTs.insert(newOrder[idx]);
   
    EVGroup* newGroup = new EVGroup(LUTs);
    groups.push_back(newGroup); 
  } 
} 



void sportCNN::writeGroups(vector<EVGroup*>& groups, vector<dEV* >& id2dEV, ofstream& output, int mode){

  if(groups.size() != _lutGroupNum){
    cerr<<"ERROR: mismatch group number\n";
    return;
  }
  for(int i = 0; i < groups.size(); ++i){
    map<string, dEVBag*> key2Bags;
    vector<dEVBag*> possibleBags;
    set<int>& LUTs = groups[i]->getLUTs();
    set<int>::iterator site = LUTs.begin();
    for(; site!= LUTs.end(); ++site){
      string key;
      if(mode%2 == 1) // single
        key = id2dEV[(*site)]->getSingleKey();
      else // double
        key = id2dEV[(*site)]->getDoubleKey();
      if(key2Bags.find(key) == key2Bags.end()){
        dEVBag* newBag;
        if(mode%2 == 1) // single
          newBag = new dEVBag(id2dEV[(*site)]->getSingleCount(), (*site));
        else // double
          newBag = new dEVBag(id2dEV[(*site)]->getDoubleCount(), (*site));

        key2Bags[key] = newBag;
        possibleBags.push_back(newBag);
      }
      else
        key2Bags[key]->addCount();
    }
    sort(possibleBags.begin(), possibleBags.end(), sortDouble);
    if(possibleBags.size() >=3){
      //if(possibleBags[0]->getCount() > 1)
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[1]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[2]->getLut()]->outputBag(output, mode);

    }
    else if(possibleBags.size() == 2){
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[1]->getLut()]->outputBag(output, mode);
    }
    else if(possibleBags.size() == 1){
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
      id2dEV[possibleBags[0]->getLut()]->outputBag(output, mode);
    }
    else
      cerr<<"Zero bag?!"<<endl;
    for(int k = 0; k < possibleBags.size(); ++k)
      delete(possibleBags[k]); 


  } 
}
void sportCNN::evenlyGrouping(int lutNum, vector<EVGroup* >& groups){
  int groupSize = lutNum/_lutGroupNum;
  if(groupSize < 1){
    cerr<<"WARN: number of LUTs is too small.\n";
    return;
  }
//  vector<EVGroup* > groups;
  int remain = lutNum%_lutGroupNum;
  int idx = 0;
  for(int i = 0; i < _lutGroupNum; ++i){
    // inside each group perform pooling
    set<int> LUTs;
    int upper = idx+groupSize;
    if(i < remain )
      upper += 1;
    for(; idx < upper; ++idx)
      LUTs.insert(idx);
   
    EVGroup* newGroup = new EVGroup(LUTs);
    groups.push_back(newGroup); 
  } 
}


void sportCNN::buildLutGraph(Gia_MapLut_Sport* pLuts,
                            int lutNum,
                            map<int, int>& out2Lut,
                            map<int, set<int>* >& id2FanoutIDs ){
  for(int i = 0; i < lutNum; ++i){
    int outputId = pLuts[i].Out;
    out2Lut[outputId] = i;
    for(int j = 0; j < pLuts[i].nFans; j++){
      int faninLut = out2Lut[pLuts[i].pFans[j]];
      if(id2FanoutIDs.find(faninLut)== id2FanoutIDs.end()){
        set<int>* newList = new set<int>;
        id2FanoutIDs[faninLut] = newList;
      }
      id2FanoutIDs[faninLut]->insert(i);
    }
  }
}

void sportCNN::writeAlan(Gia_MapLut_Sport* pLuts, int lutNum, map<int, int>& out2Lut, ofstream& output){
  int counter[196];
  for(int i = 0; i < 196; ++i)
    counter[i] = 0;
  int all = 0;
  map<int, int> mapping;
  mapping[0] = 0;
  mapping[2] = 1;
  mapping[5] = 2;
  mapping[13] =  3;
  mapping[15] = 4;
  mapping[21] = 5;
  mapping[83] = 6;
  mapping[103] = 7;
  mapping[105] = 8;
  mapping[109] = 9;
  mapping[120] = 10;
  mapping[180] = 11;
  mapping[220] = 12;
  //mapping[2] = 13;

  //set<int> exist;
  for(int idx = 0; idx < lutNum; ++idx){
    int oldID = Dar_LibReturnClass((pLuts[idx].pTruth[0]));
    int npnID = 0;
    if(mapping.find(oldID)==mapping.end()){
      npnID = 13;
      cerr<<"hey!"<<oldID<<endl;
    }
    else
      npnID = mapping[oldID];
    // exist.insert(npnID);
    /*if(npnID >= 10){
      cerr<<"ERROR!!!"<<npnID<<endl;
      
      //return;
    }*/
    for(int j = 0; j < pLuts[idx].nFans; j++){
      int faninLut = out2Lut[pLuts[idx].pFans[j]];
      int oldID2 = Dar_LibReturnClass(pLuts[faninLut].pTruth[0]);
      int npnID2 = 0;
      if(mapping.find(oldID2)==mapping.end())
        npnID2 = 13;
      else
        npnID2 = mapping[oldID2];
 
      counter[npnID*10+npnID2]++;
      all++; 
    }
  }
/*  set<int>::iterator site = exist.begin();

  for(; site!= exist.end(); ++site)
    cerr<<" "<<(*site);
  cerr<<endl;*/
  for(int i = 0; i < 196; ++i)
    output<<(double)(counter[i])/(double)(all)<<endl;
}
bool sportCNN::writeLUT(ofstream& output, int mode){
  int lutNum = 0;
  
  Gia_MapLut_Sport* pLuts = getMapLut(lutNum);

 map<int, int> out2Lut;
  map<int, set<int>* > id2FanoutIDs;

  buildLutGraph(pLuts, lutNum, out2Lut, id2FanoutIDs);
  
  if(mode == 5){ // &if -K 3
    writeAlan(pLuts, lutNum, out2Lut, output);
  }
  else{
      // step 1 assgin EV
    vector<dEV* > id2dEV;
    assignId2EV(pLuts, id2dEV, lutNum, out2Lut, id2FanoutIDs);
    vector<EVGroup* > groups;
    if(mode == 1 || mode == 2) // current order
      evenlyGrouping(lutNum, groups);
    else
      BFSGrouping(pLuts, lutNum, out2Lut, id2FanoutIDs, groups);
 
    writeGroups(groups, id2dEV, output, mode); // evenly distributed
      //or non
    for(int i = 0; i < groups.size(); ++i)
      delete groups[i];
  
    for(int i = 0; i < id2dEV.size(); ++i)
      delete id2dEV[i];

  }
    
 
  map<int, set<int>* >::iterator mite = id2FanoutIDs.begin();
  for(; mite!= id2FanoutIDs.end(); ++mite)
    delete mite->second; 

   
  ABC_FREE( pLuts );

  return true;
}
void sportCNN::getOneWindow(Gia_MapLut_Sport* pLuts, int targetLut,
                           int targetSize, map<int, int>& out2Lut,
                           map<int, set<int>* >& id2FanoutIDs,
                           set<int>* members){
  set<int> explored;
  set<int> newMember;

  members->insert(targetLut);
  newMember.insert(targetLut);
    //cerr<<"sub: "<<i<<endl; 
    // flow: output + input
  while(members->size() < targetSize ){
    if(newMember.size() == 0)
      break;
    set<int> nextNew;
    set<int>::iterator site = newMember.begin();
    for(; site!= newMember.end(); ++site){
      if(explored.find(*site)!= explored.end())
        continue;
    //  if(members->size() > targetSize*2)
    //    break; // too big
        // get outputs 
      if(id2FanoutIDs.find(*site)!= id2FanoutIDs.end()){
        set<int>* thisFanouts = id2FanoutIDs[*site];
        members->insert(thisFanouts->begin(), thisFanouts->end());
        nextNew.insert(thisFanouts->begin(), thisFanouts->end());
      }
      for(int j = 0; j < pLuts[(*site)].nFans; j++ ){
        int faninLut = out2Lut[pLuts[(*site)].pFans[j]];
        members->insert(faninLut);
        nextNew.insert(faninLut);
      }
      explored.insert(*site);
    }
    newMember.swap(nextNew);
  }

}

void sportCNN::getSubCircuits2(Gia_MapLut_Sport* pLuts,
                        int lutNum, map<int, int>& out2Lut,
                        map<int, set<int>* >& id2FanoutIDs,
                        vector<vector<int>* >& subCircuits, int num){
  // num max number of groups
  set<int> coveredLUT;
  vector<set<int>* > initialGroups;
  int jump = lutNum/num;
  for(int i = 1; i < lutNum; i+=jump){
    set<int>* members = new set<int>;
    set<int>* members2 = new set<int>;
    if(coveredLUT.find(i)!=coveredLUT.end())
      continue; // jump
    int size = (_lutGroupNum*10 > lutNum/num*5)? _lutGroupNum*10: lutNum/num*6;
    getOneWindow(pLuts, i, size, out2Lut,
                 id2FanoutIDs, members);

    getOneWindow(pLuts, i, size/2, out2Lut, id2FanoutIDs, members2);
    if(members->size() < _lutGroupNum*2){
      delete members;
      continue;
    }
    if(members2->size() < _lutGroupNum*2){
      delete members2;
      continue;
    }
    coveredLUT.insert(members->begin(), members->end());
    initialGroups.push_back(members);
    initialGroups.push_back(members2);
 /*   vector<int>* newGroup = new vector<int>;
    set<int>::iterator site3 = members.begin();
    for(; site3!= members.end(); ++site3)
      newGroup->push_back((*site3));
    subCircuits.push_back(newGroup);*/
  }
/*  if(initialGroups.size() > num){
    cerr<<"initial Groups: "<<initialGroups.size()<<endl; 
    int remain = initialGroups.size()%num;
    int idx = 0;
    int groupSize = initialGroups.size()/num;
    for(int k = 0; k < num; ++k){
      set<int> unionSet;
      int upper = idx+groupSize;
      if(k < remain)
        upper+=1;
      for(; idx < upper; idx++){
        unionSet.insert(initialGroups[idx]->begin(),
                        initialGroups[idx]->end());
        delete initialGroups[idx];
      }
      vector<int>* newGroup = new vector<int>;
      set<int>::iterator site3 = unionSet.begin();
      for(; site3!= unionSet.end(); ++site3)
        newGroup->push_back((*site3));
      subCircuits.push_back(newGroup);
    }
 
  }
  else{*/
    for(int i = 0; i < initialGroups.size(); ++i){
      set<int>* currentGroup = initialGroups[i];
      vector<int>* newGroup = new vector<int>;
      set<int>::iterator site3 = currentGroup->begin();
      for(; site3!= currentGroup->end(); ++site3)
        newGroup->push_back((*site3));
      subCircuits.push_back(newGroup);
      delete currentGroup;
    }
 // }
} 
void sportCNN::getSubCircuits(Gia_MapLut_Sport* pLuts,
                        int lutNum, map<int, int>& out2Lut,
                        map<int, set<int>* >& id2FanoutIDs,
                        vector<vector<int>* >& subCircuits){
  set<string> usedKey;
  for(int i = 1; i < lutNum; i+=3){
    //int size = 1;
    //while(size < 2){
    set<int>* members = new set<int>;
    getOneWindow(pLuts, i, 8*_lutGroupNum, out2Lut,
                 id2FanoutIDs, members);
     // done with one group!
     // size+=1;
    if(members->size() < _lutGroupNum*2){
      delete members;
      continue; 
    }
    ostringstream convert;
    set<int>::iterator site2 = members->begin();
    for(; site2 != members->end(); ++site2)
        convert<<(*site2)<<"&";
      
    string key = convert.str();
    if(usedKey.find(key) == usedKey.end()){
      usedKey.insert(key);
      vector<int>* newGroup = new vector<int>;
      set<int>::iterator site3 = members->begin();
      for(; site3!= members->end(); ++site3)
        newGroup->push_back((*site3));
      subCircuits.push_back(newGroup);
    }
    delete members; 
  }
}

bool sportCNN:: writeCNNPartition(char* pFolderSpec, int mode, int num){
  // file name : ostringstream convert;
  //  convert<<pFolderSpec<<"_"<<iter<<".v";
  int lutNum = 0;
  Gia_MapLut_Sport* pLuts = getMapLut( lutNum);
  if(lutNum/_lutGroupNum == 0)
    return false;
  map<int, int> out2Lut;
  map<int, set<int>* > id2FanoutIDs;

  ostringstream graphName;
  graphName<<pFolderSpec<<"_graph"<<".g";
  string graphStr = graphName.str();
  ofstream outputGraph(graphStr.c_str());
  // record graph
 
  buildLutGraph(pLuts, lutNum, out2Lut, id2FanoutIDs);
  for(int k = 0; k < lutNum; ++k){
    outputGraph<<k<<" ";
    for(int l = 0; l < pLuts[k].nFans; l++){
      outputGraph<<out2Lut[pLuts[k].pFans[l]]<<" ";
    }
    outputGraph<<endl;
  }
  outputGraph.close();

  vector<dEV* > id2dEV;
  assignId2EV(pLuts, id2dEV, lutNum, out2Lut, id2FanoutIDs);
  //set<string> existGroups;
  vector<vector<int>* > subCircuits;
  if(num < 0)// locate
    getSubCircuits(pLuts, lutNum, out2Lut, id2FanoutIDs, subCircuits);
  else{
    getSubCircuits2(pLuts, lutNum, out2Lut, id2FanoutIDs,
                    subCircuits, num);
  }
//  vector<EVGroup* > groups;
  //cerr<<"Start to write!"<<subCircuits.size()<<endl;
  for(int i = 0; i < subCircuits.size(); ++i){
    // generate groups
    ostringstream dataName;
    dataName<<pFolderSpec<<"_"<<i<<".data";
    string dataStr = dataName.str();
    ofstream outputData(dataStr.c_str());
    ostringstream lutName;
    lutName<<pFolderSpec<<"_"<<i<<".lut";
    string lutStr = lutName.str();
    ofstream outputLut(lutStr.c_str());

    vector<EVGroup* > groups;
    int groupSize = (subCircuits[i]->size())/_lutGroupNum;
    int remain = (subCircuits[i]->size())%_lutGroupNum;
    int idx = 0;
    for(int j = 0; j < _lutGroupNum; ++j){
      set<int> LUTs;
      int upper = idx+groupSize;
      if(j < remain)
        upper+=1;
      for(; idx < upper; ++idx){
        LUTs.insert((*(subCircuits[i]))[idx]);
        outputLut<<(*(subCircuits[i]))[idx]<<" ";
      }
      EVGroup* newGroup = new EVGroup(LUTs);
      groups.push_back(newGroup);
    } 
    writeGroups(groups, id2dEV, outputData, mode); // ideally we use mode 7 onlyi
    outputLut.close();
    outputData.close();
    for(int k = 0; k < groups.size(); ++k)
      delete groups[k];
    delete subCircuits[i];
  }
  return true;
}
bool sportCNN:: writeCNNData( char* pFileSpec , int mode){
  ofstream output(pFileSpec);
  return writeLUT(output, mode);
}
ABC_NAMESPACE_IMPL_END
