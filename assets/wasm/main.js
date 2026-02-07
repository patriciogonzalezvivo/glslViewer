import './wasm-loader.js';

const defaultFragment = `#ifdef GL_ES
precision mediump float;
#endif

uniform float   	u_time;
uniform vec2    	u_resolution;

void main() {
    vec4 color = vec4(vec3(0.0), 1.0);
    vec2 pixel = 1.0/u_resolution;
    vec2 st = gl_FragCoord.xy * pixel;

    color.rgb = vec3(st.x, st.y, abs(sin(u_time)));

    gl_FragColor = color;
}
`;

const defaultVertex = `#ifdef GL_ES
precision mediump float;
#endif

attribute vec4  a_position;
varying vec4    v_position;

void main() {
    v_position = a_position;
    gl_Position = a_position;
}
`;

// const commandsToRetainState = 

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
    let externalAssets = {};
    const loader = document.getElementById('loader');
    let loaderCount = 0;
    const showLoader = () => {
        loaderCount++;
        if (loader && loaderCount > 0) loader.classList.add('visible');
    };
    const hideLoader = () => {
        loaderCount--;
        if (loader && loaderCount <= 0) {
            loader.classList.remove('visible');
            loaderCount = 0;
        }
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
        tabSize: 4,
        indentUnit: 4,
        extraKeys: {
            "Cmd-/": "toggleComment",
            "Ctrl-/": "toggleComment",
            "Alt-Up": "swapLineUp",
            "Alt-Down": "swapLineDown"
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

    // send command
    const getFullscreen = function() {
        return wrapper && wrapper.classList.contains('fullscreen');
    }
    const setFullscreen = function(isFullscreen) {
        if (wrapper) {
            if (isFullscreen) {
                wrapper.classList.add('fullscreen');
                wrapper.classList.remove('windowed');
                document.body.classList.remove('windowed-mode');
                wrapper.style.transform = "none";
                if (editorContainer) editorContainer.style.display = 'none';
                const consoleOutput = document.getElementById('console-output');
                if (consoleOutput && consoleOutput.parentElement) 
                    consoleOutput.parentElement.style.display = 'none';
            } else {
                wrapper.classList.remove('fullscreen');
                wrapper.classList.add('windowed');
                document.body.classList.add('windowed-mode');
                if (editorContainer) editorContainer.style.display = 'block';
                const consoleOutput = document.getElementById('console-output');
                if (consoleOutput && consoleOutput.parentElement) 
                    consoleOutput.parentElement.style.display = 'flex';
            }
        }
    };

    const sendCommand = function(cmd) {
        logToConsole('> ' + cmd);
        if (cmd === 'fullscreen,on') {
            setFullscreen(true);
            return 'on';
        } else if (cmd === 'fullscreen,off') {
            setFullscreen(false);
            return 'off';
        }
        else if (cmd === 'fullscreen,toggle') {
            setFullscreen(!getFullscreen());
            return getFullscreen() ? 'on' : 'off';
        }
        else if (cmd === 'fullscreen') {
            let state = getFullscreen() ? 'on' : 'off';
            logToConsole(state);
            return state;
        }
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('command', null, ['string'], [cmd]);
            } catch(err) {
                logToConsole('Error sending command: ' + err, true);
            }
        } else {
            logToConsole('Module not ready.', true);
        }
    };

    const setFrag = function(code) {
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('setFrag', null, ['string'], [code]);
            } catch (e) {
                console.error("Error setting fragment shader:", e);
            }
        }
    };

    const setVert = function(code) {
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('setVert', null, ['string'], [code]);
            } catch (e) {
                console.error("Error setting vertex shader:", e);
            }
        }
    };

    // Update Shader function
    function updateShader() {
        const code = editor.getValue();
        // Update stored content
        content[activeTab] = code;

        if (activeTab === 'frag') {
            setFrag(code);
        } else if (activeTab === 'vert') {
            setVert(code);
        }
    }

    // Fetch Shaders from Backend
    const fetchShadersFromBackend = function() {
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

            const gistId = getQueryVariable('gist');
            if (gistId) {
                loadGist(gistId);
            } else {
                updateShader();
            }
        }
    }, 500);

    const btn = document.getElementById('resize-btn');
    if (btn && wrapper) {
        wrapper.classList.remove('fullscreen');
        wrapper.classList.add('windowed');
        document.body.classList.add('windowed-mode');

        btn.addEventListener('click', () => {
            setFullscreen(!getFullscreen());
        });
    }

    // Use ResizeObserver to handle the CSS transition smoothly
    if (wrapper) {
        const resizeObserver = new ResizeObserver(() => {
            window.dispatchEvent(new Event('resize'));
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
                    sendCommand(cmd);
                    consoleInput.value = '';
                }
            }
        });
    }
    
    // --- GitHub Integration ---
    function getQueryVariable(variable) {
        var query = window.location.search.substring(1);
        var vars = query.split('&');
        for (var i = 0; i < vars.length; i++) {
            var pair = vars[i].split('=');
            if (decodeURIComponent(pair[0]) == variable) {
                return decodeURIComponent(pair[1]);
            }
        }
        return null;
    }

    const loginBtn = document.getElementById('login-btn');
    const saveBtn = document.getElementById('save-btn');
    const openBtn = document.getElementById('open-btn');
    let githubToken = localStorage.getItem('github_token');
    let githubUser = null;

    function updateGithubUI() {
        if (githubUser) {
            loginBtn.textContent = 'Log out (' + githubUser + ')';
            saveBtn.style.display = 'block';
        } else {
            loginBtn.textContent = 'Login';
            saveBtn.style.display = 'none';
        }
    }

    function checkGithubToken() {
        if (githubToken) {
            fetch('https://api.github.com/user', {
                headers: { 'Authorization': 'token ' + githubToken }
            })
            .then(response => {
                if (response.ok) return response.json();
                throw new Error('Invalid token');
            })
            .then(data => {
                githubUser = data.login;
                updateGithubUI();
            })
            .catch(err => {
                console.warn('GitHub token invalid:', err);
                logoutGithub();
            });
        } else {
            updateGithubUI();
        }
    }

    function loginGithub() {
        if (githubUser) {
            // Logout
            logoutGithub();
        } else {
            const token = prompt('Please enter your GitHub Personal Access Token:');
            if (token) {
                githubToken = token;
                localStorage.setItem('github_token', token);
                checkGithubToken();
            }
        }
    }

    function logoutGithub() {
        githubToken = null;
        githubUser = null;
        localStorage.removeItem('github_token');
        updateGithubUI();
    }

    function openGist() {
        const id = prompt('Please enter Gist ID or URL:');
        if (id) {
            // Extract ID if full URL
            const gistId = id.split('/').pop();
            window.location.search = '?gist=' + gistId;
        }
    }

    let currentGistId = null;

    const decodeBase64 = (dataUrl) => {
        const base64 = dataUrl.split(',')[1];
        const binaryString = window.atob(base64);
        const len = binaryString.length;
        const data = new Uint8Array(len);
        for (let k = 0; k < len; k++) {
            data[k] = binaryString.charCodeAt(k);
        }
        return data;
    };

    const downloadAsset = (url) => {
        return fetch(url)
            .then(res => {
                if (!res.ok) throw new Error(res.statusText);
                return res.arrayBuffer();
            })
            .then(buffer => new Uint8Array(buffer));
    };

    const loadToWasm = (name, data) => {
        showLoader();
        return new Promise((resolve) => {
            const tryWrite = () => {
                if (window.Module && window.Module.FS && window.module_loaded) {
                    try {
                        window.Module.FS.writeFile(name, data);
                        logToConsole("Loaded asset: " + name);
                        
                        const ext = name.split('.').pop().toLowerCase();
                        window.Module.ccall('loadAsset', null, ['string', 'string'], [name, ext]);
                        if (['ply', 'obj', 'gltf', 'glb', 'splat'].includes(ext)) {
                            sendCommand('sky,on');
                        }

                        hideLoader();
                        resolve();
                    } catch (e) {
                        console.error("FS error", e);
                        setTimeout(tryWrite, 500);
                    }
                } else {
                    setTimeout(tryWrite, 500);
                }
            };
            tryWrite();
        });
    };

    function loadGist(id) {
        if (currentGistId === id) {
            console.log('Gist ' + id + ' already loading/loaded.');
            return;
        }
        currentGistId = id;
        showLoader();
        
        console.log('Loading Gist:', id);
        fetch('https://api.github.com/gists/' + id)
        .then(response => response.json())
        .then(data => {
            // Look for shader.json
            let shaderFile = null;
            if (data.files['shader.json']) {
                shaderFile = data.files['shader.json'];
            } else {
                // Fallback: look for first .json file
                const names = Object.keys(data.files);
                for (let name of names) {
                    if (name.endsWith('.json')) {
                        shaderFile = data.files[name];
                        break;
                    }
                }
            }

            const processShader = (jsonContent) => {
                try {
                    const json = JSON.parse(jsonContent);
                    
                    if (json.frag) content.frag = json.frag;
                    if (json.vert) content.vert = json.vert;

                    let assetPromises = [];

                    if (json.assets) {
                        externalAssets = json.assets;
                        // console.log('Gist assets:', externalAssets);
                        for (const [name, url] of Object.entries(externalAssets)) {
                            let p;
                            if (url.startsWith('data:')) {
                                try {
                                    const data = decodeBase64(url);
                                    p = loadToWasm(name, data);
                                } catch (e) {
                                    console.error("Error decoding base64", e);
                                    p = Promise.resolve();
                                }
                            } else {
                                showLoader();
                                p = downloadAsset(url)
                                .then(data => {
                                    hideLoader();
                                    return loadToWasm(name, data);
                                })
                                .catch(err => {
                                    hideLoader();
                                    logToConsole('Error loading asset ' + name + ': ' + err.message, true);
                                });
                            }
                            assetPromises.push(p);
                        }
                    } else {
                        externalAssets = {};
                    }
                    
                    editor.setValue(content[activeTab]);
                    updateShader();

                    Promise.all(assetPromises).then(() => {
                        updateShader();
                        
                        if (json.commands && Array.isArray(json.commands)) {
                             json.commands.forEach((cmd) => {
                                sendCommand(cmd);
                             });
                        }
                        
                        console.log('Gist loaded successfully');
                    });
                } catch (e) {
                    currentGistId = null; 
                    console.error('Error parsing Gist content:', e);
                    logToConsole('Error parsing Gist JSON', true);
                }
            };

            if (shaderFile) {
                if (shaderFile.truncated) {
                    fetch(shaderFile.raw_url)
                    .then(r => r.text())
                    .then(text => {
                        processShader(text);
                        hideLoader();
                    })
                    .catch(e => {
                        logToConsole('Error fetching raw gist: ' + e, true);
                        hideLoader();
                    });
                } else {
                    processShader(shaderFile.content);
                    hideLoader();
                } 
            } else {
                logToConsole('No valid shader JSON found in Gist', true);
                hideLoader();
            }
        })
        .catch(err => {
            console.error('Error loading Gist:', err);
            logToConsole('Error loading Gist: ' + err, true);
            hideLoader();
        });
    }

    function saveGist() {
        if (!githubToken) {
            alert('Please login first');
            return;
        }

        // Ask for a filename
        let filename = prompt("Enter a name for your shader:", "shader");
        if (!filename) return; // Cancelled

        if (!filename.endsWith('.json')) {
            filename += '.json';
        }

        // Ensure current editor content is saved to content object
        content[activeTab] = editor.getValue();

        const payload = {
            frag: content.frag,
            vert: content.vert
        };

        if (Object.keys(externalAssets).length > 0) {
            payload.assets = externalAssets;
        }

        let contentString = "";
        try {
            contentString = JSON.stringify(payload, null, 2);
        } catch (e) {
            logToConsole('Error preparing JSON: ' + e.toString(), true);
            return;
        }

        let files = {};
        files[filename] = {
            "content": contentString
        };

        const data = {
            "description": "glslViewer Shader: " + filename.replace('.json', ''),
            "public": true,
            "files": files
        };

        const currentGistId = getQueryVariable('gist');
        const method = 'POST'; 
        
        const url = 'https://api.github.com/gists';

        fetch(url, {
            method: 'POST',
            headers: {
                'Authorization': 'token ' + githubToken,
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(data)
        })
        .then(response => {
             if (response.ok) return response.json();
             throw new Error('Save failed: ' + response.statusText);
        })
        .then(data => {
            const id = data.id;
            console.log('Saved Gist:', id);
            logToConsole('Saved to Gist: ' + id);
            
            // Update URL
            const newUrl = window.location.protocol + "//" + window.location.host + window.location.pathname + '?gist=' + id;
            window.history.pushState({path:newUrl},'',newUrl);
        })
        .catch(err => {
            console.error(err);
            logToConsole(err.message, true);
        });
    }

    if (loginBtn) loginBtn.addEventListener('click', loginGithub);
    if (saveBtn) saveBtn.addEventListener('click', saveGist);
    if (openBtn) openBtn.addEventListener('click', openGist);

    // Initial check
    checkGithubToken();
    
    // --- File Drag & Drop ---
    function handleDrop(e) {
        e.preventDefault();
        e.stopPropagation();
        
        const files = e.dataTransfer.files;
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            const name = file.name;
            const ext = name.split('.').pop().toLowerCase();
            
            showLoader();
            if (ext === 'frag' || ext === 'fs' || ext === 'vert' || ext === 'vs') {
                const reader = new FileReader();
                reader.onload = (event) => {
                    const data = event.target.result;
                    if (ext === 'frag' || ext === 'fs') {
                        content.frag = data;
                        if (activeTab === 'frag') {
                            editor.setValue(data);
                        } else {
                             setFrag(data);
                        }
                    } else {
                        content.vert = data;
                        if (activeTab === 'vert') {
                            editor.setValue(data);
                        } else {
                             setVert(data);
                        }
                    }
                    hideLoader();
                };
                reader.onerror = () => hideLoader();
                reader.readAsText(file);
            } 
            else {
                // Binary or other assets
                const reader = new FileReader();
                reader.onload = (event) => {
                    const dataURL = event.target.result;
                    
                    externalAssets[name] = dataURL;

                    // if the vert and frag are the same as default, update them
                    if (content.frag === defaultFragment && content.vert === defaultVertex)  {
                        if (['ply', 'obj', 'gltf', 'glb', 'splat'].includes(ext)) {
                            if (window.module_loaded) {
                                fetchShadersFromBackend();
                                sendCommand('sky,on');
                            }
                        }
                    }
                    else {
                        // re send current shaders to trigger reload with new asset
                        // clean console
                        if (consoleOutput)
                            consoleOutput.innerHTML = '';
                        
                        try {
                            setFrag(content.frag);
                            setVert(content.vert);   
                        } catch (e) {
                            console.error("Error reloading shaders after asset drop:", e);
                        }
                    }
                    hideLoader();
                };
                reader.onerror = () => hideLoader();
                reader.readAsDataURL(file);
            }
        }
    }
    
    document.body.addEventListener('dragover', (e) => { e.preventDefault(); e.stopPropagation(); });
    document.body.addEventListener('drop', handleDrop);
});
