//
// Created by tanawin on 7/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>
#include <fstream>
#include <sstream>
#include <bitset>
#include "../controller.h"

using namespace std;

typedef unsigned long long int RVAR;
typedef uint16_t               ui16;
const int BIT_LEVEL_SIZE = 1;
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
    RVAR  index ; // performance
    float perf  ;
    ui16  rc;
    ui16  bc;
    ui16  lv;
};

/// per level information
int AMTBIT;
int level;
int levelSize [LEVEL_AMT];
int levelStIdx[LEVEL_AMT];
/// huristic infomation
ui16  levelDpGain[LEVEL_AMT+1];
float levelDpPerf[LEVEL_AMT+1];
RVAR  levelDpRES [LEVEL_AMT+1];

/// raw data
BELE a[ARR_SIZE];
BELE b[ARR_SIZE];
int  c[BIT_CT_ARR];
/// result var
RVAR EXPECTED_RESULT;
RVAR CURRENT_ANS;
ui16 CURRENT_BITCOUNT;
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
} /// check1
ui16 bitCount16(int sz){
    return c[sz & BIT_CT_MARKER] + c[(sz>>BIT_CT_PER) & BIT_CT_MARKER];
} /// check1
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
} ///check1

struct queue_cmp
{
    inline bool operator() (const BELE& struct1, const BELE& struct2)
    {
//        return (struct1.perf < struct2.perf) ||
//              ((struct1.perf == struct2.perf) && (struct1.bc < struct2.bc));

        if (struct1.rc < struct2.rc){
            return true;
        }else if (struct1.rc == struct2.rc){
            if (struct1.bc > struct2.bc){
                return true;
            }else if(struct1.bc == struct2.bc){
                return struct1.lv > struct2.lv;
            }
        }
        return false;
//        float p1 = struct1.perf / 64 * 0.9 + (struct1.lv/3) * (0.1);
//        float p2 = struct2.perf / 64 * 0.9 + (struct2.lv/3) * (0.1);

        //return  bitCount64(struct1.result)< bitCount64(struct2.result);
    }
}; ///check1
struct cleaner_cmp
{
    inline bool operator() (const BELE& struct1, const BELE& struct2)
    {
        return (struct1.perf > struct2.perf) ||
               ((struct1.perf == struct2.perf) && (struct1.bc > struct2.bc));
    }
}; ///check1

