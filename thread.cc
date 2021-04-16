// thread.cc - implementation of a user-level thread library
// Braden Fisher and Evan Brown
// Last modified 4/15/21
// CSCI 3310
// Professor Barker
//
// Implements the user-level thread library defined in the thread.h header. The thread library 
// implements operations that support creating new user-level threads, yielding the CPU to another 
// runnable thread, and lock and condition variable operations with Mesa semantics.

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"
#include <queue>
#include <cstddef>
#include <iostream>
#include <list>

using namespace std;


struct Thread {
	unsigned int id;
	char* stack;
	ucontext_t* ucontext_ptr;
	bool done;
	bool waiting;
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
	unsigned int lockid;
};

static queue<Thread*> readyQueue;
static list<Lock*> allLocks;
static list<ConditionVariable*> allCVs;

static Thread* curThread;
static Lock* curLock;
static ConditionVariable* curCV;

static ucontext_t* main_thread;

static list<Thread*> unfreed; //keeps track of all threads that have been allocated but not yet freed

static int thread_id_counter = 0;

static bool initialized = false;


static void delete_current_thread() {
	//deletes the stack and ucontext pointer allocated to the thread, then deletes the thread itself
	delete[] (curThread->stack);
	curThread->ucontext_ptr->uc_stack.ss_sp = NULL;
	curThread->ucontext_ptr->uc_stack.ss_size = 0;
	curThread->ucontext_ptr->uc_stack.ss_flags = 0;
	delete curThread->ucontext_ptr;
	delete curThread;
	curThread = NULL;
}

int thread_libinit(thread_startfunc_t func, void* arg) {
	//inititlizes the thread library and should be called exactly once before calling any other
	//thread library functions: returns 0 on failure and does not return at all on success
  
	if (initialized == true) { //can't initialize library more than once
		return -1;
	}
  	initialized = true;

	
	if (thread_create(func, arg)) {
		return -1;
	} //puts the first thread on the ready queue with the function and argument specified by the user

	curThread = readyQueue.front();
	readyQueue.pop();

	main_thread = new (nothrow) ucontext_t; //determines which thread to run next and when no threads are left
	if (main_thread == NULL) {
		delete main_thread;
		return -1;
	}

	getcontext(main_thread);

	assert_interrupts_enabled();
	interrupt_disable();
	swapcontext(main_thread, curThread->ucontext_ptr); //start running the first thread

	while (!readyQueue.empty()) { //run threads until none are left
		if (curThread->done) {
			unfreed.remove(curThread);
			delete_current_thread();
		}
		curThread = readyQueue.front();
		readyQueue.pop();
		if (swapcontext(main_thread, curThread->ucontext_ptr)){
			return -1; //means failure from swapcontext
		}

	}

	if (curThread != NULL) {//delete current thread if it is still not null but not running
		unfreed.remove(curThread);
		delete_current_thread();
	}

	for (Lock* lock : allLocks) {
		delete lock->lockQueue;
		delete lock;
	}

	for (ConditionVariable* cv : allCVs) {
		delete cv->cvQueue;
		delete cv;
	}

	
	while (!unfreed.empty()) { //delete any remaining threads
		curThread = unfreed.front();
		unfreed.pop_front();
		delete_current_thread();
	}

	delete main_thread;

	cout << "Thread library exiting.\n";
	exit(0);
	return 0; //never reaches this but the function must return an int
}

static void func_wrapper(thread_startfunc_t func, void* arg) {
	//a wrapper function that calls the function specified for a new thread to call. The wrapper function is used so that it is known when 
	//the thread is done by when its function returns
	
	assert_interrupts_disabled();
	interrupt_enable();

	func(arg); //call the function for the thread

	assert_interrupts_enabled();
	interrupt_disable();

	curThread->done = true;
	swapcontext(curThread->ucontext_ptr, main_thread); //go back to the main thread to delete this one and run the next one

}

