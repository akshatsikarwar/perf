#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <cdb2api.h>

char *dbs[] =  { "libevent", "nolibevent"};
#define NUM 1000 //num of iteration of query
#define T 1 //num of threads
#define N (sizeof(dbs) / sizeof(dbs[0]))

struct foo {
    char *name;
    pthread_t thd;
    struct timeval begin;
    struct timeval end;
};

void *run(void *arg)
{
    struct foo *f = arg;
    cdb2_hndl_tp *hndl = NULL;
    int rc = cdb2_open(&hndl, f->name, "dev-10-34-16-147", CDB2_DIRECT_CPU);
    assert(rc == 0); 

    gettimeofday(&f->begin, NULL);
    for (int i = 0; i < NUM; ++i) {
        #define str "'hello, async world!'"
        rc = cdb2_run_statement(hndl, "select " str);
        if (rc) {
            puts(cdb2_errstr(hndl));
            assert(rc == 0); 
        }   
        while ((rc = cdb2_next_record(hndl)) == CDB2_OK) {
        }
        assert(rc == CDB2_OK_DONE);
    }   
    gettimeofday(&f->end, NULL);

    assert(cdb2_close(hndl) == 0); 
    return NULL;
}

int main()
{
    struct foo f[T][N];
    for (int t = 0; t < T; ++t) {
        for (int n = 0; n < N; ++n) {
            f[t][n].name = dbs[n];
            pthread_create(&f[t][n].thd, NULL, run, &f[t][n]);
        }
    }   
    for (int t = 0; t < T; ++t) {
        for (int n = 0; n < N; ++n) {
            pthread_join(f[t][n].thd, NULL);
        }
    } 
    struct timeval d[N];
    memset(&d, 0, sizeof(d));
    for (int t = 0; t < T; ++t) {
        for (int n = 0; n < N; ++n) {
            struct timeval diff;
            timersub(&f[t][n].end, &f[t][n].begin, &diff);

            struct timeval tmp = d[n];
            timeradd(&tmp, &diff, &d[n]);
        }
    }   
    for (int n = 0; n < N; ++n) {
        printf("%s took:%lds.%ldms\n", f[0][n].name, d[n].tv_sec/T, d[n].tv_usec/(T*1000));
    }
}
