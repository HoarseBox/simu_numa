#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string> 
#include <sys/mman.h>
#include <sys/types.h>
#include <err.h>
#include <stdlib.h>

#include <chrono>
#include <ctime>
#include <ratio>
using namespace std;
 
struct tidAndAddr{
	int ID;
	int coreId;
	int* addr1;
};



void* DoWork(void* args){
	// access data according the core number
	struct tidAndAddr* p = (struct tidAndAddr*)args;
	int TID = p->ID;
	int coreId = p->coreId;
	if (coreId >= 4) {
		cout << "sleeping...\n";
		usleep(5000);
	}

	int* addr1 = p->addr1;
	for (int i=0; i<10000; i++){
		 addr1[rand()%65535] = addr1[rand()%65535]+1;
	}
	return 0;
}


int main(int argc, char** argv){
	int coreNum1=0;
	int coreNum2=4;

	// threads that we want to map	
	pthread_t thread;
	pthread_attr_t attr;
	cpu_set_t cpus;
	// create a bunch of threads randomly distributed them on different cores.
	struct tidAndAddr p;
	struct tidAndAddr p1;

	int* tmp;
	tmp = new int[100000];
	for (int i=0; i<100000; i++){
		tmp[i] = i;
	}

	p.addr1 = tmp;
	p.ID = 0;
	p1.addr1 = tmp;
	p1.ID = 1;

	
	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	p.coreId = coreNum1;
	CPU_SET(p.coreId, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&thread, &attr, DoWork, (void*)&p);

	pthread_join(thread, NULL);

	auto start = chrono::high_resolution_clock::now();
	

	pthread_attr_init(&attr);
	CPU_ZERO(&cpus);
	p1.coreId = coreNum2;
	CPU_SET(p1.coreId, &cpus);
	pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
	pthread_create(&thread, &attr, DoWork, (void*)&p1);

	pthread_join(thread, NULL);
		
	auto end = chrono::high_resolution_clock::now();
	std::chrono::duration<double> diff = end - start;
	cout<<"It took me "<<diff.count()<<"seconds."<<endl;
	
	return 0;
}
