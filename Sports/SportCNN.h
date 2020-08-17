#ifndef SPORT_CNN_H
#define SPORT_CNN_H
#include<iostream>
#include<fstream>
#include <sstream>
#include<cstring>
#include<set>
#include<vector>
#include<map>
#include<queue>
#include <stdlib.h>
#include <bitset> 
#include <time.h>  
#include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "misc/extra/extra.h"
#include <algorithm> 
using namespace std;
#define SPORT_MAX_LUT_SIZE 6

class dEVBag{
  public:
    dEVBag(int ones, int lutID){
      _ones = ones;
      _count = 1;
      _lut = lutID;
    }
    void addCount(){
      _count+=1;
    }
    int getCount(){
      return _count;
    }
    int getLut(){
      return _lut;
    }
    int getOnes(){
      return _ones;
    }

  private:
    int _lut;
    int _count;
    int _ones;
};

static bool sortDouble(dEVBag* bagFirst, dEVBag* bagSecond ){
  int countDiff = bagFirst->getCount() - bagSecond->getCount();
  if(countDiff > 0)
    return true; 
  if(countDiff < 0 )
    return false;
  if(bagFirst->getOnes() > bagSecond->getOnes())
    return true;
  return false; 
}

class EVGroup{
  public:
    EVGroup( set<int>& initial){
      _LUTs.insert(initial.begin(), initial.end());
    }
    EVGroup(int first){
      _LUTs.insert(first);
    }

    void split(set<EVGroup*>& runningGroups){
      set<int> new1;
      set<int> new2;
      set<int>::iterator site = _LUTs.begin();
      //Use this loop to split the first half of the LUT to the new1 and put the second half to the new2
      for(; site != _LUTs.end(); ++site){
        if(new1.size() >= _LUTs.size()/2)
          new2.insert((*site));
        else
          new1.insert((*site));
      }
      EVGroup* newG1 = new EVGroup(new1);
      EVGroup* newG2 = new EVGroup(new2);
      runningGroups.insert(newG1);
      runningGroups.insert(newG2);
    }
    //What the user use for 
    int order(){
      int size = _LUTs.size();
      double ave = 0.0;
      set<int>::iterator site = _LUTs.begin();
      for(; site!= _LUTs.end(); ++site)
        ave += (double)(*site)/size;

     
        return (int)(ave);
    }
    int groupSize(){
      return _LUTs.size();
    }
    // After the Luts what the new LUTS will be
    void addMembers(set<int>& newLUTs){
      _LUTs.insert(newLUTs.begin(), newLUTs.end());
    }
    
    set<int>& getLUTs(){
      return _LUTs;
    }

  private:
    set<int> _LUTs;
};

class dEV{
  public:
    dEV(set<int>& fanins, set<int>& fanouts){
      _fanins = fanins;
      _fanouts = fanouts;
    }
    string getDoubleKey(){
      ostringstream convert;
      set<int>::iterator site = _fanins.begin();
      for(; site!= _fanins.end(); ++site)
        convert<<(*site)<<"&";
      convert<<"||";
      
      site = _fanouts.begin();
      for(; site!= _fanouts.end(); ++site)
        convert<<(*site)<<"&";
      return convert.str();
    }

