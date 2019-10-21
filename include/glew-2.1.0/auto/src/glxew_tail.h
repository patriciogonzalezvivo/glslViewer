/* ------------------------------------------------------------------------ */

GLEWAPI GLenum GLEWAPIENTRY glxewInit ();
GLEWAPI GLboolean GLEWAPIENTRY glxewIsSupported (const char *name);

#ifndef GLXEW_GET_VAR
#define GLXEW_GET_VAR(x) (*(const GLboolean*)&x)
#endif

#ifndef GLXEW_GET_FUN
#define GLXEW_GET_FUN(x) x
#endif

GLEWAPI GLboolean GLEWAPIENTRY glxewGetExtension (const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __glxew_h__ */
