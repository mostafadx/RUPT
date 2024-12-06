#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "../../src/rupt.h"

char *generate_random_string(size_t size) {
    if (size == 0) return NULL;

    char *random_string = malloc(size + 1); // Allocate memory for the string
    if (!random_string) {
        perror("Failed to allocate memory");
        return NULL;
    }

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t charset_size = sizeof(charset) - 1; // Exclude null terminator

    for (size_t i = 0; i < size - 1; i++) {
        random_string[i] = charset[rand() % charset_size];
    }
    random_string[size - 1] = '\0'; // Null-terminate the string

    return random_string;
}

int main()
{
    int i, ret = 0;
    char *message;
    int size = 64;
    
    ret = rupt_init_client("10.0.1.2", 2020);
    if (ret){
        printf("rupt_init_client failed, ret=%d", ret);
        return ret;
    }

    message = generate_random_string(size );
    for (i = 0; i < 50000; i++){
        ret = rupt_send_message(message, size);
        if (ret){
            printf("rupt_send_message failed, ret=%d\n", ret);
            return ret;
        }
    }

    sleep(3);

    return ret;
}