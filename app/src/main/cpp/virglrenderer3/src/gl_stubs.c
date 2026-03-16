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

/* glFramebufferTexture2DMultisampleEXT / glRenderbufferStorageMultisampleEXT —
 * resolve at runtime via GL_EXT_multisampled_render_to_texture.
 * Enables implicit (tile-based) MSAA on Adreno 650/750 and similar GPUs.
 * Without resolution, feat_implicit_msaa is detected but MSAA is silently lost. */
typedef void (*PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
typedef void (*PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
static PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC _glFBTex2DMS = NULL;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC _glRBStorageMS = NULL;
static int _glImplicitMSAA_resolved = 0;

static void _resolve_implicit_msaa(void) {
    if (!_glImplicitMSAA_resolved) {
        _glFBTex2DMS = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)
            eglGetProcAddress("glFramebufferTexture2DMultisampleEXT");
        _glRBStorageMS = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)
            eglGetProcAddress("glRenderbufferStorageMultisampleEXT");
        _glImplicitMSAA_resolved = 1;
        __android_log_print(_glFBTex2DMS ? ANDROID_LOG_INFO : ANDROID_LOG_WARN,
            STUB_TAG, "EXT_multisampled_render_to_texture: FBTex2DMS=%s RBStorageMS=%s",
            _glFBTex2DMS ? "resolved" : "fallback",
            _glRBStorageMS ? "resolved" : "fallback");
    }
}

void glFramebufferTexture2DMultisampleEXT(GLenum t, GLenum a, GLenum tt, GLuint tex, GLint l, GLsizei s) {
    _resolve_implicit_msaa();
    if (_glFBTex2DMS) {
        _glFBTex2DMS(t, a, tt, tex, l, s);
    } else {
        /* No implicit MSAA available — attach without multisampling */
        glFramebufferTexture2D(t, a, tt, tex, l);
    }
}
void glRenderbufferStorageMultisampleEXT(GLenum t, GLsizei s, GLenum fmt, GLsizei w, GLsizei h) {
    _resolve_implicit_msaa();
    if (_glRBStorageMS) {
        _glRBStorageMS(t, s, fmt, w, h);
    } else {
        /* Fallback to core multisample renderbuffer storage */
        glRenderbufferStorageMultisample(t, s, fmt, w, h);
    }
}

/* === Texture functions === */
/* glTextureView — resolve at runtime via GL_OES_texture_view / GL_EXT_texture_view.
 * Enables VIRGL_CAP_TEXTURE_VIEW on Adreno (GL_OES_texture_view).
 * Falls back to no-op on Mali (no texture view extension). */
