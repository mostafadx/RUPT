#include <unistd.h>
#include <string.h>

#include "../../src/rupt.h"

int ret = 1;

void process_message(char *message, unsigned long size){
}   

int main()
{
    ret = rupt_init_server("10.0.1.2", 2020, process_message);
    if (ret){
        printf("rupt_init_server failed, ret=%d", ret);
        return ret;
    }

    sleep(3);
    
    return ret;
}