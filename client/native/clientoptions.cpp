#include "clientoptions.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <stdlib.h>
#include <jsoncpp/json/json.h>

extern const char *build_default_home;

enum {
  OVERRIDE_USERNAME = (1<<0),
  OVERRIDE_PLAYERNAME = (1<<1),
  OVERRIDE_SERVER_HOST = (1<<2),
  OVERRIDE_SERVER_PORT = (1<<3),
  OVERRIDE_SERVERNAME = (1<<4),
};

bool ClientOptions::parseCommandLine(int argc, char *argv[])
{
  while (1) {
    switch(getopt(argc, argv, "Ds:P:h:u:p:d:")) {
    case 'd':
      homedir = optarg;
      break;
    case 'D':
      debug_animus = fopen("/tmp/animus-debug.out","w");
      break;
    case 'h':
      server_host = optarg;
      override |= OVERRIDE_SERVER_HOST;
      break;
    case 's':
      servername = optarg;
      override |= OVERRIDE_SERVERNAME;
      break;
    case 'P':
      server_port = atoi(optarg);
      override |= OVERRIDE_SERVER_PORT;
      break;
    case 'u':
      username = optarg;
      override |= OVERRIDE_USERNAME;
      break;
    case 'p':
      playername = optarg;
      override |= OVERRIDE_PLAYERNAME;
      break;
    case '?':
      fprintf(stderr, "usage: %s [-D] [-p port] [-h host] [-u username] [-p playername]\n", argv[0]);
      return false;
    case -1:
      return true;
    }
  }
}

ClientOptions::ClientOptions()
  : override(0),
    configfile(std::string(getenv("HOME"))+"/.hexplore/local.conf"),
    homedir(build_default_home),
    servername("default"),
    username("anonymous"),
    playername("^0"),
    server_host("localhost"),
    server_port(1666),
    debug_animus(NULL)
{
}


bool ClientOptions::parseConfigFile()
{
  struct stat sb;

  if (stat(configfile.c_str(), &sb) < 0) {
    // no file, silently ignore it
    return true;
  }

  Json::Value root;
  Json::Reader reader;
  std::ifstream inp(configfile.c_str(), std::ifstream::in);


  if (!reader.parse(inp, root)) {
    fprintf(stderr, "%s: JSON parse error\n", configfile.c_str());
    return false;
  }
  // find the server block
  if (!(override & OVERRIDE_SERVERNAME)
      && root["default_server"].isString()) {
    servername = root["default_server"].asString();
  }

  root = root["servers"][servername];

  if (!(override & OVERRIDE_USERNAME) && 
      root["username"].isString()) {
    username = root["username"].asString();
  }
  if (!(override & OVERRIDE_PLAYERNAME) && 
      root["player"].isString()) {
    playername = root["player"].asString();
  }
  if (!(override & OVERRIDE_SERVER_HOST) && 
      root["host"].isString()) {
    server_host = root["host"].asString();
    printf("setting host %s\n", server_host.c_str());
  }
  if (!(override & OVERRIDE_SERVER_PORT) && 
      root["port"].isInt()) {
    server_port = root["port"].asInt();
    printf("setting port %d\n", server_port);
  }
  return true;
}

bool ClientOptions::validate()
{
  struct stat sb;

  if (stat(homedir.c_str(), &sb) < 0) {
    fprintf(stderr, "%s: hexplore home directory does not exist\n",
            homedir.c_str());
    return false;
  }
  if (!S_ISDIR(sb.st_mode)) {
    fprintf(stderr, "%s: hexplore home is not a directory\n",
            homedir.c_str());
    return false;
  }

  /*
  if (!username) {
    struct passwd *p = getpwuid(getuid());
    if (!p) {
      fprintf(stderr, "Could not determine username; use '-u'\n");
      return 1;
    }
    username = p->pw_name;
  }
  */
  return true;
}

#if UNIT_TEST
int main(int argc, char *argv[])
{
  ClientOptions conf;
  if (!conf.parseCommandLine(argc, argv)) {
    return 1;
  }

  printf("config file = \"%s\"\n", conf.configfile.c_str());

  if (!conf.parseConfigFile()) {
    return 1;
  }

  printf("username = \"%s\"\n", conf.username.c_str());
  printf("playername = \"%s\"\n", conf.playername.c_str());
  printf("server.host = \"%s\"\n", conf.server_host.c_str());
  printf("server.port = %d\n", conf.server_port);
  return 0;
}
#endif

