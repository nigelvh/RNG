/* Compile: gcc -Wall -O amls_test.c -o amls_test -std=c99 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <math.h>
#ifdef __linux
#include <linux/random.h>
#endif

#define amls_array_len 512

// Expects input array of ints containing values of 0 or 1. Maximum length determined by amls_array_len
// Returns pointer to array of ints containing values of 0 or 1, -1 value will indicate end of data
// Returned array pointer must be freed after use
int8_t * amls(int8_t data[], int data_len) {
    // Set up 'result' array
    int8_t *result;
    result = malloc(amls_array_len * sizeof(int8_t));
    memset(result, -1, amls_array_len * sizeof(int8_t));
    int result_pos = 0;
    
    // Set up 'level_1' array
    int8_t *level_1;
    level_1 = malloc(amls_array_len * sizeof(int8_t));
    memset(result, -1, amls_array_len * sizeof(int8_t));
    int level_1_pos = 0;
    
    // Set up 'level_a' array
    int8_t *level_a;
    level_a = malloc(amls_array_len * sizeof(int8_t));
    memset(result, -1, amls_array_len * sizeof(int8_t));
    int level_a_pos = 0;
    
    // Input bits
    int8_t bit_0 = -1;
    
    // Check that we've got at least two bits to compare
    if (data_len < 2){
        free(level_1);
        free(level_a);
        
        return result;
    }
    
    for (int i = 0; i < data_len; i++) {
        if (data[i] != 0 && data[i] != 1) break; // If we encounter a non-0 or 1 bit, stop
        
        if (bit_0 == -1) { // Skip a cycle to grab two bits at a time
            bit_0 = data[i];
        } else { // We've got two bits, do the processing
            if (bit_0 == data[i]) {
                level_a[level_a_pos] = 0;
                level_a_pos++;
                
                level_1[level_1_pos] = bit_0;
                level_1_pos++;
            } else {
                level_a[level_a_pos] = 1;
                level_a_pos++;
                
                result[result_pos] = bit_0;
                result_pos++;
            }
            
            // Reset bit_0
            bit_0 = -1;
        }
    }
    
    // Run AMLS on the level_a and level_1 arrays, then append their results to the main results array
    int8_t *level_a_result = amls(level_a, level_a_pos);
    int8_t *level_1_result = amls(level_1, level_1_pos);
    for (int i = 0; i < amls_array_len; i++) {
        if (level_a_result[i] == -1) break;
        result[result_pos] = level_a_result[i];
        result_pos++;
    }
    for (int i = 0; i < amls_array_len; i++) {
        if (level_1_result[i] == -1) break;
        result[result_pos] = level_1_result[i];
        result_pos++;
    }
    
    free(level_a_result);
    free(level_1_result);
    free(level_a);
    free(level_1);
    
    return result;
}

int main(int argc, char *argv[]) {
    int8_t buf [512];
    //int8_t buf [512] = {1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,0,0,1,0,1,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,1,0,0,0,1,1,0,1,1,1,1,1,1,0,1,0,1,1,1,1,0,1,1,1,1,1,0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,0,0,0,1,1,1,1,1,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,0,1,0,1,1,1,1,1,0,1,1,1,1,0,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,1,1,1,1,0,1,1,0,1,0,1,1,1,0,1,1,1,1,0,0,0,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,0,0,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,1,1,1,1,1,0,1,1,1,1,0,0,1,1,1,1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,0,1,1,1,0,0,0,0,1,1,1,1,1,1,1,0,0,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1};
    int eof, working;
    
    while(1){
        for(int i = 0; i < (sizeof(buf) / sizeof(buf[0])); i++){
            eof = read(STDIN_FILENO, &working, 1);
            if (eof == 0) return 0;
            buf[i] = (working - '0');
        }
        
        int8_t *result = amls(buf, (sizeof(buf) / sizeof(buf[0])));
        
        for (int i = 0; i < (sizeof(buf) / sizeof(buf[0])); i++) {
            if (result[i] == -1) {
                free(result);
                break;
            }
            fputc((result[i] + '0'), stdout);
        }
        
        
    }
    
    return 1;
}

