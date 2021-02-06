#ifndef WM_UTIL_H
#define WM_UTIL_H

#include <time.h>

#define wm_offset_of(_struct_, _member_)  (size_t)&(((struct _struct_ *)0)->_member_)
 
#define wm_cast(_subclass_, _superclass_pt_)                           \
                                                                           \
   ((struct _subclass_ *)(                                                 \
      (unsigned char *)(_superclass_pt_)                                   \
      - wm_offset_of(_subclass_, super)                                    \
      )                                                                    \
    )


static inline long msec_diff(struct timespec t1, struct timespec t2){
    return (t1.tv_sec - t2.tv_sec) * 1000L + (t1.tv_nsec - t2.tv_nsec) / 1000000L;
}

#define TIMER_START(TNAME) \
    static struct timespec TIMER_ ## TNAME ## _start; \
    static struct timespec TIMER_ ## TNAME ## _end; \
    static struct timespec TIMER_ ## TNAME ## _last_print = { 0 }; \
    static struct timespec TIMER_ ## TNAME ## _print; \
    static int TIMER_ ## TNAME ## _n_agg = 0; \
    static long long int TIMER_ ## TNAME ## _nsec_agg = 0; \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _start);

#define TIMER_STOP(TNAME) \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _end); \
    TIMER_ ## TNAME ## _n_agg++; \
    TIMER_ ## TNAME ## _nsec_agg += TIMER_ ## TNAME ## _end.tv_nsec - TIMER_ ## TNAME ## _start.tv_nsec; \
    TIMER_ ## TNAME ## _nsec_agg += 1000000000 * (TIMER_ ## TNAME ## _end.tv_sec - TIMER_ ## TNAME ## _start.tv_sec);

#define TIMER_PRINT(TNAME) \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _print); \
    if(msec_diff(TIMER_ ## TNAME ## _print, TIMER_ ## TNAME ## _last_print) > 1000. / 0.1){ \
        fprintf(stderr, "TIMER[%s]: %fms, %5.2fHz\n", #TNAME, \
                (double)TIMER_ ## TNAME ## _nsec_agg / TIMER_ ## TNAME ## _n_agg / 1000000., \
                (double)TIMER_ ## TNAME ## _n_agg * 0.1); \
        TIMER_ ## TNAME ## _n_agg = 0; \
        TIMER_ ## TNAME ## _nsec_agg = 0; \
        TIMER_ ## TNAME ## _last_print = TIMER_ ## TNAME ## _print; \
    }

#endif
