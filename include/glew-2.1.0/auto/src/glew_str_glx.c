    }
    ret = (len == 0);
  }
  return ret;
}

#elif !defined(GLEW_OSMESA) && !defined(GLEW_EGL) && !defined(__ANDROID__) && !defined(__native_client__) && !defined(__HAIKU__) && !defined(__APPLE__) || defined(GLEW_APPLE_GLX)

GLboolean glxewIsSupported (const char* name)
{
  const GLubyte* pos = (const GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if(_glewStrSame1(&pos, &len, (const GLubyte*)"GLX_", 4))
    {
