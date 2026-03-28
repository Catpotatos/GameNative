/**************************************************************************
 * ANGLE EGL/GLES Dispatch Table
 *
 * When VIRGL_ANGLE_MODE is defined, this header:
 *   1. Includes system EGL/GLES headers (to get type definitions)
 *   2. Declares angle_ prefixed function pointer variables
 *   3. Defines redirect macros for all gl and egl calls
 *
 * angle_dispatch.c populates the pointers from ANGLE libraries loaded
 * via dlopen. If ANGLE cannot be loaded, it falls back to the system
 * EGL/GLES implementations (the library is still linked against them).
 *
 * This file is force-included (-include) so the redirects apply to
 * every translation unit in the virglrenderer_angle build.
 **************************************************************************/

#ifndef ANGLE_DISPATCH_H
#define ANGLE_DISPATCH_H

#ifdef VIRGL_ANGLE_MODE

#include <stdbool.h>

/* ── Phase 1: Include system headers ─────────────────────────────────── */
/* We must include them BEFORE defining redirect macros so that the
 * original function declarations are visible and __typeof__ works.     */

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES3/gl32.h>

/* ── Public API ──────────────────────────────────────────────────────── */

/*
 * Load ANGLE's libEGL.so and libGLESv2.so from the given paths and
 * populate all function pointers.  Returns true on success.
 * On failure the pointers remain set to the system implementations.
 */
bool angle_dispatch_init(const char *egl_path, const char *gles_path);

/* Returns true after a successful angle_dispatch_init(). */
bool angle_dispatch_is_angle(void);

/* ── Phase 2: Declare function-pointer variables ─────────────────────
 *
 * Each variable is declared as a pointer to the function's type,
 * derived with __typeof__ from the system header declaration.
 *
 * We declare them BEFORE the redirect macros so __typeof__(fn) sees
 * the real declaration, not a macro-expanded name.
 * ──────────────────────────────────────────────────────────────────── */

/* --- EGL ----------------------------------------------------------- */
extern __typeof__(eglGetDisplay)          *angle_eglGetDisplay;
extern __typeof__(eglInitialize)          *angle_eglInitialize;
extern __typeof__(eglBindAPI)             *angle_eglBindAPI;
extern __typeof__(eglChooseConfig)        *angle_eglChooseConfig;
extern __typeof__(eglCreateContext)       *angle_eglCreateContext;
extern __typeof__(eglDestroyContext)      *angle_eglDestroyContext;
extern __typeof__(eglMakeCurrent)         *angle_eglMakeCurrent;
extern __typeof__(eglQueryString)         *angle_eglQueryString;
extern __typeof__(eglGetError)            *angle_eglGetError;
extern __typeof__(eglGetCurrentContext)   *angle_eglGetCurrentContext;
extern __typeof__(eglGetProcAddress)      *angle_eglGetProcAddress;
extern __typeof__(eglCreatePbufferSurface)*angle_eglCreatePbufferSurface;
extern __typeof__(eglDestroySurface)      *angle_eglDestroySurface;
extern __typeof__(eglTerminate)           *angle_eglTerminate;

