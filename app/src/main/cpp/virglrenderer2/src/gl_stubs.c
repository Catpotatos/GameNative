/*
 * gl_stubs.c - Stub implementations for desktop GL functions not in GLES
 *
 * These functions are referenced by virglrenderer 1.3.0 but are not available
 * in Android's GLES implementation. They provide no-op stubs so the library
 * links. At runtime, virglrenderer checks capabilities and should not actually
 * call these code paths on GLES.
 */

#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <android/log.h>
#include <string.h>
#include <time.h>

#define STUB_TAG "VirGL-Stub"
#define STUB_WARN(name) __android_log_print(ANDROID_LOG_WARN, STUB_TAG, "stub: %s", name)

/* === Depth / Range (double-precision wrappers) === */
void glClearDepth(double depth) { glClearDepthf((float)depth); }
void glDepthRange(double n, double f) { glDepthRangef((float)n, (float)f); }
void glDepthRangefOES(float n, float f) { glDepthRangef(n, f); }

/* === Indexed Viewport/Scissor/Depth === */
void glViewportIndexedf(GLuint i, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
    if (i == 0) glViewport((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
}
void glScissorIndexed(GLuint i, GLint l, GLint b, GLsizei w, GLsizei h) {
    if (i == 0) glScissor(l, b, w, h);
}
void glDepthRangeIndexed(GLuint i, double n, double f) {
    if (i == 0) glDepthRangef((float)n, (float)f);
}
void glDepthRangeIndexedfOES(GLuint i, float n, float f) {
    if (i == 0) glDepthRangef(n, f);
}

/* === Framebuffer / Renderbuffer === */
void glFramebufferTexture1D(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l) { (void)t;(void)a;(void)tt;(void)tex;(void)l; }
void glFramebufferTexture3D(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l, GLint z) {
    glFramebufferTextureLayer(t, a, tex, l, z);
}
void glFramebufferTexture2DMultisampleEXT(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l, GLsizei s) {
    glFramebufferTexture2D(t, a, tt, tex, l);
}
void glRenderbufferStorageMultisampleEXT(GLenum t, GLsizei s, GLenum fmt, GLsizei w, GLsizei h) {
    glRenderbufferStorageMultisample(t, s, fmt, w, h);
}

/* === Texture functions === */
void glTextureView(GLuint t, GLenum tg, GLuint o, GLenum f, GLuint ml, GLuint nl, GLuint mly, GLuint nly) {
    (void)t;(void)tg;(void)o;(void)f;(void)ml;(void)nl;(void)mly;(void)nly;
}
void glTextureBarrier(void) { }
void glClearTexSubImage(GLuint t, GLint l, GLint x, GLint y, GLint z, GLsizei w, GLsizei h, GLsizei d, GLenum fmt, GLenum type, const void *data) {
    (void)t;(void)l;(void)x;(void)y;(void)z;(void)w;(void)h;(void)d;(void)fmt;(void)type;(void)data;
}
void glClearTexSubImageEXT(GLuint t, GLint l, GLint x, GLint y, GLint z, GLsizei w, GLsizei h, GLsizei d, GLenum fmt, GLenum type, const void *data) {
    (void)t;(void)l;(void)x;(void)y;(void)z;(void)w;(void)h;(void)d;(void)fmt;(void)type;(void)data;
}
void glTexImage1D(GLenum t, GLint l, GLint ifmt, GLsizei w, GLint b, GLenum fmt, GLenum type, const void *data) {
    (void)t;(void)l;(void)ifmt;(void)w;(void)b;(void)fmt;(void)type;(void)data;
}
void glTexSubImage1D(GLenum t, GLint l, GLint x, GLsizei w, GLenum fmt, GLenum type, const void *data) {
    (void)t;(void)l;(void)x;(void)w;(void)fmt;(void)type;(void)data;
}
void glTexStorage1D(GLenum t, GLsizei l, GLenum fmt, GLsizei w) { (void)t;(void)l;(void)fmt;(void)w; }
void glTexImage2DMultisample(GLenum t, GLsizei s, GLenum fmt, GLsizei w, GLsizei h, GLboolean f) {
    (void)t;(void)s;(void)fmt;(void)w;(void)h;(void)f;
}
void glTexImage3DMultisample(GLenum t, GLsizei s, GLenum fmt, GLsizei w, GLsizei h, GLsizei d, GLboolean f) {
    (void)t;(void)s;(void)fmt;(void)w;(void)h;(void)d;(void)f;
}
void glCompressedTexSubImage1D(GLenum t, GLint l, GLint x, GLsizei w, GLenum fmt, GLsizei sz, const void *data) {
    (void)t;(void)l;(void)x;(void)w;(void)fmt;(void)sz;(void)data;
}
void glGetTexImage(GLenum t, GLint l, GLenum fmt, GLenum type, void *data) {
    (void)t;(void)l;(void)fmt;(void)type;(void)data;
}
void glGetnTexImageARB(GLenum t, GLint l, GLenum fmt, GLenum type, GLsizei sz, void *data) {
    (void)t;(void)l;(void)fmt;(void)type;(void)sz;(void)data;
}
void glGetCompressedTexImage(GLenum t, GLint l, void *data) { (void)t;(void)l;(void)data; }
void glGetnCompressedTexImageARB(GLenum t, GLint l, GLsizei sz, void *data) { (void)t;(void)l;(void)sz;(void)data; }

/* === Color / State === */
void glColorMaskIndexedEXT(GLuint i, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
    if (i == 0) glColorMask(r, g, b, a);
}
void glLogicOp(GLenum op) { (void)op; }
void glAlphaFunc(GLenum func, float ref) { (void)func;(void)ref; }
void glShadeModel(GLenum mode) { (void)mode; }
void glClampColor(GLenum t, GLenum c) { (void)t;(void)c; }
/* glClipControl — resolve at runtime via GL_EXT_clip_control if available */
typedef void (*PFNGLCLIPCONTROLEXTPROC)(GLenum origin, GLenum depth);
static PFNGLCLIPCONTROLEXTPROC _glClipControlEXT = NULL;
static int _glClipControl_resolved = 0;

void glClipControl(GLenum origin, GLenum depth) {
    if (!_glClipControl_resolved) {
        _glClipControlEXT = (PFNGLCLIPCONTROLEXTPROC)eglGetProcAddress("glClipControlEXT");
        if (!_glClipControlEXT) {
            _glClipControlEXT = (PFNGLCLIPCONTROLEXTPROC)eglGetProcAddress("glClipControl");
        }
        _glClipControl_resolved = 1;
        if (_glClipControlEXT) {
            __android_log_print(ANDROID_LOG_INFO, STUB_TAG, "glClipControl: resolved via GL_EXT_clip_control");
        } else {
            __android_log_print(ANDROID_LOG_WARN, STUB_TAG, "glClipControl: NOT available");
        }
    }
    if (_glClipControlEXT) {
        _glClipControlEXT(origin, depth);
    }
}
void glClipPlane(GLenum plane, const double *equation) { (void)plane;(void)equation; }
void glPolygonMode(GLenum face, GLenum mode) { (void)face;(void)mode; }
void glPolygonStipple(const GLubyte *mask) { (void)mask; }
void glLineStipple(GLint factor, GLushort pattern) { (void)factor;(void)pattern; }
void glPointSize(GLfloat size) { (void)size; }
void glPointParameteri(GLenum pname, GLint param) { (void)pname;(void)param; }
void glProvokingVertexEXT(GLenum mode) { (void)mode; }
/* glPolygonOffsetClampEXT — resolve at runtime if GL_EXT_polygon_offset_clamp is available */
typedef void (*PFNGLPOLYGONOFFSETCLAMPEXTPROC)(GLfloat f, GLfloat u, GLfloat c);
static PFNGLPOLYGONOFFSETCLAMPEXTPROC _glPolygonOffsetClampEXT = NULL;
static int _glPolygonOffsetClamp_resolved = 0;

void glPolygonOffsetClampEXT(GLfloat f, GLfloat u, GLfloat c) {
    if (!_glPolygonOffsetClamp_resolved) {
        _glPolygonOffsetClampEXT = (PFNGLPOLYGONOFFSETCLAMPEXTPROC)eglGetProcAddress("glPolygonOffsetClampEXT");
        _glPolygonOffsetClamp_resolved = 1;
    }
    if (_glPolygonOffsetClampEXT) {
        _glPolygonOffsetClampEXT(f, u, c);
    }
}

/* === Conditional Render === */
void glBeginConditionalRender(GLuint id, GLenum mode) { (void)id;(void)mode; }
void glEndConditionalRender(void) { }
void glBeginConditionalRenderNV(GLuint id, GLenum mode) { (void)id;(void)mode; }
void glEndConditionalRenderNV(void) { }

/* === Indexed Enable/Disable === */
void glEnableIndexedEXT(GLenum cap, GLuint index) { (void)cap;(void)index; }
void glDisableIndexedEXT(GLenum cap, GLuint index) { (void)cap;(void)index; }
void glEnableClientState(GLenum cap) { (void)cap; }
void glDisableClientState(GLenum cap) { (void)cap; }

/* === Blend (indexed) === */
void glBlendFuncSeparateiARB(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcA, GLenum dstA) {
    if (buf == 0) glBlendFuncSeparate(srcRGB, dstRGB, srcA, dstA);
}
void glBlendEquationSeparateiARB(GLuint buf, GLenum modeRGB, GLenum modeA) {
    if (buf == 0) glBlendEquationSeparate(modeRGB, modeA);
}

/* === Fragment Data Location === */
void glBindFragDataLocationIndexed(GLuint prog, GLuint color, GLuint index, const char *name) {
    (void)prog;(void)color;(void)index;(void)name;
}
void glBindFragDataLocationIndexedEXT(GLuint prog, GLuint color, GLuint index, const char *name) {
    (void)prog;(void)color;(void)index;(void)name;
}

/* === Primitive Restart === */
void glPrimitiveRestartIndex(GLuint index) { (void)index; }
void glPrimitiveRestartIndexNV(GLuint index) { (void)index; }

/* === Draw (advanced) === */
void glDrawArraysInstancedARB(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
    glDrawArraysInstanced(mode, first, count, primcount);
}
void glDrawElementsInstancedARB(GLenum mode, GLsizei count, GLenum type, const void *ind, GLsizei primcount) {
    glDrawElementsInstanced(mode, count, type, ind, primcount);
}
void glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint base) {
    (void)base;
    glDrawArraysInstanced(mode, first, count, primcount);
}
void glDrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *ind, GLsizei primcount, GLuint base) {
    (void)base;
    glDrawElementsInstanced(mode, count, type, ind, primcount);
}
void glDrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void *ind, GLsizei primcount, GLint basevertex, GLuint baseinstance) {
    (void)baseinstance;
    glDrawElementsInstancedBaseVertex(mode, count, type, ind, primcount, basevertex);
}
void glMultiDrawArraysIndirect(GLenum mode, const void *ind, GLsizei drawcount, GLsizei stride) {
    (void)mode;(void)ind;(void)drawcount;(void)stride;
}
void glMultiDrawElementsIndirect(GLenum mode, GLenum type, const void *ind, GLsizei drawcount, GLsizei stride) {
    (void)mode;(void)type;(void)ind;(void)drawcount;(void)stride;
}
void glMultiDrawArraysIndirectCountARB(GLenum mode, const void *ind, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    (void)mode;(void)ind;(void)drawcount;(void)maxdrawcount;(void)stride;
}
void glMultiDrawElementsIndirectCountARB(GLenum mode, GLenum type, const void *ind, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride) {
    (void)mode;(void)type;(void)ind;(void)drawcount;(void)maxdrawcount;(void)stride;
}

