/**
 * TypeScript definitions for Godot Engine Web API
 * 
 * Projects exported for the Web expose the Engine class to the JavaScript environment,
 * that allows fine control over the engine's start-up process.
 * 
 * This API is built in an asynchronous manner and requires basic understanding
 * of Promises.
 */

// ============================================================================
// Callback Type Definitions
// ============================================================================

/**
 * A callback function for handling Godot's OS.execute calls.
 * This is for example used in the Web Editor template to switch between project manager and editor, and for running the game.
 * 
 * @param path The path that Godot wants executed.
 * @param args The arguments of the "command" to execute.
 */
export type OnExecuteCallback = (path: string, args: string[]) => void;

/**
 * A callback function for being notified when the Godot instance quits.
 * 
 * Note: This function will not be called if the engine crashes or become unresponsive.
 * 
 * @param status_code The status code returned by Godot on exit.
 */
export type OnExitCallback = (status_code: number) => void;

/**
 * A callback function for displaying download progress.
 * 
 * The function is called once per frame while downloading files, so the usage of requestAnimationFrame() is not necessary.
 * 
 * If the callback function receives a total amount of bytes as 0, this means that it is impossible to calculate.
 * Possible reasons include:
 * - Files are delivered with server-side chunked compression
 * - Files are delivered with server-side compression on Chromium
 * - Not all file downloads have started yet (usually on servers without multi-threading)
 * 
 * @param current The current amount of downloaded bytes so far.
 * @param total The total amount of bytes to be downloaded.
 */
export type OnProgressCallback = (current: number, total: number) => void;

/**
 * A callback function for handling the standard output stream. This method should usually only be used in debug pages.
 * 
 * By default, console.log() is used.
 * 
 * @param var_args A variadic number of arguments to be printed.
 */
export type OnPrintCallback = (...var_args: any[]) => void;

/**
 * A callback function for handling the standard error stream. This method should usually only be used in debug pages.
 * 
 * By default, console.error() is used.
 * 
 * @param var_args A variadic number of arguments to be printed as errors.
 */
export type OnPrintErrorCallback = (...var_args: any[]) => void;

// ============================================================================
// Configuration Interfaces
// ============================================================================

/**
 * Supported features configuration for feature detection.
 */
export interface SupportedFeatures {
    /**
     * Whether threads are supported. Defaults to true.
     */
    threads?: boolean;
}

/**
 * An object used to configure the Engine instance based on godot export options,
 * and to override those in custom HTML templates if needed.
 */
export interface EngineConfig {
    /**
     * Whether to unload the engine automatically after the instance is initialized.
     * 
     * @default true
     */
    unloadAfterInit?: boolean;

    /**
     * The HTML DOM Canvas object to use.
     * 
     * By default, the first canvas element in the document will be used if none is specified.
     * 
     * @default null
     */
    canvas?: HTMLCanvasElement | null;

    /**
     * The name of the WASM file without the extension. (Set by Godot Editor export process).
     * 
     * @default ''
     */
    executable?: string;

    /**
     * An alternative name for the game pck to load. The executable name is used otherwise.
     * 
     * @default null
     */
    mainPack?: string | null;

    /**
     * Specify a language code to select the proper localization for the game.
     * 
     * The browser locale will be used if none is specified. See complete list of
     * supported locales.
     * 
     * @default null
     */
    locale?: string | null;

    /**
     * The canvas resize policy determines how the canvas should be resized by Godot.
     * 
     * - `0` means Godot won't do any resizing. This is useful if you want to control the canvas size from javascript code in your template.
     * - `1` means Godot will resize the canvas on start, and when changing window size via engine functions.
     * - `2` means Godot will adapt the canvas size to match the whole browser window.
     * 
     * @default 2
     */
    canvasResizePolicy?: 0 | 1 | 2;

    /**
     * The arguments to be passed as command line arguments on startup.
     * 
     * See command line tutorial.
     * 
     * Note: startGame() will always add the --main-pack argument.
     * 
     * @default []
     */
    args?: string[];

    /**
     * When enabled, the game canvas will automatically grab the focus when the engine starts.
     * 
     * @default true
     */
    focusCanvas?: boolean;

    /**
     * When enabled, this will turn on experimental virtual keyboard support on mobile.
     * 
     * @default false
     */
    experimentalVK?: boolean;

    /**
     * The progressive web app service worker to install.
     * 
     * @default ''
     */
    serviceWorker?: string;

    /**
     * Persistent paths for file system storage.
     * 
     * @default ['/userfs']
     */
    persistentPaths?: string[];

