//purposefully creates deadlock

#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"

using namespace std;

int g = 0; // global (shared by all threads)

#define lock1 1
#define lock2 2

void parentLoop(void* a) {
    char* id = (char*) a;
    cout << "parent loop called with id " << id << endl;

    for (int i = 0; i < 5; i++, g++) {
      
      thread_lock(lock2);
      thread_yield();
      thread_lock(lock1);
      cout << id << ":\t" << i << "\t" << g << endl;
      if (thread_yield()) {
        cout << "thread_yield failed\n";
      }
    }
}


void loop(void* a) {
    char* id = (char*) a;
    cout << "loop called with id " << id << endl;

    for (int i = 0; i < 5; i++, g++) {
      
      thread_lock(lock1);
      thread_yield();
      thread_lock(lock2);
      cout << id << ":\t" << i << "\t" << g << endl;
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

    parentLoop((void*) "parent thread");
}

int main() {
    if (thread_libinit((thread_startfunc_t) parent, (void*) 100)) {
      cout << "thread_libinit failed\n";
    }
}

