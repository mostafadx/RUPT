#include <unistd.h>

#include "../../src/rupt.h"

int main()
{
    int i, ret = 0;

    ret = rupt_init_client("10.0.1.2", 2020);
    if (ret){
        printf("rupt_init_client failed, ret=%d", ret);
        return ret;
    }

    for (i = 0; i < 5000; i++){
        ret = rupt_send_message("hello server", sizeof("hello server"));
        if (ret){
            printf("rupt_send_message failed, ret=%d\n", ret);
            return ret;
        }
    }

    sleep(3);

    return ret;
}