typedef void (*PFNGLTEXTUREVIEWPROC)(GLuint texture, GLenum target, GLuint origtexture,
    GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
static PFNGLTEXTUREVIEWPROC _glTextureViewResolved = NULL;
static int _glTextureView_resolved = 0;

void glTextureView(GLuint t, GLenum tg, GLuint o, GLenum f, GLuint ml, GLuint nl, GLuint mly, GLuint nly) {
    if (!_glTextureView_resolved) {
        _glTextureViewResolved = (PFNGLTEXTUREVIEWPROC)eglGetProcAddress("glTextureViewOES");
        if (!_glTextureViewResolved)
            _glTextureViewResolved = (PFNGLTEXTUREVIEWPROC)eglGetProcAddress("glTextureViewEXT");
        _glTextureView_resolved = 1;
        __android_log_print(_glTextureViewResolved ? ANDROID_LOG_INFO : ANDROID_LOG_WARN,
            STUB_TAG, "glTextureView: %s", _glTextureViewResolved ? "resolved (OES/EXT)" : "not available, no-op");
    }
    if (_glTextureViewResolved)
        _glTextureViewResolved(t, tg, o, f, ml, nl, mly, nly);
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
            /* Try without EXT suffix — some drivers expose it directly */
            _glClipControlEXT = (PFNGLCLIPCONTROLEXTPROC)eglGetProcAddress("glClipControl");
        }
        _glClipControl_resolved = 1;
        if (_glClipControlEXT) {
            __android_log_print(ANDROID_LOG_INFO, STUB_TAG, "glClipControl: resolved via GL_EXT_clip_control");
        } else {
            __android_log_print(ANDROID_LOG_WARN, STUB_TAG, "glClipControl: NOT available, DX11 depth may be incorrect");
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

/* glBindFragDataLocationIndexed — resolve via GL_EXT_blend_func_extended.
 * Enables dual-source blending on Adreno (GL_EXT_blend_func_extended).
 * Falls back to no-op on Mali (no blend_func_extended). */
typedef void (*PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC)(GLuint prog, GLuint colorNumber, GLuint index, const char *name);
static PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC _glBindFragDataLocationIndexedEXT = NULL;
static int _glBindFragDataLocationIndexed_resolved = 0;

static void _resolve_bind_frag_data(void) {
    if (!_glBindFragDataLocationIndexed_resolved) {
        _glBindFragDataLocationIndexedEXT = (PFNGLBINDFRAGDATALOCATIONINDEXEDEXTPROC)
            eglGetProcAddress("glBindFragDataLocationIndexedEXT");
        _glBindFragDataLocationIndexed_resolved = 1;
        __android_log_print(_glBindFragDataLocationIndexedEXT ? ANDROID_LOG_INFO : ANDROID_LOG_WARN,
            STUB_TAG, "glBindFragDataLocationIndexed: %s",
            _glBindFragDataLocationIndexedEXT ? "resolved (EXT)" : "not available, no-op");
    }
}
void glBindFragDataLocationIndexed(GLuint prog, GLuint color, GLuint index, const char *name) {
    _resolve_bind_frag_data();
    if (_glBindFragDataLocationIndexedEXT)
        _glBindFragDataLocationIndexedEXT(prog, color, index, name);
}
void glBindFragDataLocationIndexedEXT(GLuint prog, GLuint color, GLuint index, const char *name) {
    _resolve_bind_frag_data();
    if (_glBindFragDataLocationIndexedEXT)
        _glBindFragDataLocationIndexedEXT(prog, color, index, name);
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

/* === Query ===
 * glGetQueryObjecti64v / glGetQueryObjectui64v / glQueryCounter —
 * resolve at runtime via GL_EXT_disjoint_timer_query.
 * Enables PIPE_QUERY_TIMESTAMP / PIPE_QUERY_TIME_ELAPSED on Adreno 650/750.
 * Without resolution, feat_timer_query is detected but all results return 0. */
typedef void (*PFNGLGETQUERYOBJECTIVEXTPROC)(GLuint id, GLenum pname, GLint *params);
typedef void (*PFNGLGETQUERYOBJECTI64VEXTPROC)(GLuint id, GLenum pname, GLint64 *params);
typedef void (*PFNGLGETQUERYOBJECTUI64VEXTPROC)(GLuint id, GLenum pname, GLuint64 *params);
typedef void (*PFNGLQUERYCOUNTEREXTPROC)(GLuint id, GLenum target);

static PFNGLGETQUERYOBJECTIVEXTPROC _glGetQueryObjectivEXT = NULL;
static PFNGLGETQUERYOBJECTI64VEXTPROC _glGetQueryObjecti64vEXT = NULL;
static PFNGLGETQUERYOBJECTUI64VEXTPROC _glGetQueryObjectui64vEXT = NULL;
static PFNGLQUERYCOUNTEREXTPROC _glQueryCounterEXT = NULL;
static int _glTimerQuery_resolved = 0;

static void _resolve_timer_query(void) {
    if (!_glTimerQuery_resolved) {
        _glGetQueryObjectivEXT = (PFNGLGETQUERYOBJECTIVEXTPROC)
            eglGetProcAddress("glGetQueryObjectivEXT");
        _glGetQueryObjecti64vEXT = (PFNGLGETQUERYOBJECTI64VEXTPROC)
            eglGetProcAddress("glGetQueryObjecti64vEXT");
        _glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXTPROC)
            eglGetProcAddress("glGetQueryObjectui64vEXT");
        _glQueryCounterEXT = (PFNGLQUERYCOUNTEREXTPROC)
            eglGetProcAddress("glQueryCounterEXT");
        _glTimerQuery_resolved = 1;
        __android_log_print(_glQueryCounterEXT ? ANDROID_LOG_INFO : ANDROID_LOG_WARN,
            STUB_TAG, "EXT_disjoint_timer_query: Counter=%s i64v=%s ui64v=%s",
            _glQueryCounterEXT ? "resolved" : "stub",
            _glGetQueryObjecti64vEXT ? "resolved" : "stub",
            _glGetQueryObjectui64vEXT ? "resolved" : "stub");
    }
}

void glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params) {
    _resolve_timer_query();
    if (_glGetQueryObjectivEXT) {
        _glGetQueryObjectivEXT(id, pname, params);
    } else {
        /* Fallback: use core glGetQueryObjectuiv and cast */
        if (params) {
            GLuint uval = 0;
            glGetQueryObjectuiv(id, pname, &uval);
            *params = (GLint)uval;
        }
    }
}
void glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 *params) {
    _resolve_timer_query();
    if (_glGetQueryObjecti64vEXT) {
        _glGetQueryObjecti64vEXT(id, pname, params);
    } else if (params) {
        *params = 0;
    }
}
void glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 *params) {
    _resolve_timer_query();
    if (_glGetQueryObjectui64vEXT) {
        _glGetQueryObjectui64vEXT(id, pname, params);
    } else if (params) {
        *params = 0;
    }
}
void glQueryCounter(GLuint id, GLenum target) {
    _resolve_timer_query();
    if (_glQueryCounterEXT) {
        _glQueryCounterEXT(id, target);
    }
}
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

/* === Memory objects (EXT_memory_object / EXT_memory_object_fd) ===
 * Runtime-resolved for Adreno (GL_EXT_memory_object + GL_EXT_memory_object_fd).
 * Enables buffer/texture import from file descriptors for cross-process sharing.
 * Falls back to no-op stubs on Mali (no memory object extension). */