/* --- GLES 2.0 ------------------------------------------------------ */
extern __typeof__(glActiveTexture)        *angle_glActiveTexture;
extern __typeof__(glAttachShader)         *angle_glAttachShader;
extern __typeof__(glBindAttribLocation)   *angle_glBindAttribLocation;
extern __typeof__(glBindBuffer)           *angle_glBindBuffer;
extern __typeof__(glBindFramebuffer)      *angle_glBindFramebuffer;
extern __typeof__(glBindTexture)          *angle_glBindTexture;
extern __typeof__(glBlendColor)           *angle_glBlendColor;
extern __typeof__(glBlendEquationSeparate)*angle_glBlendEquationSeparate;
extern __typeof__(glBlendFuncSeparate)    *angle_glBlendFuncSeparate;
extern __typeof__(glBufferData)           *angle_glBufferData;
extern __typeof__(glBufferSubData)        *angle_glBufferSubData;
extern __typeof__(glCheckFramebufferStatus)*angle_glCheckFramebufferStatus;
extern __typeof__(glClear)                *angle_glClear;
extern __typeof__(glClearColor)           *angle_glClearColor;
extern __typeof__(glClearDepthf)          *angle_glClearDepthf;
extern __typeof__(glClearStencil)         *angle_glClearStencil;
extern __typeof__(glColorMask)            *angle_glColorMask;
extern __typeof__(glCompileShader)        *angle_glCompileShader;
extern __typeof__(glCompressedTexSubImage2D)*angle_glCompressedTexSubImage2D;
extern __typeof__(glCreateProgram)        *angle_glCreateProgram;
extern __typeof__(glCreateShader)         *angle_glCreateShader;
extern __typeof__(glCullFace)             *angle_glCullFace;
extern __typeof__(glDeleteBuffers)        *angle_glDeleteBuffers;
extern __typeof__(glDeleteFramebuffers)   *angle_glDeleteFramebuffers;
extern __typeof__(glDeleteProgram)        *angle_glDeleteProgram;
extern __typeof__(glDeleteShader)         *angle_glDeleteShader;
extern __typeof__(glDeleteTextures)       *angle_glDeleteTextures;
extern __typeof__(glDepthFunc)            *angle_glDepthFunc;
extern __typeof__(glDepthMask)            *angle_glDepthMask;
extern __typeof__(glDepthRangef)          *angle_glDepthRangef;
extern __typeof__(glDisable)              *angle_glDisable;
extern __typeof__(glDisableVertexAttribArray)*angle_glDisableVertexAttribArray;
extern __typeof__(glDrawArrays)           *angle_glDrawArrays;
extern __typeof__(glDrawElements)         *angle_glDrawElements;
extern __typeof__(glEnable)               *angle_glEnable;
extern __typeof__(glEnableVertexAttribArray)*angle_glEnableVertexAttribArray;
extern __typeof__(glFlush)                *angle_glFlush;
extern __typeof__(glFinish)               *angle_glFinish;
extern __typeof__(glFramebufferTexture2D) *angle_glFramebufferTexture2D;
extern __typeof__(glFrontFace)            *angle_glFrontFace;
extern __typeof__(glGenBuffers)           *angle_glGenBuffers;
extern __typeof__(glGenFramebuffers)      *angle_glGenFramebuffers;
extern __typeof__(glGenTextures)          *angle_glGenTextures;
extern __typeof__(glGetAttribLocation)    *angle_glGetAttribLocation;
extern __typeof__(glGetError)             *angle_glGetError;
extern __typeof__(glGetFloatv)            *angle_glGetFloatv;
extern __typeof__(glGetIntegerv)          *angle_glGetIntegerv;
extern __typeof__(glGetProgramiv)         *angle_glGetProgramiv;
extern __typeof__(glGetShaderiv)          *angle_glGetShaderiv;
extern __typeof__(glGetString)            *angle_glGetString;
extern __typeof__(glGetStringi)           *angle_glGetStringi;
extern __typeof__(glGetUniformLocation)   *angle_glGetUniformLocation;
extern __typeof__(glLineWidth)            *angle_glLineWidth;
extern __typeof__(glLinkProgram)          *angle_glLinkProgram;
extern __typeof__(glPixelStorei)          *angle_glPixelStorei;
extern __typeof__(glPolygonOffset)        *angle_glPolygonOffset;
extern __typeof__(glReadPixels)           *angle_glReadPixels;
extern __typeof__(glScissor)              *angle_glScissor;
extern __typeof__(glShaderSource)         *angle_glShaderSource;
extern __typeof__(glStencilFunc)          *angle_glStencilFunc;
extern __typeof__(glStencilFuncSeparate)  *angle_glStencilFuncSeparate;
extern __typeof__(glStencilMask)          *angle_glStencilMask;
extern __typeof__(glStencilMaskSeparate)  *angle_glStencilMaskSeparate;
extern __typeof__(glStencilOp)            *angle_glStencilOp;
extern __typeof__(glStencilOpSeparate)    *angle_glStencilOpSeparate;
extern __typeof__(glTexImage2D)           *angle_glTexImage2D;
extern __typeof__(glTexParameterf)        *angle_glTexParameterf;
extern __typeof__(glTexParameteri)        *angle_glTexParameteri;
extern __typeof__(glTexSubImage2D)        *angle_glTexSubImage2D;
extern __typeof__(glUniform1f)            *angle_glUniform1f;
extern __typeof__(glUniform1i)            *angle_glUniform1i;
extern __typeof__(glUniform4f)            *angle_glUniform4f;
extern __typeof__(glUniform4fv)           *angle_glUniform4fv;
extern __typeof__(glUseProgram)           *angle_glUseProgram;
extern __typeof__(glVertexAttribPointer)  *angle_glVertexAttribPointer;
extern __typeof__(glViewport)             *angle_glViewport;

