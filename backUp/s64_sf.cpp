//
// Created by tanawin on 8/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>
#include <cstring>

using namespace std;

typedef unsigned long long int RVAR;
typedef uint16_t               ui16;
const int BIT_LEVEL_SIZE = 16;
const int BIT_SIZE       = 64;
const int LEVEL_AMT      = BIT_SIZE/ BIT_LEVEL_SIZE;
const int ARR_PER_LEVEL  = (1 << BIT_LEVEL_SIZE);
const int ARR_SIZE       = ARR_PER_LEVEL * LEVEL_AMT;
const int ST_SIZE        = LEVEL_AMT;
const int BIT_CT_ARR     = 256; // 8 bit
const int BIT_CT_PER     = 8; // 8 bit
const int BIT_CT_MARKER  = (1<<BIT_CT_PER)-1;
const int BITAGM         = 2; // bit algnment

struct BELE{
    RVAR  result;
    RVAR  index; // performance
    float perf;
    ui16  bc;
    ui16  lv;
};
/////////////////////////////////// main variable
int level;
int levelSize[LEVEL_AMT];
int levelStIdx[LEVEL_AMT];
int AMTBIT;

ui16  levelDpGain[LEVEL_AMT+1];
float levelDpPerf[LEVEL_AMT+1];
BELE a[ARR_SIZE];
BELE b[ARR_SIZE];

int  c[BIT_CT_ARR];

RVAR EXPECTED_RESULT;
ui16 CURRENT_BITCOUNT ;
RVAR CURRENT_ANS;
///////////////////////////////////extra variable enxtened from
RVAR cp[ARR_SIZE];
///////////////////////////////////bitcounter
void buildCounter(){
    for (int i = 0; i < BIT_CT_ARR; i++){
        int j = i;
        int ct = 0;
        while (j){
            ct += j&1;
            j>>=1;
        }
        c[i] = ct;
    }
}
ui16 bitCount16(int sz){
    return c[sz & BIT_CT_MARKER] + c[(sz>>BIT_CT_PER) & BIT_CT_MARKER];
}
ui16 bitCount64(RVAR sz){
    ui16 ct = 0;
    for (int i = 0; i < 64/BIT_CT_PER; i++){
        ct += (ui16)c[sz & BIT_CT_MARKER];
        sz >>= BIT_CT_PER;
    }
    return ct;
}
/////////////////////////////////// BD
pair<BELE*, BELE*> compose(){
    int bs = 1;
    int as = 2;
    BELE* src = a;
    BELE* des = b;
    for (;bs < BIT_LEVEL_SIZE; bs <<= 1){
        //for all
        int iterSize = BIT_SIZE / bs;
        for (int iter = 0; iter < iterSize; iter+=2) {
            int bid  = iter*as; // baseId
            int fbid = (iter >> 1) * as * as;
            for (int msb = 0; msb < as; msb++) {
                for (int lsb = 0; lsb < as; lsb++) {
                    int fidx = fbid + ((msb << bs) | lsb);
                    des[fidx].result = src[bid + lsb].result | src[bid + as + msb].result;
                }
            }
        }
        swap(src, des);
        as *=  as;
    }
    return {src, des};
}

struct queue_cmp
{
    inline bool operator() (const BELE& struct1, const BELE& struct2)
    {
        return (struct1.perf < struct2.perf) ||
               ((struct1.perf == struct2.perf) && (struct1.bc < struct2.bc));
    }
};
struct cleaner_cmp
{
    inline bool operator() (const BELE& struct1, const BELE& struct2)
    {
        return (struct1.perf > struct2.perf) ||
               ((struct1.perf == struct2.perf) && (struct1.bc > struct2.bc));
    }
};

void cleanCompose(BELE* src){
    for (int lv = 0; lv < level; lv++){
        ui16 maxGain = 0;
        float maxPerf = 0;
        BELE* base = src + lv*ARR_PER_LEVEL;
        for (int  sz = 0; sz < levelSize[lv]; sz++){
            BELE* selected = base+sz;
            selected->index = ((RVAR)sz) << (lv * BIT_LEVEL_SIZE);
            selected->bc    = bitCount16(sz);
            ui16 gain       = bitCount64(selected->result);
            selected->perf  = ((float)gain) / (float)selected->bc;
            selected->lv    = lv;
            /////////////////////////////
            maxGain = max(maxGain, gain);
            maxPerf = max(maxPerf, selected->perf);
        }
        levelDpPerf[lv] = maxPerf;
        levelDpGain[lv] = maxGain;
        sort(base, base+levelSize[lv], cleaner_cmp());
    }
    for (int lv = level-2; lv >= 0; lv--){
        levelDpGain[lv] += levelDpGain[lv+1];
        levelDpPerf[lv]  = max(levelDpPerf[lv], levelDpPerf[lv+1]);
    }
}
/////////////////////////////////// extra function
void mapper(BELE* src){
    for (int lv = 0; lv < level; lv++){
        for (int idx = levelStIdx[lv]; idx < (levelStIdx[lv]+levelSize[lv]); idx++){
            cp[idx] = src[idx].result;
        }
    }
}

