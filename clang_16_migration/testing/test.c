#include <stdio.h>


void func(int val){
    printf("%s::%d\n", __FUNCTION__, __LINE__);
}

int main(int argc, char ** argv){

    if (argc > 2){
        func(1);
    }

    return 0;


}