/* --- GLES 3.0 ------------------------------------------------------ */
extern __typeof__(glBeginQuery)           *angle_glBeginQuery;
extern __typeof__(glBeginTransformFeedback)*angle_glBeginTransformFeedback;
extern __typeof__(glBindBufferBase)       *angle_glBindBufferBase;
extern __typeof__(glBindBufferRange)      *angle_glBindBufferRange;
extern __typeof__(glBindSampler)          *angle_glBindSampler;
extern __typeof__(glBindTransformFeedback)*angle_glBindTransformFeedback;
extern __typeof__(glBindVertexArray)      *angle_glBindVertexArray;
extern __typeof__(glBlitFramebuffer)      *angle_glBlitFramebuffer;
extern __typeof__(glClearBufferfv)        *angle_glClearBufferfv;
extern __typeof__(glClearBufferiv)        *angle_glClearBufferiv;
extern __typeof__(glClearBufferuiv)       *angle_glClearBufferuiv;
extern __typeof__(glClientWaitSync)       *angle_glClientWaitSync;
extern __typeof__(glCompressedTexSubImage3D)*angle_glCompressedTexSubImage3D;
extern __typeof__(glCopyBufferSubData)    *angle_glCopyBufferSubData;
extern __typeof__(glDeleteQueries)        *angle_glDeleteQueries;
extern __typeof__(glDeleteSamplers)       *angle_glDeleteSamplers;
extern __typeof__(glDeleteSync)           *angle_glDeleteSync;
extern __typeof__(glDeleteTransformFeedbacks)*angle_glDeleteTransformFeedbacks;
extern __typeof__(glDeleteVertexArrays)   *angle_glDeleteVertexArrays;
extern __typeof__(glDrawArraysInstanced)  *angle_glDrawArraysInstanced;
extern __typeof__(glDrawBuffers)          *angle_glDrawBuffers;
extern __typeof__(glDrawElementsInstanced)*angle_glDrawElementsInstanced;
extern __typeof__(glDrawRangeElements)    *angle_glDrawRangeElements;
extern __typeof__(glEndQuery)             *angle_glEndQuery;
extern __typeof__(glEndTransformFeedback) *angle_glEndTransformFeedback;
extern __typeof__(glFenceSync)            *angle_glFenceSync;
extern __typeof__(glFramebufferTextureLayer)*angle_glFramebufferTextureLayer;
extern __typeof__(glGenQueries)           *angle_glGenQueries;
extern __typeof__(glGenSamplers)          *angle_glGenSamplers;
extern __typeof__(glGenTransformFeedbacks)*angle_glGenTransformFeedbacks;
extern __typeof__(glGenVertexArrays)      *angle_glGenVertexArrays;
extern __typeof__(glGetIntegeri_v)        *angle_glGetIntegeri_v;
extern __typeof__(glGetQueryObjectuiv)    *angle_glGetQueryObjectuiv;
extern __typeof__(glGetUniformBlockIndex) *angle_glGetUniformBlockIndex;
extern __typeof__(glMapBufferRange)       *angle_glMapBufferRange;
extern __typeof__(glPauseTransformFeedback)*angle_glPauseTransformFeedback;
extern __typeof__(glReadBuffer)           *angle_glReadBuffer;
extern __typeof__(glResumeTransformFeedback)*angle_glResumeTransformFeedback;
extern __typeof__(glSamplerParameterf)    *angle_glSamplerParameterf;
extern __typeof__(glSamplerParameteri)    *angle_glSamplerParameteri;
extern __typeof__(glTexImage3D)           *angle_glTexImage3D;
extern __typeof__(glTexStorage2D)         *angle_glTexStorage2D;
extern __typeof__(glTexStorage3D)         *angle_glTexStorage3D;
extern __typeof__(glTexSubImage3D)        *angle_glTexSubImage3D;
extern __typeof__(glUniform4uiv)          *angle_glUniform4uiv;
extern __typeof__(glUniformBlockBinding)  *angle_glUniformBlockBinding;
extern __typeof__(glUnmapBuffer)          *angle_glUnmapBuffer;
extern __typeof__(glVertexAttribDivisor)  *angle_glVertexAttribDivisor;
extern __typeof__(glVertexAttribIPointer)  *angle_glVertexAttribIPointer;