void updateAns(const BELE& beleB){
    if (beleB.bc < CURRENT_BITCOUNT){
        CURRENT_BITCOUNT = beleB.bc;
        CURRENT_ANS      = beleB.index;
    }
}

BELE getBELE(int idx, int lv, BELE* beleB){ // 0 -> 3
    return  beleB[levelStIdx[lv] + idx];
}

void runner(BELE* beleB, int startIter, int size){
    ///////// dfs variable
    BELE stBele[ST_SIZE];
    int  stIter  [ST_SIZE];
    int  localLevelSize[LEVEL_AMT];
    ///////// initialize variable
    copy(levelSize, levelSize+level,localLevelSize);
    localLevelSize[0] = startIter + size;
    stIter[0] = startIter;
    memset(stIter, 0, sizeof(stIter));

    int lv = 0;
    while (lv >= 0){
        if (lv == (level-1)){
            // todo use avx instruction
            for (int lastIter = 0; lastIter < localLevelSize[lv]; lastIter++){
                BELE preCheck{};
                BELE curBELE = getBELE(lastIter, lv, beleB);
                preCheck.result = stBele[lv-1].result | curBELE.result;
                preCheck.index  = stBele[lv-1].index  | curBELE.index;
                preCheck.bc     = stBele[lv-1].bc     + curBELE.bc;
                if (preCheck.result == EXPECTED_RESULT) {
                    updateAns(preCheck);
                }
            }
            lv--;continue;
        }else if (stIter[lv] == localLevelSize[lv]){
            stIter[lv] = 0;
            lv--;continue;
        }
        //////////////////////////// huristic analyzer and condition test and next state preparation
        BELE  curBELE           = getBELE(stIter[lv], lv, beleB);
              stBele[lv].result = (lv ? stBele[lv-1].result : (RVAR)0) | curBELE.result;
              stBele[lv].index  = (lv ? stBele[lv-1].index  : (RVAR)0) | curBELE.index;
              stBele[lv].bc     = (lv ? stBele[lv-1].bc     : (ui16)0) + curBELE.bc;
        int   resultC           = bitCount64(stBele[lv].result);
        bool  bcBound           = ( (float)curBELE.bc + (float)(AMTBIT-resultC) / (levelDpPerf[lv+1]+1))    > ((float)CURRENT_BITCOUNT);
        bool  reBound           = (resultC + levelDpGain[lv+1]) < AMTBIT;
              stIter[lv]++;
        /////////////////////////// next state
        if (stBele[lv].result == EXPECTED_RESULT){
            updateAns(curBELE);
        }else if ( bcBound || reBound){ // huristic
            cout << "bound" <<endl;
        }else {
            lv = lv + 1;
        }
    }
}

////////////////////////////// SM

///////////////////////////// setup
void setup(int amtBit){
    AMTBIT = amtBit;
    level = 0;
    RVAR ob = -1;
    EXPECTED_RESULT  = (amtBit == BIT_SIZE) ? -1 : ((ob << amtBit) ^ ob);
    CURRENT_BITCOUNT = amtBit + 1;
    while (amtBit > 0){
        levelSize [level] = 1<<min(max(amtBit, BITAGM), BIT_LEVEL_SIZE);
        levelStIdx[level] = level*ARR_PER_LEVEL;
        amtBit-=BIT_LEVEL_SIZE;
        level++;
    }
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    pair<BELE*, BELE*> s = compose();
    cleanCompose(s.first);
    cout << "--------start SM phase---------" << endl;
    runner(s.first, 0, levelSize[0]);
    cout << "--------stop SM phase----------" << endl;
    cout << CURRENT_BITCOUNT <<endl;

}

void frontEnd(int amt){
    setup(amt);
}

int main(){
    RVAR sixteenset = (1<<16)-1;
//    a[0     +1].result = sixteenset;
//    a[16*1*2+1].result = sixteenset << 16;
//    a[16*2*2+1].result = sixteenset << 32;
//    a[16*3*2+1].result = sixteenset << 48;
      a[0     +1].result = (RVAR)1 << 47;
      a[0     +1].result -=1;
      a[16*3*2+1].result = (RVAR)1 << 48;
    a[16*3*2+1].result   -= 1;
    for (int i = 0; i < 48; i++){
        RVAR start = 1;
        a[i*2+1].result |= (start << i);
    }
    frontEnd(48);
}
