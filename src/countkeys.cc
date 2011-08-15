/*
  Copyleft 2011 richo
  Copyleft (ɔ) 2009 Kernc
  This program is free software. It comes with absolutely no warranty whatsoever.
  See COPYING for further information.

  This project was forked from Kernc's countkeys
  
  Project homepage: https://github.com/richoH/countkeys
*/

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <getopt.h>
#include <sys/file.h>
#include <linux/input.h>

#include <config.h>  // config produced from ./configure

#define DEFAULT_LOG_FILE "/var/log/countkeys.log"
#define TMP_PID_FILE     "/var/run/countkeys.pid.lock"

#ifdef INPUT_EVENT_PREFIX  // may be defined by ./configure --enable-evdev-path=PATH
# define INPUT_EVENT_PATH (INPUT_EVENT_PREFIX "/event")  // in which case use it
#else
# define INPUT_EVENT_PATH "/dev/input/event"  // otherwise use the default; a number is appended at runtime; one accesses keyboard e.g. via /dev/input/event1
#endif//INPUT_EVENT_PREFIX

#ifndef INPUT_EVENT_DEVICE
//# define INPUT_EVENT_DEVICE "/dev/input/event4" // uncomment this if you want your input event device statically defined rather than "calculated" at runtime
// better yet, use ./configure --enable-evdev=DEV to specify INPUT_EVENT_DEVICE !
#endif//INPUT_EVENT_DEVICE

void usage() {
  fprintf(stderr,
"Usage: countkeys [OPTION]...\n"
"Log depressed keyboard keys.\n"
"\n"
"  -s, --start               start logging keypresses\n"
"  -o, --output=FILE         log output to FILE [" DEFAULT_LOG_FILE "]\n"
"  -k, --kill                kill running countkeys process\n"
"  -f, --foreground          run in the foreground\n"
"  -d, --device=FILE         input event device [" INPUT_EVENT_PATH "X]\n"
"  -?, --help                print this help\n"
"      --no-func-keys        don't log function keys, only character keys\n"
"\n"
"Examples: countkeys -s -o ~/.secret/keys.log\n"
"          countkeys -s -d /dev/input/event6\n"
"          countkeys -k\n"
"\n"
"countkeys version: " PACKAGE_VERSION "\n"
"countkeys homepage: <https://github.com/richoH/countkeys>\n"
  );
}

// executes cmd and returns string ouput or "ERR" on pipe error
std::string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

bool flag_kill = false;  // kill any running countkeys process

void signal_handler(int interrupt) {
  flag_kill = true;
}

