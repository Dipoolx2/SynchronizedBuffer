/**
 * Assignment: synchronization
 * Operating Systems
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

// although it is good habit, you don't have to type '' before many objects by including this line
using namespace std;

class Logger
{
private:
  vector<string> log_vector;

  mutex read_mtx;
  mutex write_mtx;
  mutex turnstile; // To prevent writer starvation: idea from lecture 5.
  int reader_count = 0;

  // Internal helper (assumes mutex is already locked)
  bool read_at_nolock(const int index, string &message)
  {
    if (index < 0 || index >= static_cast<int>(log_vector.size()))
      return false;

    message = log_vector.at(index);
    return true;
  }

  // Trivial, but this groups the turnstile and write_mtx locks together into a single operation. 
  // Note: First lock turnstile to prevent deadlocks as lock_read() also locks turnstile first.
  void lock_write() {
    turnstile.lock();
    write_mtx.lock();
  }

  // Same idea as lock_write, but now for unlocking.
  void unlock_write() {
    write_mtx.unlock();
    turnstile.unlock();
  }

  // Used if a reader has started reading. In this time no one can write.
  void lock_read()
  {
    // Wait for the writers in queue to finish writing.
    turnstile.lock();
    turnstile.unlock();

    read_mtx.lock();
    reader_count++;
    if (reader_count == 1)
    {
      // Lock writers, at least one reader is active.
      write_mtx.lock();
    }

    read_mtx.unlock();
  }

  // Used if a reader has stopped reading. If there are no readers left one can start writing again.
  void unlock_read()
  {
    read_mtx.lock();
    reader_count--;
    if (reader_count == 0)
    {
      // Unlock writers, no more readers are active.
      write_mtx.unlock();
    }
    read_mtx.unlock();
  }

public:

  // We first define the write operations (log() and clear()).
  void log(const string &message)
  {
    lock_write();

    try
    {
      log_vector.push_back(message);

      unlock_write();
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      unlock_write();
    }
  }

  void clear()
  {
    lock_write();

    try
    {
      log_vector.clear();
      unlock_write();
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;
      unlock_write();
    }

  }

  // Now we define the read operations.
  // Returns whether retrieving the last message went okay. If it went ok, the message is stored to the given message reference.
  bool read_last(string &message)
  {
    lock_read();
    bool ret = false;

    try
    {
      const int last_index = log_vector.size() - 1;
      ret = read_at_nolock(last_index, message);
      unlock_read();
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      unlock_read();
      return false;
    }

    return ret;
  }

  bool read_at(const int index, string &message)
  {
    lock_read();
    bool ret = false;

    try
    {
      ret = read_at_nolock(index, message);
      unlock_read();
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      unlock_read();
      return false;
    }

    return ret;
  }

  // Returns success value
  bool read_all(string& result)
  {
    lock_read();

    try
    {
      for (string log : log_vector)
        result = result + log + "\n";

      unlock_read();
      return true;
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      unlock_read();
      return false;
    }
  }

};

class Buffer
{
  vector<int> buf;
};


void log_test_1_sync() {
  Logger logger;
  logger.log("[1] Hello world!");
  logger.log("[2] How are you doing?");
  logger.log("[3] three!");
  logger.log("[4] What why are you still here?");

  string first;
  bool success1 = logger.read_at(1, first);
  if (success1)
    cout << first << endl;
  else cout << "Failure to read log at index 1" << endl;

  string last;
  bool success2 = logger.read_last(last);
  if (success2)
    cout << last << endl;
  else cout << "Failure to read log at last index" << endl;

  string all;
  bool success3 = logger.read_all(all);
  if (success3)
    cout << all << endl;
  else cout << "Failure to read all logs" << endl;

  string fail;
  bool success4 = logger.read_at(4, fail);
  if (success4)
    cout << fail << endl;
  else cout << "Failure to read log at index 4 (this should fail)." << endl;
}

int main(int argc, char *argv[])
{
  log_test_1_sync();
  return 0;
}
