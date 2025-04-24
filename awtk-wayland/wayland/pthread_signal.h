/**
 * File:   pthread_signal.h
 * Author: AWTK Develop Team
 * Brief:  thread signal
 *
 * Copyright (c) 2018 - 2024 Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2024-07-17 Yang Zewu <yangzewu@zlg.cn> created
 *
 */

#ifndef UI_AWTK_WAYLAND_PTHREAD_SIGNAL_H
#define UI_AWTK_WAYLAND_PTHREAD_SIGNAL_H

#include <pthread.h>

typedef struct ThreadSignal_T {
	pthread_cond_t cond;
  pthread_mutex_t mutex;

  pthread_condattr_t cattr;
} ThreadSignal;

static void ThreadSignal_Init(ThreadSignal *signal) {
	pthread_mutex_init(&signal->mutex, NULL);

	pthread_cond_init(&signal->cond, NULL);
}

static void ThreadSignal_Close(ThreadSignal *signal) {
  pthread_mutex_destroy(&signal->mutex);
  pthread_cond_destroy(&signal->cond);
}

static void ThreadSignal_Wait(ThreadSignal *signal) {
  pthread_mutex_lock(&signal->mutex);
  pthread_cond_wait(&signal->cond, &signal->mutex);
  pthread_mutex_unlock(&signal->mutex);
}

static void ThreadSignal_Signal(ThreadSignal *signal) {
  pthread_mutex_lock(&signal->mutex);
  pthread_cond_signal(&signal->cond);
  pthread_mutex_unlock(&signal->mutex);
}

#endif /* UI_AWTK_WAYLAND_PTHREAD_SIGNAL_H */
