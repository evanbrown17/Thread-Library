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
		thread_wait(cv2, lock);
	}

	globalQueue.push(x);
	cout << "added " << x << endl;
	thread_signal(cv1, lock);

	thread_unlock(lock);
}

int remove() {
	thread_lock(lock);

	while(globalQueue.empty()) {
		thread_wait(cv1, lock);
	}

	int result = globalQueue.front();
	globalQueue.pop();
	cout << "removed " << result << endl;
	thread_signal(cv2, lock);

	thread_unlock(lock);

	return result;
}

void producer(void* a) {
    char* id = (char*) a;
    cout << "produce called with id " << id << endl;

    while (g < 10) {
	    add(g);
	    g++;
    }

    done = true;

    cout << "Done adding\n";
}

void consumer(void* a) {
	char* id = (char*) a;
	cout << "produce called with id " << id << endl;

    	while (!done) {
	    int removed_item = remove();
	    removed_item++;
    	}

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

