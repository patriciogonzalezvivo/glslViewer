class WasmLoader extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }
  
    connectedCallback() {
        // Intercept global event listener registration to constrain WASM keyboard input
        // This must be done before the WASM script is loaded.
        const originalAddEventListener = window.addEventListener;
        window.addEventListener = function(type, listener, options) {
            if ((type === 'keydown' || type === 'keyup' || type === 'keypress') && typeof listener === 'function') {
                const wrappedListener = function(e) {
                    const canvas = document.getElementById('canvas');
                    const wrapper = document.getElementById('wrapper');
                    
                    // 1. Never intercept if the target is an input field (Editor/Console)
                    const target = e.target;
                    const isInput = target.tagName === 'INPUT' || 
                                  target.tagName === 'TEXTAREA' || 
                                  target.isContentEditable ||
                                  (target.closest && target.closest('.CodeMirror')); // Check CodeMirror
                    
                    if (isInput) return; // Let it bubble naturally, don't pass to WASM if it's GLOBAL listener
                    
                    // 2. Only allow WASM to see keys if mouse is over the wrapper OR canvas is focused
                    const isHovering = wrapper && wrapper.matches && wrapper.matches(':hover');
                    const isFocused = document.activeElement === canvas;
                    
                    if (isHovering || isFocused) {
                        listener.call(this, e);
                    }
                };
                return originalAddEventListener.call(this, type, wrappedListener, options);
            }
            return originalAddEventListener.call(this, type, listener, options);
        };

        // Create canvas element
        const canvas = document.getElementById('canvas');
  
        // // Create loader elements
        // const loader = document.createElement('div');
        // loader.className = 'emscripten_loader';
        // loader.id = 'emscripten_loader';
        // loader.innerHTML = `
        //     <div class='emscripten_loader' id='spinner'></div>
        //     <div class='emscripten_loader' id='status'>Downloading...</div>
        //     <progress class='emscripten_loader' value='50' max='100' id='progress'></progress>
        // `;

        // // Add styles
        // const style = document.createElement('style');
        // style.textContent = `
        //     :host {
        //         display: block;
        //         width: 100vw;
        //         height: 100vh;
        //         position: fixed;
        //         top: 0;
        //         left: 0;
        //         margin: 0;
        //         padding: 0;
        //         overflow: hidden;
        //         font-family: 'Lucida Console', Monaco, monospace;
        //         pointer-events: none; /* Allow clicks to pass through by default */
        //     }

        //     * {
        //         pointer-events: auto; /* Enable clicks on children (like loader/spinner) */
        //         -webkit-box-sizing: border-box;
        //         box-sizing: border-box;
        //         -moz-osx-font-smoothing: grayscale;
        //         -webkit-font-smoothing: antialiased;
        //         -webkit-tap-highlight-color: transparent;
        //         -webkit-touch-callout: none;
        //     }

        //     .emscripten_loader {
        //         position: absolute;
        //         top: 50%;
        //         left: 50%;
        //         transform: translate(-50%, -50%);
        //         width: 100%;
        //         height: 100%;
        //         display: flex;
        //         align-items: center;
        //         justify-content: center;
        //         z-index: 1000;
        //     }

        //     #spinner {
        //         height: 100px;
        //         width: 100px;
        //         margin: 0;
        //         margin-top: -50px;
        //         margin-left: -50px;
        //         display: inline-block;
        //         vertical-align: top;
        //         -webkit-animation: rotation .8s linear infinite;
        //         -moz-animation: rotation .8s linear infinite;
        //         -o-animation: rotation .8s linear infinite;
        //         animation: rotation 0.8s linear infinite;
        //         border-left: 5px solid rgb(200, 200, 200);
        //         border-right: 5px solid rgb(200, 200, 200);
        //         border-bottom: 5px solid rgba(0, 0, 0, 0);
        //         border-top: 5px solid rgba(0, 0, 0, 0);
        //         border-radius: 100%;
        //     }

        //     @-webkit-keyframes rotation {
        //         from {-webkit-transform: rotate(0deg);}
        //         to {-webkit-transform: rotate(360deg);}
        //     }
        //     @-moz-keyframes rotation {
        //         from {-moz-transform: rotate(0deg);}
        //         to {-moz-transform: rotate(360deg);}
        //     }
        //     @-o-keyframes rotation {
        //         from {-o-transform: rotate(0deg);}
        //         to {-o-transform: rotate(360deg);}
        //     }
        //     @keyframes rotation {
        //         from {transform: rotate(0deg);}
        //         to {transform: rotate(360deg);}
        //     }

        //     #status {
        //         display: fixed;
        //         left: 50%;
        //         bottom: 25%;
        //         transform: translate(-50%, -50%);
        //         color: rgb(200, 200, 200);
        //         font-family: monospace;
        //             }

        //     #progress {
        //         height: 10px;
        //         width: 200px;
        //         margin-top: 90px;
        //         color: rgb(200, 200, 200);
        //         accent-color: rgb(200, 200, 200);
        //             }
        //     `;
        
        // this.shadowRoot.appendChild(style);
        // this.shadowRoot.appendChild(loader);

        // Store reference to shadowRoot for Module methods
        const shadowRoot = null; //this.shadowRoot;

  
        // Initialize Module before loading script
        window.Module = {
            preRun: [],
            keyboardListeningElement: canvas,
            doNotCaptureKeyboard: true,
            // canvas: canvas,
            onRuntimeInitialized: function() {
                console.log('WASM Runtime Initialized');
                if (window.glslViewerLoader) {
                   if (window.glslViewerLoader.loading) {
                       window.glslViewerLoader.hide();
                       window.glslViewerLoader.loading = false;
                   }
                }
            },
            postRun: () => {
                // set weaver-loader visibility to hidden
                const wasm_loader = document.querySelector('wasm-loader');
                if (wasm_loader) {
                    wasm_loader.style.visibility = 'hidden';
                }

                // destroy shadowRoot to prevent memory leaks
                this.shadowRoot.innerHTML = '';
                window.module_loaded = true;
                
                // Force hide the loader in case setStatus was called after monitorRunDependencies(0)
                if (window.glslViewerLoader) {
                    window.glslViewerLoader.hide();
                    window.glslViewerLoader.loading = false;
                }
            },

            print: function(text) {
                console.log('WASM:', text);
                window.dispatchEvent(new CustomEvent('wasm-stdout', { detail: text }));
            },

            printErr: function(text) {
                console.error('WASM Error:', text);
                window.dispatchEvent(new CustomEvent('wasm-stderr', { detail: text }));
            },

            // canvas: canvas,
            canvas: (function() {
                // var canvas = document.getElementById('canvas');

                // As a default initial behavior, pop up an alert when webgl context is lost. To make your
                // application robust, you may want to override this behavior before shipping!
                // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
                canvas.addEventListener("webglcontextlost", function(e) { 
                    e.preventDefault(); 
                    // alert('WebGL context lost. You will need to reload the page.'); 
                    location.reload();
                }, false);
            
                return canvas;
            })(),
            
            setStatus: function(text) {
                // Don't show loader if module is already loaded
                if (window.module_loaded) return;
                
                if (window.glslViewerLoader) {
                    if (window.glslViewerLoader.loading) {
                        window.glslViewerLoader.update(text);
                    } else {
                        // Only show if we explicitly want to (usually controlled by monitorRunDependencies)
                        // But if something else calls setStatus, we might want to show it?
                        // Emscripten calls setStatus for "Running..."
                        // Let's allow it.
                        window.glslViewerLoader.loading = true;
                        window.glslViewerLoader.show(text);
                    }
                }
            },
        
            totalDependencies: 0,
            monitorRunDependencies: function(left) {
                this.totalDependencies = Math.max(this.totalDependencies, left);
                
                if (left === 0) {
                    if (window.glslViewerLoader && window.glslViewerLoader.loading) {
                        window.glslViewerLoader.hide();
                        window.glslViewerLoader.loading = false;
                    }
                } else {
                    const status = 'Downloading... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')';
                    
                    if (window.glslViewerLoader) {
                        if (window.glslViewerLoader.loading) {
                            window.glslViewerLoader.update(status);
                        } else {
                            window.glslViewerLoader.loading = true;
                            window.glslViewerLoader.show(status);
                        }
                    }
                }
            }
        };
  
        // Load WASM script
        const script = document.createElement('script');
        script.src = 'glslViewer.js';
        script.async = true;
        document.body.appendChild(script);
    }
  
    disconnectedCallback() {
        // Clean up resize handler
        if (this._resizeHandler) {
            window.removeEventListener('resize', this._resizeHandler);
        }
    }
}
  
customElements.define('wasm-loader', WasmLoader); 