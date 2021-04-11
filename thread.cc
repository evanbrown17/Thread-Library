// thread.cc - implementation of a user-level thread library
// Braden Fisher and Evan Brown

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"
#include <queue>
#include <cstddef>
#include <iostream>
#include <list>

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

struct Lock {
	queue<Thread*>* lockQueue;
	Thread* lock_owner;
	bool free;
	unsigned int id;
};

struct ConditionVariable {
	queue<Thread*>* cvQueue;
	unsigned int id;
};

static queue<Thread*> readyQueue;
static list<Lock*> allLocks;
static list<ConditionVariable*> allCVs;
static Thread* curThread;
static Lock* curLock;
static ConditionVariable* curCV;
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
	queue<Thread*> copy = readyQueue;
	cout << "Queue: ";
	while (!copy.empty()) {
		cout << copy.front()->id << ", ";
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

	assert_interrupts_enabled();
	interrupt_disable();
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
	assert_interrupts_disabled();
	interrupt_enable();
	func(arg); //call the function for the thread
	assert_interrupts_enabled();
	interrupt_disable();
	curThread->done = true;
	swapcontext(curThread->ucontext_ptr, main_thread);

}

int thread_create(thread_startfunc_t func, void* arg) {

	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	Thread* new_thread = new (nothrow) Thread;
	if (new_thread == NULL) {
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}

	new_thread->ucontext_ptr = new (nothrow) ucontext_t;
	if (new_thread->ucontext_ptr == NULL) {
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}

	setup_thread(new_thread);
	makecontext(new_thread->ucontext_ptr, (void(*)()) func_wrapper, 2, func, arg);

	new_thread->id = thread_id_counter;
	thread_id_counter++;

	new_thread->done = false;

	readyQueue.push(new_thread);

	assert_interrupts_disabled();
	interrupt_enable();
	return 0;
	//need to implement error checking and return -1
}

int thread_yield() {

	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();
	
	//print_ready_queue();
	if (!readyQueue.empty()) {

		//Thread* newThread = readyQueue.front();
		//readyQueue.pop();

		readyQueue.push(curThread);
		//cout << "Swapping: " << curThread << ", " << newContext << endl;
		
		//Thread* swap = curThread; 
		//curThread = newThread;
		
		
		swapcontext(curThread->ucontext_ptr, main_thread); 
		
	}
	assert_interrupts_disabled();
	interrupt_enable();
	return 0;
	
}

static Lock* find_lock(unsigned lock_id) {
	for (list<Lock*>::iterator it = allLocks.begin(); it != allLocks.end(); ++it) {
		Lock* lock_ptr = *it;
		if (lock_ptr->id == lock_id) {
			return *it;
		}
	}

	return NULL;
}

int thread_lock(unsigned lock) {

	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	curLock = find_lock(lock);

	if (curLock != NULL && !curLock->free && (curLock->lock_owner->id == curThread->id)) { //error if a thread tries to acquire a lock it already has
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}

	if (curLock == NULL){
		Lock* new_lock = new (nothrow) Lock;
		if (new_lock == NULL) {
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}
		
		new_lock->free = false;
		new_lock->id = lock;
		new_lock->lock_owner = curThread;
		new_lock->lockQueue = new (nothrow) queue<Thread*>;
		if (new_lock->lockQueue == NULL) {
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}


		allLocks.push_back(new_lock);
		curLock = new_lock;
	} else {
		if (curLock->free) {
			curLock->free = false;
			curLock->lock_owner = curThread;
		}
		else {
			curLock->lockQueue->push(curThread);

			/*
			Thread* newThread = readyQueue.front();
			readyQueue.pop();
			Thread* swap = curThread; 
			curThread = newThread;
			*/

			swapcontext(curThread->ucontext_ptr, main_thread);
		}

	}

	assert_interrupts_disabled();
	interrupt_enable();	
	return 0;

}

int unlock_interrupts_disabled(unsigned lock) {

	curLock = find_lock(lock);

	if (curLock == NULL) { //error if lock does not exist
		return -1;
	}

	if (curLock->free || curLock->lock_owner->id != curThread->id) { //error if thread tries to release a lock it does not own
		return -1;
	}

	if (!curLock->lockQueue->empty()) {
		Thread* next_lock_owner = curLock->lockQueue->front();
		curLock->lock_owner = next_lock_owner;
		curLock->lockQueue->pop();
		readyQueue.push(next_lock_owner);
	} else {
		curLock->lock_owner = NULL;
		curLock->free = true;
	}

	return 0;

}

int thread_unlock(unsigned lock) {
	if (!initialized) {
		return -1;
	}
	
	assert_interrupts_enabled();
	interrupt_disable();

	int unlock_result = unlock_interrupts_disabled(lock);

	assert_interrupts_disabled();
	interrupt_enable();
	return unlock_result;
	
}

static ConditionVariable* find_cv(unsigned cvid) {
	for (list<ConditionVariable*>::iterator it = allCVs.begin(); it != allCVs.end(); ++it) {
		ConditionVariable* cv_ptr = *it;
		if (cv_ptr->id == cvid) {
			return *it;
		}
	}

	return NULL;
}


int thread_wait(unsigned lock, unsigned cond) {
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	if (unlock_interrupts_disabled(lock)) {
		assert_interrupts_disabled();
		interrupt_enable();
		return -1; //error if the calling thread does not own the lock
	}

	curCV = find_cv(cond);

	if (curCV == NULL) {
		ConditionVariable* newCV = new (nothrow) ConditionVariable;
		if (newCV == NULL) {
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}

		newCV->id = cond;
		newCV->cvQueue = new (nothrow) queue<Thread*>;
		if (newCV->cvQueue == NULL) {
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}
		allCVs.push_back(newCV);
		
		curCV = newCV;
	
	}

	curCV->cvQueue->push(curThread);
	swapcontext(curThread->ucontext_ptr, main_thread);

	assert_interrupts_disabled();
	interrupt_enable();
	return thread_lock(lock);

}

int thread_signal(unsigned lock, unsigned cond) {
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	curCV = find_cv(cond);

	if (curCV != NULL) {
		if (!curCV->cvQueue->empty()) {
			Thread* thread_to_wakeup = curCV->cvQueue->front();
			curCV->cvQueue->pop();
			readyQueue.push(thread_to_wakeup);
		}

	}

	//not an error if the CV doesn't exist or if the lock is not owned
	assert_interrupts_disabled();
	interrupt_enable();
	return 0;

}

int thread_broadcast(unsigned lock, unsigned cond) {
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	curCV = find_cv(cond);

	if (curCV != NULL) {
		while (!curCV->cvQueue->empty()) {
			Thread* thread_to_wakeup = curCV->cvQueue->front();
			curCV->cvQueue->pop();
			readyQueue.push(thread_to_wakeup);
		}

	}

	//not an error if the CV doesn't exist or if the lock is not owned
	assert_interrupts_disabled();
	interrupt_enable();
	return 0;

}

