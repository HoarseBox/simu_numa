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

void* groupA1(void* args){
	// access data according the core number
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

void* groupA2(void* args){
	// access data according the core number
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

void* groupA3(void* args){
	// access data according the core number
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

void* groupB1(void* args){
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

void* groupB2(void* args){
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

void* groupC(void* args){
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


	auto start = chrono::high_resolution_clock::now();

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[0], &attr, groupA1, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(1, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[1], &attr, groupA1, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(2, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[2], &attr, groupA1, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(8, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[3], &attr, groupA1, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(9, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[4], &attr, groupA1, (void*)&gAArg);
// ====================================================================
	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(12, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[5], &attr, groupA2, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(13, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[6], &attr, groupA2, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(14, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[7], &attr, groupA2, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(15, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[8], &attr, groupA2, (void*)&gAArg);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(16, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[9], &attr, groupA2, (void*)&gAArg);
// ====================================================================
	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(22, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[10], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(23, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[11], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(24, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[12], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(25, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[13], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(30, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[14], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(31, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[15], &attr, groupA3, (void*)&gAArgSuper);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(1, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadA[16], &attr, groupA3, (void*)&gAArgSuper);

	// =================================================================== B 
	for (int i = 0; i < 24; ++i){
		if (i % 4 == 0) {
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(0, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threadB[i], &attr, groupB1, (void*)&gBArg);
		} else if (i % 4 == 1){
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(8, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threadB[i], &attr, groupB1, (void*)&gBArg);
		} else if (i % 4 == 2){
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(0, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threadB[i], &attr, groupB2, (void*)&gBArg);
		} else {
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(17, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threadB[i], &attr, groupB2, (void*)&gBArg);
		}
	}
	// =================================================================== C

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[0], &attr, groupC, (void*)&gCArg1);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(8, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[1], &attr, groupC, (void*)&gCArg1);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(19, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[2], &attr, groupC, (void*)&gCArg1);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(0, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[3], &attr, groupC, (void*)&gCArg2);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(25, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[4], &attr, groupC, (void*)&gCArg2);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(19, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[5], &attr, groupC, (void*)&gCArg2);

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	CPU_SET(31, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&threadC[6], &attr, groupC, (void*)&gCArg2);


	for (int i=0; i<17; i++){
		if (i < 17) {
			pthread_join(threadA[i], NULL);
		}
		if (i < 7){
			pthread_join(threadC[i], NULL);
		}
		pthread_join(threadB[i], NULL);
	}

	
//	for (int j=0; j<50; j++){
	

		// for (int i=0; i<NumThreads/2; i++){
		// 	pthread_attr_init(&attr);
		// 	CPU_ZERO(&cpus);
		// 	CPU_SET(i, &cpus);
		// 	int threadNum = i;
		// 	if (i%2==0){
		// 		pthread_attr_init(&attr);
		// 		CPU_ZERO(&cpus);
		// 		p[i].coreId = i%8;
		// 		CPU_SET(p[i].coreId, &cpus);
		// 		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		// 		pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		// 	}
		// 	else{
		// 		pthread_attr_init(&attr);
		// 		CPU_ZERO(&cpus);
		// 		p[i].coreId = i%8;
		// 		CPU_SET(p[i].coreId, &cpus);
		// 		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		// 		pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		// 	}
		// }
		// for (int i=NumThreads/2; i<NumThreads; i++){
		// 	pthread_attr_init(&attr);
		// 	CPU_ZERO(&cpus);
		// 	CPU_SET(i, &cpus);
		// 	int threadNum = i;
		// 	if (i%2==0){
		// 		pthread_attr_init(&attr);
		// 		CPU_ZERO(&cpus);
		// 		p[i].coreId = i%8;
		// 		CPU_SET(p[i].coreId, &cpus);
		// 		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		// 		pthread_create(&threads[i], &attr, DoWork1, (void*)&p[i]);
		// 	}
		// 	else{
		// 		pthread_attr_init(&attr);
		// 		CPU_ZERO(&cpus);
		// 		p[i].coreId = i%8;
		// 		CPU_SET(p[i].coreId, &cpus);
		// 		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		// 		pthread_create(&threads[i], &attr, DoWork1, (void*)&p[i]);
		// 	}
		// }

		// for (int i=0; i<NumThreads; i++){
		// 	pthread_join(threads[i], NULL);
		// }
//	}

	auto end = chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;
	cout<<"It took me "<<diff.count()<<"seconds."<<endl;

//	auto start = chrono::high_resolution_clock::now();
//	for (int i=0; i<NumThreads; i++){
//		pthread_attr_init(&attr);
//		CPU_ZERO(&cpus);
//		CPU_SET(i+7, &cpus);
//		int threadNum = i;
//		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
//		pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
//	}

//	for (int i=0; i<NumThreads; i++){
//		pthread_join(threads[i], NULL);
//	}
	
//	auto end = chrono::high_resolution_clock::now();
//	std::chrono::duration<double> diff = end - start;
//	cout<<"It took me "<<diff.count()<<"seconds."<<endl;

	return 0;
}