/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef _OCEABASE_LIB_UTILITY_OB_PRELOAD_H_
#define _OCEABASE_LIB_UTILITY_OB_PRELOAD_H_

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include <execinfo.h>

#ifdef __ENABLE_PRELOAD__
inline int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
  int (*real_func)(pthread_key_t *key,
                   void (*destructor)(void *)) = (typeof(real_func))dlsym(RTLD_NEXT, "pthread_key_create");
  return real_func(key, destructor);
}

inline int pthread_key_delete(pthread_key_t key)
{
  int (*real_func)(pthread_key_t key) = (typeof(real_func))dlsym(RTLD_NEXT, "pthread_key_delete");
  return real_func(key);
}
#endif /* __ENABLE_PRELOAD__ */

#endif /* _OCEABASE_LIB_UTILITY_OB_PRELOAD_H_ */
