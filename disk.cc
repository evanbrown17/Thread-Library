// disk.cc - simulation of a concurrent disk scheduler
//
// Evan Brown and Braden Fisher
// Last modified 3/16/2021
// CSCI 3310

#include "thread.h"
#include <iostream>
#include <fstream>
#include <tuple>
#include <deque>
#include <utility>

using namespace std;

#define LOCK 1

#define CV1 2
#define CV2 3

unsigned int MAX_QUEUE_SIZE;

unsigned int QUEUE_SIZE = 0;

int CURRENT_TRACK = 0; //disk head starts at track 0

deque<pair<int,int>> DISK_QUEUE;

int ACTIVE_THREADS;
int NUM_REQUESTS = 0;
int EVERY_THREAD_HAS_REQUEST = 0; //boolean

//int inputs[5][2];
int inputs[5][2] = {{68, 390}, {1, 799}, {647, 800}, {391, 637}, {200, 887}};

/*
 * hasRequest()
 *
 * Takes a thread index number and checks if it has an outstanding request in the queue
 */
int hasRequest(int threadNumber) {
	
	for (pair<int, int> entry : DISK_QUEUE) {
		if (threadNumber == entry.first) {
			return 1;
		}
	}
	
	return 0;
}

/*
 * seek(int trackNum)
 *
 * trackNum is where the disk head is prior to calling the function.
 * Iterates through each request in the queue and find the difference between the reqeusted tracks and where the disk head currently is.
 * The next request to service is the one closest to the disk head, meaning the one with the smallest difference.
 * This next request is returned as a pair where the first entry is the index of the request and the second entry is the requested track.
 * The disk head is moved to the track of the best request.
 */
pair<int,int> seek(int trackNum) {
	
	int bestDistance = 1000; //max disk address is 999
	pair<int, int> bestRequester;

	for (unsigned int i = 0; i < DISK_QUEUE.size(); i++) {
		int difference = abs(trackNum - DISK_QUEUE.at(i).second);
	
		if (difference < bestDistance)       {
			bestDistance = difference;	
			bestRequester = make_pair(i, DISK_QUEUE.at(i).second);
			CURRENT_TRACK = DISK_QUEUE.at(i).second;
		}
	}
	
	return bestRequester;

}
	

/*
 * service()
 *
 * There is one servicer thread created by parent().
 * Waits until the queue of requests is full or when every active requester thread has a request in the queue before servicing requests.
 * Services requests by removing them from the queue and printing the correct service message.
 * Broadcasts all waiting requester threads so that any that can now make a valid request after the service will do so.
 */
void service(void* arg) {

	while (ACTIVE_THREADS || NUM_REQUESTS) { //continue to service until there are no more threads or requests

		if (thread_lock(LOCK)) {
			cout << "servicer failed to acquire lock." << endl;
		}

		//wait until queue is full or every thread has made a request
		while ((DISK_QUEUE.size() != MAX_QUEUE_SIZE) && !EVERY_THREAD_HAS_REQUEST) {
			if (thread_wait(LOCK, CV1)) {
				cout << "servicer failed waiting on CV1" << endl;
			}
		}	

		pair<int, int> nextEntry = seek(CURRENT_TRACK); //contains <index in queue of requesting thread, requested track>

		cout << "service requester " << DISK_QUEUE.at(nextEntry.first).first << " track " << nextEntry.second << endl;

		DISK_QUEUE.erase(DISK_QUEUE.begin() + nextEntry.first);
	
		NUM_REQUESTS--;
		if (NUM_REQUESTS < ACTIVE_THREADS) {
			EVERY_THREAD_HAS_REQUEST = 0;
		}
	
		if (thread_broadcast(LOCK, CV2)) {
			cout << "servicer failed signaling on CV2." << endl;
		}
	
		if (thread_unlock(LOCK)) {
			cout << "servicer failed unlocking." << endl;
		}
	
	}
}

/*
 * request()
 *
 * Each instance represents a single requester thread created by parent().
 * Takes a pair that holds the filename and file index.
 * Opens the files and starts putting requests on the queue.
 * Prints the appropriate request output.
 * */
