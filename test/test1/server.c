#include <unistd.h>

#include "../../src/rupt.h"

int main()
{
    int ret = 0;

    ret = rupt_init_server("10.0.1.2", 2020, NULL);
    if (ret){
        printf("rupt_init_server failed, ret=%d", ret);
        return ret;
    }

    sleep(1);

    return ret;
}