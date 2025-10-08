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

// although it is good habit, you don't have to type 'std::' before many objects by including this line
using namespace std;

class Logger
{
  private:
  vector<string> logs;
  mutex log_mtx;

  // Internal helper (assumes mutex is already locked)
  bool _read_at_nolock(const int index, std::string& message) {
      if (index < 0 || index >= static_cast<int>(logs.size()))
          return false;

      message = logs.at(index);
      return true;
  }
  
  public:
  void log(const string& message) {
    log_mtx.lock();
    try {
      logs.push_back(message);
      log_mtx.unlock();
    } catch (exception& ex) {
      cerr << "Caught exception: " << ex.what() << endl;
      log_mtx.unlock();
    }
  }

  // Returns whether retrieving the last message went okay. If it went ok, the message is stored to the given message reference.
  bool read_last(string& message) {
    log_mtx.lock();
    try {
      const int last_index = logs.size() - 1;
      bool ret = _read_at_nolock(last_index, message);
      log_mtx.unlock();

      return ret;
    } catch (exception& ex) {
      cerr << "Caught exception: " << ex.what() << endl;
      log_mtx.unlock();

      return false;
    }

  }

  bool read_at(const int index, string& message) {
    log_mtx.lock();
    try {
      bool ret = _read_at_nolock(index, message);
      log_mtx.unlock();

      return ret;
    } catch (exception& ex) {
      cerr << "Caught exception: " << ex.what() << endl;
      log_mtx.unlock();

      return false;
    }
  }

};

int main(int argc, char *argv[])
{
  std::cout << "Hello world" << std::endl;
  return 0;
}