/* --- GLES 3.1 ------------------------------------------------------ */
extern __typeof__(glBindImageTexture)     *angle_glBindImageTexture;
extern __typeof__(glBindVertexBuffer)     *angle_glBindVertexBuffer;
extern __typeof__(glDispatchCompute)      *angle_glDispatchCompute;
extern __typeof__(glDispatchComputeIndirect)*angle_glDispatchComputeIndirect;
extern __typeof__(glDrawArraysIndirect)   *angle_glDrawArraysIndirect;
extern __typeof__(glDrawElementsIndirect) *angle_glDrawElementsIndirect;
extern __typeof__(glFramebufferParameteri)*angle_glFramebufferParameteri;
extern __typeof__(glGetProgramResourceIndex)*angle_glGetProgramResourceIndex;
extern __typeof__(glMemoryBarrier)        *angle_glMemoryBarrier;
extern __typeof__(glTexStorage2DMultisample)*angle_glTexStorage2DMultisample;
extern __typeof__(glVertexAttribBinding)  *angle_glVertexAttribBinding;
extern __typeof__(glVertexAttribFormat)   *angle_glVertexAttribFormat;
extern __typeof__(glVertexAttribIFormat)  *angle_glVertexAttribIFormat;
extern __typeof__(glVertexBindingDivisor) *angle_glVertexBindingDivisor;

/* --- GLES 3.2 ------------------------------------------------------ */
extern __typeof__(glBlendEquationSeparatei)*angle_glBlendEquationSeparatei;
extern __typeof__(glBlendFuncSeparatei)   *angle_glBlendFuncSeparatei;
extern __typeof__(glColorMaski)           *angle_glColorMaski;
extern __typeof__(glCopyImageSubData)     *angle_glCopyImageSubData;
extern __typeof__(glDisablei)             *angle_glDisablei;
extern __typeof__(glDrawElementsBaseVertex)*angle_glDrawElementsBaseVertex;
extern __typeof__(glDrawElementsInstancedBaseVertex)*angle_glDrawElementsInstancedBaseVertex;
extern __typeof__(glDrawRangeElementsBaseVertex)*angle_glDrawRangeElementsBaseVertex;
extern __typeof__(glEnablei)              *angle_glEnablei;
extern __typeof__(glFramebufferTexture)   *angle_glFramebufferTexture;
extern __typeof__(glGetMultisamplefv)     *angle_glGetMultisamplefv;
extern __typeof__(glMinSampleShading)     *angle_glMinSampleShading;
extern __typeof__(glPatchParameteri)      *angle_glPatchParameteri;
extern __typeof__(glSampleMaski)          *angle_glSampleMaski;
extern __typeof__(glSamplerParameterIuiv) *angle_glSamplerParameterIuiv;
extern __typeof__(glTexBuffer)            *angle_glTexBuffer;
extern __typeof__(glTexBufferRange)       *angle_glTexBufferRange;
extern __typeof__(glTexParameterIuiv)     *angle_glTexParameterIuiv;
extern __typeof__(glTexStorage3DMultisample)*angle_glTexStorage3DMultisample;

