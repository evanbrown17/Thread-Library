#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#include <cstring>

//call wait/signal without having any lock
//
using namespace std;

int g = 0;

unsigned cv = 22;

void loop(void* a) {
	char* id = (char*) a;
	cout << "loop called with id " << id << endl;
	
	for (int i = 0; i < 5; i++, g++) {
		unsigned lock = (unsigned) i;
		//thread_lock(lock);
		if (!strcmp(id, "parent thread")) {
			cout << "parent thread waiting on lock " << lock << endl;
			if(thread_wait(lock, cv)) {
				cout << "wait failed\n";
			}
		} else {
			cout << "child thread signaling on lock " << lock << endl;
			if(thread_signal(lock, cv)) {
				cout << "thread failed\n";
			}
		}
		cout << id << ":\t" << i << "\t" << g << endl;
		//thread_unlock(lock);
		if (thread_yield()) {
			cout << "thread_yield failed\n";
			
		}
	}
}


void parent(void* a) {
	int arg = (intptr_t) a;
	cout << "parent called with arg " << arg << endl;

	if (thread_create((thread_startfunc_t) loop, (void*) "child thread")) {
		cout << "thread_create failed\n";
	
	}

	loop((void*) "parent thread");
}

int main() {
	if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
		cout << "thread_libinit failed\n";
		
	}
}

