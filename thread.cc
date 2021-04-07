// thread.cc - implementation of a user-level thread library
// Braden Fisher and Evan Brown

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"
#include <queue>
#include <cstddef>
#include <iostream>

using namespace std;

// Remember to declare all helper functions and global
// variables as static to avoid conflicting with names
// used by test programs.

// You should not write a main function here. Your main
// function should be part of the test program that
// uses the thread library.
//


struct Thread {
	unsigned int id;
	char* stack;
	ucontext_t* ucontext_ptr;
	bool done;
};

static queue<Thread*> readyQueue;
static Thread* curThread;
static ucontext_t* main_thread;

static int thread_id_counter = 0;

static bool initialized = false;

static void setup_thread(Thread* thread) {
	getcontext(thread->ucontext_ptr);
	thread->stack = new (nothrow) char[STACK_SIZE];
	thread->ucontext_ptr->uc_stack.ss_sp = thread->stack;
	thread->ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
	thread->ucontext_ptr->uc_stack.ss_flags = 0;
	thread->ucontext_ptr->uc_link = NULL;	
}
/*
static void print_ready_queue() {
	queue<ucontext_t*> copy = readyQueue;
	cout << "Queue: ";
	while (!copy.empty()) {
		cout << copy.front() << ", ";
		copy.pop();
}
	cout << endl;
	}
*/

static void delete_current_thread() {
	delete curThread->stack;
	curThread->ucontext_ptr->uc_stack.ss_sp = NULL;
	curThread->ucontext_ptr->uc_stack.ss_size = 0;
	curThread->ucontext_ptr->uc_stack.ss_flags = 0;
	delete curThread->ucontext_ptr;
	delete curThread;
}


int thread_libinit(thread_startfunc_t func, void* arg) {
  //does not return at all on success
  
	if (initialized == true) { //can't initialize library more than once
		return -1;
	}
  	initialized = true;

	//ucontext_t* first_ucontext_ptr = new ucontext_t;
	//setup_thread(first_ucontext_ptr);
	//makecontext(first_ucontext_ptr, (void(*)()) func, 1, arg);
	//
	
	if (thread_create(func, arg)) {
		return -1;
	} //puts the first thread on the ready queue

	curThread = readyQueue.front();
	readyQueue.pop();

	main_thread = new (nothrow) ucontext_t; //determines when no threads are left
	getcontext(main_thread);

	swapcontext(main_thread, curThread->ucontext_ptr);

	while (!readyQueue.empty()) {
		if (curThread->done) {
			delete_current_thread();
		}
		curThread = readyQueue.front();
		readyQueue.pop();
		if (swapcontext(main_thread, curThread->ucontext_ptr)){
			return -1; //means failure
		}

	}
	
	cout << "Thread library exiting.\n";
	exit(0);
	return 0;
}

static void func_wrapper(thread_startfunc_t func, void* arg) {
	func(arg); //call the function for the thread
	curThread->done = true;
	swapcontext(curThread->ucontext_ptr, main_thread);

}

int thread_create(thread_startfunc_t func, void* arg) {

	if (!initialized) {
		return -1;
	}

	Thread* new_thread = new (nothrow) Thread;
	if (new_thread == NULL) {
		return -1;
	}

	new_thread->ucontext_ptr = new (nothrow) ucontext_t;
	if (new_thread->ucontext_ptr == NULL) {
		return -1;
	}

	setup_thread(new_thread);
	makecontext(new_thread->ucontext_ptr, (void(*)()) func_wrapper, 2, func, arg);

	new_thread->id = thread_id_counter;
	thread_id_counter++;

	new_thread->done = false;

	readyQueue.push(new_thread);
	return 0;
	//need to implement error checking and return -1
}

int thread_yield() {

	if (!initialized) {
		return -1;
	}
	
	//print_ready_queue();
	if (!readyQueue.empty()) {

		Thread* newThread = readyQueue.front();
		readyQueue.pop();

		readyQueue.push(curThread);
		//cout << "Swapping: " << curThread << ", " << newContext << endl;
		
		Thread* swap = curThread; 
		curThread = newThread;

		swapcontext(swap->ucontext_ptr, newThread->ucontext_ptr); 
		
		return 0;

	}
	//need to implement error checking and return -1 --just used now so an int is always
	//returned - should still return 0 even if ready queue is empty
	return -1;
	
}

int thread_lock(unsigned lock) {
  return -1;
}

int thread_unlock(unsigned lock) {
  return -1;
}

int thread_wait(unsigned lock, unsigned cond) {
  return -1;
}

int thread_signal(unsigned lock, unsigned cond) {
  return -1;
}

int thread_broadcast(unsigned lock, unsigned cond) {
  return -1;
}

