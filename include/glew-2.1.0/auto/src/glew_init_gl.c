/* ------------------------------------------------------------------------- */

static int _glewExtensionCompare(const char *s1, const char *s2)
{
  /* http://www.chanduthedev.com/2012/07/strcmp-implementation-in-c.html */
  while (*s1 || *s2)
  {
      if (*s1 > *s2)
          return 1;
      if (*s1 < *s2)
          return -1;
      s1++;
      s2++;
  }
  return 0;
}

static ptrdiff_t _glewBsearchExtension(const char* name)
{
  ptrdiff_t lo = 0, hi = sizeof(_glewExtensionLookup) / sizeof(char*) - 2;

  while (lo <= hi)
  {
    ptrdiff_t mid = (lo + hi) / 2;
    const int cmp = _glewExtensionCompare(name, _glewExtensionLookup[mid]);
    if (cmp < 0) hi = mid - 1;
    else if (cmp > 0) lo = mid + 1;
    else return mid;
  }
  return -1;
}

static GLboolean *_glewGetExtensionString(const char *name)
{
  ptrdiff_t n = _glewBsearchExtension(name);
  if (n >= 0) return &_glewExtensionString[n];
  return NULL;
}

static GLboolean *_glewGetExtensionEnable(const char *name)
{
  ptrdiff_t n = _glewBsearchExtension(name);
  if (n >= 0) return _glewExtensionEnabled[n];
  return NULL;
}

static const char *_glewNextSpace(const char *i)
{
  const char *j = i;
  if (j)
    while (*j!=' ' && *j) ++j;
  return j;
}

static const char *_glewNextNonSpace(const char *i)
{
  const char *j = i;
  if (j)
    while (*j==' ') ++j;
  return j;
}

GLboolean GLEWAPIENTRY glewGetExtension (const char* name)
{
  GLboolean *enable = _glewGetExtensionString(name);
  if (enable)
    return *enable;
  return GL_FALSE;
}

/* ------------------------------------------------------------------------- */

typedef const GLubyte* (GLAPIENTRY * PFNGLGETSTRINGPROC) (GLenum name);
typedef void (GLAPIENTRY * PFNGLGETINTEGERVPROC) (GLenum pname, GLint *params);

static GLenum GLEWAPIENTRY glewContextInit ()
{
  PFNGLGETSTRINGPROC getString;
  const GLubyte* s;
  GLuint dot;
  GLint major, minor;
  size_t n;

  #ifdef _WIN32
  getString = glGetString;
  #else
  getString = (PFNGLGETSTRINGPROC) glewGetProcAddress((const GLubyte*)"glGetString");
  if (!getString)
    return GLEW_ERROR_NO_GL_VERSION;
  #endif

  /* query opengl version */
  s = getString(GL_VERSION);
  dot = _glewStrCLen(s, '.');
  if (dot == 0)
    return GLEW_ERROR_NO_GL_VERSION;

  major = s[dot-1]-'0';
  minor = s[dot+1]-'0';

  if (minor < 0 || minor > 9)
    minor = 0;
  if (major<0 || major>9)
    return GLEW_ERROR_NO_GL_VERSION;

  if (major == 1 && minor == 0)
  {
    return GLEW_ERROR_GL_VERSION_10_ONLY;
  }
  else
  {
    GLEW_VERSION_4_6   = ( major > 4 )                 || ( major == 4 && minor >= 6 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_5   = GLEW_VERSION_4_4   == GL_TRUE || ( major == 4 && minor >= 5 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_4   = GLEW_VERSION_4_5   == GL_TRUE || ( major == 4 && minor >= 4 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_3   = GLEW_VERSION_4_4   == GL_TRUE || ( major == 4 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_2   = GLEW_VERSION_4_3   == GL_TRUE || ( major == 4 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_1   = GLEW_VERSION_4_2   == GL_TRUE || ( major == 4 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_4_0   = GLEW_VERSION_4_1   == GL_TRUE || ( major == 4               ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_3_3   = GLEW_VERSION_4_0   == GL_TRUE || ( major == 3 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_3_2   = GLEW_VERSION_3_3   == GL_TRUE || ( major == 3 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_3_1   = GLEW_VERSION_3_2   == GL_TRUE || ( major == 3 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_3_0   = GLEW_VERSION_3_1   == GL_TRUE || ( major == 3               ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_2_1   = GLEW_VERSION_3_0   == GL_TRUE || ( major == 2 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_2_0   = GLEW_VERSION_2_1   == GL_TRUE || ( major == 2               ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_5   = GLEW_VERSION_2_0   == GL_TRUE || ( major == 1 && minor >= 5 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_4   = GLEW_VERSION_1_5   == GL_TRUE || ( major == 1 && minor >= 4 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_3   = GLEW_VERSION_1_4   == GL_TRUE || ( major == 1 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_2_1 = GLEW_VERSION_1_3   == GL_TRUE                                 ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_2   = GLEW_VERSION_1_2_1 == GL_TRUE || ( major == 1 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    GLEW_VERSION_1_1   = GLEW_VERSION_1_2   == GL_TRUE || ( major == 1 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
  }

  for (n = 0; n < sizeof(_glewExtensionString) / sizeof(_glewExtensionString[0]); ++n)
    _glewExtensionString[n] = GL_FALSE;

  if (GLEW_VERSION_3_0)
  {
    GLint n = 0;
    GLint i;
    PFNGLGETINTEGERVPROC getIntegerv;
    PFNGLGETSTRINGIPROC getStringi;
    const char *ext;
    GLboolean *enable;

    #ifdef _WIN32
    getIntegerv = glGetIntegerv;
    #else
    getIntegerv = (PFNGLGETINTEGERVPROC) glewGetProcAddress((const GLubyte*)"glGetIntegerv");
    #endif

    if (getIntegerv)
      getIntegerv(GL_NUM_EXTENSIONS, &n);

    /* glGetStringi is OpenGL 3.0 */
    getStringi = (PFNGLGETSTRINGIPROC) glewGetProcAddress((const GLubyte*)"glGetStringi");
    if (getStringi)
      for (i = 0; i<n; ++i)
      {
        ext = (const char *) getStringi(GL_EXTENSIONS, i);

        /* Based on extension string(s), glewGetExtension purposes */
        enable = _glewGetExtensionString(ext);
        if (enable)
          *enable = GL_TRUE;

        /* Based on extension string(s), experimental mode, glewIsSupported purposes */
        enable = _glewGetExtensionEnable(ext);
        if (enable)
          *enable = GL_TRUE;
      }
  }
  else
  {
    const char *extensions;
    const char *end;
    const char *i;
    const char *j;
    char ext[128];
    GLboolean *enable;

    extensions = (const char *) getString(GL_EXTENSIONS);

    if (extensions)
    {
      end = extensions + _glewStrLen((const GLubyte *) extensions);
      for (i=extensions; i<end; i = j + 1)
      {
        i = _glewNextNonSpace(i);
        j = _glewNextSpace(i);

        /* Copy extension into NUL terminated string */
        if (j-i >= (ptrdiff_t) sizeof(ext))
          continue;
        _glewStrCopy(ext, i, ' ');

        /* Based on extension string(s), glewGetExtension purposes */
        enable = _glewGetExtensionString(ext);
        if (enable)
          *enable = GL_TRUE;

        /* Based on extension string(s), experimental mode, glewIsSupported purposes */
        enable = _glewGetExtensionEnable(ext);
        if (enable)
          *enable = GL_TRUE;
      }
    }
  }
