#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/aes.h>
#include <stdatomic.h>
#include "../../src/rupt.h"

#define AES_KEY_SIZE 16
#define IV_SIZE 16

uint32_t calculate_checksum( char* message, size_t size) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum += message[i];
    }
    return checksum;
}

int filter_message( char* message, size_t size) {
    if (size >= 2 && message[0] == 0xFF && message[1] == 0xAB) {
        return 1;  // Drop the message
    }
    return 0;  // Keep the message
}

void modify_message(char* message, size_t size) {
    if (size >= 4) {
        message[0] = 0xAA;
        message[1] = 0xBB;
        message[2] = 0xCC;
        message[3] = 0xDD;
    }
}

void replace_message(char *message, int size) {
    const char find_byte = 0xAA; // Byte to find
    const char replace_byte = 0xBB; // Replacement byte

    for (int i = 0; i < size; i++) {
        if ((unsigned char)message[i] == find_byte) {
            message[i] = replace_byte;
        }
    }
    // printf("message modified.\n");
}

int load_balance(char* message, size_t size) {
    int num_ports = 1000;
    return size % num_ports;
}

void deep_inspection(unsigned char* message, size_t size) {
    const unsigned char signature[] = {0xDE, 0xAD, 0xBE, 0xEF};
    size_t sig_size = sizeof(signature);

    if (size < sig_size) {
        // printf("message is too small for inspection.\n");
        return;
    }

    for (size_t i = 0; i <= size - sig_size; i++) {
        if (memcmp(message + i, signature, sig_size) == 0) {
            // printf("Signature found at offset %zu.\n", i);
            return;
        }
    }
}

void encrypt_aes(unsigned char* message, size_t size) {
    const unsigned char key[16] = "default_aes_key";  // Default 128-bit key

    if (size % AES_BLOCK_SIZE != 0) {
        fprintf(stderr, "message size must be a multiple of AES block size (%d bytes).\n", AES_BLOCK_SIZE);
        return;
    }

    AES_KEY aes_key;
    AES_set_encrypt_key(key, 128, &aes_key);

    for (size_t i = 0; i < size; i += AES_BLOCK_SIZE) {
        AES_encrypt(message + i, message + i, &aes_key);
    }
}


void encrypt_xor(char *packet, int size) {
    const char key = 0x5A; // Default XOR key
    for (int i = 0; i < size; i++) {
        packet[i] ^= key; // XOR each byte with the key
    }
}

void decrypt_xor(char *packet, int size) {
    const char key = 0x5A; // Same XOR key
    for (int i = 0; i < size; i++) {
        packet[i] ^= key; // XOR each byte with the key to decrypt
    }
}

void encrypt_caesar(char *packet, int size) {
    const int shift = 3; // Default shift value
    for (int i = 0; i < size; i++) {
        packet[i] = (packet[i] + shift) % 256; // Add shift and wrap around
    }
}

void decrypt_caesar(char *packet, int size) {
    const int shift = 3; // Same shift value
    for (int i = 0; i < size; i++) {
        packet[i] = (packet[i] - shift + 256) % 256; // Subtract shift and wrap around
    }
}

void ipsec_vnf(char *packet, int size) {
    const char encryption_key = 0x5A; // Default XOR encryption key
    for (int i = 0; i < size; i++) {
        packet[i] ^= encryption_key; // XOR each byte with the key
    }
}

int hash_message(const unsigned char* message, size_t size) {
    int num_ports = 128;
    uint32_t hash = 0;
    for (size_t i = 0; i < size; i++) {
        hash = (hash * 31) + message[i];  // Simple polynomial hash
    }
    return hash % num_ports;
}


typedef struct {
    int message_size;           // Size of the packet
    char signature[8];         // First 8 bytes of the packet
} TelemetryData;

void telemetry(char *message, int size) {
    static TelemetryData telemetry;
    telemetry.message_size = size;

    for (int i = 0; i < 8 && i < size; i++) {
        telemetry.signature[i] = message[i];
    }
}

void print_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // Get current time with nanosecond precision
    printf("%.6f\n", ts.tv_sec + (ts.tv_nsec / 1e9));
}

int ret = 1;
atomic_int counter = 0;
void process_message(char *message, unsigned long size){

    int local_counter = atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);

    if (unlikely(local_counter == 0)){
        printf("start:\n");
        print_timestamp();
    }

    // hash_message(message, size);
    // telemetry(message, size);
    // ipsec_vnf(message, size); 
    // ipsec_vnf(message, size);
    // encrypt_aes(message, size);
    // replace_message(message, size); 
    // calculate_checksum(message, size); 

    if (unlikely(local_counter == 49999)){
        printf("end:\n");
        print_timestamp();
    }
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