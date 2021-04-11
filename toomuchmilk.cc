#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"

using namespace std;

#define LOCK 1
bool noMilk = true;

void milk(void* caller){
	char* thread = (char*) caller;
	cout << "Milk called with " << thread << endl;
	if (thread_lock(LOCK)){
		cout << "Acquiring lock failed.\n";
		exit(1);
	}
	cout << thread << " acquired lock!\n";
	if (noMilk) {
		cout << thread << " buying milk!" << endl;
		thread_yield();
		noMilk = false;
	}
	if (thread_unlock(LOCK)) {
		cout << "Acquiring lock failed.\n";
		exit(1);
	}

	cout << thread << " released lock!\n";
	cout << thread << " exiting." << endl;

}

void parent(void* a) {
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    thread_yield();

    if (thread_create((thread_startfunc_t) milk, (void*) "thread A")) {
      cout << "thread_create failed\n";
      exit(1);
    }

    if (thread_create((thread_startfunc_t) milk, (void*) "thread B")) {
      cout << "thread_create failed\n";
      exit(1);
    }

    cout << "Parent thread exiting\n";

}


int main() {
    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
      exit(1);
    }
}