    string getSingleKey(){
      set<int> total;
      total.insert(_fanins.begin(), _fanins.end());
      total.insert(_fanouts.begin(), _fanouts.end());
      ostringstream convert;
      set<int>::iterator site = total.begin();
      for(; site!= total.end(); ++site)
        convert<<(*site)<<"&";
      return convert.str();
    }
    int getSingleCount(){
      set<int> total;
      total.insert(_fanins.begin(), _fanins.end());
      total.insert(_fanouts.begin(), _fanouts.end());
      return total.size();
    }
    int getDoubleCount(){
      return _fanins.size()+_fanouts.size();
    }
    //how do you get the number 28
  void outputSingle(ofstream& output){
    int out[28];
    for(int i = 0; i < 28; ++i)
      out[i] = 0;

    set<int>::iterator site = _fanins.begin();
    for(; site!= _fanins.end(); ++site)
    //why it is equal to one
      out[(*site)] = 1;

    site = _fanouts.begin();
    for(; site!= _fanouts.end(); ++site)
      out[(*site)] = 1;

    for(int i = 0; i < 28; ++i)
      output<<out[i];

    output<<endl;
  } 
  void outputDouble(ofstream& output){
    int out[56];
    for(int i = 0; i < 56; ++i)
      out[i] = 0; 
    
    set<int>::iterator site = _fanins.begin();
    for(; site!= _fanins.end(); ++site)
      out[(*site)] = 1;

    site = _fanouts.begin();
    for(; site!= _fanouts.end(); ++site)
      out[(*site)+28] = 1;

    for(int i = 0; i < 56; ++i)
      output<<out[i];

    output<<endl;

  }
  void outputBag(ofstream& output, int mode){
    if(mode%2 == 1) // single
      outputSingle(output);
    else // double
      outputDouble(output);
  } 
  private:
    set<int> _fanins;
    set<int> _fanouts;
};
struct Gia_MapLut_Sport
{   
    int        Type;          // node type: PI=1, PO=2, LUT=3^M
    int        Out;           // ID^M
    int        StartId;       // -1^M
    int        nFans;         // fanin count^M
    float      Delay;         // 0.0^M
    int        pFans[SPORT_MAX_LUT_SIZE];  // fanin IDs^M
    unsigned   pTruth[SPORT_MAX_LUT_SIZE<6?1:(1<<(SPORT_MAX_LUT_SIZE-5))]; // the truth table^M
    int cellId;// Soheil Added: standard cell id which current gate is mapped to that cell
};

class sportCNN{ // this class is for print bad guy
  public:
    sportCNN(Abc_Ntk_t* currNtk);
    bool writeCNNData(char* pFileSpec, int mode);
    bool writeCNNPartition(char* pFolderSpec, int mode, int num);
    void getSubCircuits(Gia_MapLut_Sport* pLuts,
                        int lutNum, map<int, int>& out2Lut,
                        map<int, set<int>* >& id2FanoutIDs,
                        vector<vector<int>* >& subCircuits);
    void getSubCircuits2(Gia_MapLut_Sport* pLuts,
                        int lutNum, map<int, int>& out2Lut,
                        map<int, set<int>* >& id2FanoutIDs,
                        vector<vector<int>* >& subCircuits,
                        int num);
    void getOneWindow(Gia_MapLut_Sport* pLuts, int targetLut,
                      int targetSize, map<int, int>& out2Lut,
                      map<int, set<int>* >& id2FanoutIDs,
                      set<int>* members);
    bool writeLUT(ofstream& output, int mode);
    
    void buildLutGraph(Gia_MapLut_Sport* pLuts,
                       int lutNum, map<int, int>& out2Lut, map<int, set<int>* >& id2FanoutIDs); 
                       
    void assignId2EV( Gia_MapLut_Sport* pLuts, vector<dEV* >& id2dEV, int lutNum, map<int, int>& out2Lut, map<int, set<int>* >& id2FanoutIDs);
    void evenlyGrouping(int lutNum, vector<EVGroup* >& groups);
    void BFSGrouping( Gia_MapLut_Sport* pLuts, int lutNum, map<int, int>& out2Lut, map<int, set<int>* >& id2FanoutIDs, vector<EVGroup*>& groups);
    void writeGroups(vector<EVGroup*>& groups, vector<dEV* >& id2dEV, ofstream& output, int mode);
    void writeAlan( Gia_MapLut_Sport* pLuts, int lutNum, map<int, int>& out2Lut,
ofstream& output);
    
    Gia_MapLut_Sport* getMapLut(int& lutNum);
  private:
    Abc_Ntk_t* _currNtk; // current network
    Gia_Man_t * _cir;
    int _aigNum;
    int _lutNum;
    int _lutGroupNum;
    int _truthNum;
};

#endif
