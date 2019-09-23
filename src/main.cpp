#include <iostream>
#include "mythread.h"
#include "server.h"

int main() {
  std::cout << "Running!" << std::endl;

  Server *s;
  s = new Server();

  //Main loop
  s->AcceptAndDispatch();

  return 0;
}
