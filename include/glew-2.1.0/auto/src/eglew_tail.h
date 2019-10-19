/* ------------------------------------------------------------------------ */

GLEWAPI GLenum GLEWAPIENTRY eglewInit (EGLDisplay display);
GLEWAPI GLboolean GLEWAPIENTRY eglewIsSupported (const char *name);

#define EGLEW_GET_VAR(x) (*(const GLboolean*)&x)
#define EGLEW_GET_FUN(x) x

GLEWAPI GLboolean GLEWAPIENTRY eglewGetExtension (const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __eglew_h__ */
