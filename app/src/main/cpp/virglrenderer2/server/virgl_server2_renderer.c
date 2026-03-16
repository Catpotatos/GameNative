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
 * virgl_server2_renderer.c - Renderer implementation for virglrenderer2
 *
 * Uses the virglrenderer 1.3.0 public API (virgl_renderer_*) for
 * initialization, resource management, command submission, etc.
 * The EGL context is created for Android GLES and shared with the
 * app's GLSurfaceView.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/mman.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#include "virgl_server2.h"
#include "virgl_server_shm.h"
#include "virgl_hw.h"

#include <jni.h>

/* Fence callback from virglrenderer */
static void virgl_server2_write_fence(void *cookie, uint32_t fence_id)
{
   struct virgl_client2 *client = (struct virgl_client2 *)cookie;
   if (client && client->state)
      client->state->last_fence_id = fence_id;
}

/* GL context creation callback */
static virgl_renderer_gl_context
virgl_server2_create_gl_context(void *cookie, int scanout_idx,
                                struct virgl_renderer_gl_ctx_param *param)
{
   struct virgl_client2 *client = (struct virgl_client2 *)cookie;
   if (!client || !client->state) return NULL;

   struct virgl_server2_state *state = client->state;
   EGLint ctx_att[] = {
      EGL_CONTEXT_CLIENT_VERSION, 3,
      EGL_NONE
   };

   EGLContext egl_ctx = eglCreateContext(state->egl_display,
                                        state->egl_conf,
                                        state->egl_ctx,
                                        ctx_att);
   return (virgl_renderer_gl_context)egl_ctx;
}

/* GL context destruction callback */
static void virgl_server2_destroy_gl_context(void *cookie,
                                             virgl_renderer_gl_context ctx)
{
   struct virgl_client2 *client = (struct virgl_client2 *)cookie;
   if (!client || !client->state) return;
   eglDestroyContext(client->state->egl_display, (EGLContext)ctx);
}

/* Make GL context current callback */
static int virgl_server2_make_current(void *cookie, int scanout_idx,
                                      virgl_renderer_gl_context ctx)
{
   struct virgl_client2 *client = (struct virgl_client2 *)cookie;
   if (!client || !client->state) return -1;
   return eglMakeCurrent(client->state->egl_display,
                         EGL_NO_SURFACE, EGL_NO_SURFACE,
                         (EGLContext)ctx) ? 0 : -1;
}

static struct virgl_renderer_callbacks virgl_server2_cbs = {
   .version = 1,
   .write_fence = virgl_server2_write_fence,
   .create_gl_context = virgl_server2_create_gl_context,
   .destroy_gl_context = virgl_server2_destroy_gl_context,
   .make_current = virgl_server2_make_current,
};

static bool virgl_server2_egl_init(struct virgl_server2_state *state)
{
    static EGLint conf_att[] = {
       EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
       EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
       EGL_RED_SIZE, 8,
       EGL_GREEN_SIZE, 8,
       EGL_BLUE_SIZE, 8,
       EGL_ALPHA_SIZE, 0,
       EGL_NONE,
    };
    static const EGLint ctx_att[] = {
       EGL_CONTEXT_CLIENT_VERSION, 3,
       EGL_NONE
    };

    EGLBoolean success;
    EGLint major, minor, num_configs;

    state->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!state->egl_display)
        return false;

    success = eglInitialize(state->egl_display, &major, &minor);
    if (!success)
        return false;

    success = eglBindAPI(EGL_OPENGL_ES_API);
    if (!success)
        return false;

    success = eglChooseConfig(state->egl_display, conf_att, &state->egl_conf,
                              1, &num_configs);
    if (!success || num_configs != 1)
        return false;

    /* Get shared EGL context from the Android GLSurfaceView */
    jlong shared_egl_ctx_ptr = (*jni_info2.env)->CallLongMethod(
        jni_info2.env, jni_info2.obj, jni_info2.get_shared_egl_context);
    EGLContext shared_egl_ctx = (EGLContext)shared_egl_ctx_ptr;

    state->egl_ctx = eglCreateContext(state->egl_display,
                                      state->egl_conf,
                                      shared_egl_ctx ? shared_egl_ctx : EGL_NO_CONTEXT,
                                      ctx_att);

    eglMakeCurrent(state->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, state->egl_ctx);
    if (!state->egl_ctx)
        return false;

    return true;
}

static int virgl_block_write2(int fd, void *buf, int size)
{
   char *ptr = buf;
   int left;
   int ret;
   left = size;

   do {
      ret = write(fd, ptr, left);
      if (ret < 0)
         return -errno;

      left -= ret;
      ptr += ret;
   } while (left);

   return size;
}

