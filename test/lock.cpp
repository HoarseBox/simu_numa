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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* DoWork(void* args){
	// access data according the core number
	struct tidAndAddr* p = (struct tidAndAddr*)args;
	int TID = p->ID;
	int* addr1 = p->addr1;
	int* addr2 = p->addr2;
	for (int i=0; i<10000; i++){
		pthread_mutex_lock(&mutex);
		addr1[rand()%65535] = addr1[rand()%65535]+1;
		pthread_mutex_unlock(&mutex);
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

	auto start = chrono::high_resolution_clock::now();
	
	for (int i=0; i<NumThreads; i++){
		pthread_attr_init(&attr);
		CPU_ZERO(&cpus);
		CPU_SET(i, &cpus);
		int threadNum = i;
		if (i%2==0){
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(i%8, &cpus);
		}
		else{
			pthread_attr_init(&attr);
			CPU_ZERO(&cpus);
			CPU_SET(i%8+8, &cpus);
		}
		pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
		pthread_create(&threads[i], &attr, DoWork, (void*)&p[i]);

	}
	for (int i=0; i<NumThreads; i++){
		pthread_join(threads[i], NULL);
	}

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
