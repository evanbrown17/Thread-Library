// thread.cc - implementation of a user-level thread library
// Braden Fisher and Evan Brown

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"
#include <queue>
#include <cstddef>

using namespace std;

// Remember to declare all helper functions and global
// variables as static to avoid conflicting with names
// used by test programs.

// You should not write a main function here. Your main
// function should be part of the test program that
// uses the thread library.
//

static queue<ucontext_t*> readyQueue;

static void setup_thread(ucontext_t* uc_ptr) {
	getcontext(uc_ptr);
	char* stack = new char[STACK_SIZE];
	uc_ptr->uc_stack.ss_sp = stack;
	uc_ptr->uc_stack.ss_size = STACK_SIZE;
	uc_ptr->uc_stack.ss_flags = 0;
	uc_ptr->uc_link = NULL;	
}

int thread_libinit(thread_startfunc_t func, void* arg) {
  //does not return at all on success
  
	ucontext_t* ucontext_ptr = new ucontext_t;
	setup_thread(ucontext_ptr);
	makecontext(ucontext_ptr, (void(*)()) func, 1, arg);
	if (setcontext(ucontext_ptr)){
		return -1; //means failure
	}
	return 0;
}

int thread_create(thread_startfunc_t func, void* arg) {
  return -1;
}

int thread_yield() {
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

