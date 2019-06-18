#include <iostream>
#include <stdint.h>
 
using namespace std;


class CTest {
   public:
       int a;
};

class CBest {
   public:
       int b;
};

CTest* fun(CBest* pBest) {
    CTest* pTest = new CTest();
    pTest->a = pBest->b;
    return pTest;
}

int main() {
    CBest* pBest = new CBest();
    pBest->b = 5;
    
    CTest* pRes= fun(pBest);

    cout<<pRes->a<<endl;
    
    if(pBest!=NULL)
        delete pBest;
    if(pRes!=NULL)
        delete pRes ;
    return -1;
}