/* === Buffer === */
void glBindBufferARB(GLenum target, GLuint buffer) { glBindBuffer(target, buffer); }
void glGenBuffersARB(GLsizei n, GLuint *buffers) { glGenBuffers(n, buffers); }
/* glBufferStorage — resolve via GL_EXT_buffer_storage if available */
typedef void (*PFNGLBUFFERSTORAGEEXTPROC)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
static PFNGLBUFFERSTORAGEEXTPROC _glBufferStorageEXT = NULL;
static int _glBufferStorage_resolved = 0;

void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {
    if (!_glBufferStorage_resolved) {
        _glBufferStorageEXT = (PFNGLBUFFERSTORAGEEXTPROC)eglGetProcAddress("glBufferStorageEXT");
        _glBufferStorage_resolved = 1;
        if (_glBufferStorageEXT) {
            __android_log_print(ANDROID_LOG_INFO, STUB_TAG, "glBufferStorage: resolved via GL_EXT_buffer_storage");
        } else {
            __android_log_print(ANDROID_LOG_WARN, STUB_TAG, "glBufferStorage: falling back to glBufferData");
        }
    }
    if (_glBufferStorageEXT) {
        _glBufferStorageEXT(target, size, data, flags);
    } else {
        glBufferData(target, size, data, 0x88E4 /* GL_STATIC_DRAW */);
    }
}
void glBindVertexBuffers(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides) {
    (void)first;(void)count;(void)buffers;(void)offsets;(void)strides;
}

