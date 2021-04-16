#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#include <cstring>

//call signal without having initialized CV
//
using namespace std;

int g = 0;

unsigned lock = 22;

void loop(void* a) {
	char* id = (char*) a;
	cout << "loop called with id " << id << endl;
	
	for (int i = 0; i < 5; i++, g++) {
		unsigned cv = (unsigned) g;
		thread_lock(lock);
		cout << "thread signaling on cv " << cv << endl;
		if (thread_signal(lock, cv)) {
			cout << "Signal failed" << endl;
		}
		
		cout << id << ":\t" << i << "\t" << g << endl;
		thread_unlock(lock);
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
		exit(1);
	}
}

