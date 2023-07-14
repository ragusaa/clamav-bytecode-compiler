#include <stdio.h>


void func(){
    printf("%s::%d\n", __FUNCTION__, __LINE__);
}

int main(int argc, char ** argv){

    if (argc > 2){
        func();
    }

    return 0;


}