/* === Query === */
void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) { (void)id;(void)pname; if(params) *params = 0; }
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) { (void)id;(void)pname; if(params) *params = 0; }
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) { (void)id;(void)pname; if(params) *params = 0; }
void glQueryCounter(GLuint id, GLenum target) { (void)id;(void)target; }
void glBeginQueryIndexed(GLenum target, GLuint index, GLuint id) { (void)index; glBeginQuery(target, id); }
void glEndQueryIndexed(GLenum target, GLuint index) { (void)index; glEndQuery(target); }

/* === Tessellation === */
void glPatchParameterfv(GLenum pname, const GLfloat *values) { (void)pname;(void)values; }

/* === Pixel operations (desktop only) === */
void glDrawPixels(GLsizei w, GLsizei h, GLenum fmt, GLenum type, const void *data) {
    (void)w;(void)h;(void)fmt;(void)type;(void)data;
}
void glPixelTransferf(GLenum pname, GLfloat param) { (void)pname;(void)param; }
void glPixelZoom(GLfloat x, GLfloat y) { (void)x;(void)y; }
void glWindowPos2i(GLint x, GLint y) { (void)x;(void)y; }

/* === ReadnPixels === */
void glReadnPixelsARB(GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum type, GLsizei sz, void *data) {
    glReadPixels(x, y, w, h, fmt, type, data);
}
void glReadnPixelsKHR(GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum type, GLsizei sz, void *data) {
    glReadPixels(x, y, w, h, fmt, type, data);
}

