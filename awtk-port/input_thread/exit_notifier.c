#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/eventfd.h>
#include "exit_notifier.h"


int exit_notifier_init(exit_notifier_t *notifier, int flag) {
  assert(notifier);

  notifier->exit_efd = eventfd(0, EFD_SEMAPHORE);
  if (notifier->exit_efd < 0) {
    perror("eventfd");
    return -1;
  }
  notifier->exit_flag = flag;
  return 0;
}

void exit_notifier_deinit(exit_notifier_t *notifier) {
  assert(notifier);
  assert(notifier->exit_efd >= 0);

  close(notifier->exit_efd);
  notifier->exit_efd = -1;
  notifier->exit_flag = 0;
}

int exit_notifier_wait(exit_notifier_t *notifier, int data_fd) {
  assert(notifier);
  assert(notifier->exit_efd >= 0);

  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(data_fd, &rfds);
  FD_SET(notifier->exit_efd, &rfds);

  int maxfd = (data_fd > notifier->exit_efd ? data_fd : notifier->exit_efd) + 1;

  int s_rc;
  while ((s_rc = select(maxfd, &rfds, NULL, NULL, NULL)) < 0) {
    if (errno == EINTR) {
      FD_ZERO(&rfds);
      FD_SET(data_fd, &rfds);
      FD_SET(notifier->exit_efd, &rfds);
      continue;
    }
    perror("select");
    return -1;
  }

  if (s_rc == 0) {
    errno = ETIMEDOUT;
    return -1;
  }

  if (FD_ISSET(notifier->exit_efd, &rfds)) {
    uint64_t v;
    if (read(notifier->exit_efd, &v, sizeof(v)) != sizeof(v)) {
      perror("read eventfd");
      return -1;
    }
    return notifier->exit_efd;  // exit event
  }

  if (FD_ISSET(data_fd, &rfds)) {
    return data_fd;
  }

  return -1;
}

int exit_notifier_signal(exit_notifier_t *notifier, int thread_count) {
  assert(notifier);
  assert(notifier->exit_efd >= 0);

  uint64_t v = thread_count;
  if (write(notifier->exit_efd, &v, sizeof(v)) != sizeof(v)) {
    perror("write eventfd");
    return -1;
  }
  return 0;
}

int exit_notifier_get_flag(exit_notifier_t *notifier) {
  assert(notifier);
  assert(notifier->exit_efd >= 0);

  return notifier->exit_flag;
}

int exit_notifier_set_flag(exit_notifier_t *notifier, int flag) {
  assert(notifier);
  assert(notifier->exit_efd >= 0);

  notifier->exit_flag = flag;
  return 0;
}
