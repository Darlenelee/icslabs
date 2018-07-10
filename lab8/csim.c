/*
 *
 * Student Name: Li Yijun
 * Student ID: 516030910395
 *
 */
#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// use a struct to store the performance
typedef struct{
    int hit;
    int miss;
    int evi;
} Status;

void initStat(Status* st){
    st->hit = 0;
    st->miss =0;
    st-> evi =0;
}
// seq: the sequence of visiting blocks
// first all initialized to 0, adding 1 each time visited
// LRU: find the smallest value(if 0 found just use 0)
typedef struct{
    int valid;
    uint64_t tag;
    int seq;
} Line;

typedef struct{
    Line* lines;
} Set;

typedef struct{
    int B;
    int S;
    int E;
    Set* sets;
} Cache;

Cache* initCache(Cache* c, int cB, int cS, int cE){
    c->sets = (Set*)malloc(sizeof(Set)*cS);
    c->B = cB;
    c->S = cS;
    c->E = cE;
    for(int i=0;i<c->S;i++){
        c->sets[i].lines = (Line*)malloc(c->E*sizeof(Line));
        for(int j=0;j<c->E;j++){
            c->sets[i].lines[j].valid = 0;
            c->sets[i].lines[j].seq = 0;
            c->sets[i].lines[j].tag = 0;  
        }
    }
    return c;
}
uint64_t getTag(uint64_t addr, int s, int b){
    return (addr/(1<<(s+b)));
}
int getIdx(uint64_t addr, int s, int b){
    return (addr / (1<<b)) & ((1<<s)-1);
}
Status* visit(Set* s, Status* res, uint64_t search_tag, int E){
    // LRU strategy: find the smallest value of seq
    
    short ifHit = 0;
    for(int i=0;i<E;i++){
        if(s->lines[i].tag == search_tag && s->lines[i].valid){
            // Hit
            ifHit = 1;
            res->hit ++;
            s->lines[i].seq ++;
            break;
        }
    }
    if(!ifHit){
        // Miss
        res->miss ++;

        // Search for the oldest manipulation
        // aka the line with the smallest seq value

        int old_idx = 0;  // to be re-write
        int new_seq = 0;
        for(int i = 0;i < E; i++){
            if(s->lines[i].seq > new_seq){
                new_seq = s->lines[i].seq;
            }
            if(s->lines[i].seq < s->lines[old_idx].seq){
                old_idx=i;
            }
        }
 
        Line *locate_l = &(s->lines[old_idx]);
        locate_l->tag = search_tag;
        locate_l->seq = new_seq + 1;
        if(locate_l->valid==1){
            // valid, so eviction
            res->evi ++;
        }
        else{
            // make it valid
            locate_l->valid = 1;   
        }
    }
    
    return res;
}
int main(int argc, char* const argv[])
{
    int ch;
    int s = 0;
    int E = 0;
    int b = 0;
    char op[1];
    uint64_t addr=0;
    int size=0;
    FILE* trace = NULL;
    const char* optstr = "vs:E:b:t:";
    while((ch = getopt(argc, argv, optstr))!=-1){
        switch(ch){
            case 'v':
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't': // how to deal with wrong files
                if((trace = fopen(optarg, "r"))==NULL){
                    return 0;
                }
                break;
            default:
                return 0;
        }
        ch++;
    }

    // the whole cache pointer
    Cache *sim = (Cache*)malloc(sizeof(Cache));
    sim = initCache(sim, (1<<b), (1<<s), E);
    Status *result = (Status*)malloc(sizeof(Status));
    initStat(result);
    while(!feof(trace)){
        fscanf(trace, "%c %lx,%d\n", op, &addr, &size);
       
        if(strcmp(op," ")==0) {continue;}
        if(strcmp(op,"I")==0) {printf("I\n");continue;}
       
        uint64_t cur_tag = getTag(addr,s,b); 
        int cur_idx = getIdx(addr,s,b);
        // get the set with index
        Set* search_set = &sim->sets[cur_idx];
      
        if(strcmp(op,"L")==0||strcmp(op,"S")==0){
            visit(search_set, result, cur_tag, E);
        }
        else if(strcmp(op,"M")==0){
            // modification involves two visits
            visit(search_set, result, cur_tag, E);
            visit(search_set, result, cur_tag, E);
        }
        else {
            continue;
        }
    }
    
    fclose(trace);
    printSummary(result->hit, result->miss, result->evi);
    return 0;
}
