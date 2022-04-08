//
// Created by tanawin on 7/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>

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

struct BELE{
    RVAR  result;
    RVAR  index; // performance
    float perf;
    ui16  bc;
    ui16  lv;
};

int level;
int levelSize[LEVEL_AMT];
int levelStIdx[LEVEL_AMT];
int AMTBIT;

ui16 levelDpGain[LEVEL_AMT+1];
BELE a[ARR_SIZE];
BELE b[ARR_SIZE];
int  c[BIT_CT_ARR];

RVAR EXPECTED_RESULT;
ui16 CURRENT_BITCOUNT;
RVAR CURRENT_ANS;
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
        BELE* base = src + lv*ARR_PER_LEVEL;
        for (int  sz = 0; sz < levelSize[lv]; sz++){
            BELE* selected = base+sz;
            selected->index = ((RVAR)sz) << (lv * BIT_LEVEL_SIZE);
            selected->bc    = bitCount16(sz);
            ui16 gain       = bitCount64(selected->result);
            selected->perf  = ((float)gain) / (float)selected->index;
            selected->lv    = lv;
            maxGain = max(maxGain, gain);
        }
        levelDpGain[lv] = maxGain;
        sort(base, base+levelSize[lv], cleaner_cmp());
    }
    for (int lv = level-2; lv >= 0; lv--){
        levelDpGain[lv] += levelDpGain[lv+1];
    }
}
////////////////////////////// SM
void runner(BELE* base, BELE* src, int size){
    priority_queue<BELE, vector<BELE>, queue_cmp> q(base,  base+size);
    cout << q.size() << endl;
    while (!q.empty()){
        cout << q.size() << endl;
        //cout << q.size() << endl;
        BELE tmp = q.top();q.pop();
        RVAR  result = tmp.result;
        RVAR  index  = tmp.index; // performance
        float perf   = tmp.perf;
        ui16  bc     = tmp.bc;
        ui16  lv     = tmp.lv;

        if (result == EXPECTED_RESULT){ // stop these but not entire queue
            if (bc < CURRENT_BITCOUNT){
                CURRENT_ANS      = index;
                CURRENT_BITCOUNT = bc;
            }
        }else if ((lv == (level-1)) || ((bc+1) >= CURRENT_BITCOUNT) || ((((int)bitCount64(result)) + levelDpGain[lv+1]) < AMTBIT) ){
            continue;
        }else{
            lv++;
            for(int nxId = levelStIdx[lv];  nxId < (levelStIdx[lv] + levelSize[lv]); nxId++){
                BELE nxt{};
                nxt.result = result | src[nxId].result;
                nxt.index  = index | src[nxId].index;
                nxt.bc     = bc + src[nxId].bc;
                nxt.perf   = ((float)(bitCount64(nxt.result)))/(float)nxt.bc;
                nxt.lv     = lv;
                q.push(nxt);
            }
        }
    }
}
///////////////////////////// setup
void setup(int amtBit){
    AMTBIT = amtBit;
    level = 0;
    EXPECTED_RESULT = (amtBit == BIT_SIZE) ? -1 : ((-1 << amtBit) ^ -1);
    CURRENT_BITCOUNT=129;
    while (amtBit > 0){
        levelSize [level] = 1<<min(amtBit, BIT_LEVEL_SIZE);
        levelStIdx[level] = level*ARR_PER_LEVEL;
        amtBit-=BIT_LEVEL_SIZE;
        level++;
    }
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    pair<BELE*, BELE*> s = compose();
    cleanCompose(s.first);
    cout << "--------finish BD phase---------" << endl;
    runner(s.first, s.first, levelSize[0]);
    cout << CURRENT_BITCOUNT << endl;

}

void frontEnd(int amt){
    setup(amt);
}

void tetseteee(){
 
}

int main(){
    RVAR sixteenset = (1<<16)-1;
//    a[0     +1].result = sixteenset;
//    a[16*1*2+1].result = sixteenset << 16;
//    a[16*2*2+1].result = sixteenset << 32;
//    a[16*3*2+1].result = sixteenset << 48;
      a[0     +1].result = (-1);
      a[0     +1].result >>= 1;
      a[16*3*2+1].result = (-1);
    for (int i = 0; i < 64; i++){
        RVAR start = 1;
        a[i*2+1].result |= (start << i);
    }
    frontEnd(64);
}
