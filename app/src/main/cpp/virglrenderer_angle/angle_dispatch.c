/**************************************************************************
 * ANGLE EGL/GLES Dispatch Table — Implementation
 *
 * Populates the angle_* function pointers by dlopen'ing ANGLE's
 * libEGL.so and libGLESv2.so from the app's imagefs.  Falls back to
 * the system EGL/GLES implementations if ANGLE can't be loaded.
 **************************************************************************/

/* Suppress the redirect macros — we reference the real system symbols
 * here so they can serve as fallback values.
 * ANGLE_DISPATCH_IMPL is set via CMake compile definitions for this file. */
#include "angle_dispatch.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define ATAG "ANGLE-Dispatch"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  ATAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, ATAG, __VA_ARGS__)

/* ── State ──────────────────────────────────────────────────────────── */

static bool s_using_angle = false;
static void *s_egl_lib  = NULL;
static void *s_gles_lib = NULL;

bool angle_dispatch_is_angle(void) { return s_using_angle; }

/* ── Function-pointer variable DEFINITIONS ───────────────────────────
 *
 * Each is initialised to the corresponding system symbol so the
 * library works even before angle_dispatch_init() is called.
 * ──────────────────────────────────────────────────────────────────── */

/* EGL */
__typeof__(eglGetDisplay)          *angle_eglGetDisplay          = eglGetDisplay;
__typeof__(eglInitialize)          *angle_eglInitialize          = eglInitialize;
__typeof__(eglBindAPI)             *angle_eglBindAPI             = eglBindAPI;
__typeof__(eglChooseConfig)        *angle_eglChooseConfig        = eglChooseConfig;
__typeof__(eglCreateContext)       *angle_eglCreateContext       = eglCreateContext;
__typeof__(eglDestroyContext)      *angle_eglDestroyContext      = eglDestroyContext;
__typeof__(eglMakeCurrent)         *angle_eglMakeCurrent         = eglMakeCurrent;
__typeof__(eglQueryString)         *angle_eglQueryString         = eglQueryString;
__typeof__(eglGetError)            *angle_eglGetError            = eglGetError;
__typeof__(eglGetCurrentContext)   *angle_eglGetCurrentContext   = eglGetCurrentContext;
__typeof__(eglGetProcAddress)      *angle_eglGetProcAddress      = eglGetProcAddress;
__typeof__(eglCreatePbufferSurface)*angle_eglCreatePbufferSurface= eglCreatePbufferSurface;
__typeof__(eglDestroySurface)      *angle_eglDestroySurface      = eglDestroySurface;
__typeof__(eglTerminate)           *angle_eglTerminate           = eglTerminate;

