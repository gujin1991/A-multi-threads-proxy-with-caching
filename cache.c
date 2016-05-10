/*
 * cache.c - functions for a LRU cache in a proxy
 */
/* $begin cache.c */

#include "cache.h"

/* global variables */
cacheLine * cache;
sem_t mutex;

/*
 * initProxy - initial the global variables and malloc space for
 *             cache
 */
int initCache()
{
    int i;
    size_t size;
    Sem_init(&mutex, 0, 1);
    size = sizeof(cacheLine)*(LINE_NUM);
    if(size > MAX_CACHE_SIZE) {
        return 1;
    }
    cache = (cacheLine *)malloc(size);

    /* initialize the cache */
    for(i = 0; i < LINE_NUM; i++) {
        cache[i].valid = false;
        cache[i].count = 0;
        memset(cache[i].tag, 0, strlen(cache[i].tag));
        memset(cache[i].objects, 0, strlen(cache[i].objects));
    }

    return 0;
}


/*
 *  checkIfExists - check whether the url had been received before.
 *                  Here the tag in cache line is the url value.
 */
bool checkIfExists(char *url, char *responseData)
{
    int mark = 0;
    int i;
    /*
     * Every time we traversal the cache, and increase each valid count
     * if we find the cache line is valid and the tag of it is the
     * same as url, we get a hit and we update the cacheline,
     * especially the count number, which is used for implementing
     * LRU strategy. Otherwise we get a miss. Before careful with
     * race condition.
     */
    for(i = 0; i < LINE_NUM; i++) {
        if(cache[i].valid) {
            P(&mutex);
            cache[i].count++;
            if((strcmp(cache[i].tag, url) == 0)) {
                mark = 1;
                strcpy(responseData, cache[i].objects);
                cache[i].count = 0;
                V(&mutex);
            } else {
                V(&mutex);
            }
        }
    }

    if(mark == 1) {
        return true;
    } else {
        return false;
    }
}

/*
 * saveObjects - save the responded data into cache
 */
void saveObjects(char *url, char *cacheData)
{

    int i = 0;
    int max = -1;
    int maxIdx = 0;
    int mark = 0;

    P(&mutex);
    for (;i < LINE_NUM; i++) {

        if(cache[i].valid) {
            if(max < cache[i].count) {
                max = cache[i].count;
                maxIdx = i;
            }
            cache[i].count++;

        } else if(mark == 0){
            /* find an empty line */
            cache[i].valid = true;
            cache[i].count = 0;
            strcpy(cache[i].tag, url);
            strcpy(cache[i].objects, cacheData);
            mark = 1;
        }
    }
    if(mark != 1) {
        /* the cache is full and we do a eviction with LRU strategy */
        cache[maxIdx].valid = true;
        cache[maxIdx].count = 0;
        memset(cache[maxIdx].tag, 0, strlen(cache[maxIdx].tag));
        memset(cache[maxIdx].objects, 0, strlen(cache[maxIdx].objects));
        strcpy(cache[maxIdx].tag, url);
        strcpy(cache[maxIdx].objects, cacheData);
    }

    V(&mutex);

}