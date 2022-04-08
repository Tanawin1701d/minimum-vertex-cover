#include <cstdint>
#include <iostream>
#include <algorithm>
#include <bitset>
#include <bits/stdc++.h>
#include <chrono>
using namespace std;

typedef unsigned long long int ull;
typedef uint16_t               ui16;
const int BIT_LEVEL_SIZE = 16;
const int BIT_SIZE       = 64;
const int LEVEL_AMT      = BIT_SIZE/ BIT_LEVEL_SIZE;
const int ARR_PER_LEVEL  = (1 << BIT_LEVEL_SIZE);
const int ARR_SIZE       = ARR_PER_LEVEL * LEVEL_AMT;
const int ST_SIZE        = LEVEL_AMT;
const int BitCountSlicer = (1 << (BIT_LEVEL_SIZE/2)) -1;
const int BitCountShifter= (BIT_LEVEL_SIZE/2);

ull a[ARR_SIZE+1]; // +1 for iterator may exceed
ull b[ARR_SIZE+1];

ui16 Mapper     [ARR_PER_LEVEL];
ui16 Remaper    [ARR_PER_LEVEL];
ui16 lastMapper [ARR_PER_LEVEL];
ui16 lastRemaper[ARR_PER_LEVEL];
int lastLevelSize;

int BitC[1 << (BIT_LEVEL_SIZE/2)];
/////////////////////////ฺBoost with Dynamic programming///////////////////////
void compose(int oldSize, int oldBitSize, ull* baseArr, ull* disArr){

	 int  amountIter = BIT_SIZE / oldBitSize;
	 int  newSize    = oldSize*oldSize; 
	 ull* masterChunk;
	 ull* slaveChunk;
     ull* MasterChunkT;

	 for (int iter = 0; iter < amountIter; iter+=2){
         MasterChunkT = baseArr      + iter*oldSize;
         slaveChunk   = MasterChunkT +      oldSize;
         for (int slaveIter = 0; slaveIter < oldSize; slaveIter++){
			masterChunk = MasterChunkT;
			for (int masterIter = 0; masterIter < oldSize; masterIter++){
				//cout << "slave :" << bitset<2>(slaveIter) << "    " << "master :" << bitset<2>(masterIter) <<"  " << *slaveChunk << "   " << *masterChunk << endl;
				disArr[(iter>>1)*newSize + ((slaveIter<<oldBitSize)|(masterIter)) ] = *slaveChunk | *masterChunk;
				masterChunk++;
			}
			slaveChunk++;
		}
	 }
}

ull* composeController(int disBitSize = BIT_LEVEL_SIZE){ // bit that you want to build  must be 4 8 16

	ull* baseArr = a;
	ull* disArr  = b;
    int  curSize = 2;
	for (int curBitSize = 1; curBitSize < disBitSize; curBitSize <<= 1){
		compose(curSize,curBitSize, baseArr, disArr);
		curSize *= curSize;
		swap(baseArr,disArr);
	}

	return baseArr; // it is disArr but it have been swap at lasts iteration.
}
////////////////////////////////////////////////////////////////////////
//////////////////////////trace and
ull traceAns(int amtLevel, int* stIter){
	ull preResult = lastRemaper[stIter[amtLevel-1]-1];
	for (int i = amtLevel-2; i >= 0; i--){
		preResult <<= BIT_LEVEL_SIZE;
		preResult |= Remaper[stIter[i]-1];	
	}
	return preResult;
}
////////////////////////// mapper///////////////////////////////////////
struct preMap{
	int bitCount;
    ui16 origin;
};

struct less_than_key
{
    inline bool operator() (const preMap& struct1, const preMap& struct2)
    {
        return (struct1.bitCount < struct2.bitCount) ||
               ((struct1.bitCount == struct2.bitCount) && (struct1.origin < struct2.origin));
    }
};

void buildMapper(ui16* map, ui16* remap, int bitLevelSize){
	int disIter = 1 << bitLevelSize;
	preMap preSortArr[ARR_PER_LEVEL];
	// start counting
	for (int i = 0; i < disIter; i++){
		int j = i;
		preMap buffer = {0, static_cast<ui16>(i)};
		while (j){
			buffer.bitCount += j & 1;
			j >>= 1;
		}
	}
	// sorting
	sort(preSortArr, preSortArr + disIter, less_than_key());
	// mapping
	for (int i = 0; i < disIter; i++){
		remap[i] = preSortArr[i].origin;
		map[ preSortArr[i].origin ] = static_cast<ui16>(i);
	}
}

void buildBc(){
    int disIter = 1 << 8;
    // start counting
    for (int i = 0; i < disIter; i++){
        ui16 j = i;
        int counter = 0;
        while (j){
            counter += j & 1;
            j >>= 1;
        }
        BitC[i] = counter;
    }
}

int bitCounter(int iter){
    return (BitC[BitCountSlicer&iter]) + (BitC[BitCountSlicer&(iter >> BitCountShifter)]);
}

