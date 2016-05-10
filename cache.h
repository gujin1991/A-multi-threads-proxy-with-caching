/*
 * cache.h - prototypes and definitions for cache.c
 */
/* $begin cache.h */

#ifndef __CACHE_H__
#define __CACHE_H__
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdbool.h>
#include "csapp.h"


#define LINE_NUM 10
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct{
    bool valid;
    char tag[MAXLINE];
    unsigned long count;
    char objects[MAXLINE];
}cacheLine;


int initCache();
bool checkIfExists(char *url, char *responseData);
void saveObjects(char *url, char *cacheData);

#endif

