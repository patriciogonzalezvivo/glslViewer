#ifndef GLEW_INCLUDE
#include <GL/glew.h>
#else
#include GLEW_INCLUDE
#endif

#if defined(GLEW_OSMESA)
#  define GLAPI extern
#  include <GL/osmesa.h>
#elif defined(GLEW_EGL)
#  include <GL/eglew.h>
#elif defined(_WIN32)
/*
 * If NOGDI is defined, wingdi.h won't be included by windows.h, and thus
 * wglGetProcAddress won't be declared. It will instead be implicitly declared,
 * potentially incorrectly, which we don't want.
 */
#  if defined(NOGDI)
#    undef NOGDI
#  endif
#  include <GL/wglew.h>
#elif !defined(__ANDROID__) && !defined(__native_client__) && !defined(__HAIKU__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX))
#  include <GL/glxew.h>
#endif

#include <stddef.h>  /* For size_t */

#if defined(GLEW_EGL)
#elif defined(GLEW_REGAL)

/* In GLEW_REGAL mode we call direcly into the linked
   libRegal.so glGetProcAddressREGAL for looking up
   the GL function pointers. */

#  undef glGetProcAddressREGAL
#  ifdef WIN32
extern void *  __stdcall glGetProcAddressREGAL(const GLchar *name);
static void * (__stdcall * regalGetProcAddress) (const GLchar *) = glGetProcAddressREGAL;
#    else
extern void * glGetProcAddressREGAL(const GLchar *name);
static void * (*regalGetProcAddress) (const GLchar *) = glGetProcAddressREGAL;
#  endif
#  define glGetProcAddressREGAL GLEW_GET_FUN(__glewGetProcAddressREGAL)

#elif defined(__sgi) || defined (__sun) || defined(__HAIKU__) || defined(GLEW_APPLE_GLX)
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void* dlGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun || GLEW_APPLE_GLX */

#if defined(__APPLE__)
#include <stdlib.h>
#include <string.h>
#include <AvailabilityMacros.h>

#ifdef MAC_OS_X_VERSION_10_3

#include <dlfcn.h>

void* NSGLGetProcAddress (const GLubyte *name)
{
  static void* image = NULL;
  void* addr;
  if (NULL == image)
  {
    image = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);
  }
  if( !image ) return NULL;
  addr = dlsym(image, (const char*)name);
  if( addr ) return addr;
#ifdef GLEW_APPLE_GLX
  return dlGetProcAddress( name ); // try next for glx symbols
#else
  return NULL;
#endif
}
#else

#include <mach-o/dyld.h>

void* NSGLGetProcAddress (const GLubyte *name)
{
  static const struct mach_header* image = NULL;
  NSSymbol symbol;
  char* symbolName;
  if (NULL == image)
  {
    image = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", NSADDIMAGE_OPTION_RETURN_ON_ERROR);
  }
  /* prepend a '_' for the Unix C symbol mangling convention */
  symbolName = malloc(strlen((const char*)name) + 2);
  strcpy(symbolName+1, (const char*)name);
  symbolName[0] = '_';
  symbol = NULL;
  /* if (NSIsSymbolNameDefined(symbolName))
	 symbol = NSLookupAndBindSymbol(symbolName); */
  symbol = image ? NSLookupSymbolInImage(image, symbolName, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR) : NULL;
  free(symbolName);
  if( symbol ) return NSAddressOfSymbol(symbol);
#ifdef GLEW_APPLE_GLX
  return dlGetProcAddress( name ); // try next for glx symbols
#else
  return NULL;
#endif
}
#endif /* MAC_OS_X_VERSION_10_3 */
#endif /* __APPLE__ */

/*
 * Define glewGetProcAddress.
 */
