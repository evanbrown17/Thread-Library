// thread.cc - implementation of a user-level thread library
// YOUR NAME(S) HERE

#include "ucontext.h"
#include "thread.h"
#include "interrupt.h"

// Remember to declare all helper functions and global
// variables as static to avoid conflicting with names
// used by test programs.

// You should not write a main function here. Your main
// function should be part of the test program that
// uses the thread library.

int thread_libinit(thread_startfunc_t func, void* arg) {
  return -1;
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

