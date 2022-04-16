//
// Created by tanawin on 7/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>
#include <stack>
#include <fstream>
#include <sstream>
#include <bitset>
#include <chrono>
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
///

/// result var
RVAR EXPECTED_RESULT;
RVAR CURRENT_ANS;
ui16 CURRENT_BITCOUNT;
bool        resultBuf[MAXNODE];
/// renamer variable
bool        isRenamed[MAXNODE]; // use index of real name
vector<int> renameBuf[MAXNODE]; // use index and value of real name
int         renamer  [MAXNODE]; // index real name // value is converted name
int         derenamer[MAXNODE]; // index converted name // value is real name

///////////////////////////////////bitcounter
void   buildCounter(){
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
ui16   bitCount64  (RVAR sz){
    ui16 ct = 0;
    for (int i = 0; i < 64/BIT_CT_PER; i++){
        ct += ((ui16)c[sz & BIT_CT_MARKER]);
        sz >>= BIT_CT_PER;
    }
    return ct;
}
///////////////////////////////////renamer and initialize BELE
bool rmCmp(const int& struct1, const int& struct2){
        return renameBuf[struct1].size() < renameBuf[struct2].size();
}
void   graphRenameAndAssign(){
    queue<int> q;
    int newKey = 0;
    int conectedDetector = 0;
    vector<int> initQ;
    for (int nd = 0; nd < level; nd++){
        initQ.push_back(nd);
    }
    sort(initQ.begin(), initQ.end(), rmCmp);

    for (int nd: initQ){
        if (!isRenamed[nd]) {
            q.push(nd);
            conectedDetector++;
        }
        while (!q.empty()){
            int myRealNode = q.front(); q.pop();
            if (!isRenamed[myRealNode]){
                isRenamed[ myRealNode ] = true;
                renamer  [ myRealNode ] = newKey; // outgoing
                derenamer[ newKey     ] = myRealNode; // backward for composed result
                ////// update new key
                newKey++;
                vector<int> preQ;
                for (int e : renameBuf[myRealNode]){
                    preQ.push_back(e);
                }
                sort(preQ.begin(), preQ.end(), rmCmp);
                for (int e : preQ) {
                    q.push(e);
                }
            }
        }
    }
    cout << ">> system is connected : " << (conectedDetector == 1) << endl;

    for (int srcNd = 0; srcNd < level; srcNd++){
        for (int nxtNd : renameBuf[srcNd]){
            int srcRenamed = renamer[srcNd];
            int desRenamed = renamer[nxtNd];
            a[srcRenamed].result |= (((RVAR)1) << desRenamed);
        }
    }

}
void   cleanCompose(){
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
}
///////////////////////////////////SM
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

priority_queue<BELE, vector<BELE>, queue_cmp> pq;

void   runner      (){
    while (true){
        //cout << q.size() << endl;
        bool stoper = false;
        BELE tmp;
        #pragma omp critical (pq_re)
        {
            if (pq.empty()){
                stoper = true;
            }else {
                tmp = pq.top();
                pq.pop();
            }
        }
        if (stoper){
            break;
        }
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
                    ||( ((double ) tmp.bc + ((double) (AMTBIT - ((int)(tmp.rc)))) / (levelDpPerf[tmp.lv + 1])) > ((double) CURRENT_BITCOUNT))
                    || ((tmp.rc + levelDpGain[tmp.lv + 1]) < AMTBIT)
                 ) {
                continue;
        }else{
            /// unused next level
            #pragma omp critical (pq_re)
            {
                tmp.lv++;
                pq.push(tmp);
                /// next level
                ui16 nextLv = tmp.lv;
                tmp.result |= a[nextLv].result;
                tmp.index |= a[nextLv].index;
                tmp.bc += 1;
                tmp.rc = bitCount64(tmp.result);
                pq.push(tmp);
            }
        }
    }
}
///////////////////////////////////RESULT
void   resultTester(){
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
        cout << "expect                      " << sheOut << endl;
        cout << "actual                      " << myOut  << endl;

}
string graphDeRename(){
    RVAR BUFANS = CURRENT_ANS;
    for (int virNd = 0; virNd < level; virNd++){
        if (BUFANS & 1){
            resultBuf[ derenamer[virNd] ] = true;
        }
        BUFANS >>= 1;
    }

    string preRet = to_string(CURRENT_BITCOUNT) + ":";

    for (int realNd = 0; realNd < level; realNd++){
        preRet += (resultBuf[realNd] ? "1" : "0");
    }
    return preRet;
}
///////////////////////////// setup
string setup(int amtBit){
    cout << "------- setup 64hsrm variable -----------" << endl;
    AMTBIT          = amtBit;
    level           = 0;
    RVAR ob         = -1;
    EXPECTED_RESULT = (amtBit == BIT_SIZE) ? -1 : ((ob << amtBit) ^ ob);
    CURRENT_BITCOUNT= amtBit + 1;
    level           = AMTBIT;
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    graphRenameAndAssign();
    cleanCompose();
    cout << "--------start running phase---------" << endl;
    auto start = chrono::steady_clock::now();

    //////////////// running phrase
    for (int nd = 0; nd < level; nd++){
        pq.push(a[nd]);
    }
    #pragma omp parallel for default(none) shared(NCORE)
    for (int cId = 0; cId < NCORE; cId++) {
        runner();
    }
    ///////////////////////////////////////////////////////////////////////////

    auto stop = chrono::steady_clock::now();

    cout << "--------start result verification phase---------" << endl;
    resultTester();
    string printedResult = graphDeRename();

    cout << "Elapsed time in seconds: "
         << chrono::duration_cast<chrono::seconds>(stop - start).count()
         << " sec" << endl;
    cout << "number of used node    : "<< CURRENT_BITCOUNT << endl;
    bitset<64> myOut(CURRENT_ANS);
    cout << "used virtual node      : " << myOut << endl;
    cout << "--------finished set and run phrase---------" << endl;

    return printedResult;
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

        renameBuf[n1].push_back(n2);
        renameBuf[n2].push_back(n1);
    }
    ///////////////////// set and run
    return setup(amt);
    ///////////////////// result compose

}

int main(){
    ifstream     src = ifstream("../input/grid-60-104");
    string       s1;
    int          n1;

    getline(src, s1);
    stringstream ss(s1);
    ss >> s1;

    n1 = atoi(s1.c_str());

    cout << frontEnd64h(n1, &src) << endl;

    src.close();
    return 0;
}