/* GLES 2.0 */
__typeof__(glActiveTexture)        *angle_glActiveTexture        = glActiveTexture;
__typeof__(glAttachShader)         *angle_glAttachShader         = glAttachShader;
__typeof__(glBindAttribLocation)   *angle_glBindAttribLocation   = glBindAttribLocation;
__typeof__(glBindBuffer)           *angle_glBindBuffer           = glBindBuffer;
__typeof__(glBindFramebuffer)      *angle_glBindFramebuffer      = glBindFramebuffer;
__typeof__(glBindTexture)          *angle_glBindTexture          = glBindTexture;
__typeof__(glBlendColor)           *angle_glBlendColor           = glBlendColor;
__typeof__(glBlendEquationSeparate)*angle_glBlendEquationSeparate= glBlendEquationSeparate;
__typeof__(glBlendFuncSeparate)    *angle_glBlendFuncSeparate    = glBlendFuncSeparate;
__typeof__(glBufferData)           *angle_glBufferData           = glBufferData;
__typeof__(glBufferSubData)        *angle_glBufferSubData        = glBufferSubData;
__typeof__(glCheckFramebufferStatus)*angle_glCheckFramebufferStatus = glCheckFramebufferStatus;
__typeof__(glClear)                *angle_glClear                = glClear;
__typeof__(glClearColor)           *angle_glClearColor           = glClearColor;
__typeof__(glClearDepthf)          *angle_glClearDepthf          = glClearDepthf;
__typeof__(glClearStencil)         *angle_glClearStencil         = glClearStencil;
__typeof__(glColorMask)            *angle_glColorMask            = glColorMask;
__typeof__(glCompileShader)        *angle_glCompileShader        = glCompileShader;
__typeof__(glCompressedTexSubImage2D)*angle_glCompressedTexSubImage2D = glCompressedTexSubImage2D;
__typeof__(glCreateProgram)        *angle_glCreateProgram        = glCreateProgram;
__typeof__(glCreateShader)         *angle_glCreateShader         = glCreateShader;
__typeof__(glCullFace)             *angle_glCullFace             = glCullFace;
__typeof__(glDeleteBuffers)        *angle_glDeleteBuffers        = glDeleteBuffers;
__typeof__(glDeleteFramebuffers)   *angle_glDeleteFramebuffers   = glDeleteFramebuffers;
__typeof__(glDeleteProgram)        *angle_glDeleteProgram        = glDeleteProgram;
__typeof__(glDeleteShader)         *angle_glDeleteShader         = glDeleteShader;
__typeof__(glDeleteTextures)       *angle_glDeleteTextures       = glDeleteTextures;
__typeof__(glDepthFunc)            *angle_glDepthFunc            = glDepthFunc;
__typeof__(glDepthMask)            *angle_glDepthMask            = glDepthMask;
__typeof__(glDepthRangef)          *angle_glDepthRangef          = glDepthRangef;
__typeof__(glDisable)              *angle_glDisable              = glDisable;
__typeof__(glDisableVertexAttribArray)*angle_glDisableVertexAttribArray = glDisableVertexAttribArray;
__typeof__(glDrawArrays)           *angle_glDrawArrays           = glDrawArrays;
__typeof__(glDrawElements)         *angle_glDrawElements         = glDrawElements;
__typeof__(glEnable)               *angle_glEnable               = glEnable;
__typeof__(glEnableVertexAttribArray)*angle_glEnableVertexAttribArray = glEnableVertexAttribArray;
__typeof__(glFlush)                *angle_glFlush                = glFlush;
__typeof__(glFinish)               *angle_glFinish               = glFinish;
__typeof__(glFramebufferTexture2D) *angle_glFramebufferTexture2D = glFramebufferTexture2D;
__typeof__(glFrontFace)            *angle_glFrontFace            = glFrontFace;
__typeof__(glGenBuffers)           *angle_glGenBuffers           = glGenBuffers;
__typeof__(glGenFramebuffers)      *angle_glGenFramebuffers      = glGenFramebuffers;
__typeof__(glGenTextures)          *angle_glGenTextures          = glGenTextures;
__typeof__(glGetAttribLocation)    *angle_glGetAttribLocation    = glGetAttribLocation;
__typeof__(glGetError)             *angle_glGetError             = glGetError;
__typeof__(glGetFloatv)            *angle_glGetFloatv            = glGetFloatv;
__typeof__(glGetIntegerv)          *angle_glGetIntegerv          = glGetIntegerv;
__typeof__(glGetProgramiv)         *angle_glGetProgramiv         = glGetProgramiv;
__typeof__(glGetShaderiv)          *angle_glGetShaderiv          = glGetShaderiv;
__typeof__(glGetString)            *angle_glGetString            = glGetString;
__typeof__(glGetStringi)           *angle_glGetStringi           = glGetStringi;
__typeof__(glGetUniformLocation)   *angle_glGetUniformLocation   = glGetUniformLocation;
__typeof__(glLineWidth)            *angle_glLineWidth            = glLineWidth;
__typeof__(glLinkProgram)          *angle_glLinkProgram          = glLinkProgram;
__typeof__(glPixelStorei)          *angle_glPixelStorei          = glPixelStorei;
__typeof__(glPolygonOffset)        *angle_glPolygonOffset        = glPolygonOffset;
__typeof__(glReadPixels)           *angle_glReadPixels           = glReadPixels;
__typeof__(glScissor)              *angle_glScissor              = glScissor;
__typeof__(glShaderSource)         *angle_glShaderSource         = glShaderSource;
__typeof__(glStencilFunc)          *angle_glStencilFunc          = glStencilFunc;
__typeof__(glStencilFuncSeparate)  *angle_glStencilFuncSeparate  = glStencilFuncSeparate;
__typeof__(glStencilMask)          *angle_glStencilMask          = glStencilMask;
__typeof__(glStencilMaskSeparate)  *angle_glStencilMaskSeparate  = glStencilMaskSeparate;
__typeof__(glStencilOp)            *angle_glStencilOp            = glStencilOp;
__typeof__(glStencilOpSeparate)    *angle_glStencilOpSeparate    = glStencilOpSeparate;
__typeof__(glTexImage2D)           *angle_glTexImage2D           = glTexImage2D;
__typeof__(glTexParameterf)        *angle_glTexParameterf        = glTexParameterf;
__typeof__(glTexParameteri)        *angle_glTexParameteri        = glTexParameteri;
__typeof__(glTexSubImage2D)        *angle_glTexSubImage2D        = glTexSubImage2D;
__typeof__(glUniform1f)            *angle_glUniform1f            = glUniform1f;
__typeof__(glUniform1i)            *angle_glUniform1i            = glUniform1i;
__typeof__(glUniform4f)            *angle_glUniform4f            = glUniform4f;
__typeof__(glUniform4fv)           *angle_glUniform4fv           = glUniform4fv;
__typeof__(glUseProgram)           *angle_glUseProgram           = glUseProgram;
__typeof__(glVertexAttribPointer)  *angle_glVertexAttribPointer  = glVertexAttribPointer;
__typeof__(glViewport)             *angle_glViewport             = glViewport;

