class WasmLoader extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }
  
    connectedCallback() {
        // Create loader elements
        const loader = document.createElement('div');
        loader.className = 'emscripten_loader';
        loader.id = 'emscripten_loader';
        loader.innerHTML = `
            <div class='emscripten_loader' id='spinner'></div>
            <div class='emscripten_loader' id='status'>Downloading...</div>
            <progress class='emscripten_loader' value='50' max='100' id='progress'></progress>
        `;

        // Create canvas element
        const canvas = document.getElementById('canvas');
  
        // Add styles
        const style = document.createElement('style');
        style.textContent = `
            :host {
                display: block;
                width: 100vw;
                height: 100vh;
                position: fixed;
                top: 0;
                left: 0;
                margin: 0;
                padding: 0;
                overflow: hidden;
                font-family: 'Lucida Console', Monaco, monospace;
                pointer-events: none; /* Allow clicks to pass through by default */
            }

            * {
                pointer-events: auto; /* Enable clicks on children (like loader/spinner) */
                -webkit-box-sizing: border-box;
                box-sizing: border-box;
                -moz-osx-font-smoothing: grayscale;
                -webkit-font-smoothing: antialiased;
                -webkit-tap-highlight-color: transparent;
                -webkit-touch-callout: none;
            }

            .emscripten_loader {
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                width: 100%;
                height: 100%;
                display: flex;
                align-items: center;
                justify-content: center;
                z-index: 1000;
            }

            #spinner {
                height: 100px;
                width: 100px;
                margin: 0;
                margin-top: -50px;
                margin-left: -50px;
                display: inline-block;
                vertical-align: top;
                -webkit-animation: rotation .8s linear infinite;
                -moz-animation: rotation .8s linear infinite;
                -o-animation: rotation .8s linear infinite;
                animation: rotation 0.8s linear infinite;
                border-left: 5px solid rgb(200, 200, 200);
                border-right: 5px solid rgb(200, 200, 200);
                border-bottom: 5px solid rgba(0, 0, 0, 0);
                border-top: 5px solid rgba(0, 0, 0, 0);
                border-radius: 100%;
            }

            @-webkit-keyframes rotation {
                from {-webkit-transform: rotate(0deg);}
                to {-webkit-transform: rotate(360deg);}
            }
            @-moz-keyframes rotation {
                from {-moz-transform: rotate(0deg);}
                to {-moz-transform: rotate(360deg);}
            }
            @-o-keyframes rotation {
                from {-o-transform: rotate(0deg);}
                to {-o-transform: rotate(360deg);}
            }
            @keyframes rotation {
                from {transform: rotate(0deg);}
                to {transform: rotate(360deg);}
            }

            #status {
                display: fixed;
                left: 50%;
                bottom: 25%;
                transform: translate(-50%, -50%);
                color: rgb(200, 200, 200);
                font-family: monospace;
            }

            #progress {
                height: 10px;
                width: 200px;
                margin-top: 90px;
                color: rgb(200, 200, 200);
                accent-color: rgb(200, 200, 200);
            }
            `;
  
        this.shadowRoot.appendChild(style);
        this.shadowRoot.appendChild(loader);
    
        // Store reference to shadowRoot for Module methods
        const shadowRoot = this.shadowRoot;
  
        // Initialize Module before loading script
        window.Module = {
            preRun: [],
            keyboardListeningElement: canvas,
            // canvas: canvas,
            onRuntimeInitialized: function() {
                console.log('WASM Runtime Initialized');
                // Hide loader when runtime is initialized
                loader.style.display = 'none';
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
                // console.log('WASM Status:', text);
                const statusElement = shadowRoot.getElementById('status');
                const progressElement = shadowRoot.getElementById('progress');
                const spinnerElement = shadowRoot.getElementById('spinner');

                if (statusElement && progressElement && spinnerElement) {
                    if (!window.Module.setStatus.last) 
                        window.Module.setStatus.last = { time: Date.now(), text: '' };
            
                    if (text === window.Module.setStatus.last.text) 
                        return;
                    
                    var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
                    var now = Date.now();
                    if (m && now - window.Module.setStatus.last.time < 30) 
                        return; // if this is a progress update, skip it if too soon

                    window.Module.setStatus.last.time = now;
                    window.Module.setStatus.last.text = text;

                    if (m) {
                        text = m[1];
                        progressElement.value = parseInt(m[2])*100;
                        progressElement.max = parseInt(m[4])*100;
                        progressElement.hidden = false;
                        spinnerElement.hidden = false;
                    } 
                    else {
                        progressElement.value = null;
                        progressElement.max = null;
                        progressElement.hidden = true;
                        if (!text) 
                            spinnerElement.style.display = 'none';
                    }

                    // statusElement.innerHTML = text;
                    statusElement.textContent = text;
                }
            },
        
            totalDependencies: 0,
            monitorRunDependencies: function(left) {
                this.totalDependencies = Math.max(this.totalDependencies, left);
                const status = left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.';
                this.setStatus(status);
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