int virgl_block_read2(int fd, void *buf, int size)
{
   char *ptr = buf;
   int left;
   int ret;

   left = size;
   do {
      ret = read(fd, ptr, left);
      if (ret <= 0)
         return ret == -1 ? -errno : 0;

      left -= ret;
      ptr += ret;
   } while (left);

   return size;
}

static int virgl_server2_send_fd(int sock_fd, int fd)
{
    struct iovec iovec;
    char buf[CMSG_SPACE(sizeof(int))], c;
    struct msghdr msgh = { 0 };
    memset(buf, 0, sizeof(buf));

    iovec.iov_base = &c;
    iovec.iov_len = sizeof(char);

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = &iovec;
    msgh.msg_iovlen = 1;
    msgh.msg_control = buf;
    msgh.msg_controllen = sizeof(buf);
    msgh.msg_flags = 0;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *((int *)CMSG_DATA(cmsg)) = fd;

    int size = sendmsg(sock_fd, &msgh, 0);
    if (size < 0)
      return -EINVAL;

    return 0;
}

int virgl_server2_create_renderer(struct virgl_client2 *client, uint32_t length)
{
   int ret;
   struct virgl_server2_state *state = calloc(1, sizeof(struct virgl_server2_state));
   state->ctx_id = 1;

   client->state = state;

   virgl_server2_egl_init(state);

   /* Initialize virglrenderer 1.3.0 with GLES flag */
   ret = virgl_renderer_init(client,
                             VIRGL_RENDERER_USE_GLES | VIRGL_RENDERER_USE_SURFACELESS,
                             &virgl_server2_cbs);
   if (ret)
      return -1;

   ret = virgl_renderer_context_create(state->ctx_id, 0, NULL);
   return ret;
}

void virgl_server2_destroy_renderer(struct virgl_client2 *client)
{
   if (!client->initialized)
      return;

   if (client->state->framebuffer)
      glDeleteFramebuffers(1, &client->state->framebuffer);

   virgl_renderer_context_destroy(client->state->ctx_id);
   virgl_renderer_poll();

   free(client->state);
   client->state = NULL;
   client->initialized = false;
}

int virgl_server2_send_caps(struct virgl_client2 *client, uint32_t length)
{
   uint32_t send_buf[2];
   void *caps_buf;
   int ret;
   uint32_t max_ver, max_size;

   virgl_renderer_get_cap_set(2, &max_ver, &max_size);

   if (max_size == 0)
      return -1;

   caps_buf = malloc(max_size);
   if (!caps_buf)
      return -1;

   virgl_renderer_fill_caps(2, 1, caps_buf);

   send_buf[0] = max_size + 1;
   send_buf[1] = 2;
   ret = virgl_block_write2(client->fd, send_buf, 8);
   if (ret < 0)
      goto end;

   virgl_block_write2(client->fd, caps_buf, max_size);

end:
   free(caps_buf);
   return 0;
}

int virgl_server2_resource_create(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[11];
   struct virgl_renderer_resource_create_args args;
   struct iovec *iov;
   int ret, fd;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return -1;

   args.handle = recv_buf[0];
   args.target = recv_buf[1];
   args.format = recv_buf[2];
   args.bind = recv_buf[3];
   args.width = recv_buf[4];
   args.height = recv_buf[5];
   args.depth = recv_buf[6];
   args.array_size = recv_buf[7];
   args.last_level = recv_buf[8];
   args.nr_samples = recv_buf[9];
   args.flags = 0;

   iov = calloc(1, sizeof(struct iovec));
   if (!iov)
      return -ENOMEM;

   iov->iov_len = recv_buf[10];

   ret = virgl_renderer_resource_create(&args, NULL, 0);
   if (ret)
      return ret;

   virgl_renderer_ctx_attach_resource(client->state->ctx_id, args.handle);

   if (iov->iov_len == 0) {
      iov->iov_base = NULL;
      goto attach;
   }

   fd = virgl_server_new_shm(args.handle, iov->iov_len);
   if (fd < 0) {
      free(iov);
      return fd;
   }

   iov->iov_base = mmap(NULL, iov->iov_len, PROT_WRITE | PROT_READ,
                          MAP_SHARED, fd, 0);

   if (iov->iov_base == MAP_FAILED) {
      close(fd);
      free(iov);
      return -ENOMEM;
   }

   ret = virgl_server2_send_fd(client->fd, fd);
   if (ret < 0) {
      close(fd);
      munmap(iov->iov_base, iov->iov_len);
      free(iov);
      return ret;
   }

   close(fd);

attach:
   virgl_renderer_resource_attach_iov(args.handle, iov, 1);
   return 0;
}