/* Also used in vrend_renderer.c / virgl_server_renderer.c: */
extern __typeof__(glVertexAttrib1fv)      *angle_glVertexAttrib1fv;
extern __typeof__(glVertexAttrib2fv)      *angle_glVertexAttrib2fv;
extern __typeof__(glVertexAttrib3fv)      *angle_glVertexAttrib3fv;
extern __typeof__(glVertexAttrib4fv)      *angle_glVertexAttrib4fv;

/* ── Phase 3: Redirect macros ────────────────────────────────────────
 *
 * Every call to eglFoo() / glFoo() is rewritten to angle_eglFoo() /
 * angle_glFoo() at the preprocessor level.  The .c file that
 * IMPLEMENTS the dispatch (angle_dispatch.c) defines
 * ANGLE_DISPATCH_IMPL to suppress these macros so it can reference
 * the real system symbols as fallbacks.
 * ──────────────────────────────────────────────────────────────────── */

#ifndef ANGLE_DISPATCH_IMPL

/* EGL */
#define eglGetDisplay           angle_eglGetDisplay
#define eglInitialize           angle_eglInitialize
#define eglBindAPI              angle_eglBindAPI
#define eglChooseConfig         angle_eglChooseConfig
#define eglCreateContext        angle_eglCreateContext
#define eglDestroyContext       angle_eglDestroyContext
#define eglMakeCurrent          angle_eglMakeCurrent
#define eglQueryString          angle_eglQueryString
#define eglGetError             angle_eglGetError
#define eglGetCurrentContext    angle_eglGetCurrentContext
#define eglGetProcAddress       angle_eglGetProcAddress
#define eglCreatePbufferSurface angle_eglCreatePbufferSurface
#define eglDestroySurface       angle_eglDestroySurface
#define eglTerminate            angle_eglTerminate