int main(int argc, char **argv) {
  
  
  if (geteuid()) { fprintf(stderr, "Got r00t?\n"); return EXIT_FAILURE; }
  
  if (argc < 2) { usage(); return EXIT_FAILURE; }
  
  bool flag_start = false;   // start logger
  bool flag_foreground = false;   // start logger
  // bool flag_kill; is defined global for signal_handler to access it
  
  char *log_filename = (char*) DEFAULT_LOG_FILE;  // default log file
  char log_file_path[512]; // don't use paths longer than 512 B !!
  char *device_filename = NULL;  // path to input event device if given with -d switch

  { // process options and arguments
    
    struct option long_options[] = {
      {"start",     no_argument,       0, 's'},
      {"output",    required_argument, 0, 'o'},
      {"kill",      no_argument,       0, 'k'},
      {"foreground",no_argument,       0, 'f'},
      {"device",    required_argument, 0, 'd'},
      {"help",      no_argument,       0, '?'},
#define EXPORT_KEYMAP_INDEX 7
      {0, 0, 0, 0}
    };
    
    char c;
    int option_index;
    
    while ((c = getopt_long(argc, argv, "sm:o:ukfd:?", long_options, &option_index)) != -1)
      switch (c) {
        case 's': flag_start = true;                            break;
        case 'o': log_filename = optarg;                        break;
        case 'k': flag_kill = true;                             break;
        case 'f': flag_foreground = true;                       break;
        case 'd': device_filename = optarg;                     break;
        
        case  0 : 
          break;  // + flag_export or flag_nofunc already set
        
        case '?': usage(); return EXIT_SUCCESS;
        default : usage(); return EXIT_FAILURE;
      }

    while(optind < argc)
      fprintf(stderr, "%s: Non-option argument %s\n", argv[0], argv[optind++]);
  } //\ arguments
  
  // kill existing countkeys process
  if (flag_kill) {
    FILE *temp_file = fopen(TMP_PID_FILE, "r");
    pid_t pid;
    if ((temp_file != NULL && fscanf(temp_file, "%d", &pid) == 1 && fclose(temp_file) == 0) || 
        (sscanf( exec("pgrep countkeys").c_str(), "%d", &pid) == 1 && pid != getpid())) { // if reading PID from temp_file failed, try pgrep pipe
      remove(TMP_PID_FILE);
      kill(pid, SIGINT);
      return EXIT_SUCCESS;
    }
    fprintf(stderr, "%s: No process killed.\n", argv[0]);
    return EXIT_FAILURE;
  
  
  } else if (!flag_foreground && !flag_start) { usage(); return EXIT_FAILURE; }
  // if user provided a relative path to output file :/ stupid user :/
  if (log_filename[0] != '/') {
    if (getcwd(log_file_path, sizeof(log_file_path) - strlen(log_filename) - 2 /* '/' and '\0' */) == NULL) {
      fprintf(stderr, "%s: Error copying CWD: %s%s\n", argv[0], strerror(errno), 
        (errno == ERANGE ? " (CWD path too long, GO FUCK YOURSELF!!)" : ""));
      return EXIT_FAILURE;
    }
    strcat(log_file_path, "/");
    strncat(log_file_path, log_filename, sizeof(log_file_path) - strlen(log_file_path));
    log_filename = log_file_path;
  }
  
  // set locale to common UTF-8 for wchars to be recognized correctly
  if(setlocale(LC_CTYPE, "en_US.UTF-8") == NULL)  // if en_US.UTF-8 isn't available
    setlocale(LC_CTYPE, "");  // try the locale that corresponds to the value of the associated environment variables, LC_CTYPE and LANG
  // else just leave the burden of possible Fail to the user, without any warning :D
  
  
#ifndef INPUT_EVENT_DEVICE  // sometimes X in /dev/input/eventX is different from one reboot to another
  
  char *INPUT_EVENT_DEVICE;
  
  if (device_filename == NULL) {  // no device given with -d switch, determine it automatically
    // extract input number from /proc/bus/input/devices (I don't know how to do it better. If you have an idea, please let me know.)
    std::string output = exec("grep Name /proc/bus/input/devices | grep -nE '[Kk]eyboard|kbd'");
    if (output == "ERR") { // if pipe errors, exit
      fprintf(stderr, "%s: Cannot determine keyboard input event device: %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }
    
    // the correct input event # is (output - 1)
    std::stringstream input_fd_filename;
    input_fd_filename << INPUT_EVENT_PATH;
    input_fd_filename << (atoi(output.c_str()) - 1);
    
    INPUT_EVENT_DEVICE = (char*)input_fd_filename.str().c_str();
    
  } else INPUT_EVENT_DEVICE = device_filename;  // event device supplied as -d argument

#endif//INPUT_EVENT_DEVICE
  
  { // catch SIGHUP, SIGINT and SIGTERM signals to exit gracefully
    
    struct sigaction act = {};
    act.sa_handler = signal_handler;
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
  } //\ signals
  
  { // everything went well up to now, let's become daemon
    
   if (!flag_foreground) {
    switch (fork()) {
      case 0: break;  // child continues
      case -1:  // error
        fprintf(stderr, "%s: Error creating child process: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
      default: return EXIT_SUCCESS;  // parent exits
    }
    setsid();
    if(chdir("/"));  // wrapped in if() to avoid compiler warning
    switch (fork()) {
      case 0: break; // second child continues
      case -1: 
        fprintf(stderr, "%s: Error creating second child process: %s\n", argv[0], strerror(errno));
        return EXIT_FAILURE;
      default: return EXIT_SUCCESS;  // parent exits
    }
    close(0);  // drop stdin
   }
  } //\ daemon
  
  // create temp file carrying PID for later retrieval
  int temp_fd;
  if ((temp_fd = open(TMP_PID_FILE, O_WRONLY | O_CREAT, 0600)) == -1) {
    fprintf(stderr, "%s: Error opening temporary file '" TMP_PID_FILE "': %s\n", argv[0], strerror(errno));
    return EXIT_FAILURE;
  }
  if (flock(temp_fd, LOCK_EX | LOCK_NB) == 0) {  // this is the first process to request temp file, now block all others
    if (flock(temp_fd, LOCK_SH | LOCK_NB) == -1) {
      fprintf(stderr, "%s: Error obtaining lock on temporary file '" TMP_PID_FILE "': %s\n", argv[0], strerror(errno));
      return EXIT_FAILURE;
    }
    char pid_str[16] = {0};
    sprintf(pid_str, "%d", getpid());
    if (write(temp_fd, pid_str, strlen(pid_str)) == -1) {
      fprintf(stderr, "%s: Error writing to temporary file '" TMP_PID_FILE "': %s\n", argv[0], strerror(errno));
    }
  } else {  // another countkeys process is already running, therefore terminate this one
    fprintf(stderr, "%s: Another process already running. Quitting.\n", argv[0]);
    return EXIT_FAILURE;
  } //\ temp file
  
  // open input device for reading
  int input_fd = open(INPUT_EVENT_DEVICE, O_RDONLY | O_NONBLOCK);
  if (input_fd == -1) {
    fprintf(stderr, "%s: Error opening input event device '%s': %s\n", argv[0], INPUT_EVENT_DEVICE, strerror(errno));
    return EXIT_FAILURE;
  }
  
  // open log file as stdout (if file doesn't exist, create it with safe 0600 permissions)
  //umask(0177); FIXME
  if (!flag_foreground) {
      stdout = freopen(log_filename, "a", stdout);
      if (stdout == NULL) {
        fprintf(stderr, "%s: Error opening output file '%s': %s\n", argv[0], log_filename, strerror(errno));
        return EXIT_FAILURE;
      }
  }
  
  // we've got everything we need, now drop privileges by becoming 'nobody'
  setgid(65534); setuid(65534);
  
  struct input_event event;
  char timestamp[32];  // timestamp string, long enough to hold "\n%F %T%z > "
  int presses = 0; // Counter to keep track of presses

  time_t cur_time;
  time(&cur_time);
  strftime(timestamp, sizeof(timestamp), "\n%F %T%z > ", localtime(&cur_time));
  
  fprintf(stdout, "Logging started ...\n%s", timestamp);
  fflush(stdout);

  while (true) {  // infinite loop: exit gracefully by receiving SIGHUP, SIGINT or SIGTERM

    if (flag_kill) break;  // if process received kill signal, end it

// these event.value-s aren't defined in <linux/input.h> ?
#define EV_MAKE   1  // when key pressed
#define EV_BREAK  0  // when key released
#define EV_REPEAT 2  // when key switches to repeating after short delay
    
    while (read(input_fd, &event, sizeof(struct input_event)) > 0) {
      
      if (event.type == EV_KEY) {  // keyboard events always of type EV_KEY
        
        // on key press
        if (event.value == EV_MAKE) {
            presses++;
        }
      }
    }
    
    sleep(1);
  } //\ while (true)
  
  // append final timestamp, close files and exit
  time(&cur_time);
  strftime(timestamp, sizeof(timestamp), "%F %T%z", localtime(&cur_time));
  fprintf(stdout, "\n\nKeys pressed: %i\n\n", presses);
  fprintf(stdout, "\n\nLogging stopped at %s\n\n", timestamp);
  
  fclose(stdout);
  close(input_fd);
  close(temp_fd);
  
  remove(TMP_PID_FILE);
  
  return EXIT_SUCCESS;
}