    /**
     * Whether to enable persistent file drops.
     * 
     * @default false
     */
    persistentDrops?: boolean;

    /**
     * GDExtension libraries to load.
     * 
     * @default []
     */
    gdextensionLibs?: string[];

    /**
     * File sizes map for preloading optimization.
     * 
     * @default {}
     */
    fileSizes?: Record<string, number>;

    /**
     * A callback function for handling Godot's OS.execute calls.
     * 
     * This is for example used in the Web Editor template to switch between project manager and editor, and for running the game.
     * 
     * @default null
     */
    onExecute?: OnExecuteCallback | null;

    /**
     * A callback function for being notified when the Godot instance quits.
     * 
     * Note: This function will not be called if the engine crashes or become unresponsive.
     * 
     * @default null
     */
    onExit?: OnExitCallback | null;

    /**
     * A callback function for displaying download progress.
     * 
     * The function is called once per frame while downloading files, so the usage of requestAnimationFrame() is not necessary.
     * 
     * @default null
     */
    onProgress?: OnProgressCallback | null;

    /**
     * A callback function for handling the standard output stream. This method should usually only be used in debug pages.
     * 
     * By default, console.log() is used.
     * 
     * @default console.log
     */
    onPrint?: OnPrintCallback;

    /**
     * A callback function for handling the standard error stream. This method should usually only be used in debug pages.
     * 
     * By default, console.error() is used.
     * 
     * @default console.error
     */
    onPrintError?: OnPrintErrorCallback;
}

// ============================================================================
// Engine Class
// ============================================================================

/**
 * The Engine class provides methods for loading and starting exported projects on the Web.
 * For default export settings, this is already part of the exported HTML page.
 * 
 * To understand practical use of the Engine class, see Custom HTML page for Web export.
 */
export declare class Engine {
    /**
     * Create a new Engine instance with the given configuration.
     * 
     * @param initConfig The initial config for this instance.
     */
    constructor(initConfig?: EngineConfig);

    /**
     * Initialize the engine instance. Optionally, pass the base path to the engine to load it,
     * if it hasn't been loaded yet. See Engine.load.
     * 
     * @param basePath Base path of the engine to load.
     * @return A Promise that resolves once the engine is loaded and initialized.
     */
    init(basePath?: string): Promise<void>;

    /**
     * Load a file so it is available in the instance's file system once it runs. Must be called **before** starting the
     * instance.
     * 
     * If not provided, the path is derived from the URL of the loaded file.
     * 
     * @param file The file to preload. If a string the file will be loaded from that path.
     *              If an ArrayBuffer or a view on one, the buffer will used as the content of the file.
     * @param path Path by which the file will be accessible. Required, if file is not a string.
     * @returns A Promise that resolves once the file is loaded.
     */
    preloadFile(file: string | ArrayBuffer | ArrayBufferView, path?: string): Promise<void>;

    /**
     * Start the engine instance using the given override configuration (if any).
     * startGame() can be used in typical cases instead.
     * 
     * This will initialize the instance if it is not initialized. For manual initialization, see init().
     * The engine must be loaded beforehand.
     * 
     * Fails if a canvas cannot be found on the page, or not specified in the configuration.
     * 
     * @param override An optional configuration override.
     * @return Promise that resolves once the engine started.
     */
    start(override?: EngineConfig): Promise<void>;

    /**
     * Start the game instance using the given configuration override (if any).
     * 
     * This will initialize the instance if it is not initialized. For manual initialization, see init().
     * 
     * This will load the engine if it is not loaded, and preload the main pck.
     * 
     * This method expects the initial config (or the override) to have both the executable and mainPack
     * properties set (normally done by the editor during export).
     * 
     * @param override An optional configuration override.
     * @return Promise that resolves once the game started.
     */
    startGame(override?: EngineConfig): Promise<void>;

    /**
     * Create a file at the specified path with the passed as buffer in the instance's file system.
     * 
     * @param path The location where the file will be created.
     * @param buffer The content of the file.
     */
    copyToFS(path: string, buffer: ArrayBuffer | ArrayBufferView): void;

    /**
     * Request that the current instance quit.
     * 
     * This is akin the user pressing the close button in the window manager, and will
     * have no effect if the engine has crashed, or is stuck in a loop.
     */
    requestQuit(): void;

    /**
     * Install the progressive-web app service worker.
     * @returns The service worker registration promise.
     */
    installServiceWorker(): Promise<ServiceWorkerRegistration>;

