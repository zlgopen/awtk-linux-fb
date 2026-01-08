#ifndef TK_EXIT_NOTIFIER_H
#define TK_EXIT_NOTIFIER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct exit_notifier {
  int exit_efd;
  volatile int exit_flag;
} exit_notifier_t;

int exit_notifier_init(exit_notifier_t *notifier, int flag);
void exit_notifier_deinit(exit_notifier_t *notifier);
int exit_notifier_wait(exit_notifier_t *notifier, int data_fd);
int exit_notifier_signal(exit_notifier_t *notifier, int thread_count);
int exit_notifier_get_flag(exit_notifier_t *notifier);
int exit_notifier_set_flag(exit_notifier_t *notifier, int flag);


#ifdef __cplusplus
}
#endif

#endif /*TK_EXIT_NOTIFIER_H*/