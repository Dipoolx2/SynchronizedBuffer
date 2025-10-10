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
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <sstream>

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
  void lock_write()
  {
    turnstile.lock();
    write_mtx.lock();
  }

  // Same idea as lock_write, but now for unlocking.
  void unlock_write()
  {
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
  // Appends a message to the log.
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

  // Clears the entire log.
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
  bool read_last(string &result)
  {
    lock_read();
    bool ret = false;

    try
    {
      const int last_index = log_vector.size() - 1;
      ret = read_at_nolock(last_index, result);
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

  // Returns whether retrieving the message at the given index went okay. If it went ok, the result is stored in the given result reference.
  bool read_at(const int index, string &result)
  {
    lock_read();
    bool ret = false;

    try
    {
      ret = read_at_nolock(index, result);
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
  bool read_all(string &result)
  {
    lock_read();

    try
    {
      ostringstream oss;
      for (const string& log : log_vector) {
        oss << log << "\n";
      }

      result = oss.str();

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
private:
  vector<int> buf;
  const shared_ptr<Logger> logger; // Use a smart pointer so we prevent memory leaks.

  const string buffer_name;

  int bound = -1; // -1 = infinite
  int sequence_number = 1;

  mutex buf_mtx;
  mutex bound_mtx;
  mutex seq_num_mtx;

  // Logs message in format `<sequence_number> <action> <STATUS>: <message>`
  // Updates sequence number accordingly.
  void log_message(const string &action, const bool &fail, const string &error_message)
  {
    seq_num_mtx.lock();
    int seq = sequence_number++;
    seq_num_mtx.unlock();
    string sequence_num_str = "[" + to_string(seq) + "]";

    string status_str = fail ? "(FAIL)" : "(SUCCESS)";
    string error_message_processed = fail ? " - " + error_message : "";
    string buffer_prefix = buffer_name.empty() ? "" : buffer_name + ": ";

    try {
      logger->log(buffer_prefix + sequence_num_str + " " + status_str + " " + action + error_message_processed);
    } catch (exception& ex) {
      cerr << "Error logging " << (fail ? "error " : "") << "message for " << action << ":\n" << ex.what();
    }
  }

  // Group buf and bound mutexes lock operations together. Ensures it always happens in the same order.
  void lock_buffer() {
    buf_mtx.lock();
    bound_mtx.lock();
  }

  void unlock_buffer() {
    buf_mtx.unlock();
    bound_mtx.unlock();
  }

public:
  virtual ~Buffer() = default; // Default destructor makes sure that all ptr references (logger) are removed.
  Buffer(shared_ptr<Logger> logger, const string buffer_name) : logger(logger), buffer_name(buffer_name) {};

  void add_back(const int value)
  {
    // For logging consistency
    string action = "Buffer write " + to_string(value);

    lock_buffer();
    
    if ((int)buf.size() < bound || bound == -1)
    {
      try
      {
        buf.push_back(value);
        
        // NOTE: uncomment this if you want to test actual concurrency 
        //(because otherwise this operation is very fast and it might not look like its concurrent with threads)
        //this_thread::sleep_for(chrono::milliseconds(50));

        unlock_buffer();

        log_message(action, false, "");
      }
      catch (exception &ex)
      {
        unlock_buffer();

        log_message(action, true, ex.what());
      }
      return;
    }

    unlock_buffer();

    log_message(action, true, "Buffer full.");
  }

  // Removes the front of the buffer. In case of success, the value of the removed element is written to 'dest'.
  // The bound should be locked too, since otherwise you could have a race condition where the front is removed first but
  // the bound is shrunk concurrently, the program still reads the wrong prior buffer size and wrongfully gives an error.
  void remove_front(int &dest)
  {
    string action = "Buffer remove front";
    lock_buffer();

    if (buf.size() == 0)
    {
      unlock_buffer();
      log_message(action, true, "Buffer empty.");
      return;
    }

    try
    {
      int val = buf[0];
      buf.erase(buf.begin());

      unlock_buffer();

      dest = val; // Replace after erasing succeeds.
      log_message(action, false, "");
    }
    catch (exception &ex)
    {
      unlock_buffer();

      log_message(action, true, ex.what());
    }
  }

  // Returns success status of the set bound operation.
  bool set_bound(const int new_bound)
  {
    // For logging consistency
    string action = "Buffer set bound to " + to_string(new_bound);

    lock_buffer();

    if (new_bound < 0) {
      log_message(action, true, "Set negative bound attempted.");

      unlock_buffer();
      return false;
    }

    try
    {
      // Warn in case vector entries are truncated.
      bool truncated = new_bound < (int) buf.size();

      // First try to resize the actual buffer before updating any values.
      if (new_bound < bound)
        buf.resize(new_bound); // Inefficient O(n), but the only implementation with a vector.
      bound = new_bound;

      unlock_buffer();

      log_message(action, false, truncated ? "Warning: Entries were truncated." : "");
      return true;
    }
    catch (exception &ex)
    {
      unlock_buffer();

      log_message(action, true, ex.what());
      return false;
    }
  }

  // Returns success status of the set infinite bound operation.
  // The whole buffer should be protected to avoid race conditions with concurrent push_backs.
  bool set_infinite_buffer()
  {
    // For logging consistency
    string action = "Buffer set infinite bound";

    lock_buffer();
    
    try
    {
      // No buffer resizing is necessary since vector will already do that.
      bound = -1;

      unlock_buffer();
      
      log_message(action, false, "");
      return true;
    }
    catch (exception &ex)
    {
      unlock_buffer();

      log_message(action, true, ex.what());
      return false;
    }
  }
};

void log_test_1_sync()
{
  Logger logger;
  logger.log("[1] Hello world!");
  logger.log("[2] How are you doing?");
  logger.log("[3] three!");
  logger.log("[4] What why are you still here?");

  string first;
  bool success1 = logger.read_at(1, first);
  if (success1)
    cout << first << endl;
  else
    cout << "Failure to read log at index 1" << endl;

  string last;
  bool success2 = logger.read_last(last);
  if (success2)
    cout << last << endl;
  else
    cout << "Failure to read log at last index" << endl;

  string all;
  bool success3 = logger.read_all(all);
  if (success3)
    cout << all << endl;
  else
    cout << "Failure to read all logs" << endl;

  string fail;
  bool success4 = logger.read_at(4, fail);
  if (success4)
    cout << fail << endl;
  else
    cout << "Failure to read log at index 4 (this should fail)." << endl;
}

void buff_log_test_1()
{
  shared_ptr<Logger> logger = std::make_shared<Logger>();
  Buffer buffer(logger, "b1");

  buffer.add_back(1);
  buffer.add_back(4);
  buffer.add_back(6);

  int removed;
  buffer.remove_front(removed);

  string result = "";
  logger->read_all(result);

  cout << "=== Buffer Log Test 1 ===" << endl;

  cout << "removed front: " << removed << endl;

  cout << result << endl;
}

void buff_log_test_2()
{
  auto logger = make_shared<Logger>();
  Buffer bufferA(logger, "b1");
  Buffer bufferB(logger, "b2");

  bufferA.add_back(10);
  bufferB.add_back(20);
  bufferA.add_back(30);
  bufferB.add_back(40);

  int outA, outB;
  bufferA.remove_front(outA);
  bufferB.remove_front(outB);

  string result;
  logger->read_all(result);

  cout << "=== Buffer Log Test 2 ===" << endl;
  cout << "BufferA removed: " << outA << endl;
  cout << "BufferB removed: " << outB << endl;
  cout << "Logger content:" << endl;
  cout << result << endl;
}

void buff_log_test_3()
{
  auto logger = make_shared<Logger>();
  Buffer buffer(logger, "b1");

  auto writer = [&](int id) {

    for (int i = 0; i < 5; ++i)
      buffer.add_back(id * 10 + i);
  };

  thread t1(writer, 1);
  thread t2(writer, 2);
  thread t3(writer, 3);

  t1.join();
  t2.join();
  t3.join();

  string result;
  logger->read_all(result);

  cout << "=== Buffer Log Test 3 (Concurrent writes) ===" << endl;
  cout << result << endl;
}

void buff_log_test_4()
{
  auto logger = make_shared<Logger>();
  Buffer buffer(logger, "b1");

  buffer.set_bound(3);
  buffer.add_back(1);
  buffer.add_back(2);
  buffer.add_back(3);
  buffer.add_back(4); // should fail: buffer full

  buffer.remove_front(*(new int)); // consume one

  buffer.add_back(5); // now allowed

  string result;
  logger->read_all(result);

  cout << "=== Buffer Log Test 4 (Bound handling) ===" << endl;
  cout << result << endl;
}

void logger_concurrency_test()
{
  Logger logger;

  auto writer = [&]() {
    for (int i = 0; i < 10; ++i)
      logger.log("Writer log " + to_string(i));
  };

  auto reader = [&]() {
    for (int i = 0; i < 5; ++i)
    {
      string result;
      logger.read_all(result);
    }
  };

  thread t1(writer);
  thread t2(writer);
  thread t3(reader);
  thread t4(reader);

  t1.join();
  t2.join();
  t3.join();
  t4.join();

  string final_log;
  logger.read_all(final_log);

  cout << "=== Logger Concurrency Test ===" << endl;
  cout << final_log << endl;
}

int main(int argc, char *argv[])
{
  buff_log_test_1();
  buff_log_test_2();
  buff_log_test_3();
  buff_log_test_4();

  logger_concurrency_test();
  
  return 0;
}
