/* ------------------------------------------------------------------------- */

GLEWAPI GLenum GLEWAPIENTRY wglewInit ();
GLEWAPI GLboolean GLEWAPIENTRY wglewIsSupported (const char *name);

#ifndef WGLEW_GET_VAR
#define WGLEW_GET_VAR(x) (*(const GLboolean*)&x)
#endif

#ifndef WGLEW_GET_FUN
#define WGLEW_GET_FUN(x) x
#endif

GLEWAPI GLboolean GLEWAPIENTRY wglewGetExtension (const char *name);

#ifdef __cplusplus
}
#endif

#undef GLEWAPI

#endif /* __wglew_h__ */
