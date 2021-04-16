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
#define max_size 1

bool done = false;

void add(int x, char* id) {

	cout << "add called by " << id << endl;
	thread_lock(lock);

	bool waiting = false;

	if (globalQueue.size() >= max_size) {
		waiting = true;
		cout << id << " going to wait on cv2 because queue is full.\n";
	}

	while(globalQueue.size() >= max_size) {
		if (thread_wait(lock, cv2)) {
			cout << "thread_wait failed\n";
			exit(1);
		}
	}

	if (waiting) { //if the thread had to wait, print that it has now been signaled
		cout << id << " was signaled on cv2 because queue is no longer full.\n";
	}

	globalQueue.push(x);
	cout << "added " << x << endl;
	thread_signal(lock, cv1);

	thread_unlock(lock);
}

int remove(char* id) {

	cout << "remove called by " << id << endl;
	thread_lock(lock);

	if (globalQueue.empty()) {
		cout << id << " going to wait on cv1 because the queue is empty.\n";
	}

	while(globalQueue.empty()) {
		if (thread_wait(lock, cv1)) {
			cout << "thread_wait failed\n";
		}
	}


	int result = globalQueue.front();
	globalQueue.pop();
	cout << "removed " << result << endl;
	
	if (thread_signal(lock, cv2)) {
		cout << "Thread signal failed.\n";
	} else {
		cout << id << " successfully signaled.\n";
	}

	if (thread_unlock(lock)) {
		cout << "Thread unlock failed.\n";
	}

	return result;
}

void producer(void* a) {
    char* id = (char*) a;
    cout << "producer called with id " << id << endl;

    while (g < 5) {
	    add(g, id);
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
	    int removed_item = remove(id);
	    removed_item++; //not necessary - just to avoid the error of removed_item not being used again
	    thread_yield();
    	}

	thread_yield();

	cout << "Done removing\n";
}


void parent(void* a) {
    int arg = (intptr_t) a;
    cout << "parent called with arg " << arg << endl;

    if (thread_create((thread_startfunc_t) producer, (void*) "producer 1")) {
      cout << "thread_create failed\n";
    }

    if (thread_create((thread_startfunc_t) producer, (void*) "producer 2")) {
      cout << "thread_create failed\n";
    }

    if (thread_create((thread_startfunc_t) producer, (void*) "producer 3")) {
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

