#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#include <cstring>

//threads will try to unlock each other's locks
//
using namespace std;

int g = 0;

//unsigned cv = 22;

void loop(void* a) {
	char* id = (char*) a;
	cout << "loop called with id " << id << endl;
	
	for (int i = 0; i < 5; i++, g++) {
		unsigned lock = (unsigned) i;
		cout << id << ":\t" << i << "\t" << g << endl;
		if (thread_unlock(lock)) {
			cout << "We failed to unlock this lock: " << lock << endl;
		} else {
			cout << "We succeeded in unlocking this lock: " << lock << endl;
		}
		thread_lock(lock);	
		if (thread_yield()) {
			cout << "thread_yield failed\n";
			exit(1);
		}
	}
}


void parent(void* a) {
	int arg = (intptr_t) a;
	cout << "parent called with arg " << arg << endl;
	if (thread_create((thread_startfunc_t) loop, (void*) "child thread")) {
		cout << "thread_create failed\n";
		exit(1);
	}
	
	loop((void*) "parent thread");
}

int main() {
	if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
		cout << "thread_libinit failed\n";
		exit(1);
	}
}
