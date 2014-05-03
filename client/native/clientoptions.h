#ifndef _H_HEXPLORE_CLIENT_OPTIONS
#define _H_HEXPLORE_CLIENT_OPTIONS

#include <stdio.h>
#include <string>

struct ClientOptions {
  ClientOptions();
  bool parseCommandLine(int argc, char *argv[]);
  bool parseConfigFile();
  bool validate();

  unsigned override;
  std::string configfile;
  // NOTE: We chdir() to homedir at startup, so all file operations
  //       should be relative to the homedir
  std::string homedir;
  std::string servername;
  std::string username;
  std::string playername;
  std::string server_host;
  int server_port;
  FILE *debug_animus;
};


#endif /* _H_HEXPLORE_CLIENT_OPTIONS */
