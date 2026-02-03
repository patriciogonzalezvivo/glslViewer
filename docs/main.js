import './wasm-loader.js';

const defaultFragment = `#ifdef GL_ES
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

const defaultVertex = `#ifdef GL_ES
precision mediump float;
#endif

attribute vec4 a_position;
varying vec4 v_position;

void main() {
    v_position = a_position;
    gl_Position = a_position;
}
`;

function getJSON(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function() {
      var status = xhr.status;
      if (status === 200) {
        callback(null, xhr.response);
      } else {
        callback(status, xhr.response);
      }
    };
    xhr.send();
};

document.addEventListener('DOMContentLoaded', () => {
    // State
    let activeTab = 'frag';
    let content = {
        frag: defaultFragment,
        vert: defaultVertex
    };
    
    // DeBounce timeout
    let updateTimeout = null;

    // Editor Setup
    const editorContainer = document.getElementById('editor-container');
    const editor = CodeMirror(editorContainer, {
        value: content.frag,
        mode: 'x-shader/x-fragment',
        theme: 'monokai',
        lineNumbers: true,
        matchBrackets: true,
        keyMap: 'sublime',
        extraKeys: {
            "Cmd-/": "toggleComment",
            "Ctrl-/": "toggleComment"
        }
    });
    editor.setSize(null, "100%");

    // Lygia Autocomplete
    let lygia_glob = null;
    let lygia_fetching = false;
    editor.on('inputRead', (cm, change) => {
        let cur = cm.getCursor();
        let line = cm.getLine(cur.line);
        let trimmedLine = line.trim();
          
        if (trimmedLine.startsWith('#include')) {
            let path = line.substring(10);
            if (lygia_glob === null) {
            getJSON('https://lygia.xyz/glsl.json', (err, data) => {
                if (err === null) {
                lygia_glob = data;
                }
            });
            }
            console.log('autocomplete for', path);

            let result = []

            if (lygia_glob !== null) {
                lygia_glob.forEach((w) => {
                    if (w.startsWith(path)) {
                        result.push('#include "' + w + '"');
                    }
                });
                result.sort();
            }

            if (result.length > 0) {
                CodeMirror.showHint(cm, () => {
                    let start = line.indexOf('#include');
                    let end = cur.ch;
                    if (line.length > end && line[end] === '"') {
                        end++;
                    }

                    let rta = {
                        list: result, 
                        from: CodeMirror.Pos(cur.line, start),
                        to: CodeMirror.Pos(cur.line, end)
                    };
                    
                    console.log(rta);
                    return rta;
                }, {completeSingle: true, alignWithWord: true});
            }
        }
    });

    // Tabs logic
    const tabFrag = document.querySelector('.tab[data-type="frag"]');
    const tabVert = document.querySelector('.tab[data-type="vert"]');

    function switchTab(type) {
        if (type === activeTab) return;
        
        // Flush pending updates
        if (updateTimeout) {
            clearTimeout(updateTimeout);
            updateShader();
        }

        // Save current content
        content[activeTab] = editor.getValue();
        
        // Switch state
        activeTab = type;
        
        // Update UI
        if (type === 'frag') {
            if (tabFrag) tabFrag.classList.add('active');
            if (tabVert) tabVert.classList.remove('active');
            editor.setOption('mode', 'x-shader/x-fragment');
        } else {
            if (tabFrag) tabFrag.classList.remove('active');
            if (tabVert) tabVert.classList.add('active');
            editor.setOption('mode', 'x-shader/x-vertex');
        }
        
        // Set new content
        editor.setValue(content[activeTab]);
    }

    if (tabFrag) {
        tabFrag.addEventListener('click', () => switchTab('frag'));
    }
    if (tabVert) {
        tabVert.style.display = 'inline-block';
        tabVert.addEventListener('click', () => switchTab('vert'));
    }

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
            if (document.activeElement === canvas) {
                canvas.blur();
            }
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
            // Update stored content
            content[activeTab] = code;

            try {
                if (activeTab === 'frag') {
                    window.Module.ccall('setFrag', null, ['string'], [code]);
                } else if (activeTab === 'vert') {
                    window.Module.ccall('setVert', null, ['string'], [code]);
                }
            } catch (e) {
                console.error("Error setting shader:", e);
            }
        }
    }

    // Fetch Shaders from Backend
    function fetchShadersFromBackend() {
        if (window.Module && window.Module.ccall) {
            try {
                 const cFrag = window.Module.ccall('getDefaultSceneFrag', 'string', [], []);
                 const cVert = window.Module.ccall('getDefaultSceneVert', 'string', [], []);

                 if (cFrag && cFrag.length > 0) content.frag = cFrag;
                 if (cVert && cVert.length > 0) content.vert = cVert;
                 
                 if (editor.getValue() !== content[activeTab]) {
                    editor.setValue(content[activeTab]);
                 }
            } catch(e) {
                 console.log("Could not fetch shaders: " + e);
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
    if (btn && wrapper) {
        // Always start in windowed mode regardless of device
        let isFullscreen = false;
        wrapper.classList.remove('fullscreen');
        wrapper.classList.add('windowed');
        document.body.classList.add('windowed-mode');

        btn.addEventListener('click', () => {
            isFullscreen = !isFullscreen;

            if (isFullscreen) {
                wrapper.classList.add('fullscreen');
                wrapper.classList.remove('windowed');
                document.body.classList.remove('windowed-mode');
                wrapper.style.transform = "none";
                setTimeout(() => {
                    if (window.Module && window.Module.ccall) {
                        const w = window.innerWidth;
                        const h = window.innerHeight;
                        const cnv = document.getElementById('canvas');
                        if (cnv) { cnv.width = w; cnv.height = h; }
                    }
                }, 100);
                if (editorContainer) editorContainer.style.display = 'none';
                if (consoleOutput && consoleOutput.parentElement) 
                    consoleOutput.parentElement.style.display = 'none';
            } else {
                wrapper.classList.remove('fullscreen');
                wrapper.classList.add('windowed');
                document.body.classList.add('windowed-mode');
                if (editorContainer) editorContainer.style.display = 'block';
                if (consoleOutput && consoleOutput.parentElement) 
                    consoleOutput.parentElement.style.display = 'flex';
            }
        });
    }

    // Use ResizeObserver to handle the CSS transition smoothly
    if (wrapper) {
        const resizeObserver = new ResizeObserver(() => {
            window.dispatchEvent(new Event('resize'));
            if (window.Module && window.Module.ccall) {
                const cnv = document.getElementById('canvas');
                if (cnv) {
                    const rect = cnv.getBoundingClientRect();
                    cnv.width = rect.width;
                    cnv.height = rect.height;
                }
            }
        });
        resizeObserver.observe(wrapper);
    }

    // --- Console & Error Handling ---
    const consoleOutput = document.getElementById('console-output');
    function logToConsole(text, isError = false) {
        if (!consoleOutput) return;
        const msg = document.createElement('div');
        msg.textContent = text;
        if (isError) msg.style.color = '#ff5555';
        consoleOutput.appendChild(msg);
        consoleOutput.scrollTop = consoleOutput.scrollHeight;
    }

    window.addEventListener('wasm-stdout', (e) => {
        logToConsole(e.detail, false);
    });

    window.addEventListener('wasm-stderr', (e) => {
        const text = e.detail;
        logToConsole(text, true);

        // Regex to match GLSL errors
        const errorRegex = /^0:(\d+):(.*)$/;
        const match = text.match(errorRegex);
        
        if (match) {
            const line = parseInt(match[1], 10);
            const cmLine = line - 1; 
            
            if (editor) {
               if (cmLine >= 0 && cmLine < editor.lineCount()) {
                   editor.addLineClass(cmLine, 'background', 'error-line');
               }
            }
        }
    });

    editor.on('change', () => {
        editor.eachLine((lineHandle) => {
             editor.removeLineClass(lineHandle, 'background', 'error-line');
        });

        // Clear existing timeout
        if (updateTimeout) clearTimeout(updateTimeout);

        // Set new timeout using stored content to detect changes
        updateTimeout = setTimeout(() => {
            const currentCode = editor.getValue();
            if (currentCode !== content[activeTab]) {
                updateShader();
            }
            updateTimeout = null;
        }, 300);
    });

    // Console Input
    const consoleDiv = document.getElementById('console');
    if (consoleDiv && consoleInput) {
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
    
    // --- File Drag & Drop ---
    function handleDrop(e) {
        e.preventDefault();
        e.stopPropagation();
        
        const files = e.dataTransfer.files;
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            const name = file.name;
            const ext = name.split('.').pop().toLowerCase();
            
            if (ext === 'frag' || ext === 'fs' || ext === 'vert' || ext === 'vs') {
                const reader = new FileReader();
                reader.onload = (event) => {
                    const data = event.target.result;
                    if (ext === 'frag' || ext === 'fs') {
                        content.frag = data;
                        if (activeTab === 'frag') {
                            editor.setValue(data);
                        } else {
                             // Push update without Editor change
                             if (window.Module && window.Module.ccall) {
                                window.Module.ccall('setFrag', null, ['string'], [data]);
                             }
                        }
                    } else {
                        content.vert = data;
                        if (activeTab === 'vert') {
                            editor.setValue(data);
                        } else {
                             // Push update without Editor change
                             if (window.Module && window.Module.ccall) {
                                window.Module.ccall('setVert', null, ['string'], [data]);
                             }
                        }
                    }
                };
                reader.readAsText(file);
            } else {
                // Binary or other assets
                const reader = new FileReader();
                reader.onload = (event) => {
                    const data = new Uint8Array(event.target.result);
                    if (window.Module && window.Module.FS) {
                        try {
                            window.Module.FS.writeFile(name, data);
                            logToConsole("Loaded file: " + name);

                            // Files like ply, obj, gltf might reset shaders. Fetch them.

                            if (ext === 'ply' || ext === 'obj' || ext === 'gltf' || ext === 'glb' || ext === 'splat') {
                                // Small delay to ensure file is processed
                                setTimeout(() => {
                                    fetchShadersFromBackend();
                                }, 100);
                            }

                        } catch (err) {
                            logToConsole("Error saving file: " + err, true);
                        }
                    }
                };
                reader.readAsArrayBuffer(file);
            }
        }
    }
    
    document.body.addEventListener('dragover', (e) => { e.preventDefault(); e.stopPropagation(); });
    document.body.addEventListener('drop', handleDrop);
});
