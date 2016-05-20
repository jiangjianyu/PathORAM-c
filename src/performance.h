//
// Created by maxxie on 16-5-8.
//

#ifndef PATHORAM_PERFORMANCE_H
#define PATHORAM_PERFORMANCE_H

typedef struct {
    long long p_total_bandwidth;
    long long p_original_bandwidth;
    long long p_stash_size;
    long long p_start_time;
    long long p_now_time;
    int p_sock;
    pthread_t p_pid;
} p_per_ctx;

#ifdef ORAM_DEBUG_PERFORAMANCE
extern p_per_ctx p_ctx;

#define __P_ADD(size, total) total += size
#define __P_DEL(size, total) total -= size

#define P_INIT() 
#define P_ADD_BANDWIDTH(size) __P_ADD(size, p_ctx.p_total_bandwidth)
#define P_ADD_BANDWIDTH_ORIGINAL(size) __P_ADD(size, p_ctx.p_original_bandwidth)
#define P_ADD_STASH(size) __P_ADD(size, p_ctx.p_stash_size)
#define P_DEL_STASH(size) __P_DEL(size, p_ctx.P_stash_size)

#else

#define P_ADD_BANDWIDTH(size)
#define P_ADD_BANDWIDTH_ORIGINAL(size)
#define P_ADD_STASH(size)
#define P_DEL_STASH(size)

#endif 

#endif //PATHORAM_PERFORMANCE_H