/* GLES 2.0 */
#define glActiveTexture         angle_glActiveTexture
#define glAttachShader          angle_glAttachShader
#define glBindAttribLocation    angle_glBindAttribLocation
#define glBindBuffer            angle_glBindBuffer
#define glBindFramebuffer       angle_glBindFramebuffer
#define glBindTexture           angle_glBindTexture
#define glBlendColor            angle_glBlendColor
#define glBlendEquationSeparate angle_glBlendEquationSeparate
#define glBlendFuncSeparate     angle_glBlendFuncSeparate
#define glBufferData            angle_glBufferData
#define glBufferSubData         angle_glBufferSubData
#define glCheckFramebufferStatus angle_glCheckFramebufferStatus
#define glClear                 angle_glClear
#define glClearColor            angle_glClearColor
#define glClearDepthf           angle_glClearDepthf
#define glClearStencil          angle_glClearStencil
#define glColorMask             angle_glColorMask
#define glCompileShader         angle_glCompileShader
#define glCompressedTexSubImage2D angle_glCompressedTexSubImage2D
#define glCreateProgram         angle_glCreateProgram
#define glCreateShader          angle_glCreateShader
#define glCullFace              angle_glCullFace
#define glDeleteBuffers         angle_glDeleteBuffers
#define glDeleteFramebuffers    angle_glDeleteFramebuffers
#define glDeleteProgram         angle_glDeleteProgram
#define glDeleteShader          angle_glDeleteShader
#define glDeleteTextures        angle_glDeleteTextures
#define glDepthFunc             angle_glDepthFunc
#define glDepthMask             angle_glDepthMask
#define glDepthRangef           angle_glDepthRangef
#define glDisable               angle_glDisable
#define glDisableVertexAttribArray angle_glDisableVertexAttribArray
#define glDrawArrays            angle_glDrawArrays
#define glDrawElements          angle_glDrawElements
#define glEnable                angle_glEnable
#define glEnableVertexAttribArray angle_glEnableVertexAttribArray
#define glFlush                 angle_glFlush
#define glFinish                angle_glFinish
#define glFramebufferTexture2D  angle_glFramebufferTexture2D
#define glFrontFace             angle_glFrontFace
#define glGenBuffers            angle_glGenBuffers
#define glGenFramebuffers       angle_glGenFramebuffers
#define glGenTextures           angle_glGenTextures
#define glGetAttribLocation     angle_glGetAttribLocation
#define glGetError              angle_glGetError
#define glGetFloatv             angle_glGetFloatv
#define glGetIntegerv           angle_glGetIntegerv
#define glGetProgramiv          angle_glGetProgramiv
#define glGetShaderiv           angle_glGetShaderiv
#define glGetString             angle_glGetString
#define glGetStringi            angle_glGetStringi
#define glGetUniformLocation    angle_glGetUniformLocation
#define glLineWidth             angle_glLineWidth
#define glLinkProgram           angle_glLinkProgram
#define glPixelStorei           angle_glPixelStorei
#define glPolygonOffset         angle_glPolygonOffset
#define glReadPixels            angle_glReadPixels
#define glScissor               angle_glScissor
#define glShaderSource          angle_glShaderSource
#define glStencilFunc           angle_glStencilFunc
#define glStencilFuncSeparate   angle_glStencilFuncSeparate
#define glStencilMask           angle_glStencilMask
#define glStencilMaskSeparate   angle_glStencilMaskSeparate
#define glStencilOp             angle_glStencilOp
#define glStencilOpSeparate     angle_glStencilOpSeparate
#define glTexImage2D            angle_glTexImage2D
#define glTexParameterf         angle_glTexParameterf
#define glTexParameteri         angle_glTexParameteri
#define glTexSubImage2D         angle_glTexSubImage2D
#define glUniform1f             angle_glUniform1f
#define glUniform1i             angle_glUniform1i
#define glUniform4f             angle_glUniform4f
#define glUniform4fv            angle_glUniform4fv
#define glUseProgram            angle_glUseProgram
#define glVertexAttribPointer   angle_glVertexAttribPointer
#define glViewport              angle_glViewport

/* GLES 3.0 */
#define glBeginQuery            angle_glBeginQuery
#define glBeginTransformFeedback angle_glBeginTransformFeedback
#define glBindBufferBase        angle_glBindBufferBase
#define glBindBufferRange       angle_glBindBufferRange
#define glBindSampler           angle_glBindSampler
#define glBindTransformFeedback angle_glBindTransformFeedback
#define glBindVertexArray       angle_glBindVertexArray
#define glBlitFramebuffer       angle_glBlitFramebuffer
#define glClearBufferfv         angle_glClearBufferfv
#define glClearBufferiv         angle_glClearBufferiv
#define glClearBufferuiv        angle_glClearBufferuiv
#define glClientWaitSync        angle_glClientWaitSync
#define glCompressedTexSubImage3D angle_glCompressedTexSubImage3D
#define glCopyBufferSubData     angle_glCopyBufferSubData
#define glDeleteQueries         angle_glDeleteQueries
#define glDeleteSamplers        angle_glDeleteSamplers
#define glDeleteSync            angle_glDeleteSync
#define glDeleteTransformFeedbacks angle_glDeleteTransformFeedbacks
#define glDeleteVertexArrays    angle_glDeleteVertexArrays
#define glDrawArraysInstanced   angle_glDrawArraysInstanced
#define glDrawBuffers           angle_glDrawBuffers
#define glDrawElementsInstanced angle_glDrawElementsInstanced
#define glDrawRangeElements     angle_glDrawRangeElements
#define glEndQuery              angle_glEndQuery
#define glEndTransformFeedback  angle_glEndTransformFeedback
#define glFenceSync             angle_glFenceSync
#define glFramebufferTextureLayer angle_glFramebufferTextureLayer
#define glGenQueries            angle_glGenQueries
#define glGenSamplers           angle_glGenSamplers
#define glGenTransformFeedbacks angle_glGenTransformFeedbacks
#define glGenVertexArrays       angle_glGenVertexArrays
#define glGetIntegeri_v         angle_glGetIntegeri_v
#define glGetQueryObjectuiv     angle_glGetQueryObjectuiv
#define glGetUniformBlockIndex  angle_glGetUniformBlockIndex
#define glMapBufferRange        angle_glMapBufferRange
#define glPauseTransformFeedback angle_glPauseTransformFeedback
#define glReadBuffer            angle_glReadBuffer
#define glResumeTransformFeedback angle_glResumeTransformFeedback
#define glSamplerParameterf     angle_glSamplerParameterf
#define glSamplerParameteri     angle_glSamplerParameteri
#define glTexImage3D            angle_glTexImage3D
#define glTexStorage2D          angle_glTexStorage2D
#define glTexStorage3D          angle_glTexStorage3D
#define glTexSubImage3D         angle_glTexSubImage3D
#define glUniform4uiv           angle_glUniform4uiv
#define glUniformBlockBinding   angle_glUniformBlockBinding
#define glUnmapBuffer           angle_glUnmapBuffer
#define glVertexAttribDivisor   angle_glVertexAttribDivisor
#define glVertexAttribIPointer  angle_glVertexAttribIPointer

