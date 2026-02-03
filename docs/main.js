import './wasm-loader.js';

const defaultShader = `
#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;

void main() {
    vec2 st = gl_FragCoord.xy/u_resolution.xy;
    st.x *= u_resolution.x/u_resolution.y;

    vec3 color = vec3(0.0);
    color = vec3(st.x, st.y, abs(sin(u_time)));

    gl_FragColor = vec4(color,1.0);
}
`;

document.addEventListener('DOMContentLoaded', () => {
    // Editor Setup
    const editorContainer = document.getElementById('editor-container');
    const editor = CodeMirror(editorContainer, {
        value: defaultShader,
        mode: 'x-shader/x-fragment',
        theme: 'monokai',
        lineNumbers: true,
        matchBrackets: true
    });
    editor.setSize(null, "100%");

    // Canvas focus management
    const canvas = document.getElementById('canvas');
    const wrapper = document.getElementById('wrapper');
    
    // Blur canvas when clicking on editor or console
    if (editorContainer) {
        editorContainer.addEventListener('mousedown', () => {
            if (canvas) canvas.blur();
        });
    }

    // Focus canvas when mouse is over it or wrapper
    if (wrapper && canvas) {
        wrapper.addEventListener('mouseenter', () => {
            canvas.focus();
        });
        wrapper.addEventListener('mouseleave', () => {
            canvas.blur();
        });
    }

    // Stop keyboard events from propagating to the module when in editor/console
    function stopPropagation(e) {
        e.stopPropagation();
    }

    if (editorContainer) {
        editorContainer.addEventListener('keydown', stopPropagation);
        editorContainer.addEventListener('keypress', stopPropagation);
        editorContainer.addEventListener('keyup', stopPropagation);
    }

    const consoleInput = document.getElementById('console-input');
    if (consoleInput) {
        consoleInput.addEventListener('keydown', stopPropagation);
        consoleInput.addEventListener('keypress', stopPropagation);
        consoleInput.addEventListener('keyup', stopPropagation);
    }

    // Update Shader function
    function updateShader() {
        if (window.Module && window.Module.ccall) {
            const code = editor.getValue();
            try {
                window.Module.ccall('setFrag', null, ['string'], [code]);
            } catch (e) {
                console.error("Error setting shader:", e);
            }
        }
    }

    // Wait for Module to be ready
    const checkModule = setInterval(() => {
        if (window.Module && window.module_loaded) {
            clearInterval(checkModule);
            console.log("Module loaded, sending initial shader.");
            updateShader();
        }
    }, 500);

    const btn = document.getElementById('resize-btn');
    if (!btn) return;

    if (!wrapper) return;
    
    // Always start in windowed mode regardless of device
    let isFullscreen = false;
    wrapper.classList.remove('fullscreen');
    wrapper.classList.add('windowed');
    document.body.classList.add('windowed-mode');

    // Drag and Drop files
    wrapper.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.stopPropagation();
    });

    wrapper.addEventListener('drop', (e) => {
        e.preventDefault();
        e.stopPropagation();

        if (window.Module && window.Module.ccall) {
            const files = e.dataTransfer.files;
            for (let i = 0; i < files.length; i++) {
                const file = files[i];
                const reader = new FileReader();
                
                reader.onload = (event) => {
                    const data = new Uint8Array(event.target.result);
                    const filename = file.name;
                    
                    try {
                        // Write file to Emscripten FS
                        window.Module.FS.writeFile(filename, data);
                        // Notify C++
                        window.Module.ccall('loadFile', null, ['string'], [filename]);
                        console.log("Loaded file:", filename);
                    } catch (err) {
                        console.error("Error loading file:", err);
                    }
                };
                reader.readAsArrayBuffer(file);
            }
        }
    });

    function setTranslate(xPos, yPos, el) {
        el.style.transform = "translate3d(" + xPos + "px, " + yPos + "px, 0)";
    }

    btn.addEventListener('click', () => {
        isFullscreen = !isFullscreen;

        if (isFullscreen) {
            wrapper.classList.add('fullscreen');
            wrapper.classList.remove('windowed');
            document.body.classList.remove('windowed-mode');
            
            // Reset position on fullscreen
            wrapper.style.transform = "none";
            
            // Forces update size
            setTimeout(() => {
                if (window.Module && window.Module.ccall) {
                       const w = window.innerWidth;
                       const h = window.innerHeight;
                       const canvas = document.getElementById('canvas');
                       if (canvas) {
                           canvas.width = w;
                           canvas.height = h;
                       }
                }
            }, 100);

            // Hide editor and console
            if (editorContainer) editorContainer.style.display = 'none';
            if (consoleOutput && consoleOutput.parentElement) 
                consoleOutput.parentElement.style.display = 'none';

        } else {
            wrapper.classList.remove('fullscreen');
            wrapper.classList.add('windowed');
            document.body.classList.add('windowed-mode');

            // Show editor and console
            if (editorContainer) editorContainer.style.display = 'block';
            if (consoleOutput && consoleOutput.parentElement) 
                consoleOutput.parentElement.style.display = 'flex';
        }
    });

    // Use ResizeObserver to handle the CSS transition smoothly
    const resizeObserver = new ResizeObserver(() => {
        window.dispatchEvent(new Event('resize'));
        
        // Update C++ window size
        if (window.Module && window.Module.ccall) {
            const canvas = document.getElementById('canvas');
            if (canvas) {
                const rect = canvas.getBoundingClientRect();
                
                // Explicitly set the canvas buffer size to match the displayed size
                // This ensures the viewport has the correct physical pixels to work with
                canvas.width = rect.width;
                canvas.height = rect.height;
            }
        }
    });
    resizeObserver.observe(wrapper);

    // --- Console & Error Handling ---
    const consoleOutput = document.getElementById('console-output');
    // consoleInput is already declared above
    
    function logToConsole(text, isError = false) {
        if (!consoleOutput) return;
        const msg = document.createElement('div');
        msg.textContent = text;
        if (isError) msg.style.color = '#ff5555';
        consoleOutput.appendChild(msg);
        consoleOutput.scrollTop = consoleOutput.scrollHeight;
    }

    // Capture standard output
    window.addEventListener('wasm-stdout', (e) => {
        logToConsole(e.detail, false);
    });

    // Capture standard error and parse for GLSL errors
    window.addEventListener('wasm-stderr', (e) => {
        const text = e.detail;
        logToConsole(text, true);

        // Regex to match GLSL errors: "0:LINE: ERROR: ..."
        // Example: 0:13: S0001: Type mismatch, cannot convert from 'int' to 'float'
        const errorRegex = /^0:(\d+):(.*)$/;
        const match = text.match(errorRegex);
        
        if (match) {
            const line = parseInt(match[1], 10);
            const message = match[2];
            
            // Mark the error in CodeMirror
            // CodeMirror lines are 0-based, GLSL usually 1-based
            const cmLine = line - 1; 
            
            if (editor) {
               // Verify line exists
               if (cmLine >= 0 && cmLine < editor.lineCount()) {
                   // Add a class to highlighting the line
                   editor.addLineClass(cmLine, 'background', 'error-line');
                   
                   // Optionally add a widget or tooltip. For now, let's just highlight.
                   // We'll clear errors on next change.
               }
            }
        }
    });

    // Clear errors on change
    editor.on('change', () => {
        // Remove all error line classes
        editor.eachLine((lineHandle) => {
             editor.removeLineClass(lineHandle, 'background', 'error-line');
        });
        updateShader();
    });

    // Handle character deletion issue by ensuring input isn't blocked by other events
    // (Usually CodeMirror just works, if deletion fails check if something is capturing keydown)
    
    // Console Input
    const consoleDiv = document.getElementById('console');
    if (consoleInput) {
        // Blur canvas when clicking on console
        consoleDiv.addEventListener('mousedown', () => {
            if (canvas) canvas.blur();
        });

        consoleInput.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
                const cmd = consoleInput.value.trim();
                if (cmd) {
                    logToConsole('> ' + cmd);
                    if (window.Module && window.Module.ccall) {
                        try {
                            window.Module.ccall('command', null, ['string'], [cmd]);
                        } catch(err) {
                            logToConsole('Error sending command: ' + err, true);
                        }
                    } else {
                        logToConsole('Module not ready.', true);
                    }
                    consoleInput.value = '';
                }
            }
        });
    }

});
