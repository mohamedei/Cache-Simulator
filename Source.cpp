#include <iostream>
#include <iomanip>
#include <math.h>
#include <vector>
struct cache { int v; int tag; };
using namespace std;
#define DBG 1
#define DRAM_SIZE (64*1024*1024)
#define CACHE_SIZE (64*1024)
#define LineSize 32
#define SetSize 4
int counter = 0;
int timer = 0;
bool fullcache = 0;
enum mode { DMC = 0, FAC = 1, SAC = 2 };
enum pol { LRU = 0, LFU = 1, FIFO = 2, RAND = 3 };
enum cacheResType { MISS = 0, HIT = 1 };
mode CacheMode = SAC;
pol policy = LRU;
unsigned int m_w = 0xABABAB55; /* must not be zero, nor 0x464fffff
*/
unsigned int m_z = 0x05080902; /* must not be zero, nor 0x9068ffff
*/
void createFAC(vector <cache> &A, int &tag, int &offset, int &size);
void createDMC(vector <cache> &A, int &tag, int &offset, int &size,
int &index);
void createSAC(vector <vector<cache>> & A, int &tag, int &offset, int
&size, int &index);
unsigned int rand_()
{
m_z = 36969 * (m_z & 65535) + (m_z >> 16);
m_w = 18000 * (m_w & 65535) + (m_w >> 16);
return (m_z << 16) + m_w; /* 32-bit result */
}
unsigned int memGen1()
{
static unsigned int addr = 0;
return (addr++) % (DRAM_SIZE);
}
unsigned int memGen2()
{
static unsigned int addr = 0;
return rand_() % (128 * 1024);
}
unsigned int memGen3()
{
return rand_() % (DRAM_SIZE);
}
unsigned int memGen4()
{
static unsigned int addr = 0;
return (addr++) % (1024);
}
unsigned int memGen5()
{
static unsigned int addr = 0;
return (addr++) % (1024 * 64);
}
unsigned int memGen6()
{
static unsigned int addr = 0;
return (addr += 256) % (DRAM_SIZE);
}
// Cache Simulator
cacheResType cacheSim(unsigned int addr, vector <cache> &A, vector
<vector<cache>> &C, int &tag, int &offset, int &size, int &index, int*
B)
{
int addoffset, addtag, addindex, temp, seti;
bool full;
// This function accepts the memory address for the read and
// returns whether it caused a cache miss or a cache hit
// The current implementation assumes there is no cache; so,
every transaction is a miss
if (CacheMode == DMC)
{
addoffset = (addr << (32 - offset)) >> (32 - offset);
addtag = addr >> (offset + index);
addindex = ((addr >> offset) << (offset + tag)) >>
(tag + offset);
if (A[addindex].v == 0)
{
A[addindex].v = 1;
A[addindex].tag = addtag;
return MISS;
}
else if (addtag == A[addindex].tag)
return HIT;
else
{
A[addindex].v = 1;
A[addindex].tag = addtag;
return MISS;
}
}
if (CacheMode == FAC)
{
if (policy == FIFO)
{
addoffset = (addr << (32 - offset)) >> (32 -
offset);
addtag = addr >> offset;
for (int i = 0; i < size; i++)
{
if (A[i].v == 1 && A[i].tag == addtag)
return HIT;
}
if (fullcache == 0)
{
for (int i = 0; i < size; i++)
{
if (A[i].v == 0)
break;
if (i == (size - 1))
fullcache = 1;
}
}
if (fullcache == 1)
{
A[counter%size].tag = addtag;
counter++;
return MISS;
}
else
{
A[counter].v = 1;
A[counter].tag = addtag;
counter++;
return MISS;
}
}
if (policy == RAND)
{
addoffset = (addr << (32 - offset)) >> (32 -
offset);
addtag = addr >> offset;
for (int i = 0; i < size; i++)
{
if (A[i].v == 1 && A[i].tag == addtag)
return HIT;
}
if (fullcache == 0)
{
for (int i = 0; i < size; i++)
{
if (A[i].v == 0)
break;
if (i == (size - 1))
fullcache = 1;
}
}
if (fullcache == 1)
{
A[rand_() % (size)].tag = addtag;
return MISS;
}
else
{
A[counter].v = 1;
A[counter].tag = addtag;
counter++;
return MISS;
}
}
if (policy == LRU)
{
addoffset = (addr << (32 - offset)) >> (32 -
offset);
addtag = addr >> offset;
for (int i = 0; i < size; i++)
{
if (A[i].v == 1 && A[i].tag == addtag)
{
B[i] = timer;
timer++;
return HIT;
}
}
if (fullcache == 0)
{
for (int i = 0; i < size; i++)
{
if (A[i].v == 0)
break;
if (i == (size - 1))
fullcache = 1;
}
}
if (fullcache == 1)
{
temp = 0;
for (int i = 0; i < size; i++)
if (B[i] < B[temp])
{
temp = i;
}
A[temp].tag = addtag;
B[temp] = timer;
timer++;
return MISS;
}
else
{
A[counter].v = 1;
A[counter].tag = addtag;
B[counter] = timer;
counter++;
timer++;
return MISS;
}
}
if (policy == LFU)
{
addoffset = (addr << (32 - offset)) >> (32 -
offset);
addtag = addr >> offset;
for (int i = 0; i < size; i++)
{
if (A[i].v == 1 && A[i].tag == addtag)
{
B[i]++;
return HIT;
}
}
if (fullcache == 0)
{
for (int i = 0; i < size; i++)
{
if (A[i].v == 0)
break;
if (i == (size - 1))
fullcache = 1;
}
}
if (fullcache == 1)
{
temp = 0;
for (int i = 0; i < size; i++)
if (B[i] < B[temp])
{
temp = i;
}
A[temp].tag = addtag;
B[temp] = 0;
return MISS;
}
else
{
A[counter].v = 1;
A[counter].tag = addtag;
B[counter] = 0;
counter++;
return MISS;
}
}
}
if (CacheMode == SAC)
{
addoffset = (addr << (32 - offset)) >> (32 - offset);
addtag = addr >> (offset + index);
addindex = ((addr >> offset) << (offset + tag)) >>
(tag + offset);
for (int i = 0; i < SetSize; i++)
{
if (C[addindex][i].v ==1 && C[addindex][i].tag
== addtag)
return HIT;
}
full = 1;
for(int i=0;i<SetSize;i++)
if (C[addindex][i].v == 0)
{
full = 0;
break;//
}
if (full)
{
seti = rand() % SetSize;
C[addindex][seti].tag = addtag;
return MISS;
}
for(int i=0;i<SetSize;i++)
if (C[addindex][i].v == 0)
{
C[addindex][i].v = 1;
C[addindex][i].tag = addtag;
return MISS;
}
}
return MISS;
}
char *msg[2] = { "Miss","Hit" };
int main()
{
int*B;
int counthit = 0;
int misscount = 0;
int inst = 0;
cacheResType r;
vector <cache> A;
vector <vector<cache>> C;
int tag, index, size, offset;
unsigned int addr;
if (CacheMode == DMC)
{
createDMC(A, tag, offset, size, index);
}
else
{
if (CacheMode == FAC)
{
createFAC(A, tag, offset, size);
}
else
createSAC(C, tag, offset, size, index);
}
B = new int[size];
cout << "Cache Simulator\n";
// change the number of iterations into 10,000,000
for (; inst < 100000; inst++)
{
addr = memGen6();
r = cacheSim(addr, A, C, tag, offset, size, index, B);
if (r == HIT)
counthit++;
else
misscount++;
cout << "0x" << setfill('0') << setw(8) << hex << addr
<< " (" << msg[r] << ")\n";
}
cout << "Hit Ratio: " << (float(counthit) / float((counthit +
misscount))) * 100 << "%\n";
delete[] B;
system("pause");
}
void createFAC(vector <cache> &A, int &tag, int &offset, int &size) //
caches are ints needs to change
{
offset = log2(LineSize);
tag = 32 - offset;
size = CACHE_SIZE / (4 + LineSize);
A.resize(size);
for (int i = 0; i < size; i++)
{
A[i].v = 0;
}
}
void createDMC(vector <cache> &A, int &tag, int &offset, int &size,
int &index)
{
size = CACHE_SIZE / (4 + LineSize);
index = log2(size);
offset = log2(LineSize);
tag = 32 - index - offset;
A.resize(size);
for (int i = 0; i < size; i++)
{
A[i].v = 0;
}
}
void createSAC(vector <vector<cache>> &A, int &tag, int &offset, int
&size, int &index)
{
size = CACHE_SIZE / SetSize*(4 + LineSize);
index = log2(size);
offset = log2(LineSize);
tag = 32 - index - offset;
A.resize(size);
for (int i = 0; i < size; i++)
{
A[i].resize(SetSize);
}
for (int i = 0; i < size; i++)
for (int j = 0; j < SetSize; j++)
A[i][j].v = 0;
}
