    }
    ret = (len == 0);
  }
  return ret;
}

#elif defined(GLEW_EGL)

GLboolean eglewIsSupported (const char* name)
{
  const GLubyte* pos = (const GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if(_glewStrSame1(&pos, &len, (const GLubyte*)"EGL_", 4))
    {
