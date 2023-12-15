#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

long long s, S, E, b;
char* t;
unsigned long long time = 0;

typedef struct cache_line
{
    unsigned long long tag;
    //unsigned long long index;
    int valid;
    long long Recent_Use;
} cache_line;

cache_line **cache_set;

void initial(cache_line ***cache_set)
{
    (*cache_set) = (cache_line **)malloc(S * (sizeof(cache_line *)));
    for (int i = 0; i < S; i++)
    {
        (*cache_set)[i] = (cache_line *)malloc(E * sizeof(cache_line));
    }
    for (int i = 0; i < S; i++)
    {
        for (int j = 0; j < E; j++)
        {
            (*cache_set)[i][j].Recent_Use = -1;
            (*cache_set)[i][j].valid = 0;
            (*cache_set)[i][j].tag = 0xffffffff;
        }
    }
}

void free_cache(cache_line** cache_set)
{
    for (int i = 0; i < S; i++)
    {
        free(cache_set[i]);
    }
    free(cache_set);
}

void get_parameter(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch(opt)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                t = optarg;
                break;
            default:
                break;
        }
    }
}
void update(unsigned long long address, long long *hit, long long *miss, long long *eviction)
{
    unsigned long long index = (address >> b) & ((0xffffffff) >> (32 - s));
    unsigned long long tag = address >> (s + b);
    for (int i = 0; i < E; i++)
    {
        if(cache_set[index][i].tag == tag)
        {
            cache_set[index][i].Recent_Use = time;
            (*hit)++;
            return;
        }
    }
    for (int i = 0; i < E; i++)
    {
        if(!(cache_set[index][i].valid))
        {
            cache_set[index][i].tag = tag;
            cache_set[index][i].valid = 1;
            cache_set[index][i].Recent_Use = time;
            (*miss)++;
            return;
        }
    }
    long long which;
    long long min_time = time;
    for (int i = 0; i < E; i++)
    {
        if(cache_set[index][i].Recent_Use < min_time)
        {
            min_time = cache_set[index][i].Recent_Use;
            which = i;
        }
    }
    (*miss)++;
    (*eviction)++;
    cache_set[index][which].tag = tag;
    cache_set[index][which].Recent_Use = time;
    return;
}

void work(long long *hit, long long *miss, long long *eviction)
{
    FILE *fp = fopen(t, "r");
    char operand;
    int bin;
    unsigned long long address;
    while ((fscanf(fp, " %c %llx,%d", &operand, &address, &bin)) > 0)
    {
        if(operand == 'I')
        {
            continue;
        }
        else if(operand == 'M')
        {
            (*hit)++;
            update(address, hit, miss, eviction);
        }
        else
        {
            update(address, hit, miss, eviction);
        }
        time++;
    }
    fclose(fp);
}

int main(int argc, char** argv)
{
    get_parameter(argc, argv);
    S = 1 << s;
    initial(&cache_set);
    long long hit = 0, miss = 0, eviction = 0;
    work(&hit, &miss, &eviction);
    free_cache(cache_set);
    printSummary(hit, miss, eviction);
    return 0;
}