int virgl_server2_resource_destroy(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[1];
   int ret;
   uint32_t handle;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return -1;

   handle = recv_buf[0];

   virgl_renderer_resource_detach_iov(handle, NULL, NULL);
   virgl_renderer_resource_unref(handle);
   return 0;
}

int virgl_server2_transfer_get(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[10];
   int ret;
   struct virgl_box box;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return ret;

   box.x = recv_buf[2];
   box.y = recv_buf[3];
   box.z = recv_buf[4];
   box.w = recv_buf[5];
   box.h = recv_buf[6];
   box.d = recv_buf[7];

   ret = virgl_renderer_transfer_read_iov(recv_buf[0],          /* handle */
                                          client->state->ctx_id,
                                          recv_buf[1],           /* level */
                                          0,                     /* stride */
                                          0,                     /* layer_stride */
                                          &box,
                                          recv_buf[9],           /* offset */
                                          NULL, 0);

   return ret;
}

int virgl_server2_transfer_put(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[10];
   int ret;
   struct virgl_box box;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return ret;

   box.x = recv_buf[2];
   box.y = recv_buf[3];
   box.z = recv_buf[4];
   box.w = recv_buf[5];
   box.h = recv_buf[6];
   box.d = recv_buf[7];

   ret = virgl_renderer_transfer_write_iov(recv_buf[0],          /* handle */
                                           client->state->ctx_id,
                                           recv_buf[1],           /* level */
                                           0,                     /* stride */
                                           0,                     /* layer_stride */
                                           &box,
                                           recv_buf[9],           /* offset */
                                           NULL, 0);

   return ret;
}

int virgl_server2_submit_cmd(struct virgl_client2 *client, uint32_t length)
{
   uint32_t *cbuf;
   int cbuf_len, ret;

   cbuf_len = length * 4;
   cbuf = malloc(cbuf_len);
   if (!cbuf)
      return -1;

   ret = virgl_block_read2(client->fd, cbuf, cbuf_len);
   if (ret != cbuf_len) {
      free(cbuf);
      return -1;
   }

   virgl_renderer_submit_cmd(cbuf, client->state->ctx_id, length);

   free(cbuf);
   virgl_server2_renderer_create_fence(client);
   return 0;
}

int virgl_server2_resource_busy_wait(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[2];
   uint32_t send_buf[3];
   int ret;
   int flags;
   bool busy = false;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return -1;

   flags = recv_buf[1];

   do {
      busy = client->state->last_fence_id != client->state->fence_id;
      if (!busy || !(flags & VCMD_BUSY_WAIT_FLAG_WAIT))
         break;

      virgl_renderer_poll();
   } while (1);

   send_buf[0] = 1;
   send_buf[1] = VCMD_RESOURCE_BUSY_WAIT;
   send_buf[2] = busy ? 1 : 0;

   ret = virgl_block_write2(client->fd, send_buf, sizeof(send_buf));
   if (ret < 0)
      return ret;

   return 0;
}

int virgl_server2_flush_frontbuffer(struct virgl_client2 *client, uint32_t length)
{
   uint32_t recv_buf[2];
   uint32_t handle, drawable;
   int ret;

   ret = virgl_block_read2(client->fd, &recv_buf, sizeof(recv_buf));
   if (ret != sizeof(recv_buf))
      return -1;

   handle = recv_buf[0];
   drawable = recv_buf[1];

   /* For frontbuffer flush, we need to use the vrend resource's GL texture.
    * Since we're using the public API, we need a different approach.
    * We'll request the texture FD and bind it to a framebuffer.
    */
   if (handle != client->state->handle) {
      if (client->state->framebuffer)
         glDeleteFramebuffers(1, &client->state->framebuffer);

      GLuint framebuffer;
      glGenFramebuffers(1, &framebuffer);

      /* TODO: For proper frontbuffer binding with 1.3.0, we may need
       * to use virgl_renderer_get_fd_for_texture or access the resource
       * through the internal API. For now, use the same approach as v1. */
      client->state->framebuffer = framebuffer;
      client->state->handle = handle;
   }

   (*jni_info2.env)->CallVoidMethod(jni_info2.env, jni_info2.obj,
      jni_info2.flush_frontbuffer, drawable, client->state->framebuffer);
   return 0;
}

int virgl_server2_renderer_create_fence(struct virgl_client2 *client)
{
   virgl_renderer_create_fence(++client->state->fence_id, client->state->ctx_id);
   return 0;
}

JNIEXPORT jlong JNICALL
Java_com_winlator_xenvironment_components_VirGLRenderer2Component_getCurrentEGLContextPtr(JNIEnv *env, jobject obj) {
   EGLContext egl_ctx = eglGetCurrentContext();
   return egl_ctx != EGL_NO_CONTEXT ? (jlong)egl_ctx : 0;
}

