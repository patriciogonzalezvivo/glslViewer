// glslViewer WASM Integration Module
// Handles interaction with the WASM module, shader updates, commands, and asset loading

const cmds_state = ['plot', 'textures', 'buffers', 'floor', 'cubemap', 'axis', 'grid', 'bboxes', 'fullscreen'];
const cmds_plot_modes = ['off', 'fps', 'rgb', 'luma'];
const cmds_camera = ['camera_position', 'camera_look_at','camera'];
const cmds_listen = ['plane', 'pcl_plane', 'sphere', 'pcl_sphere', 'icosphere', 'cylinder'];

export class GlslViewerIntegration {
    constructor(logCallback) {
        this.logCallback = logCallback || ((msg) => console.log(msg));
        this.cmdsHistory = [];
        this.externalAssets = {};
    }

    isModuleReady() {
        return window.Module && window.module_loaded;
    }

    sendCommand(cmd) {
        // Log command if it starts with any of the cmds_listen
        if (cmds_listen.some(c => cmd.startsWith(c))) {
            console.log('Command added to history:', cmd);
            this.cmdsHistory.push(cmd);
        }

        this.logCallback('> ' + cmd);
        
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('command', null, ['string'], [cmd]);
            } catch(err) {
                this.logCallback('Error sending command: ' + err, true);
            }
        } else {
            this.logCallback('Module not ready.', true);
        }
    }

    query(cmd) {
        if (window.Module && window.Module.ccall) {
            try {
                return window.Module.ccall('query', 'string', ['string'], [cmd]);
            } catch(err) {
                console.error('Error querying ' + cmd + ': ' + err);
                return null;
            }
        }
        return null;
    }

    getRetainedState() {
        let results = [...this.cmdsHistory];

        const cmds_to_check = [...new Set([...cmds_state, ...cmds_camera])];
        cmds_to_check.forEach((cmd) => {
            let answer = this.query(cmd);
            if (answer) {
                results.push(cmd + ',' + answer);
            }
        });

        // Remove duplicates while preserving order
        results = [...new Set(results)];
        return results;
    }

    setFrag(code) {
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('setFrag', null, ['string'], [code]);
            } catch (e) {
                console.error("Error setting fragment shader:", e);
            }
        }
    }

    setVert(code) {
        if (window.Module && window.Module.ccall) {
            try {
                window.Module.ccall('setVert', null, ['string'], [code]);
            } catch (e) {
                console.error("Error setting vertex shader:", e);
            }
        }
    }

    getDefaultSceneFrag() {
        if (window.Module && window.Module.ccall) {
            try {
                return window.Module.ccall('getDefaultSceneFrag', 'string', [], []);
            } catch(e) {
                console.log("Could not fetch default frag: " + e);
            }
        }
        return null;
    }

    getDefaultSceneVert() {
        if (window.Module && window.Module.ccall) {
            try {
                return window.Module.ccall('getDefaultSceneVert', 'string', [], []);
            } catch(e) {
                console.log("Could not fetch default vert: " + e);
            }
        }
        return null;
    }

    fetchShadersFromBackend() {
        const cFrag = this.getDefaultSceneFrag();
        const cVert = this.getDefaultSceneVert();
        return { frag: cFrag, vert: cVert };
    }

    decodeBase64(dataUrl) {
        const base64 = dataUrl.split(',')[1];
        const binaryString = window.atob(base64);
        const len = binaryString.length;
        const data = new Uint8Array(len);
        for (let k = 0; k < len; k++) {
            data[k] = binaryString.charCodeAt(k);
        }
        return data;
    }

    async downloadAsset(url) {
        const res = await fetch(url);
        if (!res.ok) throw new Error(res.statusText + ' (' + res.status + ') ' + url);
        const buffer = await res.arrayBuffer();
        return new Uint8Array(buffer).slice(0);
    }

    loadToWasm(name, data, updateLoaderCallback) {
        return new Promise((resolve, reject) => {
            let attempts = 0;
            const tryWrite = () => {
                attempts++;
                if (attempts > 20) { // 10 seconds timeout
                    console.error("Timeout waiting for WASM filesystem");
                    resolve();
                    return;
                }

                if (window.Module && window.Module.FS && window.module_loaded) {
                    try {
                        if (updateLoaderCallback) updateLoaderCallback("Loading " + name);
                        
                        window.Module.FS.writeFile(name, data);
                        this.logCallback("Loaded asset: " + name);
                        
                        const ext = name.split('.').pop().toLowerCase();
                        window.Module.ccall('loadAsset', null, ['string', 'string'], [name, ext]);

                        if (['hdr'].includes(ext)) {
                            this.sendCommand('cubemap,on');
                        }

                        resolve();
                    } catch (e) {
                        console.error("FS error", e);
                        resolve();
                    }
                } else {
                    setTimeout(tryWrite, 500);
                }
            };
            tryWrite();
        });
    }

    async loadAssetsFromGist(assets, updateLoaderCallback) {
        this.externalAssets = assets;
        const assetPromises = [];
        
        for (const [name, url] of Object.entries(assets)) {
            let p;
            if (url.startsWith('data:')) {
                try {
                    const data = this.decodeBase64(url);
                    p = this.loadToWasm(name, data, updateLoaderCallback);
                } catch (e) {
                    console.error("Error decoding base64", e);
                    p = Promise.resolve();
                }
            } else {
                p = this.downloadAsset(url)
                    .then(data => this.loadToWasm(name, data, updateLoaderCallback))
                    .catch(err => {
                        this.logCallback('Error loading asset ' + name + ': ' + err.message, true);
                        return Promise.resolve();
                    });
            }
            assetPromises.push(p);
        }
        
        return Promise.all(assetPromises);
    }

    async handleFileDrop(file, onShaderUpdate, updateLoaderCallback) {
        const name = file.name;
        const ext = name.split('.').pop().toLowerCase();
        
        if (ext === 'frag' || ext === 'fs' || ext === 'vert' || ext === 'vs') {
            const reader = new FileReader();
            return new Promise((resolve, reject) => {
                reader.onload = (event) => {
                    const data = event.target.result;
                    if (onShaderUpdate) {
                        onShaderUpdate(ext === 'frag' || ext === 'fs' ? 'frag' : 'vert', data);
                    }
                    resolve();
                };
                reader.onerror = () => reject(reader.error);
                reader.readAsText(file);
            });
        } else {
            // Binary or other assets
            const reader = new FileReader();
            return new Promise((resolve, reject) => {
                reader.onload = (event) => {
                    const dataURL = event.target.result;
                    this.externalAssets[name] = dataURL;
                    
                    const data = this.decodeBase64(dataURL);
                    this.loadToWasm(name, data, updateLoaderCallback).then(resolve);
                };
                reader.onerror = () => reject(reader.error);
                reader.readAsDataURL(file);
            });
        }
    }

    getExternalAssets() {
        return this.externalAssets;
    }

    getCommandsState() {
        return cmds_state;
    }

    getPlotModes() {
        return cmds_plot_modes;
    }

    getCmdsHistory() {
        return this.cmdsHistory;
    }
}
