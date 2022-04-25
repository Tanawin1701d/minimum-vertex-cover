//
// Created by tanawin on 13/4/65.
//
#include<iostream>
#include <fstream>
#include "controller.h"

using namespace std;

ifstream src;
ofstream des;

int main(int argc, char* argv[]){
    if (argc != 3){
        cerr << "invalid input";
    }

    src = ifstream(argv[1]);
    des = ofstream (argv[2]);

    int bitAmt;
    string snode;
    string result;
    getline(src, snode);
    bitAmt = atoi(snode.c_str());

    if (bitAmt > 128){
        cout << "node amount exceed\n";
    }else if (bitAmt > 64){
        result = frontEnd128h(bitAmt, &src);
    }else if (bitAmt > 39){
        result = frontEnd64h(bitAmt, &src);
    }else if (bitAmt > 32){
        result = frontEnd64(bitAmt, &src);
    }else if (bitAmt > 16){
        result = frontEnd32(bitAmt, &src);
    }else if (bitAmt){
        result = frontEnd16(bitAmt, &src);
    }else{
        cout << "node is not a positive integer\n";
    }

    des << result;

    src.close();
    des.close();

}