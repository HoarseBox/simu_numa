#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string> 
#include <sys/mman.h>
#include <sys/types.h>
#include <err.h>

#include <chrono>
#include <ctime>
#include <ratio>
using namespace std;
 
struct tidAndAddr{
	int ID;
	int* addr1;
	int* addr2;
};

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* DoWork(void* args){
	// access data according the core number
	struct tidAndAddr* p = (struct tidAndAddr*)args;
	int TID = p->ID;
	int* addr1 = p->addr1;
	int* addr2 = p->addr2;
	if (TID>500){
		for (int i=0; i<10000; i++){
			pthread_mutex_lock(&mutex1);
			addr1[rand()%65535] = addr1[rand()%65535]+1;
			pthread_mutex_unlock(&mutex1);
		}
	}
	else{
		for (int i=0; i<10000; i++){
			pthread_mutex_lock(&mutex2);
			addr2[rand()%65535] = addr2[rand()%65535]+1;
			pthread_mutex_unlock(&mutex2);
		}
	}
	

	return 0;
}


int main(int argc, char** argv){
	int coreNum1=0;
	int coreNum2=1;

	// threads that we want to map	
	int NumThreads = 1000;
	pthread_t threads[NumThreads];
	pthread_attr_t attr;
	cpu_set_t cpus;
	// create a bunch of threads randomly distributed them on different cores.
	struct tidAndAddr p[NumThreads];

	int* tmp1; 
	int* tmp2;
	tmp1 = new int[100000];
	tmp2 = new int[100000];

	for (int i=0; i<100000; i++){
		tmp1[i] = i;
	}

	for (int i=0; i<100000; i++){
		tmp2[i] = i;
	}

	for (int i=0; i<NumThreads; i++){
		p[i].addr1 = tmp1;
		p[i].addr2 = tmp2;
		p[i].ID = i;
	}
	
	// in the first scenario, the thread 1,3,5,7,9... 
	// are put on one node
	// thread 2,4,6,8,10 are put on another core
	auto start = chrono::high_resolution_clock::now();
	
	for (int i=0; i<NumThreads; i++){
		pthread_attr_init(&attr);
		CPU_ZERO(&cpus);
		CPU_SET(i, &cpus);
		int threadNum = i;
		if (i%2==0){
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(i % 8, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		}
		else{
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(i % 8 + 8, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		}
	}
	for (int i=0; i<NumThreads; i++){
		pthread_join(threads[i], NULL);
	}

	auto end = chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;
	cout<<"It took me "<<diff.count()<<"seconds."<<endl;

	start = chrono::high_resolution_clock::now();
	
	for (int i=0; i<NumThreads; i++){
		pthread_attr_init(&attr);
		CPU_ZERO(&cpus);
		int threadNum = i;
		if (i<500){
			CPU_SET(i % 8, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		}
		else{
			CPU_SET(i % 8 + 8, &cpus);
			pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
			pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);
		}

	}
	for (int i=0; i<NumThreads; i++){
		pthread_join(threads[i], NULL);
	}

	end = chrono::high_resolution_clock::now();
	diff = end - start;
	cout<<"It took me "<<diff.count()<<"seconds."<<endl;

	return 0;
}