/* GLES 3.0 */
__typeof__(glBeginQuery)           *angle_glBeginQuery           = glBeginQuery;
__typeof__(glBeginTransformFeedback)*angle_glBeginTransformFeedback = glBeginTransformFeedback;
__typeof__(glBindBufferBase)       *angle_glBindBufferBase       = glBindBufferBase;
__typeof__(glBindBufferRange)      *angle_glBindBufferRange      = glBindBufferRange;
__typeof__(glBindSampler)          *angle_glBindSampler          = glBindSampler;
__typeof__(glBindTransformFeedback)*angle_glBindTransformFeedback= glBindTransformFeedback;
__typeof__(glBindVertexArray)      *angle_glBindVertexArray      = glBindVertexArray;
__typeof__(glBlitFramebuffer)      *angle_glBlitFramebuffer      = glBlitFramebuffer;
__typeof__(glClearBufferfv)        *angle_glClearBufferfv        = glClearBufferfv;
__typeof__(glClearBufferiv)        *angle_glClearBufferiv        = glClearBufferiv;
__typeof__(glClearBufferuiv)       *angle_glClearBufferuiv       = glClearBufferuiv;
__typeof__(glClientWaitSync)       *angle_glClientWaitSync       = glClientWaitSync;
__typeof__(glCompressedTexSubImage3D)*angle_glCompressedTexSubImage3D = glCompressedTexSubImage3D;
__typeof__(glCopyBufferSubData)    *angle_glCopyBufferSubData    = glCopyBufferSubData;
__typeof__(glDeleteQueries)        *angle_glDeleteQueries        = glDeleteQueries;
__typeof__(glDeleteSamplers)       *angle_glDeleteSamplers       = glDeleteSamplers;
__typeof__(glDeleteSync)           *angle_glDeleteSync           = glDeleteSync;
__typeof__(glDeleteTransformFeedbacks)*angle_glDeleteTransformFeedbacks = glDeleteTransformFeedbacks;
__typeof__(glDeleteVertexArrays)   *angle_glDeleteVertexArrays   = glDeleteVertexArrays;
__typeof__(glDrawArraysInstanced)  *angle_glDrawArraysInstanced  = glDrawArraysInstanced;
__typeof__(glDrawBuffers)          *angle_glDrawBuffers          = glDrawBuffers;
__typeof__(glDrawElementsInstanced)*angle_glDrawElementsInstanced= glDrawElementsInstanced;
__typeof__(glDrawRangeElements)    *angle_glDrawRangeElements    = glDrawRangeElements;
__typeof__(glEndQuery)             *angle_glEndQuery             = glEndQuery;
__typeof__(glEndTransformFeedback) *angle_glEndTransformFeedback = glEndTransformFeedback;
__typeof__(glFenceSync)            *angle_glFenceSync            = glFenceSync;
__typeof__(glFramebufferTextureLayer)*angle_glFramebufferTextureLayer = glFramebufferTextureLayer;
__typeof__(glGenQueries)           *angle_glGenQueries           = glGenQueries;
__typeof__(glGenSamplers)          *angle_glGenSamplers          = glGenSamplers;
__typeof__(glGenTransformFeedbacks)*angle_glGenTransformFeedbacks= glGenTransformFeedbacks;
__typeof__(glGenVertexArrays)      *angle_glGenVertexArrays      = glGenVertexArrays;
__typeof__(glGetIntegeri_v)        *angle_glGetIntegeri_v        = glGetIntegeri_v;
__typeof__(glGetQueryObjectuiv)    *angle_glGetQueryObjectuiv    = glGetQueryObjectuiv;
__typeof__(glGetUniformBlockIndex) *angle_glGetUniformBlockIndex = glGetUniformBlockIndex;
__typeof__(glMapBufferRange)       *angle_glMapBufferRange       = glMapBufferRange;
__typeof__(glPauseTransformFeedback)*angle_glPauseTransformFeedback = glPauseTransformFeedback;
__typeof__(glReadBuffer)           *angle_glReadBuffer           = glReadBuffer;
__typeof__(glResumeTransformFeedback)*angle_glResumeTransformFeedback = glResumeTransformFeedback;
__typeof__(glSamplerParameterf)    *angle_glSamplerParameterf    = glSamplerParameterf;
__typeof__(glSamplerParameteri)    *angle_glSamplerParameteri    = glSamplerParameteri;
__typeof__(glTexImage3D)           *angle_glTexImage3D           = glTexImage3D;
__typeof__(glTexStorage2D)         *angle_glTexStorage2D         = glTexStorage2D;
__typeof__(glTexStorage3D)         *angle_glTexStorage3D         = glTexStorage3D;
__typeof__(glTexSubImage3D)        *angle_glTexSubImage3D        = glTexSubImage3D;
__typeof__(glUniform4uiv)          *angle_glUniform4uiv          = glUniform4uiv;
__typeof__(glUniformBlockBinding)  *angle_glUniformBlockBinding  = glUniformBlockBinding;
__typeof__(glUnmapBuffer)          *angle_glUnmapBuffer          = glUnmapBuffer;
__typeof__(glVertexAttribDivisor)  *angle_glVertexAttribDivisor  = glVertexAttribDivisor;
__typeof__(glVertexAttribIPointer) *angle_glVertexAttribIPointer = glVertexAttribIPointer;

/* GLES 3.1 */
__typeof__(glBindImageTexture)     *angle_glBindImageTexture     = glBindImageTexture;
__typeof__(glBindVertexBuffer)     *angle_glBindVertexBuffer     = glBindVertexBuffer;
__typeof__(glDispatchCompute)      *angle_glDispatchCompute      = glDispatchCompute;
__typeof__(glDispatchComputeIndirect)*angle_glDispatchComputeIndirect = glDispatchComputeIndirect;
__typeof__(glDrawArraysIndirect)   *angle_glDrawArraysIndirect   = glDrawArraysIndirect;
__typeof__(glDrawElementsIndirect) *angle_glDrawElementsIndirect = glDrawElementsIndirect;
__typeof__(glFramebufferParameteri)*angle_glFramebufferParameteri= glFramebufferParameteri;
__typeof__(glGetProgramResourceIndex)*angle_glGetProgramResourceIndex = glGetProgramResourceIndex;
__typeof__(glMemoryBarrier)        *angle_glMemoryBarrier        = glMemoryBarrier;
__typeof__(glTexStorage2DMultisample)*angle_glTexStorage2DMultisample = glTexStorage2DMultisample;
__typeof__(glVertexAttribBinding)  *angle_glVertexAttribBinding  = glVertexAttribBinding;
__typeof__(glVertexAttribFormat)   *angle_glVertexAttribFormat   = glVertexAttribFormat;
__typeof__(glVertexAttribIFormat)  *angle_glVertexAttribIFormat  = glVertexAttribIFormat;
__typeof__(glVertexBindingDivisor) *angle_glVertexBindingDivisor = glVertexBindingDivisor;