int thread_create(thread_startfunc_t func, void* arg) {
	//creates a new thread, which calls func with the argument arg. The new thread is not run immediately but is put on the 
	//ready queue. Returns 0 on success and -1 on failure (such as if there is no memory left)

	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	Thread* new_thread = new (nothrow) Thread;

	if (new_thread == NULL) {
		delete new_thread;
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}


	new_thread->ucontext_ptr = new (nothrow) ucontext_t;

	if (new_thread->ucontext_ptr == NULL) {
		delete new_thread->ucontext_ptr;
		delete new_thread;
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}


	getcontext(new_thread->ucontext_ptr);

	new_thread->stack = new (nothrow) char[STACK_SIZE];
	if (new_thread->stack == NULL) {
		delete[] new_thread->stack;
		delete new_thread->ucontext_ptr;
		delete new_thread;
		assert_interrupts_disabled();
		interrupt_enable();
		return -1;
	}

	new_thread->ucontext_ptr->uc_stack.ss_sp = new_thread->stack;
	new_thread->ucontext_ptr->uc_stack.ss_size = STACK_SIZE;
	new_thread->ucontext_ptr->uc_stack.ss_flags = 0;
	new_thread->ucontext_ptr->uc_link = NULL;
	
	unfreed.push_back(new_thread);

	makecontext(new_thread->ucontext_ptr, (void(*)()) func_wrapper, 2, func, arg); //direct the new thread to the wrapper function

	new_thread->id = thread_id_counter;
	thread_id_counter++;

	new_thread->done = false;
	new_thread->waiting = false;

	readyQueue.push(new_thread);

	assert_interrupts_disabled();
	interrupt_enable();
	return 0;
}

int thread_yield() {
	//causes the currently-running thread to stop running and yield the CPU to the next runnable 
	//thread on the ready queue. Returns 0 on success and -1 on failure

	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();
	
	if (!readyQueue.empty()) { //nothing happens if there are no other threads to run
		readyQueue.push(curThread);
		swapcontext(curThread->ucontext_ptr, main_thread); //let the main thread start running the next thread
	}

	assert_interrupts_disabled(); //thread enables interrupts when it runs again
	interrupt_enable();
	return 0;
	
}

static Lock* find_lock(unsigned lock_id) {
	//searches the list, by lock id number, of all the locks that have been used before and returns a pointer to the 
	//lock if it is found and NULL if it is not found
	
	for (list<Lock*>::iterator it = allLocks.begin(); it != allLocks.end(); ++it) {
		Lock* lock_ptr = *it;
		if (lock_ptr->id == lock_id) {
			return *it;
		}
	}

	return NULL;
}

int thread_lock(unsigned lock) {
	//called by threads that are attempting to acquire the lock. If the lock is free, the thread then owns the lock,
	//but is put on the queue of threads waiting for the lock otherwise. Returns 0 on success and -1 on failure, 
	//which occurs when a thread tries to acquire a lock it already has

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

	if (curLock == NULL){ //new lock
		Lock* new_lock = new (nothrow) Lock;
		if (new_lock == NULL) {
			delete new_lock;
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}
		
		new_lock->free = false;
		new_lock->id = lock;
		new_lock->lock_owner = curThread;

		new_lock->lockQueue = new (nothrow) queue<Thread*>;
		if (new_lock->lockQueue == NULL) {
			delete new_lock->lockQueue;
			delete new_lock;
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}

		allLocks.push_back(new_lock);
		curLock = new_lock;

	} else { //lock already exists
		if (curLock->free) {
			curLock->free = false;
			curLock->lock_owner = curThread; //current thread owns the lock if it was free before
		}
		else {
			curLock->lockQueue->push(curThread); //gets added to the queue waiting on the lock otherwise
			swapcontext(curThread->ucontext_ptr, main_thread);
		}

	}

	assert_interrupts_disabled(); //interrupts are always enabled by awoken threads before they return to user code
	interrupt_enable();	
	return 0;

}

static int unlock_interrupts_disabled(unsigned lock) {
	//attempts to cause a thread to release the lock, and it must be run with interrupts disabled. It was 
	//included as a helper function because it is used by both thread_unlock and thread_wait. Returns 0 on 
	//success and -1 on failure

	curLock = find_lock(lock);

	if (curLock == NULL) { //error if lock does not exist
		return -1;
	}

	if (curLock->free || curLock->lock_owner->id != curThread->id) { //error if thread tries to release a lock it does not own
		return -1;
	}

	if (!curLock->lockQueue->empty()) { //the owner becomes the thread at the front of the queue waiting for the lock
		Thread* next_lock_owner = curLock->lockQueue->front();
		curLock->lock_owner = next_lock_owner;
		curLock->lockQueue->pop();
		readyQueue.push(next_lock_owner);//"wakes up" the thread that now has the lock
	} else { //if no other thread wants the lock
		curLock->lock_owner = NULL;
		curLock->free = true;
	}

	return 0;

}