typedef void (*PFNGLCREATEMEMORYOBJECTSEXTPROC)(GLsizei n, GLuint *memoryObjects);
typedef void (*PFNGLDELETEMEMORYOBJECTSEXTPROC)(GLsizei n, const GLuint *memoryObjects);
typedef void (*PFNGLMEMORYOBJECTPARAMETERIVEXTPROC)(GLuint memoryObject, GLenum pname, const GLint *params);
typedef void (*PFNGLIMPORTMEMORYFDEXTPROC)(GLuint memoryObject, GLuint64 size, GLenum handleType, GLint fd);
typedef void (*PFNGLTEXSTORAGEMEM2DEXTPROC)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei w, GLsizei h, GLuint memory, GLuint64 offset);

static PFNGLCREATEMEMORYOBJECTSEXTPROC _glCreateMemoryObjectsEXT = NULL;
static PFNGLDELETEMEMORYOBJECTSEXTPROC _glDeleteMemoryObjectsEXT = NULL;
static PFNGLMEMORYOBJECTPARAMETERIVEXTPROC _glMemoryObjectParameterivEXT = NULL;
static PFNGLIMPORTMEMORYFDEXTPROC _glImportMemoryFdEXT = NULL;
static PFNGLTEXSTORAGEMEM2DEXTPROC _glTexStorageMem2DEXT = NULL;
static int _glMemoryObject_resolved = 0;

static void _resolve_memory_objects(void) {
    if (!_glMemoryObject_resolved) {
        _glCreateMemoryObjectsEXT = (PFNGLCREATEMEMORYOBJECTSEXTPROC)eglGetProcAddress("glCreateMemoryObjectsEXT");
        _glDeleteMemoryObjectsEXT = (PFNGLDELETEMEMORYOBJECTSEXTPROC)eglGetProcAddress("glDeleteMemoryObjectsEXT");
        _glMemoryObjectParameterivEXT = (PFNGLMEMORYOBJECTPARAMETERIVEXTPROC)eglGetProcAddress("glMemoryObjectParameterivEXT");
        _glImportMemoryFdEXT = (PFNGLIMPORTMEMORYFDEXTPROC)eglGetProcAddress("glImportMemoryFdEXT");
        _glTexStorageMem2DEXT = (PFNGLTEXSTORAGEMEM2DEXTPROC)eglGetProcAddress("glTexStorageMem2DEXT");
        _glMemoryObject_resolved = 1;
        __android_log_print(_glCreateMemoryObjectsEXT ? ANDROID_LOG_INFO : ANDROID_LOG_WARN,
            STUB_TAG, "EXT_memory_object: %s", _glCreateMemoryObjectsEXT ? "resolved" : "not available, no-op");
    }
}
void glCreateMemoryObjectsEXT(GLsizei n, GLuint *objs) {
    _resolve_memory_objects();
    if (_glCreateMemoryObjectsEXT) _glCreateMemoryObjectsEXT(n, objs);
    else if (objs) memset(objs, 0, n * sizeof(GLuint));
}
void glDeleteMemoryObjectsEXT(GLsizei n, const GLuint *objs) {
    _resolve_memory_objects();
    if (_glDeleteMemoryObjectsEXT) _glDeleteMemoryObjectsEXT(n, objs);
}
void glMemoryObjectParameterivEXT(GLuint obj, GLenum pname, const GLint *params) {
    _resolve_memory_objects();
    if (_glMemoryObjectParameterivEXT) _glMemoryObjectParameterivEXT(obj, pname, params);
}
void glImportMemoryFdEXT(GLuint obj, GLuint64 size, GLenum handleType, GLint fd) {
    _resolve_memory_objects();
    if (_glImportMemoryFdEXT) _glImportMemoryFdEXT(obj, size, handleType, fd);
}
void glTexStorageMem2DEXT(GLuint texture, GLsizei levels, GLenum fmt, GLsizei w, GLsizei h, GLuint mem, GLuint64 offset) {
    _resolve_memory_objects();
    if (_glTexStorageMem2DEXT) _glTexStorageMem2DEXT(texture, levels, fmt, w, h, mem, offset);
}

/* glEGLImageTargetTexStorageEXT — resolve via GL_EXT_EGL_image_storage.
 * Enables efficient EGL image import into textures on Adreno.
 * Falls back to no-op on Mali (no EGL_image_storage). */
typedef void (*PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC)(GLenum target, void *image, const int *attrib_list);
static PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC _glEGLImageTargetTexStorageEXT = NULL;
static int _glEGLImageTargetTexStorage_resolved = 0;

void glEGLImageTargetTexStorageEXT(GLenum target, void *image, const int *attrib_list) {
    if (!_glEGLImageTargetTexStorage_resolved) {
        _glEGLImageTargetTexStorageEXT = (PFNGLEGLIMAGETARGETTEXSTORAGEEXTPROC)
            eglGetProcAddress("glEGLImageTargetTexStorageEXT");
        _glEGLImageTargetTexStorage_resolved = 1;
    }
    if (_glEGLImageTargetTexStorageEXT)
        _glEGLImageTargetTexStorageEXT(target, image, attrib_list);
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
