#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"
#include <queue>

using namespace std;

int g = 0; // global (shared by all threads)

#define lock 1
#define cv1 2
#define cv2 3
static queue<int> globalQueue;
#define max_size 4

bool done = false;

void add(int x) {
	thread_lock(lock);

	while(globalQueue.size() >= max_size) {
		if (thread_wait(lock, cv2)) {
			cout << "thread_wait failed\n";
			exit(1);
		}
	}

	globalQueue.push(x);
	cout << "added " << x << endl;
	thread_signal(lock, cv1);

	thread_unlock(lock);
}

int remove() {
	thread_lock(lock);

	while(globalQueue.empty()) {
		if (thread_wait(lock, cv1)) {
			cout << "thread_wait failed\n";
		}
	}

	thread_yield();

	int result = globalQueue.front();
	globalQueue.pop();
	cout << "removed " << result << endl;
	
	if (thread_signal(lock, cv2)) {
		cout << "Thread signal failed.\n";
	}

	if (thread_unlock(lock)) {
		cout << "Thread unlock failed.\n";
	}

	return result;
}

void producer(void* a) {
    char* id = (char*) a;
    cout << "producer called with id " << id << endl;

    while (g < 10) {
	    add(g);
	    g++;
	    thread_yield();
    }

    done = true;

    cout << "Done adding\n";
}

void consumer(void* a) {
	char* id = (char*) a;
	cout << "consumer called with id " << id << endl;

    	while (!done || !globalQueue.empty()) {
	    int removed_item = remove();
	    removed_item++;
    	}

	thread_yield();

	cout << "Done removing\n";
}


void parent(void* a) {
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    if (thread_create((thread_startfunc_t) producer, (void*) "producer")) {
      cout << "thread_create failed\n";
    }

    if (thread_create((thread_startfunc_t) consumer, (void*) "consumer")) {
      cout << "thread_create failed\n";
    }

}

int main() {

    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
    }

    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
    }

}

