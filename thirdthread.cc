#include <iostream>
#include <cstdlib>
#include <stdint.h>
#include "thread.h"

using namespace std;

int g = 0;

void print(void* id) {
	char* print_id = (char*) id;

	if (thread_yield()) {
          cout << "thread_yield failed\n";
          exit(1);
        }
	
	cout << "Print called by thread named " << print_id << endl;
}

void loop(void* a) {
    char* id = (char*) a;
    cout << "loop called with id " << id << endl;

    for (int i = 0; i < 5; i++, g++) {
      cout << id << ":\t" << i << "\t" << g << endl;
      
      if (thread_yield()) {
        cout << "thread_yield failed\n";
        exit(1);
      }
      
    }


    if (thread_create((thread_startfunc_t) print, (void*) id)) {
        cout << "thread_create failed\n";
        exit(1);
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