/* GLES 3.2 */
__typeof__(glBlendEquationSeparatei)*angle_glBlendEquationSeparatei = glBlendEquationSeparatei;
__typeof__(glBlendFuncSeparatei)   *angle_glBlendFuncSeparatei   = glBlendFuncSeparatei;
__typeof__(glColorMaski)           *angle_glColorMaski           = glColorMaski;
__typeof__(glCopyImageSubData)     *angle_glCopyImageSubData     = glCopyImageSubData;
__typeof__(glDisablei)             *angle_glDisablei             = glDisablei;
__typeof__(glDrawElementsBaseVertex)*angle_glDrawElementsBaseVertex = glDrawElementsBaseVertex;
__typeof__(glDrawElementsInstancedBaseVertex)*angle_glDrawElementsInstancedBaseVertex = glDrawElementsInstancedBaseVertex;
__typeof__(glDrawRangeElementsBaseVertex)*angle_glDrawRangeElementsBaseVertex = glDrawRangeElementsBaseVertex;
__typeof__(glEnablei)              *angle_glEnablei              = glEnablei;
__typeof__(glFramebufferTexture)   *angle_glFramebufferTexture   = glFramebufferTexture;
__typeof__(glGetMultisamplefv)     *angle_glGetMultisamplefv     = glGetMultisamplefv;
__typeof__(glMinSampleShading)     *angle_glMinSampleShading     = glMinSampleShading;
__typeof__(glPatchParameteri)      *angle_glPatchParameteri      = glPatchParameteri;
__typeof__(glSampleMaski)          *angle_glSampleMaski          = glSampleMaski;
__typeof__(glSamplerParameterIuiv) *angle_glSamplerParameterIuiv = glSamplerParameterIuiv;
__typeof__(glTexBuffer)            *angle_glTexBuffer            = glTexBuffer;
__typeof__(glTexBufferRange)       *angle_glTexBufferRange       = glTexBufferRange;
__typeof__(glTexParameterIuiv)     *angle_glTexParameterIuiv     = glTexParameterIuiv;
__typeof__(glTexStorage3DMultisample)*angle_glTexStorage3DMultisample = glTexStorage3DMultisample;

/* Misc */
__typeof__(glVertexAttrib1fv)      *angle_glVertexAttrib1fv      = glVertexAttrib1fv;
__typeof__(glVertexAttrib2fv)      *angle_glVertexAttrib2fv      = glVertexAttrib2fv;
__typeof__(glVertexAttrib3fv)      *angle_glVertexAttrib3fv      = glVertexAttrib3fv;
__typeof__(glVertexAttrib4fv)      *angle_glVertexAttrib4fv      = glVertexAttrib4fv;

/* ── Helper: resolve a symbol from a dlopen'd library ────────────── */

