#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string> 
#include <sys/mman.h>
#include <sys/types.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <ctime>
#include <ratio>

#include <algorithm>
#include <vector>

using namespace std;
 
struct tidAndAddr{
  int ID;
  int* addr1;
  int* addr2;
};

struct superSet{
  tidAndAddr* A;
};

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* func1(void* args){
  struct tidAndAddr* p = (struct tidAndAddr*)args;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  int a;
  for (int i=0; i<10000; i++){
    a = addr1[rand()%65535];
  }
  return 0;
}

void* func2(void* args){
  struct tidAndAddr* p = (struct tidAndAddr*)args;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  for (int i=0; i<5000; i++){
    pthread_mutex_lock(&mutex1);
    addr1[rand()%65535] = addr1[rand()%65535]+1;
    pthread_mutex_unlock(&mutex1);
  }
  return 0;
}

void* func3(void* args){
  struct superSet* ss = (struct superSet*)args;
  struct tidAndAddr* p = ss->A;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  for (int i=2000; i<10000; i++){
    pthread_mutex_lock(&mutex1);
    addr1[rand()%65535] = addr1[rand()%65535]+1;
    pthread_mutex_unlock(&mutex1);
  }
  return 0;
}

void* func4(void* args){
  struct tidAndAddr* p = (struct tidAndAddr*)args;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  int a;
  for (int i=0; i<5000; i++){
    a = addr1[rand()%65535];
  }
  return 0;
}

void* func5(void* args){
  struct tidAndAddr* p = (struct tidAndAddr*)args;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  int a;
  for (int i=5000; i<10000; i++){
    a = addr1[rand()%65535];
  }
  return 0;
}

void* func6(void* args){
  struct tidAndAddr* p = (struct tidAndAddr*)args;
  int TID = p->ID;
  int* addr1 = p->addr1;
  int* addr2 = p->addr2;
  for (int i=0; i<5000; i++){
    pthread_mutex_lock(&mutex2);
    addr2[rand()%65535] = addr2[rand()%65535]+1;
    pthread_mutex_unlock(&mutex2);
  }
  return 0;
}

int main(int argc, char** argv){
  pthread_t threadA[17];
  pthread_t threadB[24];
  pthread_t threadC[7];
  pthread_attr_t attr;
  cpu_set_t cpus;
  // create a bunch of threads randomly distributed them on different cores.
  struct tidAndAddr gAArg;
  struct superSet gAArgSuper;
  struct tidAndAddr gBArg;
  struct tidAndAddr gCArg1;
  struct tidAndAddr gCArg2;

  int* tmpA; 
  int* tmpB;
  int* tmpC;
  tmpA = new int[100000];
  tmpB = new int[100000];
  tmpC = new int[100000];

  for (int i=0; i<100000; i++){
    tmpA[i] = i;
    tmpB[i] = i;
    tmpC[i] = i;
  }

  gAArg.addr1 = tmpA;
  gAArg.addr2 = NULL;
  gAArg.ID = 0;
  gAArgSuper.A = &gAArg;

  gBArg.addr1 = tmpB;
  gBArg.addr2 = NULL;

  gCArg1.addr1 = NULL;
  gCArg1.addr2 = tmpC;
  gCArg2.addr1 = NULL;
  gCArg2.addr2 = tmpC;

  int totalCases = 27;
  std::vector<int> executeOrder;
   for (int i = 0; i < totalCases; ++i) 
    executeOrder.push_back(i);
  std::random_shuffle( executeOrder.begin(), executeOrder.end() );

  auto start = chrono::high_resolution_clock::now();

  for (int i = 0; i < totalCases; ++i) {
    switch (executeOrder[i]) {

      case 0:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[0], &attr, func1, (void*)&gAArg);
        break;

      case 1:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[1], &attr, func1, (void*)&gAArg);
        break;

      case 2:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[2], &attr, func1, (void*)&gAArg);
        break;

      case 3:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[3], &attr, func1, (void*)&gAArg);
        break;

      case 4:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[4], &attr, func1, (void*)&gAArg);
        break;
      // ====================================================================
      case 5:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[5], &attr, func2, (void*)&gAArg);
        break;

      case 6:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[6], &attr, func2, (void*)&gAArg);
        break;

      case 7:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[7], &attr, func2, (void*)&gAArg);
        break;

      case 8:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[8], &attr, func2, (void*)&gAArg);
        break;

      case 9:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[9], &attr, func2, (void*)&gAArg);
        break;
      // ====================================================================
      case 10:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[10], &attr, func3, (void*)&gAArgSuper);
        break;

      case 11:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[11], &attr, func3, (void*)&gAArgSuper);
        break;

      case 12:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[12], &attr, func3, (void*)&gAArgSuper);
        break;

      case 13:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[13], &attr, func3, (void*)&gAArgSuper);
        break;

      case 14:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[14], &attr, func3, (void*)&gAArgSuper);
        break;

      case 15:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[15], &attr, func3, (void*)&gAArgSuper);
        break;

      case 16:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadA[16], &attr, func3, (void*)&gAArgSuper);
        break;

      // =================================================================== B 
      case 17:
        for (int i = 0; i < 6; ++i){
          pthread_attr_init(&attr);
          CPU_ZERO(&cpus);
          CPU_SET(rand()%32, &cpus);
          pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
          pthread_create(&threadB[i], &attr, func4, (void*)&gBArg);
        }
        break;
      
      case 18:
        for (int i = 6; i < 18; ++i){
          if (i % 2 == 0) {
            pthread_attr_init(&attr);
            CPU_ZERO(&cpus);
            CPU_SET(rand()%32, &cpus);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
            pthread_create(&threadB[i], &attr, func4, (void*)&gBArg);
          } else if (i % 2 == 1){
            pthread_attr_init(&attr);
            CPU_ZERO(&cpus);
            CPU_SET(rand()%32, &cpus);
            pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
            pthread_create(&threadB[i], &attr, func5, (void*)&gBArg);
          } 
        }
        break;

      case 19:
        for (int i = 18; i < 24; ++i){
          pthread_attr_init(&attr);
          CPU_ZERO(&cpus);
          CPU_SET(rand()%32, &cpus);
          pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
          pthread_create(&threadB[i], &attr, func5, (void*)&gBArg);
        }
        break;
      // =================================================================== C

      case 20:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[0], &attr, func6, (void*)&gCArg1);
        break;

      case 21:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[1], &attr, func6, (void*)&gCArg1);
        break;

      case 22:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[2], &attr, func6, (void*)&gCArg1);
        break;

      case 23:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[3], &attr, func6, (void*)&gCArg2);
        break;

      case 24:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[4], &attr, func6, (void*)&gCArg2);
        break;

      case 25:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[5], &attr, func6, (void*)&gCArg2);
        break;

      case 26:
        pthread_attr_init(&attr);
        CPU_ZERO(&cpus);
        CPU_SET(rand()%32, &cpus);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&threadC[6], &attr, func6, (void*)&gCArg2);
        break;
      }
  }

  for (int i=0; i<17; i++){
    if (i < 17) {
      pthread_join(threadA[i], NULL);
    }
    if (i < 7){
      pthread_join(threadC[i], NULL);
    }
    pthread_join(threadB[i], NULL);
  }

  auto end = chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  cout<<"It took me "<<diff.count()<<"seconds."<<endl;

  return 0;
}
