/*
	Mert KELKİT
	1501153013
	Project 3
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "cachelab.h"

/*
	I used unsigned long long instead of int because my computer had some problems about 64 bit data types
	However declaring all variables as unsigned long long didn't solve the problem, I still get segmentation fault
	addresses which are larger than 32 bits.
*/


#define Address unsigned long long int

//Line struct, holds information about address, hits, misses and evictions occur here
typedef struct line
{
    Address tagBit;
    unsigned long long int validBit;
    unsigned long long int lru;
    unsigned long long int isFull;
} Line;

//Set struct which has line/lines
typedef struct set
{
    Line* line;
} Set;

//Cache struct which has sets
typedef struct cache
{
    Set* set;
} Cache;

//Function prototypes
void getArgs(int, char**);
unsigned long long int square(unsigned long long int);
unsigned long long int myPow(unsigned long long int, unsigned long long int);
unsigned long long int getSet(Address);
void openCache(void);
void openTrace(void);
void insert(char, Address, unsigned long long int);
unsigned long long int findLRUIndex(Line*);
void freeCache(void);

//parameters from the command line
unsigned long long int s;
unsigned long long int E;
unsigned long long int b;
char* t;

//set and block sizes
unsigned long long int S;
unsigned long long int B;

//hit, miss and eviction counters
unsigned long long int hits = 0, misses = 0, evictions = 0;

//global declarations of structs
Cache mainCache;
Set mainSet;
Line mainLine;

int main(int argc, char** argv)
{
	//getting parameters from command line
    getArgs(argc, argv);
    //creating an empty cache
    openCache();
    //openin text files and getting cache instructions
    openTrace();
    //printing summary
    printSummary(hits, misses, evictions);
    //freeing caches
    freeCache();
    return 0;
}

void getArgs(int argc, char** argv)
{
    char c;

    while((c = getopt(argc, argv, "s:E:b:t")) != -1)
    {
        switch(c)
        {
            case 's':
            	//convert s parameter to int
                s = atoi(optarg);
                break;
            case 'E':
                //convert E parameter to int
                E = atoi(optarg);
                break;
            case 'b':
            	//convert b parameter to int
                b = atoi(optarg);
                break;
            case 't':
            	//get trace file's directory
                t = argv[8];
                break;
        }
    }

    //compute set and block sizes
    S = square(s);
    B = square(b);
}

//bitwise square function 
unsigned long long int square(unsigned long long int base)
{
    return base << 1ULL;
}

//power function
unsigned long long int myPow(unsigned long long int base, unsigned long long int exponent)
{
    unsigned long long int ret = 1;
    unsigned long long int i;
    for(i=0; i<exponent; i++)
        ret *= base;
    return ret;
}

//the function which gives set index
unsigned long long int getSet(Address address)
{
	//this function shift address right b times and ands it with the 111...1 which is b times 1s.
    unsigned long long int index = 0ULL;
    Address temp = address >> b;
    index = temp & (myPow(2, b) - 1);
    return index;
}

//creating cache
void openCache()
{
    unsigned long long int i, j;
    //allocating memory for S time Set
    mainCache.set = (Set*)malloc(sizeof(Set) * S);
    for(i=0; i<S; i++)
    {
    	//allocating memory for every line
        mainSet.line = (Line*)malloc(sizeof(Line) * E);
        mainCache.set[i] = mainSet;
        //initializing them as 0
        for(j=0; j<E; j++)
        {
            mainLine.tagBit = 0ULL;
            mainLine.validBit = 0ULL;
            mainLine.isFull = 0ULL;
            mainLine.lru = 0ULL;
            mainSet.line[j] = mainLine;
        }
    }
}

//opening text file
void openTrace()
{
    Address address;
    char mode;
    unsigned long long int size;
    FILE* stream = fopen(t, "r");
    //while there are 3 successful scans in the text file
    while(fscanf(stream, " %c %llx,%lld", &mode, &address, &size) == 3)
    {
    	//insert the address with the specified mode
        if(mode == 'L' || mode == 'M' || mode == 'S')
            insert(mode, address, size);
    }
    //close file
    fclose(stream);
}

//inserting address to cache
void insert(char mode, Address address, unsigned long long int size)
{
    unsigned long long int i, j;
    //getting set index from address
    unsigned long long int setIndex = getSet(address);
    //getting tag bit
    unsigned long long int tag = address >> (b+s);
    Line temp;
    printf("%c %llx, %lld ", mode, address, size);
    for(i=0; i<E; i++)
    {
    	//if one of the lines is empty, there will be a miss
        if(mainCache.set[setIndex].line[i].isFull == 0)
        {
            printf("miss ");
            //initializing parameters
            temp.tagBit = tag;
            temp.validBit = 1ULL;
            temp.isFull = 1ULL;
            mainCache.set[setIndex].line[i] = temp;
            misses++;
            //end loop
            break;
        }
        //if tag bit of address and tag bit of the line are equal to each other and line is valid and full, there will be a hit
        else if(mainCache.set[setIndex].line[i].tagBit == tag && mainCache.set[setIndex].line[i].isFull == 1 && mainCache.set[setIndex].line[i].validBit == 1)
        {
            printf("hit ");
            //making it's lru 0 because it's the last used
            mainCache.set[setIndex].line[i].lru = 0ULL;
            hits++;
            //end loop directly
            break;
        }
        //if there is no empty line and there is no hit, there must be a miss eviction
        else if(i == E-1 && mainCache.set[setIndex].line[i].isFull == 1)
        {
            printf("miss eviction ");
            //initializing parameters
            temp.tagBit = tag;
            temp.validBit = 1ULL;
            temp.isFull = 1ULL;
            //making lru 0 because it's the last used
            temp.lru = 0ULL;
            //get the least recently used index of line and write the line istead of it
            mainCache.set[setIndex].line[findLRUIndex(mainCache.set[setIndex].line)] = temp;
            evictions++;
            misses++;
            //end loop
            break;
        }
    }
    for(j=0; j<E; j++)
    {
    	//for using lru, increment every filled lines' lru.
        if(mainCache.set[setIndex].line[j].isFull == 1)
            mainCache.set[setIndex].line[j].lru++;
    }
    //if address wanted to be modified, there will be one/one more hit
    if(mode == 'M')
    {
        printf("hit");
        hits++;
    }
    printf("\n");
}

//my program increments lru variable of a line if this line is not used, if it's used, it's lru will be 0.
//this function gives us the least recently used line's index.
//for example if there are 8 lines, least recently used line's lru variable will be 7.
unsigned long long int findLRUIndex(Line* lines)
{
    unsigned long long int i;
    for(i=0; i<E; i++)
        if(lines[i].lru == E-1)
            return i;
    return 0;
}

//freeing pointers
void freeCache()
{
	free(mainSet.line);
	free(mainCache.set -> line);
	free(mainCache.set);
}