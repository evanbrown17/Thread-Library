#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#include <cstring>

//will exploit programs that are not checking which lock a CV corresponds to
//
using namespace std;

int g = 0;

//unsigned cv = 22;

void loop(void* a) {
	char* id = (char*) a;
	cout << "loop called with id " << id << endl;
	
	for (int i = 0; i < 5; i++, g++) {
		unsigned lock = (unsigned) i;
		thread_lock(lock);
		//if (!strcmp(id, "parent thread")) {
		//	cout << "parent thread waiting on lock " << lock << endl;
		//	thread_wait(lock, cv);
		//} else {
		//	cout << "child thread signaling on lock " << lock << endl;
		//	thread_signal(lock, cv);
	//	}
		cout << id << ":\t" << i << "\t" << g << endl;
		if (thread_lock(lock)) {
			cout << "We failed to doubly-acquire this lock: " << lock << endl;
		} else {
			cout << "We succeeded in doubly-acquiring this lock: " << lock << endl;
		}
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
	}
}
