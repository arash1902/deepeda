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
#include "ext/Sport/SportCNN.h"
#include "aig/gia/gia.h"

//soheil added includes:

//#include "base/io/ioAbc.h"
#include "map/mio/mioInt.h"


//

#ifndef _WIN32
#include <unistd.h>
#endif

#include<iostream>
using namespace std;
ABC_NAMESPACE_IMPL_START

static int Sport_CommandCNN(Abc_Frame_t* pAbc, int argc, char** argv);

void SPORT_Init(Abc_Frame_t * pAbc)
{
    Cmd_CommandAdd(pAbc, "ZZsport", "sportCNN", Sport_CommandCNN, 0);
}

extern void sportUsageCNN();


int Sport_CommandCNN(Abc_Frame_t* pAbc, int argc, char** argv){

  /* printf("**Soheil Codes Started...\n\n"); */

  Abc_Ntk_t * currNtk = pAbc->pNtkCur;
  
/*   int ntkSize = currNtk->vObjs->nSize;
 
 
  printf("network size: %d\n", ntkSize); */

  /* Abc_Obj_t * currObj = (Abc_Obj_t*)(currNtk->vObjs->pArray[40]); */
/*   int id = currObj->Id;
  printf("id=%d\n", id);
  printf("fanins size=%d\n", currObj->vFanins.nSize);
  printf("fanouts size=%d\n", currObj->vFanouts.nSize);

  Mio_Gate_t * mappedGate = (Mio_Gate_t *) currObj->pData;
   
  char * mappedGateName = mappedGate->pName;
  char * mappedGateFunc = mappedGate->pForm;
  int mappedGateId = mappedGate->Cell; */
  

/*   printf("mapped to gate name: %s\n", mappedGateName);
  printf("mapped to gate function: %s\n", mappedGateFunc);
  printf("mapped to gate id: %d\n", mappedGateId); */

  int i;

//  Abc_NtkForEachObj(currNtk, currObj, i) { 
//	if(Abc_ObjIsNode(currObj)) {
/* 
	Abc_NtkForEachNode(currNtk, currObj, i) {
	 	int objId = currObj->Id;
		int fanInsN = currObj->vFanins.nSize;
		int fanOutsN = currObj->vFanins.nSize;
		Mio_Gate_t * cell = (Mio_Gate_t *) currObj->pData;
		char * cellName = cell->pName;
		int  cellId = cell->Cell;
	  
		printf("objId:%d fanIns:%d fanOuts:%d cellName:%s cellId:%d\n", objId, fanInsN, fanOutsN, cellName, cellId); 	 
	
  	} */

/*    Abc_NtkForEachPi(currNtk, currObj, i) {
	printf("PI id=%d\n", currObj->Id);
	printf("FanOuts=%d\n", currObj->vFanouts.nSize);
   }

   Abc_NtkForEachPo(currNtk, currObj, i) {
	printf("Po id=%d\n", currObj->Id);
	printf("FanIns=%d\n", currObj->vFanins.nSize);
	printf("FanInId=%d\n", currObj->vFanins.pArray[0]);
	assert( currObj->vFanins.nSize == 1); // Soheil assert every PO has only 1 fanIn
   } */


/*
  for(i=1; i<ntkSize; i++) {	
        currObj = (Abc_Obj_t*)(currNtk->vObjs->pArray[i]);
	if(Abc_ObjIsNode(currObj)) {
	 	int objId = currObj->Id;
		int fanInsN = currObj->vFanins.nSize;
		int fanOutsN = currObj->vFanins.nSize;
		Mio_Gate_t * cell = (Mio_Gate_t *) currObj->pData;
		char * cellName = cell->pName;
		int  cellId = cell->Cell;
	  
		printf("objId:%d fanIns:%d fanOuts:%d cellName:%s cellId:%d\n", objId, fanInsN, fanOutsN, cellName, cellId);  		 
	}
  }
*/

//  printf("ntkID: %d\n", currObj->Id);

//  Abc_Obj_t* currObj = Abc_NtkObj( currNtk, 0);
//  Abc_NtkObj( currNtk, 0);
//Vec_PtrEntry( pNtk->vObjs, i )

  
/*  Abc_Obj_t * pObj;
  int i;

  Abc_NtkForEachObj( currNtk, pObj, i)
	printf("Object ID:%d\n", pObj->pCopy->Id);
*/
/*   printf("**Soheil Codes End\n\n");
   */


   char * pFileSpec = NULL;
   char * pFolderSpec = NULL;
   //Gia_Man_t* pCircuit;
   Extra_UtilGetoptReset();
   int c = 0;
   int mode = 0;
   int simple = 0;
   int partitionNum = -1;
   sportCNN* sportAPI = NULL;
   while(( c = Extra_UtilGetopt( argc, argv, "hFMSPn" ) ) != EOF ) {
      switch(c){
         case 'F':
            if ( globalUtilOptind >= argc ){
               Abc_Print( -1, "Command line switch \"-F\" should be followed by a file name.\n" );
               goto usage;
            }
            pFileSpec = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
         case 'P':
            if ( globalUtilOptind >= argc ){
               Abc_Print( -1, "Command line switch \"-F\" should be followed by a file name.\n" );
               goto usage;
            }
            pFolderSpec = argv[globalUtilOptind];
            globalUtilOptind++;
            break;
 
         case 'S':
            simple = 1;
            break;
         case 'n':
            if ( globalUtilOptind >= argc ){
               Abc_Print( -1, "Command line switch \"-M\" should be followed by a nature number.\n" );
               goto usage;
            }
            partitionNum = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            break;
         case 'M':
            if ( globalUtilOptind >= argc ){
               Abc_Print( -1, "Command line switch \"-M\" should be followed by a nature number.\n" );
               goto usage;
            }
            mode = atoi(argv[globalUtilOptind]);
            
            globalUtilOptind++;
            break;
   
         case 'h':
            goto usage; 
            break; 
         default:
            goto usage;
      }
   }    
/*
  if ( pAbc->pGia == NULL ){
     Abc_Print( -1, "Sport_CommandCNN(): There is no GIA.\n" );
     return 1;
  }
*/

   if(mode < 1 || mode > 5){
      Abc_Print( -1, "Sport_CommandCNN(): mode should be 1-5\n" );
      return 1;

   }
// Temporaty Commend by Soheil
//  if ( !Gia_ManHasMapping(pAbc->pGia) ){
//    Abc_Print( -1, "Abc_CommandCNN(): AIG has no mapping.\n" );
//    return 1;
//  }
 
  
   sportAPI = new sportCNN(currNtk);

   if(pFileSpec){
      if (!sportAPI->writeCNNData( pFileSpec, mode))
         Abc_Print(-1, "writeCNNData fails!\n");
   }   
   if(pFolderSpec){
      if(!sportAPI->writeCNNPartition(pFolderSpec, mode,partitionNum))
         Abc_Print(-1, "writeCNNPartition fails!\n");
   }
   delete sportAPI;
   return 0;
   usage:
      sportUsageCNN();
      return 1;
}


ABC_NAMESPACE_IMPL_END
