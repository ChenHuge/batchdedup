#ifndef _CPU_USAGE_H_
#define _CPU_USAGE_H_

struct pstat {
    long unsigned int utime_ticks;
    long int cutime_ticks;
    long unsigned int stime_ticks;
    long int cstime_ticks;
    long unsigned int vsize; // virtual memory size in bytes
    long unsigned int rss; //Resident  Set  Size in bytes

    long unsigned int cpu_total_time;
};

/*
 * read /proc data into the passed struct pstat
 * returns 0 on success, -1 on error
 */
int get_usage(const pid_t pid, struct pstat* result);

/*
 * calculates the elapsed CPU usage between 2 measuring points. in percent
 */
void calc_cpu_usage_pct(const struct pstat* cur_usage,
                        const struct pstat* last_usage,
                        double* ucpu_usage, double* scpu_usage);

/*
 * calculates the elapsed CPU usage between 2 measuring points in ticks
 */
void calc_cpu_usage(const struct pstat* cur_usage,
                    const struct pstat* last_usage,
                    long unsigned int* ucpu_usage,
                    long unsigned int* scpu_usage);

#endif
