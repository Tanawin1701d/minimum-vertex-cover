//
// Created by tanawin on 9/4/65.
//
#include<iostream>
using namespace std;
#include<omp.h>

void run(){
    int a[4] = {0,1,2,3};
    cout << "result :"<<a[0]+a[1]+a[2]+a[3] << endl;
    cout << "thread num :" << omp_get_thread_num()            << endl;
}

int main(){
    #pragma omp parallel for
    for (int i = 0; i < 10; i++){
        run();
    }
}