#ifndef WM_UTIL_H
#define WM_UTIL_H

#include <wlr/util/log.h>
#include <time.h>

/* Warning - very chatty */
// #define DEBUG_PERFORMANCE_ENABLED

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

static inline double secs_now(){
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return ((double)now.tv_sec) + (double)now.tv_nsec / 1000000000.;
}

#define TIMER_DEFINE(TNAME) \
    static struct timespec TIMER_ ## TNAME ## _start; \
    static struct timespec TIMER_ ## TNAME ## _end; \
    static long long int TIMER_ ## TNAME ## _tmp; \
    static long long int TIMER_ ## TNAME ## _max = 0; \
    static struct timespec TIMER_ ## TNAME ## _last_print = { 0 }; \
    static struct timespec TIMER_ ## TNAME ## _print; \
    static int TIMER_ ## TNAME ## _n_agg = 0; \
    static long long int TIMER_ ## TNAME ## _nsec_agg = 0;

#define TIMER_STARTONLY(TNAME) \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _start);

#define TIMER_START(TNAME) \
    TIMER_DEFINE(TNAME); \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _start);

#define TIMER_STOP(TNAME) \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _end); \
    TIMER_ ## TNAME ## _n_agg++; \
    TIMER_ ## TNAME ## _tmp = TIMER_ ## TNAME ## _end.tv_nsec - TIMER_ ## TNAME ## _start.tv_nsec; \
    TIMER_ ## TNAME ## _tmp += 1000000000 * (TIMER_ ## TNAME ## _end.tv_sec - TIMER_ ## TNAME ## _start.tv_sec);\
    TIMER_ ## TNAME ## _nsec_agg += TIMER_ ## TNAME ## _tmp; \
    TIMER_ ## TNAME ## _max = TIMER_ ## TNAME ## _tmp > TIMER_ ## TNAME ## _max ? TIMER_ ## TNAME ## _tmp : TIMER_ ## TNAME ## _max;


#define TIMER_PRINT(TNAME) \
    clock_gettime(CLOCK_REALTIME, &TIMER_ ## TNAME ## _print); \
    if(msec_diff(TIMER_ ## TNAME ## _print, TIMER_ ## TNAME ## _last_print) > 1000. / 0.1){ \
        wlr_log(WLR_DEBUG, "\nTIMER[%-30s] %s: %7.2fms (%7.2fms max), %5.2fHz", #TNAME, \
                ((double)TIMER_ ## TNAME ## _max / 1000000.) > 10. ? "X" : \
                ((double)TIMER_ ## TNAME ## _max / 1000000.) > 5. ? "o" : \
                ((double)TIMER_ ## TNAME ## _max / 1000000.) > 1. ? "." : " ", \
                (double)TIMER_ ## TNAME ## _nsec_agg / TIMER_ ## TNAME ## _n_agg / 1000000., \
                (double)TIMER_ ## TNAME ## _max / 1000000., \
                (double)TIMER_ ## TNAME ## _n_agg * 0.1); \
        TIMER_ ## TNAME ## _n_agg = 0; \
        TIMER_ ## TNAME ## _max = 0; \
        TIMER_ ## TNAME ## _nsec_agg = 0; \
        TIMER_ ## TNAME ## _last_print = TIMER_ ## TNAME ## _print; \
    }

#ifdef DEBUG_PERFORMANCE_ENABLED
#define DEBUG_PERFORMANCE(name, output) wlr_log(WLR_DEBUG, "DEBUGPERFORMANCE[%s(%d)]: %.6f", #name, output, secs_now());
#else
#define DEBUG_PERFORMANCE(name, output);
#endif


#endif
