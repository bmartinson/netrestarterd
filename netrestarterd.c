#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <signal.h>
#include <sys/wait.h>

#define CHECK_INTERVAL_SEC  10
#define PIDFILE "/var/run/netrestarterd.pid"

// Returns true if we can reach the Internet (e.g., 8.8.8.8)
int isInternetReachable() {
  SCNetworkReachabilityRef ref = SCNetworkReachabilityCreateWithName(NULL, "8.8.8.8");
  if (!ref) return 0;

  SCNetworkReachabilityFlags flags = 0;
  Boolean ok = SCNetworkReachabilityGetFlags(ref, &flags);
  CFRelease(ref);

  return ok &&
    (flags & kSCNetworkFlagsReachable) &&
    !(flags & kSCNetworkFlagsConnectionRequired);
}

// Stores the last known DNS server addresses
static char *last_resolv_conf = NULL;
  static size_t last_resolv_conf_len = 0;

// Returns 1 if DNS servers have changed since last call, 0 otherwise
int hasDNSChanged() {
  FILE *fp = fopen("/etc/resolv.conf", "r");
  if (!fp) return 0;

  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (len < 0) {
    fclose(fp);
    return 0;
  }

  char *buf = malloc(len + 1);
  if (!buf) {
    fclose(fp);
    return 0;
  }

  size_t read_len = fread(buf, 1, len, fp);
  buf[read_len] = '\0';
  fclose(fp);

  int changed = 0;
  if (!last_resolv_conf) {
    last_resolv_conf = buf;
    last_resolv_conf_len = read_len;
    return 0;
  }

  if (last_resolv_conf_len != read_len || memcmp(last_resolv_conf, buf, read_len) != 0) {
    free(last_resolv_conf);
    last_resolv_conf = buf;
    last_resolv_conf_len = read_len;
    changed = 1;
  } else {
    free(buf);
  }

  return changed;
}

/*
 * Takes down all network interfaces, flushes all route tables,
 * then brings all interfaces back up.
 */
void resetNetworkInterfaces() {
  FILE *fp = popen("ifconfig | grep -v '127.0.0.1' | grep -v 'broadcast' | grep 'netmask'", "r");
  if (fp) {
    char buf[256];
    if (fgets(buf, sizeof(buf), fp) != NULL && buf[0] != '\0') {
      pclose(fp);

      // Skip reset if networking masking is detected
      return;
    }
    pclose(fp);
  }

  system(
    "for i in $(ifconfig | egrep -o '^[a-z0-9]+:' | sed 's/://'); do "
    "ifconfig \"$i\" down; "
    "done; "
    "route -n flush; "
    "for i in $(ifconfig | egrep -o '^[a-z0-9]+:' | sed 's/://'); do "
    "ifconfig \"$i\" up; "
    "done"
  );
}

// Detach from terminal and become a true daemon
void daemonize() {
  pid_t pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);      // Fork failed
  if (pid > 0) exit(EXIT_SUCCESS);      // Parent exits

  // Child continues
  if (setsid() < 0) exit(EXIT_FAILURE); // Become session leader

  // Optional second fork to prevent reacquisition of a controlling terminal
  pid = fork();
  if (pid < 0) exit(EXIT_FAILURE);
  if (pid > 0) exit(EXIT_SUCCESS);

  umask(0);                            // Clear file mode creation mask
  chdir("/");                          // Change working directory

  // Close all open file descriptors (not just stdin/out/err)
  for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
    close(fd);
  }

  // Redirect stdin, stdout, stderr to /dev/null
  int fd0 = open("/dev/null", O_RDWR);
  dup2(fd0, STDIN_FILENO);
  dup2(fd0, STDOUT_FILENO);
  dup2(fd0, STDERR_FILENO);
  if (fd0 > 2) close(fd0);
}

// SIGCHLD handler to reap child processes
void reap_child(int sig) {
  (void)sig;
  while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

int check_and_write_pidfile() {
  int fd = open(PIDFILE, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    perror("open pidfile");
    return 1;
  }
  if (lockf(fd, F_TLOCK, 0) < 0) {
    close(fd);
    return 1; // Another instance is running
  }
  // Write our PID
  char buf[32];
  snprintf(buf, sizeof(buf), "%d\n", getpid());
  ftruncate(fd, 0);
  write(fd, buf, strlen(buf));
  // Keep fd open to hold lock
  return 0;
}

int main(int argc, char *argv[]) {
  // Install SIGCHLD handler to reap child processes
  struct sigaction sa;
  sa.sa_handler = reap_child;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigaction(SIGCHLD, &sa, NULL);

  int debug = 0;

  if (argc > 1 && strcmp(argv[1], "--debug") == 0) {
    debug = 1;
    printf("Running in interactive mode...\n\n");
  } else {
    printf("Starting daemon...\n");
    daemonize();
  }

  if (!debug) {
    if (check_and_write_pidfile()) {
      fprintf(stderr, "Another instance of netrestarterd is already running.\n");
      exit(EXIT_FAILURE);
    }
  }

  int was_reachable = isInternetReachable();
  int now_reachable = was_reachable;
  int dns_changed = hasDNSChanged();
  int was_reset = 0;

  while (1) {
    now_reachable = isInternetReachable();
    dns_changed = hasDNSChanged();

    if (debug) {
      if ((!now_reachable && was_reachable) || dns_changed) {
        if (dns_changed) {
          printf("[!] DNS changed\n");
        } else {
          printf("[!] Connection lost\n");
        }

        resetNetworkInterfaces();

        was_reset = 1;
      } else if ((now_reachable && !was_reachable) || was_reset) {
        printf("[+] Connection restored!!\n");
        was_reset = 0;
      } else if (now_reachable) {
        printf("[-] Internet is reachable\n");
      } else {
        printf("[!] Internet is NOT reachable\n");
      }
      fflush(stdout);
    } else {
      if ((!now_reachable && was_reachable) || dns_changed) {
        resetNetworkInterfaces();
      }
    }

    was_reachable = now_reachable;
    sleep(CHECK_INTERVAL_SEC);
  }

  return EXIT_SUCCESS;
}
