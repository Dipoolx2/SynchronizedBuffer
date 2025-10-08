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
  vector<string> log_vector;
  mutex log_mtx;

  // Internal helper (assumes mutex is already locked)
  bool _read_at_nolock(const int index, std::string &message)
  {
    if (index < 0 || index >= static_cast<int>(log_vector.size()))
      return false;

    message = log_vector.at(index);
    return true;
  }

public:
  void log(const string &message)
  {
    log_mtx.lock();

    try
    {
      log_vector.push_back(message);

      log_mtx.unlock();
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      log_mtx.unlock();
    }
  }

  // Returns whether retrieving the last message went okay. If it went ok, the message is stored to the given message reference.
  bool read_last(string &message)
  {
    log_mtx.lock();

    try
    {
      const int last_index = log_vector.size() - 1;
      bool ret = _read_at_nolock(last_index, message);

      log_mtx.unlock();
      return ret;
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;

      log_mtx.unlock();
      return false;
    }
  }

  bool read_at(const int index, string &message)
  {
    log_mtx.lock();

    try
    {
      bool ret = _read_at_nolock(index, message);
      log_mtx.unlock();

      return ret;
    }
    catch (exception &ex)
    {
      cerr << "Caught exception: " << ex.what() << endl;
      log_mtx.unlock();

      return false;
    }
  }

  string read_all()
  {
    log_mtx.lock();

    string ret = "";
    try
    {
      for (string log : log_vector)
      {
        ret = ret + log + "\n";
      }
      log_mtx.unlock();
    }
    catch (const exception &e)
    {
      cerr << "Caught exception: " << e.what() << endl;
      log_mtx.unlock();
    }

    return ret;
  }

  void clear()
  {
    // clear the logger
    log_mtx.lock();
    try
    {
      log_vector.erase(log_vector.begin(), log_vector.end());

      log_mtx.unlock();
    }
    catch (const exception &e)
    {
      cerr << "Caught exception: " << e.what() << endl;

      log_mtx.unlock();
    }
  }
};

class Buffer {
  vector<int>
}

int main(int argc, char *argv[])
{
  std::cout << "Hello world" << std::endl;
  return 0;
}