/* GLES 3.1 */
#define glBindImageTexture      angle_glBindImageTexture
#define glBindVertexBuffer      angle_glBindVertexBuffer
#define glDispatchCompute       angle_glDispatchCompute
#define glDispatchComputeIndirect angle_glDispatchComputeIndirect
#define glDrawArraysIndirect    angle_glDrawArraysIndirect
#define glDrawElementsIndirect  angle_glDrawElementsIndirect
#define glFramebufferParameteri angle_glFramebufferParameteri
#define glGetProgramResourceIndex angle_glGetProgramResourceIndex
#define glMemoryBarrier         angle_glMemoryBarrier
#define glTexStorage2DMultisample angle_glTexStorage2DMultisample
#define glVertexAttribBinding   angle_glVertexAttribBinding
#define glVertexAttribFormat    angle_glVertexAttribFormat
#define glVertexAttribIFormat   angle_glVertexAttribIFormat
#define glVertexBindingDivisor  angle_glVertexBindingDivisor

/* GLES 3.2 */
#define glBlendEquationSeparatei angle_glBlendEquationSeparatei
#define glBlendFuncSeparatei    angle_glBlendFuncSeparatei
#define glColorMaski            angle_glColorMaski
#define glCopyImageSubData      angle_glCopyImageSubData
#define glDisablei              angle_glDisablei
#define glDrawElementsBaseVertex angle_glDrawElementsBaseVertex
#define glDrawElementsInstancedBaseVertex angle_glDrawElementsInstancedBaseVertex
#define glDrawRangeElementsBaseVertex angle_glDrawRangeElementsBaseVertex
#define glEnablei               angle_glEnablei
#define glFramebufferTexture    angle_glFramebufferTexture
#define glGetMultisamplefv      angle_glGetMultisamplefv
#define glMinSampleShading      angle_glMinSampleShading
#define glPatchParameteri       angle_glPatchParameteri
#define glSampleMaski           angle_glSampleMaski
#define glSamplerParameterIuiv  angle_glSamplerParameterIuiv
#define glTexBuffer             angle_glTexBuffer
#define glTexBufferRange        angle_glTexBufferRange
#define glTexParameterIuiv      angle_glTexParameterIuiv
#define glTexStorage3DMultisample angle_glTexStorage3DMultisample

/* Misc */
#define glVertexAttrib1fv       angle_glVertexAttrib1fv
#define glVertexAttrib2fv       angle_glVertexAttrib2fv
#define glVertexAttrib3fv       angle_glVertexAttrib3fv
#define glVertexAttrib4fv       angle_glVertexAttrib4fv

#endif /* !ANGLE_DISPATCH_IMPL */

#endif /* VIRGL_ANGLE_MODE */
#endif /* ANGLE_DISPATCH_H */