int thread_unlock(unsigned lock) {
	//called when the current thread releases the lock. It cannot release a lock that does not exist
	//or a lock it does not own. Returns 0 on success and -1 on failure

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

static ConditionVariable* find_cv(unsigned lock, unsigned cvid) {
	//searches, by condition variable id, the list of all CVs that have been used and returns 
	//the CV if it is found and NULL if it is not
	for (list<ConditionVariable*>::iterator it = allCVs.begin(); it != allCVs.end(); ++it) {
		ConditionVariable* cv_ptr = *it;
		if (cv_ptr->id == cvid && cv_ptr->lockid == lock) { //condition variable is tuple of CV number, lock numer
			return *it;
		}
	}

	return NULL;
}


int thread_wait(unsigned lock, unsigned cond) {
	//called when a thread is waiting on a condition variable. The thread should have the associated lock to wait
	//on a condition variable, else a new condition variable is made. Returns 0 on success and -1 on failure
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	if (unlock_interrupts_disabled(lock)) { //the thread releases the lock when it waits
		assert_interrupts_disabled();
		interrupt_enable();
		return -1; //error if the calling thread does not own the lock
	}

	curCV = find_cv(lock, cond);

	if (curCV == NULL) {
		ConditionVariable* newCV = new (nothrow) ConditionVariable;
		if (newCV == NULL) {
			delete newCV;
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}

		newCV->id = cond;
		newCV->lockid = lock;
		newCV->cvQueue = new (nothrow) queue<Thread*>;
		if (newCV->cvQueue == NULL) {
			delete newCV->cvQueue;
			delete newCV;
			assert_interrupts_disabled();
			interrupt_enable();
			return -1;
		}

		allCVs.push_back(newCV);
		
		curCV = newCV;
	
	}

	curCV->cvQueue->push(curThread); //thread is now waiting on this CV, so it goes in its queue
	curThread->waiting = true;
	swapcontext(curThread->ucontext_ptr, main_thread);
	assert_interrupts_disabled();
	interrupt_enable();
	return thread_lock(lock); //when the awaiting thread is woken up, the first thing it must do is acquire the lock again

}

int thread_signal(unsigned lock, unsigned cond) {
	//called when a thread signals on a condition variable, meaning it should "wake up" the first
	//thread in the condition variable's waiting queue and put it on the ready queue. 
	//Returns 0 on failure and -1 on success
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	curCV = find_cv(lock, cond);

	if (curCV != NULL) {
		if (!curCV->cvQueue->empty()) { //nothing happens if no threads are waiting on the condition variable
			Thread* thread_to_wakeup = curCV->cvQueue->front();
			curCV->cvQueue->pop();
			readyQueue.push(thread_to_wakeup);
			thread_to_wakeup->waiting = false;
		}

	}

	//not an error if the CV doesn't exist or if the lock is not owned by Mesa semantics
	assert_interrupts_disabled();
	interrupt_enable();
	return 0;

}

int thread_broadcast(unsigned lock, unsigned cond) {
	//called when a thread wants to "wake up" every thread that is waiting on a condition variable, 
	//meaning it takes out all the threads in the condition variable's waiting queue and puts them 
	//on the ready queue. Returns 0 on success and -1 on failure
	
	if (!initialized) {
		return -1;
	}

	assert_interrupts_enabled();
	interrupt_disable();

	curCV = find_cv(lock, cond);

	if (curCV != NULL) { 
		while (!curCV->cvQueue->empty()) { //wake up every thread in the CV's waiting queue
			Thread* thread_to_wakeup = curCV->cvQueue->front();
			curCV->cvQueue->pop();
			readyQueue.push(thread_to_wakeup);
			thread_to_wakeup->waiting = true;
		}

	}

	//not an error if the CV doesn't exist or if the lock is not owned by Mesa semantics
	assert_interrupts_disabled();
	interrupt_enable();
	return 0;

}

