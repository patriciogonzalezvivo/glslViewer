/* ------------------------------------------------------------------------ */

const GLubyte * GLEWAPIENTRY glewGetErrorString (GLenum error)
{
  static const GLubyte* _glewErrorString[] =
  {
    (const GLubyte*)"No error",
    (const GLubyte*)"Missing GL version",
    (const GLubyte*)"GL 1.1 and up are not supported",
    (const GLubyte*)"GLX 1.2 and up are not supported",
    (const GLubyte*)"Unknown error"
  };
  const size_t max_error = sizeof(_glewErrorString)/sizeof(*_glewErrorString) - 1;
  return _glewErrorString[(size_t)error > max_error ? max_error : (size_t)error];
}

const GLubyte * GLEWAPIENTRY glewGetString (GLenum name)
{
  static const GLubyte* _glewString[] =
  {
    (const GLubyte*)NULL,
    (const GLubyte*)"GLEW_VERSION_STRING",
    (const GLubyte*)"GLEW_VERSION_MAJOR_STRING",
    (const GLubyte*)"GLEW_VERSION_MINOR_STRING",
    (const GLubyte*)"GLEW_VERSION_MICRO_STRING"
  };
  const size_t max_string = sizeof(_glewString)/sizeof(*_glewString) - 1;
  return _glewString[(size_t)name > max_string ? 0 : (size_t)name];
}

/* ------------------------------------------------------------------------ */

GLboolean glewExperimental = GL_FALSE;

GLenum GLEWAPIENTRY glewInit (void)
{
  GLenum r;
#if defined(GLEW_EGL)
  PFNEGLGETCURRENTDISPLAYPROC getCurrentDisplay = NULL;
#endif
  r = glewContextInit();
  if ( r != 0 ) return r;
#if defined(GLEW_EGL)
  getCurrentDisplay = (PFNEGLGETCURRENTDISPLAYPROC) glewGetProcAddress("eglGetCurrentDisplay");
  return eglewInit(getCurrentDisplay());
#elif defined(GLEW_OSMESA) || defined(__ANDROID__) || defined(__native_client__) || defined(__HAIKU__)
  return r;
#elif defined(_WIN32)
  return wglewInit();
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX) /* _UNIX */
  return glxewInit();
#else
  return r;
#endif /* _WIN32 */
}

#if defined(_WIN32) && defined(GLEW_BUILD) && defined(__GNUC__)
/* GCC requires a DLL entry point even without any standard library included. */
/* Types extracted from windows.h to avoid polluting the rest of the file. */
int __stdcall DllMainCRTStartup(void* instance, unsigned reason, void* reserved)
{
  (void) instance;
  (void) reason;
  (void) reserved;
  return 1;
}
#endif
