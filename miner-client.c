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
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "sha1.h"
#include "common.h"
#include "task.h"
#include "logger.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct thread_data_t{
    char *data_block;
    uint32_t difficulty_mask;
    // int difficulty;
    uint64_t nonce_start;
    uint64_t nonce_end;
    uint8_t digest[SHA1_HASH_SIZE];
    uint64_t solution_nonce;
    bool *found;
};

struct heartbeat_info_t{
    uint64_t sequence_num;
    int fd;
};

bool stop_threads = false;
#define USERNAME "TheSegFaults"


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

void *thread_heartbeat(void *arg){
    struct heartbeat_info_t *heartbeat_info = (struct heartbeat_info_t *)arg;
    int fd = heartbeat_info->fd;
    union msg_wrapper wrapper = create_msg(MSG_HEARTBEAT);
    struct msg_heartbeat *heartbeat = &wrapper.heartbeat;
    strncpy(heartbeat->username, USERNAME, 19);

    union msg_wrapper msg;
    // bool next = false;
    while(!stop_threads){
        write_msg(fd, (union msg_wrapper *) &wrapper);
        LOGP("heartbeating...\n");
        sleep(10);
        if (read_msg(fd, &msg) <= 0) {
            LOGP("Disconnecting\n");
            return NULL;
        }
        if(msg.heartbeat_reply.sequence_num != heartbeat_info->sequence_num){
            // pthread_mutex_lock(&mutex);
            stop_threads = true;
            // pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }
    return NULL;
}

void *thread_mine(void *arg) {
    LOG("thread%ld start mining\n", pthread_self());
    struct thread_data_t *data = (struct thread_data_t *)arg;

    for (uint64_t nonce = data->nonce_start; nonce < data->nonce_end; nonce++) {
        if (stop_threads) {
            break;
        }
        /* A 64-bit unsigned number can be up to 20 characters  when printed: */
        size_t buf_sz = sizeof(char) * (strlen(data->data_block) + 20 + 1);
        char *buf = malloc(buf_sz);
        /* Create a new string by concatenating the block and nonce string.*/
        snprintf(buf, buf_sz, "%s%lu", data->data_block, nonce);
        /* Hash the combined string */
        sha1sum(data->digest, (uint8_t *) buf, strlen(buf));
        free(buf);
        total_inversions++;

        /* Get the first 32 bits of the hash */
        uint32_t hash_front = 0;
        hash_front |= data->digest[0] << 24;
        hash_front |= data->digest[1] << 16;
        hash_front |= data->digest[2] << 8;
        hash_front |= data->digest[3];

        if ((hash_front & data->difficulty_mask) == hash_front) {
            // printf("Thread %ld found a solution: nonce=%lu\n", data->nonce_start, nonce);
            data->solution_nonce = nonce;
            *data->found = true;
            LOG("solution: %ld\n", nonce);
            // pthread_mutex_lock(&mutex);
            stop_threads = true;
            // pthread_mutex_unlock(&mutex);
            break;
        }
    }

    return NULL;
}



int main(int argc, char *argv[]) {

    if (argc != 3) {
       printf("Usage: %s hostname port\n", argv[0]);
       return 1;
    }

    char *server_hostname = argv[1];
    int port = atoi(argv[2]);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("socket");
        return 1;
    }

    struct hostent *server = gethostbyname(server_hostname);
    if (server == NULL) {
        fprintf(stderr, "Could not resolve host: %s\n", server_hostname);
        return 1;
    }

    struct sockaddr_in serv_addr = { 0 };
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr = *((struct in_addr *) server->h_addr);

    // connect to the server
    if (connect(
                socket_fd,
                (struct sockaddr *) &serv_addr,
                sizeof(struct sockaddr_in)) == -1) {

        perror("connect");
        return 1;
    }

    LOG("Connected to server %s:%d\n", server_hostname, port);

    printf("Press ^c to quit.\n");

    while (true) {
        int num_threads = 4;
        // start loop keep requesting problems
        // request the problem
        union msg_wrapper wrapper_req = create_msg(MSG_REQUEST_TASK);
        struct msg_request_task *request = &wrapper_req.request_task;
        strncpy(request->username, USERNAME, 19);
        write_msg(socket_fd, (union msg_wrapper *) &wrapper_req);

        // read the message from the server (problem)
        union msg_wrapper msg_req;
        ssize_t bytes_read_req = read_msg(socket_fd, &msg_req);
        if (bytes_read_req < 0) {
            perror("read_msg_req");
            return 1;
        }else if(bytes_read_req == 0){
            LOGP("no request message read\n");
            continue;
        }

        // int difficulty = 0;
        uint32_t difficulty_mask = msg_req.task.difficulty_mask;
        printf("  Difficulty Mask: ");
        print_binary32(difficulty_mask);

        printf("\nmessage>> %s\n",msg_req.task.block);

        /* We use the input string passed in (argv[3]) as our block data. In a
        * complete bitcoin miner implementation, the block data would be composed
        * of bitcoin transactions. */
        char *bitcoin_block_data = msg_req.task.block;
        printf("       Block data: [%s]\n", bitcoin_block_data);

        printf("\n----------- Starting up miner threads!  -----------\n\n");

        double start_time = get_time();

        // mining thread initiallization
        pthread_t threads[num_threads], heartbeat;
        struct thread_data_t thread_data[num_threads];
        uint64_t nonce_partition = UINT64_MAX / num_threads;
        bool solution_found = false;

        // heartbeat initialization
        struct heartbeat_info_t heartbeat_info;
        heartbeat_info.sequence_num = msg_req.task.sequence_num;        
        heartbeat_info.fd = socket_fd;

        for (int i = 0; i < num_threads; i++) {
            thread_data[i].data_block = bitcoin_block_data;
            thread_data[i].difficulty_mask = difficulty_mask;
            thread_data[i].nonce_start = 1 + i * nonce_partition; // give each thread a different part
            thread_data[i].nonce_end = (i == num_threads - 1) ? UINT64_MAX : 1 + (i + 1) * nonce_partition;
            thread_data[i].found = &solution_found;

            pthread_create(&threads[i], NULL, thread_mine, &thread_data[i]);
        }
        LOGP("mining threads created\n");

        pthread_create(&heartbeat, NULL, thread_heartbeat, &heartbeat_info);
        LOGP("heartbeat thread created\n");

        int solution_thread = -1;
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);

            if (thread_data[i].solution_nonce != 0) {
                solution_thread = i;
            }
        }
        LOGP("mining over\n");
        pthread_join(heartbeat, NULL);
        LOGP("heartbeat over\n");

        if (solution_thread != -1) {
            /* When printed in hex, a SHA-1 checksum will be 40 characters. */
            char solution_hash[41];
            sha1tostring(solution_hash, thread_data[solution_thread].digest);
            printf("Solution found by thread %d:\n", solution_thread);
            printf("Nonce: %lu\n", thread_data[solution_thread].solution_nonce);
            printf("Hash: %s\n", solution_hash);
            /* Send solution to server*/
            union msg_wrapper wrapper_sol = create_msg(MSG_SOLUTION);
            struct msg_solution *solution = &wrapper_sol.solution;
            strncpy(solution->username, USERNAME, 19);
            solution->nonce = thread_data[solution_thread].solution_nonce;
            solution->sequence_num = heartbeat_info.sequence_num;
            write_msg(socket_fd, (union msg_wrapper *) &wrapper_sol);
        } else {
            printf("No solution found!\n");
            continue;
        }

        double end_time = get_time();

        double total_time = end_time - start_time;
        printf("%llu hashes in %.2fs (%.2f hashes/sec)\n",
            total_inversions, total_time, total_inversions / total_time);

        stop_threads = false;

        // get verification
        union msg_wrapper msg_ver;
        ssize_t bytes_read_ver = read_msg(socket_fd, &msg_ver);
        if(bytes_read_ver == -1){
            perror("read_msg");
            break;
        }
        else if (bytes_read_ver == 0) {
            LOGP("no verification message read\n");
            continue;
        }
        printf("find solution?(1 : true) %i\n", msg_ver.verification.ok);
        printf("verification: %s\n", msg_ver.verification.error_description);

    }


    return 0;
}