#define RESOLVE(lib, dst, name)                                         \
    do {                                                                \
        void *_p = dlsym((lib), #name);                                 \
        if (_p) (dst) = (__typeof__(dst))_p;                            \
    } while (0)

/* ── angle_dispatch_init ─────────────────────────────────────────── */

bool angle_dispatch_init(const char *egl_path, const char *gles_path)
{
    /*
     * CRITICAL: Load libGLESv2.so FIRST, before libEGL.so!
     *
     * ANGLE's libEGL.so is a thin dispatcher that internally dlopen's
     * libGLESv2.so to find the actual ANGLE backend implementation.
     * If we load libEGL.so first, its internal initialization triggers
     * a search for "libGLESv2.so" — and may find the SYSTEM's
     * libGLESv2.so (already loaded by our app's native dependencies)
     * instead of ANGLE's.  This causes a SIGSEGV because ANGLE's EGL
     * dispatcher calls into incompatible system GLES internals.
     *
     * By loading ANGLE's libGLESv2.so first with RTLD_GLOBAL, we ensure
     * it's available and preferred when libEGL.so's internal dlopen
     * searches for "libGLESv2.so".
     *
     * RTLD_GLOBAL is safe here because:
     * - Our dispatch table already captures all function pointers we need
     * - Java-side GLES (GLSurfaceView) uses Android framework's linker
     *   namespace, which is separate from the app's default namespace
     * - Any GLES calls from other native code go through the dispatch
     *   macros when VIRGL_ANGLE_MODE is defined
     */
    ALOGI("Loading ANGLE GLES from: %s", gles_path);
    s_gles_lib = dlopen(gles_path, RTLD_NOW | RTLD_GLOBAL);
    if (!s_gles_lib) {
        ALOGE("dlopen(%s) RTLD_GLOBAL failed: %s — trying RTLD_LOCAL", gles_path, dlerror());
        s_gles_lib = dlopen(gles_path, RTLD_NOW | RTLD_LOCAL);
    }
    if (!s_gles_lib) {
        ALOGE("dlopen(%s) failed completely: %s", gles_path, dlerror());
        return false;
    }
    ALOGI("ANGLE GLES loaded successfully (handle=%p)", s_gles_lib);

    /*
     * Set LD_LIBRARY_PATH to include ANGLE's directory before loading
     * libEGL.so.  ANGLE's EGL dispatcher uses dladdr + dirname to find
     * libGLESv2.so, but as a fallback it also searches LD_LIBRARY_PATH.
     * This ensures the fallback finds ANGLE's copy, not the system's.
     */
    {
        /* Extract directory from gles_path */
        char angle_dir[512];
        strncpy(angle_dir, gles_path, sizeof(angle_dir) - 1);
        angle_dir[sizeof(angle_dir) - 1] = '\0';
        char *last_slash = strrchr(angle_dir, '/');
        if (last_slash) *last_slash = '\0';

        const char *old_ld_path = getenv("LD_LIBRARY_PATH");
        char new_ld_path[1024];
        if (old_ld_path && old_ld_path[0]) {
            snprintf(new_ld_path, sizeof(new_ld_path), "%s:%s", angle_dir, old_ld_path);
        } else {
            snprintf(new_ld_path, sizeof(new_ld_path), "%s", angle_dir);
        }
        ALOGI("Setting LD_LIBRARY_PATH=%s (for ANGLE internal dlopen)", new_ld_path);
        setenv("LD_LIBRARY_PATH", new_ld_path, 1);
    }

    ALOGI("Loading ANGLE EGL from: %s", egl_path);
    s_egl_lib = dlopen(egl_path, RTLD_NOW | RTLD_LOCAL);
    if (!s_egl_lib) {
        ALOGE("dlopen(%s) RTLD_LOCAL failed: %s — trying RTLD_GLOBAL", egl_path, dlerror());
        s_egl_lib = dlopen(egl_path, RTLD_NOW | RTLD_GLOBAL);
    }
    if (!s_egl_lib) {
        ALOGE("dlopen(%s) failed completely: %s", egl_path, dlerror());
        dlclose(s_gles_lib); s_gles_lib = NULL;
        return false;
    }
    ALOGI("ANGLE EGL loaded successfully (handle=%p)", s_egl_lib);

    /* Verify the loaded EGL is actually ANGLE (not the system EGL) by
     * checking for an ANGLE-specific symbol: ANGLEGetDisplayPlatform.
     * If this symbol is missing, we loaded the wrong library. */
    {
        void *angle_check = dlsym(s_egl_lib, "ANGLEGetDisplayPlatform");
        if (!angle_check) {
            /* Try another ANGLE-specific symbol */
            angle_check = dlsym(s_egl_lib, "eglGetPlatformDisplayEXT");
        }
        if (angle_check) {
            ALOGI("ANGLE EGL verification passed (ANGLE-specific symbol found)");
        } else {
            ALOGE("WARNING: Loaded EGL library may not be ANGLE! "
                  "ANGLE-specific symbols not found.");
        }
    }

    /* ── EGL ─────────────────────────────────────────────────────── */
    RESOLVE(s_egl_lib, angle_eglGetDisplay,        eglGetDisplay);
    RESOLVE(s_egl_lib, angle_eglInitialize,        eglInitialize);
    RESOLVE(s_egl_lib, angle_eglBindAPI,           eglBindAPI);
    RESOLVE(s_egl_lib, angle_eglChooseConfig,      eglChooseConfig);
    RESOLVE(s_egl_lib, angle_eglCreateContext,      eglCreateContext);
    RESOLVE(s_egl_lib, angle_eglDestroyContext,     eglDestroyContext);
    RESOLVE(s_egl_lib, angle_eglMakeCurrent,        eglMakeCurrent);
    RESOLVE(s_egl_lib, angle_eglQueryString,        eglQueryString);
    RESOLVE(s_egl_lib, angle_eglGetError,           eglGetError);
    RESOLVE(s_egl_lib, angle_eglGetCurrentContext,  eglGetCurrentContext);
    RESOLVE(s_egl_lib, angle_eglGetProcAddress,     eglGetProcAddress);
    RESOLVE(s_egl_lib, angle_eglCreatePbufferSurface,eglCreatePbufferSurface);
    RESOLVE(s_egl_lib, angle_eglDestroySurface,     eglDestroySurface);
    RESOLVE(s_egl_lib, angle_eglTerminate,          eglTerminate);

    /* ── GLES 2.0 ────────────────────────────────────────────────── */
    RESOLVE(s_gles_lib, angle_glActiveTexture,        glActiveTexture);
    RESOLVE(s_gles_lib, angle_glAttachShader,         glAttachShader);
    RESOLVE(s_gles_lib, angle_glBindAttribLocation,   glBindAttribLocation);
    RESOLVE(s_gles_lib, angle_glBindBuffer,           glBindBuffer);
    RESOLVE(s_gles_lib, angle_glBindFramebuffer,      glBindFramebuffer);
    RESOLVE(s_gles_lib, angle_glBindTexture,          glBindTexture);
    RESOLVE(s_gles_lib, angle_glBlendColor,           glBlendColor);
    RESOLVE(s_gles_lib, angle_glBlendEquationSeparate,glBlendEquationSeparate);
    RESOLVE(s_gles_lib, angle_glBlendFuncSeparate,    glBlendFuncSeparate);
    RESOLVE(s_gles_lib, angle_glBufferData,           glBufferData);
    RESOLVE(s_gles_lib, angle_glBufferSubData,        glBufferSubData);
    RESOLVE(s_gles_lib, angle_glCheckFramebufferStatus,glCheckFramebufferStatus);
    RESOLVE(s_gles_lib, angle_glClear,                glClear);
    RESOLVE(s_gles_lib, angle_glClearColor,           glClearColor);
    RESOLVE(s_gles_lib, angle_glClearDepthf,          glClearDepthf);
    RESOLVE(s_gles_lib, angle_glClearStencil,         glClearStencil);
    RESOLVE(s_gles_lib, angle_glColorMask,            glColorMask);
    RESOLVE(s_gles_lib, angle_glCompileShader,        glCompileShader);
    RESOLVE(s_gles_lib, angle_glCompressedTexSubImage2D,glCompressedTexSubImage2D);
    RESOLVE(s_gles_lib, angle_glCreateProgram,        glCreateProgram);
    RESOLVE(s_gles_lib, angle_glCreateShader,         glCreateShader);
    RESOLVE(s_gles_lib, angle_glCullFace,             glCullFace);
    RESOLVE(s_gles_lib, angle_glDeleteBuffers,        glDeleteBuffers);
    RESOLVE(s_gles_lib, angle_glDeleteFramebuffers,   glDeleteFramebuffers);
    RESOLVE(s_gles_lib, angle_glDeleteProgram,        glDeleteProgram);
    RESOLVE(s_gles_lib, angle_glDeleteShader,         glDeleteShader);
    RESOLVE(s_gles_lib, angle_glDeleteTextures,       glDeleteTextures);
    RESOLVE(s_gles_lib, angle_glDepthFunc,            glDepthFunc);
    RESOLVE(s_gles_lib, angle_glDepthMask,            glDepthMask);
    RESOLVE(s_gles_lib, angle_glDepthRangef,          glDepthRangef);
    RESOLVE(s_gles_lib, angle_glDisable,              glDisable);
    RESOLVE(s_gles_lib, angle_glDisableVertexAttribArray,glDisableVertexAttribArray);
    RESOLVE(s_gles_lib, angle_glDrawArrays,           glDrawArrays);
    RESOLVE(s_gles_lib, angle_glDrawElements,         glDrawElements);
    RESOLVE(s_gles_lib, angle_glEnable,               glEnable);
    RESOLVE(s_gles_lib, angle_glEnableVertexAttribArray,glEnableVertexAttribArray);
    RESOLVE(s_gles_lib, angle_glFlush,                glFlush);
    RESOLVE(s_gles_lib, angle_glFinish,               glFinish);
    RESOLVE(s_gles_lib, angle_glFramebufferTexture2D, glFramebufferTexture2D);
    RESOLVE(s_gles_lib, angle_glFrontFace,            glFrontFace);
    RESOLVE(s_gles_lib, angle_glGenBuffers,           glGenBuffers);
    RESOLVE(s_gles_lib, angle_glGenFramebuffers,      glGenFramebuffers);
    RESOLVE(s_gles_lib, angle_glGenTextures,          glGenTextures);
    RESOLVE(s_gles_lib, angle_glGetAttribLocation,    glGetAttribLocation);
    RESOLVE(s_gles_lib, angle_glGetError,             glGetError);
    RESOLVE(s_gles_lib, angle_glGetFloatv,            glGetFloatv);
    RESOLVE(s_gles_lib, angle_glGetIntegerv,          glGetIntegerv);
    RESOLVE(s_gles_lib, angle_glGetProgramiv,         glGetProgramiv);
    RESOLVE(s_gles_lib, angle_glGetShaderiv,          glGetShaderiv);
    RESOLVE(s_gles_lib, angle_glGetString,            glGetString);
    RESOLVE(s_gles_lib, angle_glGetStringi,           glGetStringi);
    RESOLVE(s_gles_lib, angle_glGetUniformLocation,   glGetUniformLocation);
    RESOLVE(s_gles_lib, angle_glLineWidth,            glLineWidth);
    RESOLVE(s_gles_lib, angle_glLinkProgram,          glLinkProgram);
    RESOLVE(s_gles_lib, angle_glPixelStorei,          glPixelStorei);
    RESOLVE(s_gles_lib, angle_glPolygonOffset,        glPolygonOffset);
    RESOLVE(s_gles_lib, angle_glReadPixels,           glReadPixels);
    RESOLVE(s_gles_lib, angle_glScissor,              glScissor);
    RESOLVE(s_gles_lib, angle_glShaderSource,         glShaderSource);
    RESOLVE(s_gles_lib, angle_glStencilFunc,          glStencilFunc);
    RESOLVE(s_gles_lib, angle_glStencilFuncSeparate,  glStencilFuncSeparate);
    RESOLVE(s_gles_lib, angle_glStencilMask,          glStencilMask);
    RESOLVE(s_gles_lib, angle_glStencilMaskSeparate,  glStencilMaskSeparate);
    RESOLVE(s_gles_lib, angle_glStencilOp,            glStencilOp);
    RESOLVE(s_gles_lib, angle_glStencilOpSeparate,    glStencilOpSeparate);
    RESOLVE(s_gles_lib, angle_glTexImage2D,           glTexImage2D);
    RESOLVE(s_gles_lib, angle_glTexParameterf,        glTexParameterf);
    RESOLVE(s_gles_lib, angle_glTexParameteri,        glTexParameteri);
    RESOLVE(s_gles_lib, angle_glTexSubImage2D,        glTexSubImage2D);
    RESOLVE(s_gles_lib, angle_glUniform1f,            glUniform1f);
    RESOLVE(s_gles_lib, angle_glUniform1i,            glUniform1i);
    RESOLVE(s_gles_lib, angle_glUniform4f,            glUniform4f);
    RESOLVE(s_gles_lib, angle_glUniform4fv,           glUniform4fv);
    RESOLVE(s_gles_lib, angle_glUseProgram,           glUseProgram);
    RESOLVE(s_gles_lib, angle_glVertexAttribPointer,  glVertexAttribPointer);
    RESOLVE(s_gles_lib, angle_glViewport,             glViewport);

    /* ── GLES 3.0 ────────────────────────────────────────────────── */
    RESOLVE(s_gles_lib, angle_glBeginQuery,           glBeginQuery);
    RESOLVE(s_gles_lib, angle_glBeginTransformFeedback,glBeginTransformFeedback);
    RESOLVE(s_gles_lib, angle_glBindBufferBase,       glBindBufferBase);
    RESOLVE(s_gles_lib, angle_glBindBufferRange,      glBindBufferRange);
    RESOLVE(s_gles_lib, angle_glBindSampler,          glBindSampler);
    RESOLVE(s_gles_lib, angle_glBindTransformFeedback,glBindTransformFeedback);
    RESOLVE(s_gles_lib, angle_glBindVertexArray,      glBindVertexArray);
    RESOLVE(s_gles_lib, angle_glBlitFramebuffer,      glBlitFramebuffer);
    RESOLVE(s_gles_lib, angle_glClearBufferfv,        glClearBufferfv);
    RESOLVE(s_gles_lib, angle_glClearBufferiv,        glClearBufferiv);
    RESOLVE(s_gles_lib, angle_glClearBufferuiv,       glClearBufferuiv);
    RESOLVE(s_gles_lib, angle_glClientWaitSync,       glClientWaitSync);
    RESOLVE(s_gles_lib, angle_glCompressedTexSubImage3D,glCompressedTexSubImage3D);
    RESOLVE(s_gles_lib, angle_glCopyBufferSubData,    glCopyBufferSubData);
    RESOLVE(s_gles_lib, angle_glDeleteQueries,        glDeleteQueries);
    RESOLVE(s_gles_lib, angle_glDeleteSamplers,       glDeleteSamplers);
    RESOLVE(s_gles_lib, angle_glDeleteSync,           glDeleteSync);
    RESOLVE(s_gles_lib, angle_glDeleteTransformFeedbacks,glDeleteTransformFeedbacks);
    RESOLVE(s_gles_lib, angle_glDeleteVertexArrays,   glDeleteVertexArrays);
    RESOLVE(s_gles_lib, angle_glDrawArraysInstanced,  glDrawArraysInstanced);
    RESOLVE(s_gles_lib, angle_glDrawBuffers,          glDrawBuffers);
    RESOLVE(s_gles_lib, angle_glDrawElementsInstanced,glDrawElementsInstanced);
    RESOLVE(s_gles_lib, angle_glDrawRangeElements,    glDrawRangeElements);
    RESOLVE(s_gles_lib, angle_glEndQuery,             glEndQuery);
    RESOLVE(s_gles_lib, angle_glEndTransformFeedback, glEndTransformFeedback);
    RESOLVE(s_gles_lib, angle_glFenceSync,            glFenceSync);
    RESOLVE(s_gles_lib, angle_glFramebufferTextureLayer,glFramebufferTextureLayer);
    RESOLVE(s_gles_lib, angle_glGenQueries,           glGenQueries);
    RESOLVE(s_gles_lib, angle_glGenSamplers,          glGenSamplers);
    RESOLVE(s_gles_lib, angle_glGenTransformFeedbacks,glGenTransformFeedbacks);
    RESOLVE(s_gles_lib, angle_glGenVertexArrays,      glGenVertexArrays);
    RESOLVE(s_gles_lib, angle_glGetIntegeri_v,        glGetIntegeri_v);
    RESOLVE(s_gles_lib, angle_glGetQueryObjectuiv,    glGetQueryObjectuiv);
    RESOLVE(s_gles_lib, angle_glGetUniformBlockIndex,  glGetUniformBlockIndex);
    RESOLVE(s_gles_lib, angle_glMapBufferRange,       glMapBufferRange);
    RESOLVE(s_gles_lib, angle_glPauseTransformFeedback,glPauseTransformFeedback);
    RESOLVE(s_gles_lib, angle_glReadBuffer,           glReadBuffer);
    RESOLVE(s_gles_lib, angle_glResumeTransformFeedback,glResumeTransformFeedback);
    RESOLVE(s_gles_lib, angle_glSamplerParameterf,    glSamplerParameterf);
    RESOLVE(s_gles_lib, angle_glSamplerParameteri,    glSamplerParameteri);
    RESOLVE(s_gles_lib, angle_glTexImage3D,           glTexImage3D);
    RESOLVE(s_gles_lib, angle_glTexStorage2D,         glTexStorage2D);
    RESOLVE(s_gles_lib, angle_glTexStorage3D,         glTexStorage3D);
    RESOLVE(s_gles_lib, angle_glTexSubImage3D,        glTexSubImage3D);
    RESOLVE(s_gles_lib, angle_glUniform4uiv,          glUniform4uiv);
    RESOLVE(s_gles_lib, angle_glUniformBlockBinding,  glUniformBlockBinding);
    RESOLVE(s_gles_lib, angle_glUnmapBuffer,          glUnmapBuffer);
    RESOLVE(s_gles_lib, angle_glVertexAttribDivisor,  glVertexAttribDivisor);
    RESOLVE(s_gles_lib, angle_glVertexAttribIPointer, glVertexAttribIPointer);

    /* ── GLES 3.1 ────────────────────────────────────────────────── */
    RESOLVE(s_gles_lib, angle_glBindImageTexture,     glBindImageTexture);
    RESOLVE(s_gles_lib, angle_glBindVertexBuffer,     glBindVertexBuffer);
    RESOLVE(s_gles_lib, angle_glDispatchCompute,      glDispatchCompute);
    RESOLVE(s_gles_lib, angle_glDispatchComputeIndirect,glDispatchComputeIndirect);
    RESOLVE(s_gles_lib, angle_glDrawArraysIndirect,   glDrawArraysIndirect);
    RESOLVE(s_gles_lib, angle_glDrawElementsIndirect, glDrawElementsIndirect);
    RESOLVE(s_gles_lib, angle_glFramebufferParameteri,glFramebufferParameteri);
    RESOLVE(s_gles_lib, angle_glGetProgramResourceIndex,glGetProgramResourceIndex);
    RESOLVE(s_gles_lib, angle_glMemoryBarrier,        glMemoryBarrier);
    RESOLVE(s_gles_lib, angle_glTexStorage2DMultisample,glTexStorage2DMultisample);
    RESOLVE(s_gles_lib, angle_glVertexAttribBinding,  glVertexAttribBinding);
    RESOLVE(s_gles_lib, angle_glVertexAttribFormat,   glVertexAttribFormat);
    RESOLVE(s_gles_lib, angle_glVertexAttribIFormat,  glVertexAttribIFormat);
    RESOLVE(s_gles_lib, angle_glVertexBindingDivisor, glVertexBindingDivisor);

    /* ── GLES 3.2 ────────────────────────────────────────────────── */
    RESOLVE(s_gles_lib, angle_glBlendEquationSeparatei,glBlendEquationSeparatei);
    RESOLVE(s_gles_lib, angle_glBlendFuncSeparatei,   glBlendFuncSeparatei);
    RESOLVE(s_gles_lib, angle_glColorMaski,           glColorMaski);
    RESOLVE(s_gles_lib, angle_glCopyImageSubData,     glCopyImageSubData);
    RESOLVE(s_gles_lib, angle_glDisablei,             glDisablei);
    RESOLVE(s_gles_lib, angle_glDrawElementsBaseVertex,glDrawElementsBaseVertex);
    RESOLVE(s_gles_lib, angle_glDrawElementsInstancedBaseVertex,glDrawElementsInstancedBaseVertex);
    RESOLVE(s_gles_lib, angle_glDrawRangeElementsBaseVertex,glDrawRangeElementsBaseVertex);
    RESOLVE(s_gles_lib, angle_glEnablei,              glEnablei);
    RESOLVE(s_gles_lib, angle_glFramebufferTexture,   glFramebufferTexture);
    RESOLVE(s_gles_lib, angle_glGetMultisamplefv,     glGetMultisamplefv);
    RESOLVE(s_gles_lib, angle_glMinSampleShading,     glMinSampleShading);
    RESOLVE(s_gles_lib, angle_glPatchParameteri,      glPatchParameteri);
    RESOLVE(s_gles_lib, angle_glSampleMaski,          glSampleMaski);
    RESOLVE(s_gles_lib, angle_glSamplerParameterIuiv, glSamplerParameterIuiv);
    RESOLVE(s_gles_lib, angle_glTexBuffer,            glTexBuffer);
    RESOLVE(s_gles_lib, angle_glTexBufferRange,       glTexBufferRange);
    RESOLVE(s_gles_lib, angle_glTexParameterIuiv,     glTexParameterIuiv);
    RESOLVE(s_gles_lib, angle_glTexStorage3DMultisample,glTexStorage3DMultisample);

    /* ── Misc ────────────────────────────────────────────────────── */
    RESOLVE(s_gles_lib, angle_glVertexAttrib1fv,      glVertexAttrib1fv);
    RESOLVE(s_gles_lib, angle_glVertexAttrib2fv,      glVertexAttrib2fv);
    RESOLVE(s_gles_lib, angle_glVertexAttrib3fv,      glVertexAttrib3fv);
    RESOLVE(s_gles_lib, angle_glVertexAttrib4fv,      glVertexAttrib4fv);

    s_using_angle = true;
    ALOGI("ANGLE dispatch initialised — all EGL/GLES calls now routed through ANGLE");
    return true;
}

