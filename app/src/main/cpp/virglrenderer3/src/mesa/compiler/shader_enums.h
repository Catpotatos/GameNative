/*
 * shader_enums.h - Minimal shim for Mesa 24.3.4 compatibility.
 * The full shader_enums.h defines gl_shader_stage, MESA_SHADER_*, etc.
 * For virglrenderer, the relevant shader stage enums come from
 * pipe/p_shader_tokens.h (PIPE_SHADER_*) instead.
 * This stub satisfies the #include without pulling in Mesa compiler internals.
 */
#ifndef SHADER_ENUMS_H
#define SHADER_ENUMS_H

/* No additional definitions needed for virglrenderer */

#endif /* SHADER_ENUMS_H */

