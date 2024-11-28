/**
  * Assignment: synchronization
  * Operating Systems
  * Student names: Sofie Vos and Philipp Kuppers
  */

/**
  Hint: F2 (or Control-klik) on a functionname to jump to the definition
  Hint: Ctrl-space to auto complete a functionname/variable.
  */

// function/class definitions you are going to use
#include <algorithm>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;

int no_readers = 0; //for having multiple readers reading at the same time

mutex m_bound;
mutex m_buf;
mutex m_log;
mutex read;
mutex m_no_readers; 

vector<string> logger;

int bound = -1; //if the bound is negative, the buffer is unbounded

vector<int> buffer;

void write_to_log(string s){
  m_log.lock();
  read.lock();
  logger.push_back(s); // CRITICAL SECTION
  read.unlock();
  m_log.unlock();
}

// Read from the logger. The element that is read, should stay in the log
// return all the messages in the vector so that the buffer/user can print it
void read_from_log(){
  m_log.lock();
  m_log.unlock();
  m_no_readers.lock();
  if (no_readers == 0) // CRITICAL SECTION
    read.lock();
  no_readers++; // CRITICAL SECTION
  m_no_readers.unlock();
  //uncomment the next comment for run_test2()
  cout << "one read" << endl;
  for(int i = 0; i < logger.size(); i++){ // CRITICAL SECTION
    cout << logger[i] << endl; // CRITICAL SECTION
  }
  m_no_readers.lock();
  no_readers--; // CRITICAL SECTION
  if (no_readers == 0) // CRITICAL SECTION
    read.unlock();
  m_no_readers.unlock();
}

// Add an integer to the back of the buffer
void add_to_buffer(int newElem) {
  m_buf.lock();
  if(bound >= 0 && buffer.size() == bound){ // CRITICAL SECTION
    write_to_log("adding " + to_string(newElem) + " to the buffer failed");
    m_buf.unlock();
  } else {
    buffer.push_back(newElem); // CRITICAL SECTION
    write_to_log("added " + to_string(newElem) + " to buffer succes");
    m_buf.unlock();
  }
}

// removing element in buffer
int rm_from_buffer() {
  m_buf.lock();
  if(buffer.empty()){ // CRITICAL SECTION
    write_to_log("remove from buffer failure");
    m_buf.unlock();
    return NULL;
  } else {
    int removed = buffer[0]; // CRITICAL SECTION
    buffer.erase(buffer.begin()); // CRITICAL SECTION
    write_to_log("removed " + to_string(removed) + " from the buffer ");
    m_buf.unlock();
    return removed;
  }
}

// the user can set a bound of the buffer
void set_bound(int new_bound) {
  m_bound.lock();
  if (buffer.size() > new_bound) { // CRITICAL SECTION
    write_to_log("bound set failure");
    m_bound.unlock();
  } else {
    bound = new_bound; // CRITICAL SECTION
    write_to_log("bound set success");
    m_bound.unlock();
  }
}

// make the buffer unbounded
void make_undounded() {
  // making the buffer unbounded always succeeds once the lock is acquired.
  m_bound.lock();
  bound = -1; // CRITICAL SECTION
  write_to_log("set unbounded succces");
  m_bound.unlock();
}

void consumer() {
  for(int i = 0; i < 100; i++) {
    rm_from_buffer();
  }
}

void producer() {
  for(int i = 0; i < 100; i++) {
    add_to_buffer(i);
  }
}

vector<thread> threads;

// Test with equal amount of producers and consumers
void run_test1() {
  for (int i = 0; i < 50; i++) {
    threads.push_back(thread(add_to_buffer, i));
    threads.push_back(thread(rm_from_buffer));
  }

  for (int i = 0; i < threads.size() ; i++) {
    threads[i].join();
  }
  read_from_log();
  cout << "The size of the log is: " << logger.size() << endl;
}

//Test with one reader and 50 writers
//For this test please uncomment line 55
void run_test2() {
  threads.push_back(thread(read_from_log));
  for (int i = 0; i < 50; i++) {
       threads.push_back(thread(write_to_log,"test thread"));  
  }   
  
  for (int i = 0; i < threads.size(); i++) {
       threads[i].join();
   }
 }

//Test with one writer and 50 readers
void run_test3() {
  threads.push_back(thread(write_to_log,"test thread"));
  for (int i = 0; i < 50; i++) {
      threads.push_back(thread(read_from_log));
  }

  for (int i = 0; i < threads.size(); i++) {
    threads[i].join();
  }
  read_from_log(); //to see if "test thread" was able to be written
}

//Test with 50 writers and 50 reader
//For this test please uncomment line 55
void run_test4(){
  for (int i = 0; i < 50; i++) {
      threads.push_back(thread(read_from_log));
      threads.push_back(thread(write_to_log,"test thread"));
  }
  
  for (int i = 0; i < threads.size(); i++) {
    threads[i].join();
  }
}

void producer1()
{
  int i = 0;
  while (i < 10) {
    add_to_buffer(i);
    i++;
  }
}

void producer2()
{
  int i = 0;
  while (i < 10) {
    add_to_buffer(i);
    i++;
  }
}

void consumer1()
{
  int i = 0;
  while (i < 10) {
    rm_from_buffer();
    i++;
  }
}

void consumer2()
{
  int i = 0;
  while (i < 10) {
    rm_from_buffer();
    i++;
  }
}

// testing a bounded buffer
void run_test5() {
  set_bound(3);
  thread t1(producer1);
  thread t2(producer2);
  thread t3(consumer1);
  thread t4(consumer2);
  t1.join();
  t2.join();
  t3.join();
  t4.join();
  read_from_log();
}

int main(int argc, char* argv[]) {
  run_test1();
	return 0;
}