    /**
     * Export project to PCK file as ArrayBuffer.
     * Only available in editor builds after the engine is initialized.
     * 
     * @param options Export options (preset name and debug flag)
     * @returns Promise that resolves to PCK file data as ArrayBuffer
     * @throws Error if export fails or engine not initialized
     */
    exportPack(options?: ExportPackOptions): Promise<ArrayBuffer>;

    /**
     * Export project to PCK patch file as ArrayBuffer.
     * Only available in editor builds after the engine is initialized.
     * 
     * @param options Export options including patch file paths
     * @returns Promise that resolves to PCK patch file data as ArrayBuffer
     * @throws Error if export fails or engine not initialized
     */
    exportPackPatch(options?: ExportPackPatchOptions): Promise<ArrayBuffer>;

    /**
     * Register a callback for editor save events.
     * Only available in editor builds after the engine is initialized.
     * 
     * @param callback Function to call when a save event occurs
     * @throws Error if engine not initialized or callback registration fails
     */
    onSave(callback: SaveEventListener): void;

    /**
     * Unregister the save event listener.
     * Only available in editor builds.
     */
    offSave(): void;

    // Static methods

    /**
     * Load the engine from the specified base path.
     * 
     * @param basePath Base path of the engine to load.
     * @param size The file size if known.
     * @returns A Promise that resolves once the engine is loaded.
     */
    static load(basePath: string, size?: number): Promise<void>;

    /**
     * Unload the engine to free memory.
     * 
     * This method will be called automatically depending on the configuration. See unloadAfterInit.
     */
    static unload(): void;

    /**
     * Check whether WebGL is available. Optionally, specify a particular version of WebGL to check for.
     * 
     * @param majorVersion The major WebGL version to check for. Defaults to 1.
     * @returns If the given major version of WebGL is available.
     */
    static isWebGLAvailable(majorVersion?: number): boolean;

    /**
     * Check whether the Fetch API available and supports streaming responses.
     * 
     * @returns If the Fetch API is available and supports streaming responses.
     */
    static isFetchAvailable(): boolean;

    /**
     * Check whether the engine is running in a Secure Context.
     * 
     * @returns If the engine is running in a Secure Context.
     */
    static isSecureContext(): boolean;

    /**
     * Check whether the engine is cross origin isolated.
     * This value is dependent on Cross-Origin-Opener-Policy and Cross-Origin-Embedder-Policy headers sent by the server.
     * 
     * @returns If the engine is running in a cross-origin isolated context.
     */
    static isCrossOriginIsolated(): boolean;

    /**
     * Check whether SharedArrayBuffer is available.
     * 
     * Most browsers require the page to be running in a secure context, and the
     * the server to provide specific CORS headers for SharedArrayBuffer to be available.
     * 
     * @returns If SharedArrayBuffer is available.
     */
    static isSharedArrayBufferAvailable(): boolean;

    /**
     * Check whether the AudioContext supports AudioWorkletNodes.
     * 
     * @returns If AudioWorkletNode is available.
     */
    static isAudioWorkletAvailable(): boolean;

    /**
     * Return an array of missing required features (as string).
     * 
     * @param supportedFeatures Configuration object specifying which features to check.
     * @returns A list of human-readable missing features.
     */
    static getMissingFeatures(supportedFeatures?: SupportedFeatures): string[];
}

// ============================================================================
// Export API
// ============================================================================

/**
 * Export options for PCK export.
 */
export interface ExportPackOptions {
    /**
     * Name of the export preset to use. If empty or not provided, uses the first available preset.
     * 
     * @default ""
     */
    presetName?: string;

    /**
     * Whether to export debug version.
     * 
     * @default false
     */
    debug?: boolean;
}

/**
 * Export options for PCK patch export.
 */
export interface ExportPackPatchOptions extends ExportPackOptions {
    /**
     * Array of patch file paths to include in the patch export.
     * 
     * @default []
     */
    patches?: string[];
}

// ============================================================================
// Editor Events API
// ============================================================================

/**
 * Save event data structure.
 */
export interface SaveEvent {
    /**
     * Type of save event: 'scene' for scene saves, 'resource' for resource saves.
     */
    type: 'scene' | 'resource';

    /**
     * Path to the saved file.
     */
    path: string;

    /**
     * Resource type (only present for resource saves).
     */
    resourceType?: string;
}

/**
 * Callback function for save events.
 */
export type SaveEventListener = (event: SaveEvent) => void;

// ============================================================================
// Global Window Augmentation
// ============================================================================

declare global {
    interface Window {
        /**
         * The Godot Engine class exposed globally for web exports.
         */
        Engine: typeof Engine;
    }
}

export { };

