/**
 * @file mine.c
 *
 * Parallelizes the hash inversion technique used by cryptocurrencies such as
 * bitcoin.
 *
 * Input:    Number of threads, block difficulty, and block contents (string)
 * Output:   Hash inversion solution (nonce) and timing statistics.
 *
 * Compile:  (run make)
 *           When your code is ready for performance testing, you can add the
 *           -O3 flag to enable all compiler optimizations.
 *
 * Run:      ./miner 4 24 'Hello CS 521!!!'
 *
 *   Number of threads: 4
 *     Difficulty Mask: 00000000000000000000000011111111
 *          Block data: [Hello CS 521!!!]
 *
 *   ----------- Starting up miner threads!  -----------
 *
 *   Solution found by thread 3:
 *   Nonce: 10211906
 *   Hash: 0000001209850F7AB3EC055248EE4F1B032D39D0
 *   10221196 hashes in 0.26s (39312292.30 hashes/sec)
 */

#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "sha1.h"

struct thread_data_t{
    char *data_block;
    uint32_t difficulty_mask;
    uint64_t nonce_start;
    uint64_t nonce_end;
    uint8_t digest[SHA1_HASH_SIZE];
    uint64_t solution_nonce;
    bool *found;
};


unsigned long long total_inversions;
// returns the current time in seconds
double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
// prints the binary form of a 32 bit integer
void print_binary32(uint32_t num) {
    int i;
    for (i = 31; i >= 0; --i) {// iterates over all 32 bits
        uint32_t position = (unsigned int) 1 << i; // the current bit
        printf("%c", ((num & position) == position) ? '1' : '0'); // prints either 0 or 1
    }
    puts("");
}

void *thread_mine(void *arg) {
    struct thread_data_t *data = (struct thread_data_t *)arg;

    for (uint64_t nonce = data->nonce_start; nonce < data->nonce_end; nonce++) {
        if (*data->found) {
            break;
        }

        size_t buf_sz = sizeof(char) * (strlen(data->data_block) + 20 + 1);
        char *buf = malloc(buf_sz);

        snprintf(buf, buf_sz, "%s%lu", data->data_block, nonce);

        sha1sum(data->digest, (uint8_t *) buf, strlen(buf));
        free(buf);
        total_inversions++;

        uint32_t hash_front = 0;
        hash_front |= data->digest[0] << 24;
        hash_front |= data->digest[1] << 16;
        hash_front |= data->digest[2] << 8;
        hash_front |= data->digest[3];

        if ((hash_front & data->difficulty_mask) == hash_front) {
            // printf("Thread %ld found a solution: nonce=%lu\n", data->nonce_start, nonce);
            data->solution_nonce = nonce;
            *data->found = true;
            break;
        }
    }

    return NULL;
}



int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Usage: %s threads difficulty 'block data (string)'\n", argv[0]);
        return EXIT_FAILURE;
    }

    // allow user to specify the number of threads
    int num_threads = atoi(argv[1]);
    printf("Number of threads: %d\n", num_threads);

    // allow user to specify the difficulty
    int difficulty = atoi(argv[2]);
    if (difficulty < 1 || difficulty > 32) {
        printf("Error: Difficulty must be between 1 and 32.\n");
        return EXIT_FAILURE;
    }
    uint32_t difficulty_mask = UINT32_MAX >> difficulty;
    printf("  Difficulty Mask: ");
    print_binary32(difficulty_mask);

    /* We use the input string passed in (argv[3]) as our block data. In a
     * complete bitcoin miner implementation, the block data would be composed
     * of bitcoin transactions. */
    char *bitcoin_block_data = argv[3];
    printf("       Block data: [%s]\n", bitcoin_block_data);

    printf("\n----------- Starting up miner threads!  -----------\n\n");

    double start_time = get_time();

    pthread_t threads[num_threads];
    struct thread_data_t thread_data[num_threads];
    uint64_t nonce_partition = UINT64_MAX / num_threads;

    bool solution_found = false;

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].data_block = bitcoin_block_data;
        thread_data[i].difficulty_mask = difficulty_mask;
        thread_data[i].nonce_start = 1 + i * nonce_partition;
        thread_data[i].nonce_end = (i == num_threads - 1) ? UINT64_MAX : 1 + (i + 1) * nonce_partition;
        thread_data[i].found = &solution_found;

        pthread_create(&threads[i], NULL, thread_mine, &thread_data[i]);
    }

    int solution_thread = -1;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);

        if (thread_data[i].solution_nonce != 0) {
            solution_thread = i;
        }
    }

    if (solution_thread != -1) {
        char solution_hash[41];
        sha1tostring(solution_hash, thread_data[solution_thread].digest);
                printf("Solution found by thread %d:\n", solution_thread);
        printf("Nonce: %lu\n", thread_data[solution_thread].solution_nonce);
        printf(" Hash: %s\n", solution_hash);
    } else {
        printf("No solution found!\n");
    }

    double end_time = get_time();

    double total_time = end_time - start_time;
    printf("%llu hashes in %.2fs (%.2f hashes/sec)\n",
            total_inversions, total_time, total_inversions / total_time);

    return 0;
}
