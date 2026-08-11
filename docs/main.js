// Main application entry point
// Coordinates all modules and initializes the application

import './wasm-loader.js';
import { GitHubIntegration } from './github.js';
import { GlslViewerIntegration } from './glslViewerIntegration.js';
import { EditorManager } from './editor.js';
import { UIManager, getQueryVariable } from './ui.js';

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

document.addEventListener('DOMContentLoaded', () => {
    // Initialize UI Manager
    const ui = new UIManager();
    ui.showLoader("Loading...");
    
    // Initialize glslViewer Integration
    const glslviewer = new GlslViewerIntegration((msg, isError) => {
        ui.logToConsole(msg, isError);
    });
    
    // Initialize Editor
    const editorManager = new EditorManager('editor-container', {
        frag: defaultFragment,
        vert: defaultVertex
    });
    
    // Initialize GitHub Integration
    const github = new GitHubIntegration();
    
    // Setup UI components
    ui.setupConsoleEvents();
    ui.setupCanvasFocus('editor-container');
    ui.setupTabSwitching(editorManager);
    ui.setupResizeObserver();
    ui.setupScreenshotButton();
    ui.setupViewDropdown(glslviewer);
    
    // Setup error highlighting
    editorManager.setupErrorHighlighting();
    
    // Handle fullscreen with glslviewer commands
    ui.setupResizeButton((isFullscreen) => {
        // The UI already handles visual changes, just sync with glslviewer if needed
    });
    
    // Update shader function
    const updateShader = () => {
        const code = editorManager.getValue();
        const activeTab = editorManager.getActiveTab();
        
        if (activeTab === 'frag') {
            glslviewer.setFrag(code);
        } else if (activeTab === 'vert') {
            glslviewer.setVert(code);
        }
    };
    
    // Setup editor change handler
    editorManager.onChange(updateShader);
    
    // Console input handler
    ui.setupConsoleInput((cmd) => {
        // Handle fullscreen commands through UI
        if (cmd === 'fullscreen,on') {
            ui.setFullscreen(true);
            ui.logToConsole('> ' + cmd);
            ui.logToConsole('on');
        } else if (cmd === 'fullscreen,off') {
            ui.setFullscreen(false);
            ui.logToConsole('> ' + cmd);
            ui.logToConsole('off');
        } else if (cmd === 'fullscreen,toggle') {
            ui.setFullscreen(!ui.getFullscreen());
            ui.logToConsole('> ' + cmd);
            ui.logToConsole(ui.getFullscreen() ? 'on' : 'off');
        } else if (cmd === 'fullscreen') {
            ui.logToConsole('> ' + cmd);
            ui.logToConsole(ui.getFullscreen() ? 'on' : 'off');
        } else {
            glslviewer.sendCommand(cmd);
        }
    });
    
    // File drag & drop handler
    ui.setupFileDragDrop((files) => {
        ui.showLoader();
        
        const promises = [];
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            const name = file.name;
            const ext = name.split('.').pop().toLowerCase();
            
            if (ext === 'frag' || ext === 'fs' || ext === 'vert' || ext === 'vs') {
                // Shader files
                promises.push(
                    glslviewer.handleFileDrop(file, (type, data) => {
                        editorManager.setContent(type, data);
                    }, ui.updateLoader.bind(ui))
                );
            } else {
                // Asset files
                promises.push(
                    glslviewer.handleFileDrop(file, null, ui.updateLoader.bind(ui))
                    .then(() => {
                        const content = editorManager.getAllContent();
                        
                        // If default shaders and asset is 3D model, fetch backend shaders
                        if (content.frag === defaultFragment && content.vert === defaultVertex) {
                            if (['ply', 'obj', 'gltf', 'glb', 'splat'].includes(ext)) {
                                if (glslviewer.isModuleReady()) {
                                    const shaders = glslviewer.fetchShadersFromBackend();
                                    if (shaders.frag) editorManager.setContent('frag', shaders.frag);
                                    if (shaders.vert) editorManager.setContent('vert', shaders.vert);
                                    glslviewer.sendCommand('sky,on');
                                }
                            }
                        } else {
                            // Reload shaders to trigger reload with new asset
                            ui.clearConsole();
                            glslviewer.setFrag(content.frag);
                            glslviewer.setVert(content.vert);
                        }
                    })
                );
            }
        }
        
        Promise.all(promises).finally(() => {
            ui.hideLoader();
        });
    });
    
    // GitHub buttons setup
    ui.setupGitHubButtons(github, {
        onSave: async () => {
            let filename = prompt("Enter a name for your shader:", "shader");
            if (!filename) return;
            
            const content = editorManager.getAllContent();
            const payload = {
                frag: content.frag,
                vert: content.vert,
                commands: [
                    ...glslviewer.getRetainedState(),
                    ...(ui.getFullscreen() ? ['fullscreen,on'] : [])
                ],
                assets: glslviewer.getExternalAssets()
            };
            
            try {
                const id = await github.saveGist(payload, filename);
                ui.logToConsole('Saved to Gist: ' + id);
                
                const newUrl = window.location.protocol + "//" + window.location.host + 
                               window.location.pathname + '?gist=' + id;
                window.history.pushState({path:newUrl},'',newUrl);
                
                // Upload canvas thumbnail to lygia.xyz
                await ui.uploadCanvasToLygia(id);
            } catch (err) {
                ui.logToConsole(err.message, true);
            }
        }
    });
    
    // Expose methods for external use
    window.getRetainedState = () => {
        return [
            ...glslviewer.getRetainedState(),
            ...(ui.getFullscreen() ? ['fullscreen,on'] : [])
        ];
    };
    window.getGistHistory = () => github.getGistHistory();
    
    // Wait for Module to be ready
    const checkModule = setInterval(() => {
        if (glslviewer.isModuleReady()) {
            clearInterval(checkModule);
            console.log("Module loaded, sending initial shader.");
            
            ui.hideLoader();

            const gistId = getQueryVariable('gist');
            if (gistId) {
                // Load gist
                github.loadGist(gistId, {
                    onStart: () => ui.showLoader("Loading Gist..."),
                    onUpdate: (text) => ui.updateLoader(text),
                    onSuccess: async (json) => {
                        if (json.frag) editorManager.setContent('frag', json.frag);
                        if (json.vert) editorManager.setContent('vert', json.vert);
                        
                        updateShader();
                        
                        if (json.assets) {
                            await glslviewer.loadAssetsFromGist(json.assets, ui.updateLoader.bind(ui));
                        }
                        
                        // Apply commands
                        if (json.commands && Array.isArray(json.commands)) {
                            json.commands.forEach((cmd) => {
                                if (cmd.startsWith('fullscreen,')) {
                                    const state = cmd.split(',')[1];
                                    ui.setFullscreen(state === 'on');
                                } else {
                                    glslviewer.sendCommand(cmd);
                                }
                            });
                        }
                        
                        // Re-send shaders to trigger reload with assets
                        const content = editorManager.getAllContent();
                        glslviewer.setFrag(content.frag);
                        glslviewer.setVert(content.vert);
                        
                        ui.hideLoader();
                    },
                    onError: (error) => {
                        ui.logToConsole('Error loading Gist: ' + error, true);
                        ui.hideLoader();
                    }
                });
            } else {
                updateShader();
            }
        }
    }, 500);
});
