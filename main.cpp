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
  
  public:
  void log(const string& message) {
    logs.push_back(message);
  }

  // Returns whether retrieving the last message went okay. If it went ok, the message is stored to the given message reference.
  bool read_last(string& message) {
    const int last_index = logs.size() - 1;

    return read_at(last_index, message);
  }

  bool read_at(const int index, string& message) {
    if (index < 0 || index >= logs.size()) {
      return false;
    }

    message = logs.at( index );
    return true;
  }
};

int main(int argc, char *argv[])
{
  std::cout << "Hello world" << std::endl;
  return 0;
}
