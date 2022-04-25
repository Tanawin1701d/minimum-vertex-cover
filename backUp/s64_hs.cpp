//
// Created by tanawin on 7/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>
#include <fstream>
#include <sstream>
#include <bitset>
#include <chrono>
#include "controller.h"

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
const int NCORE          = 8;
const int MAXNODE        = 100;

struct BELE{
    RVAR  result;
    RVAR  index ; // performance
    ui16  rc;
    ui16  bc;
    ui16  lv;
};

/// per level information
int AMTBIT;
int level;

/// huristic infomation
ui16  levelDpGain[LEVEL_AMT+1];
float levelDpPerf[LEVEL_AMT+1];
RVAR  levelDpRES [LEVEL_AMT+1];

/// raw data
BELE a[ARR_SIZE];
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
ui16 bitCount64(RVAR sz){
    ui16 ct = 0;
    for (int i = 0; i < 64/BIT_CT_PER; i++){
        ct += (ui16)c[sz & BIT_CT_MARKER];
        sz >>= BIT_CT_PER;
    }
    return ct;
}
/////////////////////////////////// BD

void cleanCompose(){
    for (int lv = 0; lv < level; lv++){
        BELE* selected          = a + lv;
              selected->result |= (((RVAR)1) << lv);
              selected->index   = (((RVAR)1) << lv);
              selected->bc      = 1;
              selected->rc      = bitCount64(selected->result);
              selected->lv      = lv;
        /////////////////// herustic build
              levelDpRES [lv]       = selected->result;
              levelDpPerf[lv]       = ((float) selected->rc) / ((float)selected->bc);
              levelDpGain[lv]       = selected->rc;
    }

    for (int lv = level-2; lv >= 0; lv--){ /// we ensure that there are at least 3 level
        levelDpRES [lv] |= levelDpRES [lv+1];
        levelDpGain[lv] += levelDpGain[lv+1];
        levelDpPerf[lv]  = max(levelDpPerf[lv], levelDpPerf[lv+1]);
    }
} ///check1
////////////////////////////// SM
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
};
void runner(int cId){
    priority_queue<BELE, vector<BELE>, queue_cmp> q;
    for (; cId < level; cId += NCORE){
        q.push(a[cId]);
    }
    //cout << q.size() << endl;
    while (!q.empty()){
        //cout << q.size() << endl;
        BELE  tmp    = q.top();q.pop();
        /////////////// debugger
        //cout << tmp.lv << "    " << tmp.bc << endl;
        ///////////////////////////////////////////
        if (tmp.result == EXPECTED_RESULT){
            /// critical section
            #pragma omp critical (upresult)
            {
                if (tmp.bc < CURRENT_BITCOUNT) {
                    CURRENT_ANS = tmp.index;
                    CURRENT_BITCOUNT = tmp.bc;
                }
            }
        }else if (    (  tmp.lv == (level-1) )
                    ||( (tmp.result | levelDpRES[tmp.lv + 1]) != EXPECTED_RESULT )
                    ||( ((float) tmp.bc + ((float) (AMTBIT - tmp.rc)) / (levelDpPerf[tmp.lv + 1])) > (float) CURRENT_BITCOUNT)
                    || ((tmp.rc + levelDpGain[tmp.lv + 1]) < AMTBIT)
                 ) {
                continue;
        }else{
            /// unused next level
            tmp.lv++;
            q.push(tmp);
            /// next level
            ui16 nextLv  = tmp.lv;
            tmp.result  |= a[nextLv].result;
            tmp.index   |= a[nextLv].index;
            tmp.bc      += 1;
            tmp.rc       = bitCount64(tmp.result);
            q.push(tmp);
        }
    }
}
////////////////////////////// RESULT check
void resultTester(){
    RVAR MOCKRESULT = 0;
    RVAR REIDX      = CURRENT_ANS;

    for (int i = 0; i < AMTBIT; i++){
        if (REIDX & 1){
            MOCKRESULT |= a[i].result;
        }
        REIDX >>= 1;
    }
        bitset<64> myOut( MOCKRESULT);
        bitset<64> sheOut(EXPECTED_RESULT);
        cout << "expect " << sheOut << endl;
        cout << "actual " << myOut  << endl;

}
///////////////////////////// setup
void setup(int amtBit){
    cout << "------- setup 64h variable -----------" << endl;
    AMTBIT          = amtBit;
    level           = 0;
    RVAR ob         = -1;
    EXPECTED_RESULT = (amtBit == BIT_SIZE) ? -1 : ((ob << amtBit) ^ ob);
    CURRENT_BITCOUNT= amtBit + 1;
    level           = AMTBIT;
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    cleanCompose();
    cout << "--------start running phase---------" << endl;
    auto start = chrono::steady_clock::now();
    #pragma omp parallel for default(none) shared(NCORE)
    for (int cId = 0; cId < NCORE; cId++) {
        runner(cId);
    }
    auto stop = chrono::steady_clock::now();
    cout << "--------start result verification phase---------" << endl;
    resultTester();
    cout << "Elapsed time in seconds: "
         << chrono::duration_cast<chrono::seconds>(stop - start).count()
         << " sec" << endl;

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

        n1            = atoi(buf1.c_str());
        n2            = atoi(buf2.c_str());
        a[n1].result |= (((RVAR)1) << n2);
        a[n2].result |= (((RVAR)1) << n1);
    }
    ///////////////////// set and run
    setup(amt);
    ///////////////////// result compose
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
    ifstream  src = ifstream("../input/ring-60-60");
    string bbbbbb;
    getline(src, bbbbbb);
    frontEnd64h(60, &src);
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
