/*
 * tgsi_exec.h - Minimal stub for Mesa 24.3.4 TGSI compatibility.
 * The full tgsi_exec is the software TGSI interpreter which is not needed
 * for virglrenderer (it uses vrend_shader.c for TGSI -> GLSL translation).
 * This stub satisfies the #include without pulling in the full interpreter.
 */
#ifndef TGSI_EXEC_H
#define TGSI_EXEC_H

/* No exec functionality needed for virglrenderer */

#endif /* TGSI_EXEC_H */

