
#include "base/main/main.h"
#include "base/main/mainInt.h"

//extern void Meow_Init(Abc_Frame_t * pAbc);
extern void SPORT_Init(Abc_Frame_t* pAbc);

#include <iostream>

using namespace std;

static void init(Abc_Frame_t* pAbc)
{
    //cout<<"test!\n";
    //cout << "Init()" << endl;
    SPORT_Init(pAbc);
}

static void destroy(Abc_Frame_t* pAbc)
{
    // cout << "Destroy()" << endl;
}

static Abc_FrameInitializer_t abcFrameInit = { init, destroy };

struct AbcInitializer {
    AbcInitializer() {
        Abc_FrameAddInitializer(&abcFrameInit);
    }
    ~AbcInitializer() {}
};

static AbcInitializer abcInit;