void request(pair<string, int>* fileNameNum) {
	//ifstream input;
  	//input.open(fileNameNum->first);	
	int input[2];
       	input[0] = inputs[fileNameNum->second][0];
	
       	input[1] = inputs[fileNameNum->second][1];

//  	if (input) { //if argument file exists
		
		//for (string requestNum; getline(input, requestNum);) {
		for (int requestNum : input){	
			if (thread_lock(LOCK)) {
				cout << "requester " << fileNameNum->second << " failed to acquire lock." << endl;
			}
			
			//We should not make a request and wait if the queue is full or the thread has an outstanding request
			while (DISK_QUEUE.size() == MAX_QUEUE_SIZE || hasRequest(fileNameNum->second)) {
				if (thread_wait(LOCK, CV2)) {
					cout << "requester " << fileNameNum->second << " failed waiting on CV2." << endl;
				}
			}
			
			//Push a valid request (pair<file number, requested disk address>) to the queue
			//DISK_QUEUE.push_back(make_pair(fileNameNum->second, stoi(requestNum, nullptr, 10)));
			DISK_QUEUE.push_back(make_pair(fileNameNum->second, requestNum));


			NUM_REQUESTS++;
			if (NUM_REQUESTS >= ACTIVE_THREADS) {
				EVERY_THREAD_HAS_REQUEST = 1;
			}
		
			cout << "requester " << fileNameNum->second << " track " << requestNum << endl;
			
			//When there are no more requests that can be made, signal the servicer
			if (DISK_QUEUE.size() == MAX_QUEUE_SIZE || EVERY_THREAD_HAS_REQUEST) {
				if (thread_signal(LOCK, CV1)) {
					cout << "requester " << fileNameNum->second << " failed signaling CV1." << endl;
				}
			}

			if (thread_unlock(LOCK)) {
				cout << "requester " << fileNameNum->second << " failed to release lock." << endl;
			}
		}	
 
		//after reading through the file:
	
		if (thread_lock(LOCK)) {
			cout << "requester " << fileNameNum->second << " failed to acquire lock." << endl;
		}
		
		//thread should wait when it has outstanding request
		while (hasRequest(fileNameNum->second)) {
			if (thread_wait(LOCK, CV2)) {
				cout << "Requester " << fileNameNum->second << " failed waiting on CV2." << endl;
			}
		}

		ACTIVE_THREADS--;
		if (NUM_REQUESTS >= ACTIVE_THREADS) {
			EVERY_THREAD_HAS_REQUEST = 1;
		}
		
		//Signal the servicer when no more valid requests are possible to make and there are still other active threads
		if ((DISK_QUEUE.size() == MAX_QUEUE_SIZE || EVERY_THREAD_HAS_REQUEST) && ACTIVE_THREADS) {
			if (thread_signal(LOCK, CV1)) {
				cout << "requester " << fileNameNum->second << " failed signaling CV1." << endl;
			}
		}

		if (thread_unlock(LOCK)) {
			cout << "requester " << fileNameNum->second << " failed to release lock." << endl;
		}
		
		delete (fileNameNum); //delete since it was made with "new"
	
	
}



/*
 * parent()
 * 
 * Called by thread_libnit() in main().
 * Takes a pair that holds argc and argv as parameters.
 *
 * Creates all necessary threads and sends them to the request() and service() functions.
 */
void parent(pair<int, char**>* input) {
      
	//MAX_QUEUE_SIZE = atoi(input->second[1]); //grabbing inputted queue size from argv
	MAX_QUEUE_SIZE = 5;

	if (thread_lock(LOCK)) {
		cout << "parent failed to acquire lock." << endl;
	}
	
	//create our service thread -- function requires an argument but we don't actually need to pass it anything
	if (thread_create((thread_startfunc_t) service, (void*) 0)) {
		cout << "thread create failed\n";
		exit(1);
	}

	//if (input->second[2] != NULL) { //as long as there is at least one file name given
		int currentFile = 0;
		//loop through argv, creating a requester thread for each filename 
		//for (int i = 2; i < input->first; i++) {
		for (int i = 0; i < 5; i++) {	
			//string name = input->second[i];
			string name = to_string(i);
			pair<string, int>* nameNumPair; 
		        nameNumPair = new pair<string, int> (make_pair(name, currentFile));
			//pair holds the filename and the file index

			currentFile++;
			
			//create new requester threads
			if (thread_create((thread_startfunc_t) request, nameNumPair)) {
				cout << "thread create failed\n";
				exit(1);
			}
				
		}

	

	//start_preemptions(true, false, 10);

	if (thread_unlock(LOCK)) {
		cout << "parent failed to release lock." << endl;
	}

}

int main(int argc, char** argv) {
	
	//ACTIVE_THREADS = argc - 2; //first argument is the queue size, which does not correspond to an active thread 
	ACTIVE_THREADS = 3;
	pair<int, char**> inputPair = make_pair(argc, argv);
	
	//initialize thread library starting in parent function, passing a pointer to the argc/argv pair
	if (thread_libinit((thread_startfunc_t) parent, &inputPair)) {
		cout << "thread_libinit failed!\n" ;
		exit(1);
	}
  
	return 0;
}



