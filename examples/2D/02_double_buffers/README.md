Each time you add a `uniform sampler2D u_doubleBuffer[NUMBER]`, glslViewer will run another version of the same shader code but with the `#define DOUBLE_BUFFER_[NUMBER]` on top and save the content into `u_doubleBuffer[NUMBER]` texture buffer. This way you can code different buffers with a single source file.

![](https://github.com/patriciogonzalezvivo/glslViewer/blob/main/.github/images/buffers.gif)