#if defined(GLEW_REGAL)
#  define glewGetProcAddress(name) regalGetProcAddress((const GLchar *)name)
#elif defined(GLEW_OSMESA)
#  define glewGetProcAddress(name) OSMesaGetProcAddress((const char *)name)
#elif defined(GLEW_EGL)
#  define glewGetProcAddress(name) eglGetProcAddress((const char *)name)
#elif defined(_WIN32)
#  define glewGetProcAddress(name) wglGetProcAddress((LPCSTR)name)
#elif defined(__APPLE__) && !defined(GLEW_APPLE_GLX)
#  define glewGetProcAddress(name) NSGLGetProcAddress(name)
#elif defined(__sgi) || defined(__sun) || defined(__HAIKU__)
#  define glewGetProcAddress(name) dlGetProcAddress(name)
#elif defined(__ANDROID__)
#  define glewGetProcAddress(name) NULL /* TODO */
#elif defined(__native_client__)
#  define glewGetProcAddress(name) NULL /* TODO */
#else /* __linux */
#  define glewGetProcAddress(name) (*glXGetProcAddressARB)(name)
#endif

/*
 * Redefine GLEW_GET_VAR etc without const cast
 */

#undef GLEW_GET_VAR
# define GLEW_GET_VAR(x) (x)

#ifdef WGLEW_GET_VAR
# undef WGLEW_GET_VAR
# define WGLEW_GET_VAR(x) (x)
#endif /* WGLEW_GET_VAR */

#ifdef GLXEW_GET_VAR
# undef GLXEW_GET_VAR
# define GLXEW_GET_VAR(x) (x)
#endif /* GLXEW_GET_VAR */

#ifdef EGLEW_GET_VAR
# undef EGLEW_GET_VAR
# define EGLEW_GET_VAR(x) (x)
#endif /* EGLEW_GET_VAR */

/*
 * GLEW, just like OpenGL or GLU, does not rely on the standard C library.
 * These functions implement the functionality required in this file.
 */

static GLuint _glewStrLen (const GLubyte* s)
{
  GLuint i=0;
  if (s == NULL) return 0;
  while (s[i] != '\0') i++;
  return i;
}

static GLuint _glewStrCLen (const GLubyte* s, GLubyte c)
{
  GLuint i=0;
  if (s == NULL) return 0;
  while (s[i] != '\0' && s[i] != c) i++;
  return i;
}

static GLuint _glewStrCopy(char *d, const char *s, char c)
{
  GLuint i=0;
  if (s == NULL) return 0;
  while (s[i] != '\0' && s[i] != c) { d[i] = s[i]; i++; }
  d[i] = '\0';
  return i;
}

#if !defined(GLEW_OSMESA)
#if !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
static GLboolean _glewStrSame (const GLubyte* a, const GLubyte* b, GLuint n)
{
  GLuint i=0;
  if(a == NULL || b == NULL)
    return (a == NULL && b == NULL && n == 0) ? GL_TRUE : GL_FALSE;
  while (i < n && a[i] != '\0' && b[i] != '\0' && a[i] == b[i]) i++;
  return i == n ? GL_TRUE : GL_FALSE;
}
#endif
#endif

static GLboolean _glewStrSame1 (const GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  while (*na > 0 && (**a == ' ' || **a == '\n' || **a == '\r' || **a == '\t'))
  {
    (*a)++;
    (*na)--;
  }
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if(i == nb)
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

static GLboolean _glewStrSame2 (const GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if(i == nb)
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

static GLboolean _glewStrSame3 (const GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if (i == nb && (*na == nb || (*a)[i] == ' ' || (*a)[i] == '\n' || (*a)[i] == '\r' || (*a)[i] == '\t'))
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

/*
 * Search for name in the extensions string. Use of strstr()
 * is not sufficient because extension names can be prefixes of
 * other extension names. Could use strtok() but the constant
 * string returned by glGetString might be in read-only memory.
 */
#if !defined(GLEW_OSMESA)
#if !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
static GLboolean _glewSearchExtension (const char* name, const GLubyte *start, const GLubyte *end)
{
  const GLubyte* p;
  GLuint len = _glewStrLen((const GLubyte*)name);
  p = start;
  while (p < end)
  {
    GLuint n = _glewStrCLen(p, ' ');
    if (len == n && _glewStrSame((const GLubyte*)name, p, n)) return GL_TRUE;
    p += n+1;
  }
  return GL_FALSE;
}
#endif
#endif
