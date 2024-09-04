#include <unistd.h>
#include <string.h>

#include "../../src/rupt.h"

int ret = 1;

void process_message(char *message, unsigned long size){
    if (strcmp(message, "hello server") == 0 && strlen(message) == size)
        ret = 0;
}

int main()
{
    ret = rupt_init_server("10.0.1.2", 2020, process_message);
    if (ret){
        printf("rupt_init_server failed, ret=%d", ret);
        return ret;
    }

    sleep(1);
    
    return ret;
}