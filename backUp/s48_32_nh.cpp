//
// Created by tanawin on 7/4/65.
//
#include<iostream>
#include <algorithm>
#include <queue>

#include <immintrin.h>
#include <cstdio>
#include <chrono>

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
const int N_JOB          = 16;


int level;
int levelSize [LEVEL_AMT];
int AMTBIT;

RVAR a[ARR_SIZE];
RVAR b[ARR_SIZE];
RVAR bc[ARR_SIZE];
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
pair<RVAR*, RVAR*> compose(){
    int bs = 1;
    int as = 2;
    RVAR* src = a;
    RVAR* des = b;
    for (;bs < BIT_LEVEL_SIZE; bs <<= 1){
        //for all
        int iterSize = BIT_SIZE / bs;
        for (int iter = 0; iter < iterSize; iter+=2) {
            int bid  = iter*as; // baseId
            int fbid = (iter >> 1) * as * as;
            for (int msb = 0; msb < as; msb++) {
                for (int lsb = 0; lsb < as; lsb++) {
                    int fidx = fbid + ((msb << bs) | lsb);
                    des[fidx] = src[bid + lsb] | src[bid + as + msb];
                }
            }
        }
        swap(src, des);
        as *=  as;
    }
    return {src, des};
}

void cleanCompose(){
        // simd may help
        for (int iter = 0; iter < ARR_PER_LEVEL; iter++){
            ui16 counter = bitCount64(iter);
            bc[0*ARR_PER_LEVEL + iter] = counter;
        }
}

////////////////////////////// SM
void runner(int startIter0, RVAR* src){
    //initializer
    __m256i   Rexpr  = _mm256_set1_epi64x(EXPECTED_RESULT);
    __m256i   Rbc    = _mm256_set1_epi64x(AMTBIT+1);
    __m256i   Rid    = _mm256_set1_epi64x(0);

    int stopIter   = startIter0 + ARR_PER_LEVEL/N_JOB;



    for (;startIter0 < stopIter; startIter0++){
        ///////////// varible
        RVAR res0 = src[startIter0];
        RVAR bc0  = bc [startIter0];
        RVAR idx0 = startIter0;
        ///// next stage
        for(int Iter1 = 0; Iter1 < levelSize[2]; Iter1++){
            RVAR res1 = src[(ARR_PER_LEVEL << 1) + Iter1] | res0;
            RVAR bc1  = bc[startIter0]                    + bc0 ;
            RVAR idx1 = idx0 << (BIT_LEVEL_SIZE << 1)    | (Iter1 << BIT_LEVEL_SIZE);
            /// openSIMD for level 3
            for (int Iter2 = 0; Iter2 < ARR_PER_LEVEL; Iter2 += 4){
                /// compose the result_mm256_cmpeq_epi64
                __m256i   res2     = _mm256_load_si256 ((const __m256i *)(src + ARR_PER_LEVEL + Iter2));
                __m256i   resBase  = _mm256_set1_epi64x(res1);
                __m256i   res2f    = _mm256_or_si256(res2, resBase);
                __m256i   resMark  = _mm256_cmpeq_epi64(Rexpr, res2f);
                /// compose the bitcount
                __m256i   bc2      = _mm256_load_si256 ((const __m256i *)(bc + Iter2));
                __m256i   bcBase   = _mm256_set1_epi64x( bc1); // we use only upto 128 not 256
                __m256i   bcAdded  = _mm256_add_epi64  (bc2,bcBase);
                __m256i   bcMarkOld= _mm256_cmpgt_epi64(bcAdded, Rbc);
                __m256i   bcMarkNew= _mm256_xor_si256  (_mm256_set1_epi64x(-1), bcMarkOld) ;
                /// compose Idx
                __m256i   idxBase  = _mm256_set1_epi64x(idx1);
                __m256i   idx2     = _mm256_set_epi64x (Iter2, Iter2+1, Iter2+2, Iter2+3);
                __m256i   idx2f    = _mm256_or_si256(idx2, idxBase);
                /// store index
                __m256i   MarkNew  = _mm256_and_si256  (resMark, bcMarkNew);
                __m256i   MarkOld  = _mm256_and_si256  (_mm256_set1_epi64x(-1), MarkNew);
                          Rbc      = _mm256_or_si256( _mm256_and_si256(Rbc    , MarkOld)
                                                     ,_mm256_and_si256(bcAdded, MarkNew)
                                                    );
                          Rid     = _mm256_or_si256( _mm256_and_si256(Rid, MarkOld)
                                                    ,_mm256_and_si256(idx2f, MarkNew)
                                                   );
            }
        }
    }
}
///////////////////////////// setup
void setup(int amtBit){
    AMTBIT           = amtBit;
    level            =  3;
    RVAR ob          = -1;
    EXPECTED_RESULT  = (amtBit == BIT_SIZE) ? -1 : ((ob << amtBit) ^ ob);
    CURRENT_BITCOUNT = amtBit + 1;
    levelSize[0]     = ARR_PER_LEVEL;
    levelSize[1]     = ARR_PER_LEVEL;
    levelSize[2]     = (amtBit == 48) ? ARR_PER_LEVEL: (1 << (amtBit % BIT_LEVEL_SIZE));
    cout << "--------start BD phase---------" << endl;
    buildCounter();
    pair<RVAR*, RVAR*> s = compose();
    cleanCompose();
    cout << "--------finish BD phase---------" << endl;
    auto start = chrono::steady_clock::now();
    # pragma omp parallel for
    for (int j = 0; j < N_JOB; j++) {
        runner(j*(ARR_PER_LEVEL/N_JOB), s.first);
    }

    auto stop = chrono::steady_clock::now();
    cout << chrono::duration_cast<chrono::seconds>(stop - start).count();


    cout << CURRENT_BITCOUNT << endl;

}

void frontEnd(int amt){
    setup(amt);
}

int main(){
//    a[0     +1].result = (RVAR)1 << 47;
//    a[0     +1].result -=1;
//    a[47*2+1].result = (RVAR)1 << 48;
//    a[47*2+1].result   -= 1;
//    for (int i = 0; i < 48; i++){
//        RVAR start = 1;
//        a[i*2+1].result |= (start << i);
//    }
      frontEnd(35);
}