void huristicMapper(const ull* source, ull* des, const ui16* mapper, int arrSize){
    for (int i = 0; i < arrSize; i++){
        des[mapper[i]] = source[i];
    }
}
///////////////////////// Sucide with Multicore
void dfs(ull* source, ull expectedResult, int amtLevel, int amtBit){  // amt amount bit per level
	int amtIterLevel = 1 << amtBit; // size for array that we must iterate in each level
	int targetLevel  = amtLevel-1;
	ull preAns = 0;
    int bitUsedAns = INT32_MAX;
    /////////// first level counter
	for (int flvIter = 0; flvIter < amtIterLevel; flvIter++){
        cout << "first level "<< flvIter << endl;
		/////////// stack data
		int  stIter[ST_SIZE];
        int  stamtused[ST_SIZE];
		ull  stResult[ST_SIZE];
		//////////    running variable
		int lv = 0;
		int lastIter;
		/////////    initialize variable
        memset(stIter, 0, sizeof(stIter));
        stResult[lv] = source[flvIter];
        stamtused[lv]= bitCounter(Remaper[stIter[lv]]);
        stIter[lv]   = flvIter+1;
		lv           = 1;
		////////////////////////////////////////
		while (lv){
			if (lv == targetLevel){
				// TODO:use avx
				for (lastIter = 0; lastIter < lastLevelSize; lastIter++){
					if ( (stResult[lv-1]|source[lv*amtIterLevel + lastIter]) == expectedResult){
						stIter[lv] = lastIter+1;
                        // use mutex in these variable : preAns, bitUsedAns
						preAns = traceAns(amtLevel, stIter);
                        bitUsedAns = stamtused[lv-1] + bitCounter(lastIter);
						// break; // bound
					}
				}
                lv--;
			}else if (stIter[lv] == amtIterLevel){
					stIter[lv] = 0;
					lv--;
			}else{
				stResult [lv] = stResult [lv-1] | source[ lv*amtIterLevel + stIter[lv] ];
                stamtused[lv] = stamtused[lv-1] + bitCounter(Remaper[stIter[lv]]);
				// you may run bound here
//				if (bitUsedAns <= stamtused[lv]){
//                    stIter[lv] = 0;
//                    lv--;
//                }else {
                    stIter[lv]++;
                    lv++;
//                }
			}
		}
	}
	cout << bitUsedAns<<":"<< preAns;
}
////////////////////////////////////////////////


void run(int amtBit){
    cout << amtBit << endl;
    /////////////////////////////////////////bd
    int level = (amtBit+BIT_LEVEL_SIZE-1)/BIT_LEVEL_SIZE;
    int preLastbz = amtBit%BIT_LEVEL_SIZE;
    int lastLevelBitSize = max(preLastbz ? preLastbz : BIT_LEVEL_SIZE, 2);
    lastLevelSize = 1 << lastLevelBitSize;
    ull* source = composeController();
    cout << "----------------------composed complete" << endl;
    //////////////////////////////////////// mapper
    buildBc();
    buildMapper(Mapper, Remaper, BIT_LEVEL_SIZE);
    buildMapper(lastMapper, lastRemaper, lastLevelBitSize);
    for (int i = 0; i < level-1; i++) {
        huristicMapper(source + i*ARR_PER_LEVEL, ((source == a) ? b : a) + i*ARR_PER_LEVEL, Mapper, ARR_PER_LEVEL);
    }
    huristicMapper(source + (level-1) * ARR_PER_LEVEL, ((source == a) ? b : a) + (level-1) * ARR_PER_LEVEL, lastMapper, lastLevelSize);
    cout << "----------------------heristic mapper complete" << endl;
    ////////////////////////////////////////// sm
    ull testbit = 0 ;
    testbit-=1;
    testbit <<= amtBit;



    dfs(source, testbit, level, BIT_LEVEL_SIZE);
}

void testCompose(){
    // test two bit to four bit


    for (int i = 1; i < 4; i++){
        a[i] = a[i+4] = a[i+8] = a[i+12] = 1 << (i-1);
    }
    cout << "[dbg@s64.compose]========== compose test ============" << endl;
    //compose( 4, 2, a, b);
    composeController(16);
    for (int i = 0; i < 16; i++){
        cout << bitset<5>(i) << " ====> " << bitset<5>(b[i]) << endl;
    }
    cout << "[dbg@s64.compose]========== second result set" << endl;
    for (int i = 16; i < 32; i++){
        cout << bitset<5>(i) << " ====> " << bitset<5>(b[i]) << endl;
    }
    cout << "[dbg@s64.compose]========= end test==================";


}
void testCase1(){
    ull sixteenset = (1<<16)-1;
    a[0     +1] = sixteenset;
    a[16*1*2+1] = sixteenset << 16;
    a[16*2*2+1] = sixteenset << 32;
    a[16*3*2+1] = sixteenset << 48;
    run(64);
}

int main(){
    testCase1();
	//testCompose();
}
