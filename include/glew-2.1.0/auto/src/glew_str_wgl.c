    }
    ret = (len == 0);
  }
  return ret;
}

#if defined(_WIN32) && !defined(GLEW_EGL) && !defined(GLEW_OSMESA)

GLboolean GLEWAPIENTRY wglewIsSupported (const char* name)
{
  const GLubyte* pos = (const GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if (_glewStrSame1(&pos, &len, (const GLubyte*)"WGL_", 4))
    {
