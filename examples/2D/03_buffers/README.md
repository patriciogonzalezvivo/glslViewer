Each time you add a `uniform sampler2D u_buffer[NUMBER]`, glslViewer will run another version of the same shader code but with the `#define BUFFER_[NUMBER]` on top and save the content into `u_buffer[NUMBER]` texture buffer. This way you can code different buffers with a single source file.

![](https://github.com/patriciogonzalezvivo/glslViewer/blob/main/.github/images/control.gif)