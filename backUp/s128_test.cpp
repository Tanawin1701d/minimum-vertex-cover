//
// Created by tanawin on 19/4/65.
//

#include<iostream>
#include <fstream>
#include <sstream>
#include <bitset>

using namespace std;

typedef unsigned __int128 RVAR;

RVAR a[100];

RVAR EXPECTED_RESULT;
int  CURRENT_BITCOUNT = 20000;
RVAR CURRENT_ANS;

void runner(RVAR start, int amt, int ch){

    RVAR currentAns   = 0;
    int  currentCount = 3000;
    RVAR stop         = min( start + ch, (((RVAR)1) << amt));
    for (; start < stop; start ++){
        RVAR j    = start;
        RVAR r    = 0;
        int count = 0;
        int iter  = 0;
        while(j){
            if (j & 1){
                count++;
                r |= a[iter];
            }
            iter++;
            j >>= 1;
        }
        if ( (r == EXPECTED_RESULT) && (count < currentCount) ){
            currentCount = count;
            currentAns   = start;
        }

    }
    #pragma omp critical (upresult)
    {
        if (currentCount < CURRENT_BITCOUNT) {
            CURRENT_BITCOUNT = currentCount;
            CURRENT_ANS = currentAns;
        }
    }

}

string frontEnd64h(int amt, ifstream* specFile){
    RVAR ob = -1;
    EXPECTED_RESULT = ((ob << amt) ^ ob);
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

        a[n1] |= ( ((RVAR) 1) << n2);
        a[n2] |= ( ((RVAR) 1) << n1);
    }
    for (int nd = 0; nd < amt; nd++){
        a[nd] |= ((RVAR)1 << nd);
    }
    ///////////////////// set and run
    #pragma omp parallel for default(none) shared(amt)
    for ( int i = 0; i < (1 << amt); i+= 1000000000){
            runner(i, amt, 1000000000);
    }

    ///////////////////// result compose

}

int main(int argc, char* argv[]){
    ifstream     srcfile  = ifstream(argv[1]);

    cout << "--------initialize verified program------------------" << endl;
    cout << "source file :" << argv[1]<< endl;

    string       s1;
    int          n1;
    getline(srcfile, s1);
    stringstream ss(s1);
    ss >> s1;
    n1 = atoi(s1.c_str());
    frontEnd64h(n1, &srcfile);

    bitset<64> aaa(CURRENT_ANS);
    cout << CURRENT_BITCOUNT<< " "<<aaa;

    srcfile.close();

    return 0;
}