void cleanCompose(BELE* src){
    for (int lv = 0; lv < level; lv++){
        ui16  maxGain = 0;
        float maxPerf = 0;
        RVAR  maxRES  = 0;
        BELE* base = src + lv*ARR_PER_LEVEL;
        for (int  sz = 0; sz < levelSize[lv]; sz++){
            BELE* selected = base+sz;
            selected->index = ((RVAR)sz) << (lv * BIT_LEVEL_SIZE);
            selected->bc    = bitCount16(sz);
            selected->rc    = bitCount64(selected->result);
            selected->perf  = ((float)selected->rc) / (float)(selected->bc ? selected->bc : 1);
            selected->lv    = lv;
            /////////////////////////////
            maxGain = max(maxGain, selected->rc);
            maxPerf = max(maxPerf, selected->perf);
            maxRES  |= selected->result;
        }
        levelDpRES [lv] = maxRES;
        levelDpPerf[lv] = maxPerf;
        levelDpGain[lv] = maxGain;
        sort(base, base+levelSize[lv], cleaner_cmp());
    }
    for (int lv = level-2; lv >= 0; lv--){ /// we ensure that there are at least 3 level
        levelDpRES [lv] |= levelDpRES [lv+1];
        levelDpGain[lv] += levelDpGain[lv+1];
        levelDpPerf[lv]  = max(levelDpPerf[lv], levelDpPerf[lv+1]);
    }
    //cout << "test" << endl;
} ///check1
////////////////////////////// SM
void runner(BELE* base, BELE* src, int size){
    priority_queue<BELE, vector<BELE>, queue_cmp> q(base,  base+size);
    //cout << q.size() << endl;
    while (!q.empty()){
        //cout << q.size() << endl;
        //cout << q.size() << endl;
        BELE  tmp    = q.top();q.pop();
        RVAR  result = tmp.result;
        RVAR  index  = tmp.index; // performance
        //cout << index<<endl;
        float perf   = tmp.perf;
        ui16  rc     = tmp.rc;
        ui16  bc     = tmp.bc;
        ui16  lv     = tmp.lv;
        /////////////////////
        cout << lv << " " << bc << endl;
        /////////////////////

        if (result == EXPECTED_RESULT){ // stop these but not entire queue
            if (bc < CURRENT_BITCOUNT){
                CURRENT_ANS      = index;
                CURRENT_BITCOUNT = bc;
            }
        }else if (     (lv == (level-1))
                    || ((result | levelDpRES[lv+1]) != EXPECTED_RESULT)
                    || ( ((float)bc + ((float)(AMTBIT-rc))/(levelDpPerf[lv+1])) > (float)CURRENT_BITCOUNT)
                    || ((rc + levelDpGain[lv+1]) < AMTBIT)
                 ){
            continue;
        }else{
            lv++;
            for(int nxId = levelStIdx[lv];  nxId < (levelStIdx[lv] + levelSize[lv]); nxId++){
                BELE nxt{};

                nxt.result = result | src[nxId].result;
                /*if (result > ((RVAR)1 << 61)){
                    cout << "invalid";
                }*/
                nxt.index  = index | src[nxId].index;
                nxt.bc     = bc + src[nxId].bc;
                nxt.rc     = bitCount64(nxt.result);
                nxt.perf   = ((float)(nxt.rc))/(float)nxt.bc;
                nxt.lv     = lv;
                q.push(nxt);
            }
        }
    }
} ///check1
///////////////////////////// setup
void setup(int amtBit){
    cout << "------- setup 64h variable -----------" << endl;
    AMTBIT          = amtBit;
    level           = 0;
    RVAR ob         = -1;
    EXPECTED_RESULT = (amtBit == BIT_SIZE) ? -1 : ((ob << amtBit) ^ ob);
    CURRENT_BITCOUNT = amtBit + 1;
    while (amtBit > 0){
        levelSize [level] = 1<<min(amtBit, BIT_LEVEL_SIZE);
        levelStIdx[level] = level*ARR_PER_LEVEL;
        amtBit-=BIT_LEVEL_SIZE;
        level++;
    }
    for (int init = 0; init < AMTBIT; init++) {
        a[(init << 1) + 1].result |= (((RVAR)1) << init);
    }
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    pair<BELE*, BELE*> s = compose();
    cleanCompose(s.first);
    cout << "--------finish BD phase---------" << endl;
    runner(s.first, s.first, levelSize[0]);
    cout << CURRENT_BITCOUNT << endl;
    bitset<64> myOut(CURRENT_ANS);
    cout << myOut << endl;
}

string frontEnd64h(int amt, ifstream* specFile){
    string buf1;
    string buf2;
    int    n1;
    int    n2;
    getline(*specFile, buf1);
    int edgeAmt = atoi(buf1.c_str());
    for(int e = 0; e < edgeAmt; e++){
        getline(*specFile, buf1);
        stringstream ss(buf1);
        ss >> buf2 >> buf1;
        n1 = atoi(buf1.c_str());
        n2 = atoi(buf2.c_str());
        if (n1 == 18 || n2 == 18){
            cout << "false";
        }
        a[(n1 << 1) + 1].result |= (((RVAR)1) << n2);
        a[(n2 << 1) + 1].result |= (((RVAR)1) << n1);
        if (a[37].result > (1 << 60)){
            cout << "false";
        }
    }
    setup(amt);
    string re;
    re += to_string(CURRENT_BITCOUNT);
    re += ":";
    for (int rec = 0; rec < AMTBIT; rec++){
        re += (CURRENT_ANS & 1) ? "1" : "0";
        CURRENT_ANS >>= 1;
    }
    return re;
}

int main(){
    ifstream  src = ifstream("../input/grid-49-84");
    string bbbbbb;
    getline(src, bbbbbb);
    frontEnd64h(49, &src);
    return 0;
}

//int main(){
//    a[0     +1].result = (RVAR)1 << 47;
//    a[0     +1].result -=1;
//    a[47*2+1].result = (RVAR)1 << 48;
//    a[47*2+1].result   -= 1;
//    for (int i = 0; i < 48; i++){
//        RVAR start = 1;
//        a[i*2+1].result |= (start << i);
//    }
//    frontEnd(48);
//}