/* === Vertex Attrib === */
void glVertexAttribDivisorARB(GLuint index, GLuint divisor) {
    glVertexAttribDivisor(index, divisor);
}

/* === Memory objects (EXT_memory_object) === */
void glCreateMemoryObjectsEXT(GLsizei n, GLuint *objs) { (void)n; if(objs) memset(objs, 0, n*sizeof(GLuint)); }
void glDeleteMemoryObjectsEXT(GLsizei n, const GLuint *objs) { (void)n;(void)objs; }
void glMemoryObjectParameterivEXT(GLuint obj, GLenum pname, const GLint *params) { (void)obj;(void)pname;(void)params; }
void glImportMemoryFdEXT(GLuint obj, GLuint64 size, GLenum handleType, GLint fd) { (void)obj;(void)size;(void)handleType;(void)fd; }
void glTexStorageMem2DEXT(GLuint texture, GLsizei levels, GLenum fmt, GLsizei w, GLsizei h, GLuint mem, GLuint64 offset) {
    (void)texture;(void)levels;(void)fmt;(void)w;(void)h;(void)mem;(void)offset;
}

/* === EGL Image target === */
void glEGLImageTargetTexStorageEXT(GLenum target, void *image, const int *attrib_list) {
    (void)target;(void)image;(void)attrib_list;
}

/* === EGL fence === */
int eglDupNativeFenceFDANDROID(EGLDisplay dpy, EGLSync sync) {
    (void)dpy; (void)sync;
    return -1;
}

/* === Android logging stub === */
int LOG_PRI(int priority, const char *tag, const char *fmt, ...) {
    (void)priority; (void)tag; (void)fmt;
    return 0;
}

/* === C11 timespec_get (not available on all Android API levels) === */
#ifndef TIME_UTC
#define TIME_UTC 1
#endif
int timespec_get(struct timespec *ts, int base) {
    if (base == TIME_UTC) {
        clock_gettime(CLOCK_REALTIME, ts);
        return TIME_UTC;
    }
    return 0;
}
