/**************************************************************************
 *
 * Copyright (C) 2015 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * virgl_server2.h - Server bridge for virglrenderer 1.3.0 + Android/GameNative
 *
 * This is an adapted version of the original Winlator server code,
 * updated to use the virglrenderer 1.3.0 public API instead of the
 * old per-client vrend_* functions from Mesa 23.1.9.
 */

#ifndef VIRGL_SERVER3_H
#define VIRGL_SERVER3_H

#include <errno.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdbool.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include "virglrenderer.h"
#include "virgl_server_protocol.h"

#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "VirGL3", __VA_ARGS__);

struct jni_info3 {
   jobject obj;
   JNIEnv *env;
   jmethodID kill_connection;
   jmethodID get_shared_egl_context;
   jmethodID flush_frontbuffer;
};

struct virgl_server3_state {
   int ctx_id;
   int fence_id;
   int last_fence_id;
   GLuint framebuffer;
   int handle;

   EGLDisplay egl_display;
   EGLConfig egl_conf;
   EGLContext egl_ctx;

   bool initialized;
};

struct virgl_client3 {
   int fd;
   struct virgl_server3_state *state;
   bool initialized;
};

extern struct jni_info3 jni_info3;

int virgl_server3_create_renderer(struct virgl_client3 *client, uint32_t length);
int virgl_server3_send_caps(struct virgl_client3 *client, uint32_t length);
int virgl_server3_resource_create(struct virgl_client3 *client, uint32_t length);
int virgl_server3_resource_destroy(struct virgl_client3 *client, uint32_t length);
int virgl_server3_transfer_get(struct virgl_client3 *client, uint32_t length);
int virgl_server3_transfer_put(struct virgl_client3 *client, uint32_t length);
int virgl_server3_submit_cmd(struct virgl_client3 *client, uint32_t length);
int virgl_server3_resource_busy_wait(struct virgl_client3 *client, uint32_t length);
int virgl_server3_flush_frontbuffer(struct virgl_client3 *client, uint32_t length);

int virgl_block_read3(int fd, void *buf, int size);

int virgl_server3_renderer_create_fence(struct virgl_client3 *client);

void virgl_server3_destroy_renderer(struct virgl_client3 *client);

#endif

