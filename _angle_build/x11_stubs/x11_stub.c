/*
 * Comprehensive X11 function stubs for gl4es + Wine runtime on Android/Bionic.
 *
 * gl4es's GLX implementation internally wraps EGL calls. It references
 * a handful of Xlib functions for type conversions and cleanup, but with
 * LIBGL_FB=3 (FBO/pbuffer mode), none of the window-management functions
 * are actually exercised for real rendering. These stubs satisfy the dynamic
 * linker and provide safe no-op/sentinel returns.
 *
 * Wine's winex11.drv also calls various Xlib functions during initialization
 * (keyboard mapping, extension queries, selection/clipboard, window naming).
 * These stubs ensure those calls don't crash or return unexpected NULLs.
 *
 * Key features:
 *   - Window dimension tracking (so GL4ES gets correct FBO sizes)
 *   - Event queue with ConfigureNotify on resize (GL4ES FBO resize)
 *   - Atom table (for GL4ES GLX extension queries)
 *   - Screen dimensions from env vars STUB_SCREEN_W/H (default 800x600)
 *   - Visual ID from env var STUB_VISUAL_ID (default 1)
 *   - Window name tracking (XStoreName/XFetchName round-trip)
 *   - Keyboard mapping stubs (XKeysymToKeycode, XLookupString)
 *   - Extension listing (advertises GLX)
 *   - Clipboard/selection stubs
 *
 * Deploy this statically linked into gl4es's libGL.so.1 in opt/gl4es/lib/.
 * Do NOT deploy as a separate libX11.so — that would shadow Wine's real
 * libX11.so which communicates with the GameNative Java X server.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Include our stub headers — they define all needed X11 types/macros */
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* XRandr types (not in Xlib.h — only used locally by these stubs) */
typedef unsigned short Rotation;
typedef unsigned short SizeID;

typedef struct {
    int width, height;
    int mwidth, mheight;
} XRRScreenSize;

typedef struct _XRRScreenConfiguration {
    int dummy; /* opaque handle — never inspected by gl4es */
} XRRScreenConfiguration;

/* ══════════════════════════════════════════════════════════════════════════
 * Window dimension tracking
 * ══════════════════════════════════════════════════════════════════════════ */

#define MAX_TRACKED_WINDOWS 128

typedef struct {
    Window id;
    int x, y;
    unsigned int width, height;
    unsigned int border_width;
    unsigned int depth;
    int mapped;
    long event_mask;
    char name[256]; /* window title — round-trips via XStoreName/XFetchName */
} TrackedWindow;

static TrackedWindow _win_table[MAX_TRACKED_WINDOWS];
static int _win_count = 0;

static TrackedWindow *_find_window(Window id) {
    for (int i = 0; i < _win_count; i++)
        if (_win_table[i].id == id) return &_win_table[i];
    return NULL;
}

static TrackedWindow *_register_window(Window id, int x, int y,
    unsigned int w, unsigned int h, unsigned int depth)
{
    TrackedWindow *win = _find_window(id);
    if (!win) {
        if (_win_count >= MAX_TRACKED_WINDOWS) return NULL;
        win = &_win_table[_win_count++];
    }
    win->id = id;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->border_width = 0;
    win->depth = depth > 0 ? depth : 24;
    win->mapped = 0;
    win->event_mask = 0;
    win->name[0] = '\0';
    return win;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Event queue (ring buffer)
 * ══════════════════════════════════════════════════════════════════════════ */

#define MAX_QUEUED_EVENTS 64

static XEvent _evt_queue[MAX_QUEUED_EVENTS];
static int _evt_head = 0;
static int _evt_tail = 0;
static int _evt_count = 0;

static void _push_event(const XEvent *ev) {
    if (_evt_count >= MAX_QUEUED_EVENTS) return; /* drop if full */
    _evt_queue[_evt_tail] = *ev;
    _evt_tail = (_evt_tail + 1) % MAX_QUEUED_EVENTS;
    _evt_count++;
}

static int _pop_event(XEvent *ev) {
    if (_evt_count <= 0) return 0;
    *ev = _evt_queue[_evt_head];
    _evt_head = (_evt_head + 1) % MAX_QUEUED_EVENTS;
    _evt_count--;
    return 1;
}

/* Helper: push a ConfigureNotify event for a tracked window.
 * GL4ES uses ConfigureNotify to detect size changes and resize FBOs. */
static void _push_configure_notify(Display *display, TrackedWindow *tw) {
    if (!tw) return;
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = ConfigureNotify;
    ev.xconfigure.display = display;
    ev.xconfigure.event = tw->id;
    ev.xconfigure.window = tw->id;
    ev.xconfigure.x = tw->x;
    ev.xconfigure.y = tw->y;
    ev.xconfigure.width = tw->width;
    ev.xconfigure.height = tw->height;
    ev.xconfigure.border_width = tw->border_width;
    ev.xconfigure.above = 0;
    ev.xconfigure.override_redirect = False;
    _push_event(&ev);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Atom table (sequential IDs, simple linear search)
 * ══════════════════════════════════════════════════════════════════════════ */

#define MAX_ATOMS 256

static struct {
    char name[128];
} _atom_names[MAX_ATOMS];
static Atom _next_atom_id = 100; /* Start above predefined XA_* constants */

/* ══════════════════════════════════════════════════════════════════════════
 * Static sentinel objects — screen and display
 * ══════════════════════════════════════════════════════════════════════════ */

static int _screen_width = 800;  /* overridden by STUB_SCREEN_W env var */
static int _screen_height = 600; /* overridden by STUB_SCREEN_H env var */
static VisualID _visual_id = 1;  /* overridden by STUB_VISUAL_ID env var */

static Visual _stub_visual;
static Depth _stub_depth;
static Screen _stub_screen;
static struct _XDisplay _stub_display;
static int _stub_inited = 0;

static void _ensure_init(void) {
    if (_stub_inited) return;
    _stub_inited = 1;

    /* Read screen dimensions from environment (set by XServerScreen.kt) */
    const char *env_w = getenv("STUB_SCREEN_W");
    const char *env_h = getenv("STUB_SCREEN_H");
    const char *env_vid = getenv("STUB_VISUAL_ID");
    if (env_w && atoi(env_w) > 0) _screen_width = atoi(env_w);
    if (env_h && atoi(env_h) > 0) _screen_height = atoi(env_h);
    if (env_vid && atoi(env_vid) > 0) _visual_id = (VisualID)atoi(env_vid);

    _stub_visual.ext_data = NULL;
    _stub_visual.visualid = _visual_id;
    _stub_visual.class = TrueColor;
    _stub_visual.red_mask   = 0x00FF0000;
    _stub_visual.green_mask = 0x0000FF00;
    _stub_visual.blue_mask  = 0x000000FF;
    _stub_visual.bits_per_rgb = 8;
    _stub_visual.map_entries = 256;

    _stub_depth.depth = 24;
    _stub_depth.nvisuals = 1;
    _stub_depth.visuals = &_stub_visual;

    _stub_screen.ext_data = NULL;
    _stub_screen.display = &_stub_display;
    _stub_screen.root = 1;
    _stub_screen.width = _screen_width;
    _stub_screen.height = _screen_height;
    _stub_screen.mwidth = _screen_width * 340 / 1280;  /* approximate mm */
    _stub_screen.mheight = _screen_height * 190 / 720;
    _stub_screen.ndepths = 1;
    _stub_screen.depths = &_stub_depth;
    _stub_screen.root_depth = 24;
    _stub_screen.root_visual = &_stub_visual;
    _stub_screen.default_gc = NULL;
    _stub_screen.cmap = 1;
    _stub_screen.white_pixel = 0xFFFFFF;
    _stub_screen.black_pixel = 0x000000;

    _stub_display.ext_data = NULL;
    _stub_display.fd = -1;
    _stub_display.default_screen = 0;
    _stub_display.nscreens = 1;
    _stub_display.screens = &_stub_screen;
    _stub_display.proto_major_version = 11;
    _stub_display.proto_minor_version = 0;
    _stub_display.vendor = "X11Stub";

    /* Register the root window in the tracking table */
    _register_window(1, 0, 0, _screen_width, _screen_height, 24);
}

/* ══════════════════════════════════════════════════════════════════════════
 * Core Xlib function stubs
 * ══════════════════════════════════════════════════════════════════════════ */

Display *XOpenDisplay(const char *display_name) {
    _ensure_init();
    return &_stub_display;
}

int XCloseDisplay(Display *display) {
    return 0;
}

int XFree(void *data) {
    if (data) free(data);
    return 0;
}

int XSync(Display *display, Bool discard) {
    if (discard) {
        _evt_head = _evt_tail = _evt_count = 0;
    }
    return 0;
}

int XFlush(Display *display) {
    return 0;
}

/* ── Window creation / destruction ── */

Window XCreateWindow(Display *display, Window parent,
    int x, int y, unsigned int width, unsigned int height,
    unsigned int border_width, int depth, unsigned int class,
    Visual *visual, unsigned long valuemask,
    XSetWindowAttributes *attributes)
{
    _ensure_init();
    static Window next_id = 100;
    Window id = next_id++;

    /* Track the window dimensions so XGetWindowAttributes returns correct values */
    TrackedWindow *tw = _register_window(id, x, y,
        width > 0 ? width : _screen_width,
        height > 0 ? height : _screen_height,
        depth > 0 ? depth : 24);
    if (tw) tw->border_width = border_width;

    return id;
}

Window XCreateSimpleWindow(Display *display, Window parent,
    int x, int y, unsigned int width, unsigned int height,
    unsigned int border_width, unsigned long border, unsigned long background)
{
    _ensure_init();
    /* Inherit depth from parent if tracked, else default 24 */
    int depth = 24;
    TrackedWindow *pw = _find_window(parent);
    if (pw) depth = pw->depth;

    return XCreateWindow(display, parent, x, y, width, height,
        border_width, depth, InputOutput, &_stub_visual, 0, NULL);
}

int XDestroyWindow(Display *display, Window w) {
    TrackedWindow *tw = _find_window(w);
    if (tw) tw->mapped = 0;
    return 0;
}

int XMapWindow(Display *display, Window w) {
    _ensure_init();
    TrackedWindow *tw = _find_window(w);
    if (tw) {
        tw->mapped = 1;
        /* Queue a MapNotify event */
        XEvent map_ev;
        memset(&map_ev, 0, sizeof(map_ev));
        map_ev.type = MapNotify;
        map_ev.xmap.display = display;
        map_ev.xmap.event = w;
        map_ev.xmap.window = w;
        map_ev.xmap.override_redirect = False;
        _push_event(&map_ev);

        /* Queue an Expose event — GL4ES needs this to know the window is visible */
        XEvent expose_ev;
        memset(&expose_ev, 0, sizeof(expose_ev));
        expose_ev.type = Expose;
        expose_ev.xexpose.display = display;
        expose_ev.xexpose.window = w;
        expose_ev.xexpose.x = 0;
        expose_ev.xexpose.y = 0;
        expose_ev.xexpose.width = tw->width;
        expose_ev.xexpose.height = tw->height;
        expose_ev.xexpose.count = 0;
        _push_event(&expose_ev);
    }
    return 0;
}

int XMapRaised(Display *display, Window w) {
    return XMapWindow(display, w);
}

int XMapSubwindows(Display *display, Window w) {
    return 0;
}

int XUnmapWindow(Display *display, Window w) {
    TrackedWindow *tw = _find_window(w);
    if (tw) tw->mapped = 0;
    return 0;
}

int XUnmapSubwindows(Display *display, Window w) {
    return 0;
}

/* ── Window queries ── */

Status XGetWindowAttributes(Display *display, Window w,
    XWindowAttributes *attrs)
{
    if (!attrs) return 0;
    _ensure_init();
    memset(attrs, 0, sizeof(*attrs));

    TrackedWindow *tw = _find_window(w);
    if (tw) {
        attrs->x = tw->x;
        attrs->y = tw->y;
        attrs->width = tw->width;
        attrs->height = tw->height;
        attrs->border_width = tw->border_width;
        attrs->depth = tw->depth;
        attrs->map_state = tw->mapped ? 2 /* IsViewable */ : 0;
    } else {
        /* Unknown window — return screen dimensions as fallback */
        attrs->x = 0;
        attrs->y = 0;
        attrs->width = _screen_width;
        attrs->height = _screen_height;
        attrs->depth = 24;
    }
    attrs->visual = &_stub_visual;
    attrs->root = 1;
    attrs->class = InputOutput;
    attrs->screen = &_stub_screen;
    return 1;
}

XVisualInfo *XGetVisualInfo(Display *display, long vinfo_mask,
    XVisualInfo *vinfo_template, int *nitems_return)
{
    _ensure_init();
    XVisualInfo *info = (XVisualInfo *)malloc(sizeof(XVisualInfo));
    if (!info) {
        if (nitems_return) *nitems_return = 0;
        return NULL;
    }
    info->visual = &_stub_visual;
    info->visualid = _visual_id;
    info->screen = 0;
    info->depth = 24;
    info->class = TrueColor;
    info->red_mask   = 0x00FF0000;
    info->green_mask = 0x0000FF00;
    info->blue_mask  = 0x000000FF;
    info->colormap_size = 256;
    info->bits_per_rgb = 8;
    if (nitems_return) *nitems_return = 1;
    return info;
}

Colormap XCreateColormap(Display *display, Window w,
    Visual *visual, int alloc)
{
    return 1;
}

int XFreeColormap(Display *display, Colormap colormap) {
    return 0;
}

int XAllocColor(Display *display, Colormap colormap, void *screen_in_out) {
    return 1;
}

/* ── Error handling (proper signature) ── */

static XErrorHandler _current_error_handler = NULL;
XErrorHandler XSetErrorHandler(XErrorHandler handler) {
    XErrorHandler old = _current_error_handler;
    _current_error_handler = handler;
    return old;
}

static XIOErrorHandler _current_io_error_handler = NULL;
XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler) {
    XIOErrorHandler old = _current_io_error_handler;
    _current_io_error_handler = handler;
    return old;
}

/* ── Event handling ── */

int XPending(Display *display) {
    return _evt_count;
}

int XNextEvent(Display *display, XEvent *event_return) {
    if (event_return && _pop_event(event_return)) {
        return 0;
    }
    /* No events — return a no-op event */
    if (event_return) memset(event_return, 0, sizeof(XEvent));
    return 0;
}

int XEventsQueued(Display *display, int mode) {
    return _evt_count;
}

int XSelectInput(Display *display, Window w, long event_mask) {
    TrackedWindow *tw = _find_window(w);
    if (tw) tw->event_mask = event_mask;
    return 0;
}

Status XSendEvent(Display *display, Window w, Bool propagate,
    long event_mask, XEvent *event_send)
{
    if (event_send) _push_event(event_send);
    return 1;
}

/* ── Window geometry / movement ── */

int XMoveResizeWindow(Display *display, Window w,
    int x, int y, unsigned int width, unsigned int height)
{
    TrackedWindow *tw = _find_window(w);
    if (tw) {
        tw->x = x;
        tw->y = y;
        if (width > 0) tw->width = width;
        if (height > 0) tw->height = height;
        _push_configure_notify(display, tw);
    }
    return 0;
}

int XResizeWindow(Display *display, Window w,
    unsigned int width, unsigned int height)
{
    TrackedWindow *tw = _find_window(w);
    if (tw) {
        if (width > 0) tw->width = width;
        if (height > 0) tw->height = height;
        _push_configure_notify(display, tw);
    }
    return 0;
}

int XMoveWindow(Display *display, Window w, int x, int y) {
    TrackedWindow *tw = _find_window(w);
    if (tw) {
        tw->x = x;
        tw->y = y;
        _push_configure_notify(display, tw);
    }
    return 0;
}

int XRaiseWindow(Display *display, Window w) { return 0; }
int XLowerWindow(Display *display, Window w) { return 0; }

int XConfigureWindow(Display *display, Window w,
    unsigned int value_mask, XWindowChanges *changes)
{
    if (!changes) return 0;
    TrackedWindow *tw = _find_window(w);
    if (tw) {
        if (value_mask & CWX) tw->x = changes->x;
        if (value_mask & CWY) tw->y = changes->y;
        if (value_mask & CWWidth) tw->width = changes->width;
        if (value_mask & CWHeight) tw->height = changes->height;
        if (value_mask & CWBorderWidth) tw->border_width = changes->border_width;
        _push_configure_notify(display, tw);
    }
    return 0;
}

Status XGetGeometry(Display *display, Drawable d,
    Window *root_return, int *x_return, int *y_return,
    unsigned int *width_return, unsigned int *height_return,
    unsigned int *border_width_return, unsigned int *depth_return)
{
    _ensure_init();
    TrackedWindow *tw = _find_window(d);

    if (root_return) *root_return = 1;
    if (tw) {
        if (x_return) *x_return = tw->x;
        if (y_return) *y_return = tw->y;
        if (width_return) *width_return = tw->width;
        if (height_return) *height_return = tw->height;
        if (border_width_return) *border_width_return = tw->border_width;
        if (depth_return) *depth_return = tw->depth;
    } else {
        if (x_return) *x_return = 0;
        if (y_return) *y_return = 0;
        if (width_return) *width_return = _screen_width;
        if (height_return) *height_return = _screen_height;
        if (border_width_return) *border_width_return = 0;
        if (depth_return) *depth_return = 32;
    }
    return 1;
}

Bool XTranslateCoordinates(Display *display,
    Window src_w, Window dest_w, int src_x, int src_y,
    int *dest_x_return, int *dest_y_return, Window *child_return)
{
    if (dest_x_return) *dest_x_return = src_x;
    if (dest_y_return) *dest_y_return = src_y;
    if (child_return) *child_return = 0;
    return True;
}

Status XMatchVisualInfo(Display *display, int screen,
    int depth, int class, XVisualInfo *vinfo_return)
{
    if (!vinfo_return) return 0;
    _ensure_init();
    vinfo_return->visual = &_stub_visual;
    vinfo_return->visualid = _visual_id;
    vinfo_return->screen = 0;
    vinfo_return->depth = depth > 0 ? depth : 24;
    vinfo_return->class = class;
    vinfo_return->red_mask   = 0x00FF0000;
    vinfo_return->green_mask = 0x0000FF00;
    vinfo_return->blue_mask  = 0x000000FF;
    vinfo_return->colormap_size = 256;
    vinfo_return->bits_per_rgb = 8;
    return 1;
}

int XDefaultDepth(Display *display, int screen) {
    _ensure_init();
    if (display && screen >= 0 && screen < display->nscreens)
        return display->screens[screen].root_depth;
    return 24;
}

int XDefaultScreen(Display *display) {
    if (display) return display->default_screen;
    return 0;
}

int XScreenCount(Display *display) {
    return 1;
}


/* ── Atom / Property stubs ── */

Atom XInternAtom(Display *display, const char *atom_name, Bool only_if_exists) {
    if (!atom_name) return None;

    /* Search existing atoms */
    for (Atom i = 100; i < _next_atom_id; i++) {
        int idx = (int)(i - 100);
        if (idx < MAX_ATOMS && strcmp(_atom_names[idx].name, atom_name) == 0)
            return i;
    }

    if (only_if_exists) return None;

    /* Register new atom */
    int idx = (int)(_next_atom_id - 100);
    if (idx >= MAX_ATOMS) return None;

    Atom id = _next_atom_id++;
    strncpy(_atom_names[idx].name, atom_name, 127);
    _atom_names[idx].name[127] = '\0';
    return id;
}

char *XGetAtomName(Display *display, Atom atom) {
    if (atom >= 100) {
        int idx = (int)(atom - 100);
        if (idx < MAX_ATOMS && _atom_names[idx].name[0] != '\0') {
            char *copy = (char *)malloc(strlen(_atom_names[idx].name) + 1);
            if (copy) strcpy(copy, _atom_names[idx].name);
            return copy;
        }
    }
    /* Return a dummy name for predefined atoms */
    char *name = (char *)malloc(32);
    if (name) snprintf(name, 32, "ATOM_%lu", atom);
    return name;
}

int XChangeProperty(Display *display, Window w, Atom property,
    Atom type, int format, int mode, const unsigned char *data, int nelements)
{
    return 0; /* no-op — properties are not stored */
}

int XGetWindowProperty(Display *display, Window w, Atom property,
    long long_offset, long long_length, Bool delete,
    Atom req_type, Atom *actual_type_return, int *actual_format_return,
    unsigned long *nitems_return, unsigned long *bytes_after_return,
    unsigned char **prop_return)
{
    /* Report "property not found" */
    if (actual_type_return) *actual_type_return = None;
    if (actual_format_return) *actual_format_return = 0;
    if (nitems_return) *nitems_return = 0;
    if (bytes_after_return) *bytes_after_return = 0;
    if (prop_return) *prop_return = NULL;
    return Success;
}

int XDeleteProperty(Display *display, Window w, Atom property) {
    return 0;
}

Atom *XListProperties(Display *display, Window w, int *num_prop_return) {
    if (num_prop_return) *num_prop_return = 0;
    return NULL;
}

/* ── Window tree / reparent ── */

Status XQueryTree(Display *display, Window w,
    Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return)
{
    if (root_return) *root_return = 1;
    if (parent_return) *parent_return = 1;
    if (children_return) *children_return = NULL;
    if (nchildren_return) *nchildren_return = 0;
    return 1;
}

int XReparentWindow(Display *display, Window w, Window parent, int x, int y) {
    TrackedWindow *tw = _find_window(w);
    if (tw) { tw->x = x; tw->y = y; }
    return 0;
}

/* ── Window naming ── */

int XStoreName(Display *display, Window w, const char *window_name) {
    TrackedWindow *tw = _find_window(w);
    if (tw && window_name) {
        strncpy(tw->name, window_name, 255);
        tw->name[255] = '\0';
    } else if (tw) {
        tw->name[0] = '\0';
    }
    return 0;
}

int XSetIconName(Display *display, Window w, const char *icon_name) {
    return 0;
}

Status XFetchName(Display *display, Window w, char **window_name_return) {
    if (!window_name_return) return 0;
    TrackedWindow *tw = _find_window(w);
    if (tw && tw->name[0] != '\0') {
        *window_name_return = (char *)malloc(strlen(tw->name) + 1);
        if (*window_name_return) {
            strcpy(*window_name_return, tw->name);
            return 1;
        }
    }
    *window_name_return = NULL;
    return 0;
}

Status XGetWMName(Display *display, Window w, XTextProperty *text_prop_return) {
    if (!text_prop_return) return 0;
    TrackedWindow *tw = _find_window(w);
    if (tw && tw->name[0] != '\0') {
        size_t len = strlen(tw->name);
        text_prop_return->value = (unsigned char *)malloc(len + 1);
        if (text_prop_return->value) {
            memcpy(text_prop_return->value, tw->name, len + 1);
            text_prop_return->encoding = XA_STRING;
            text_prop_return->format = 8;
            text_prop_return->nitems = len;
            return 1;
        }
    }
    memset(text_prop_return, 0, sizeof(XTextProperty));
    return 0;
}

void XSetWMName(Display *display, Window w, XTextProperty *text_prop) {
    if (text_prop && text_prop->value) {
        XStoreName(display, w, (const char *)text_prop->value);
    }
}

void XSetWMProperties(Display *display, Window w,
    XTextProperty *window_name, XTextProperty *icon_name,
    char **argv, int argc, XSizeHints *normal_hints,
    void *wm_hints, void *class_hints)
{
    if (window_name && window_name->value) {
        XStoreName(display, w, (const char *)window_name->value);
    }
}

void XSetWMIconName(Display *display, Window w, XTextProperty *text_prop) {
    /* no-op */
}

/* ── Input focus ── */

int XSetInputFocus(Display *display, Window focus, int revert_to, Time time) {
    return 0;
}

int XGetInputFocus(Display *display, Window *focus_return, int *revert_to_return) {
    if (focus_return) *focus_return = 1; /* root window */
    if (revert_to_return) *revert_to_return = RevertToPointerRoot;
    return 0;
}

/* ── Server grab ── */

int XGrabServer(Display *display) { return 0; }
int XUngrabServer(Display *display) { return 0; }

/* ── Pointer ── */

Bool XQueryPointer(Display *display, Window w,
    Window *root_return, Window *child_return,
    int *root_x_return, int *root_y_return,
    int *win_x_return, int *win_y_return, unsigned int *mask_return)
{
    if (root_return) *root_return = 1;
    if (child_return) *child_return = 0;
    if (root_x_return) *root_x_return = 0;
    if (root_y_return) *root_y_return = 0;
    if (win_x_return) *win_x_return = 0;
    if (win_y_return) *win_y_return = 0;
    if (mask_return) *mask_return = 0;
    return True;
}

int XWarpPointer(Display *display, Window src_w, Window dest_w,
    int src_x, int src_y, unsigned int src_width, unsigned int src_height,
    int dest_x, int dest_y)
{
    return 0;
}

int XGrabPointer(Display *display, Window grab_window,
    Bool owner_events, unsigned int event_mask,
    int pointer_mode, int keyboard_mode,
    Window confine_to, Cursor cursor, Time time)
{
    return GrabSuccess;
}

int XUngrabPointer(Display *display, Time time) { return 0; }

/* ── Keyboard ── */

int XGrabKeyboard(Display *display, Window grab_window,
    Bool owner_events, int pointer_mode, int keyboard_mode, Time time)
{
    return GrabSuccess;
}

int XUngrabKeyboard(Display *display, Time time) { return 0; }

/* ── Keyboard mapping ── */

KeyCode XKeysymToKeycode(Display *display, KeySym keysym) {
    return 0;
}

KeySym XKeycodeToKeysym(Display *display, KeyCode keycode, int index) {
    return 0; /* NoSymbol */
}

int XLookupString(XKeyEvent *event_struct, char *buffer_return,
    int bytes_buffer, KeySym *keysym_return, void *status_in_out)
{
    if (keysym_return) *keysym_return = 0;
    if (buffer_return && bytes_buffer > 0) buffer_return[0] = '\0';
    return 0;
}

KeySym XLookupKeysym(XKeyEvent *key_event, int index) {
    return 0; /* NoSymbol */
}

void XDisplayKeycodes(Display *display, int *min_keycodes_return, int *max_keycodes_return) {
    if (min_keycodes_return) *min_keycodes_return = 8;
    if (max_keycodes_return) *max_keycodes_return = 255;
}

KeySym *XGetKeyboardMapping(Display *display, KeyCode first_keycode,
    int keycode_count, int *keysyms_per_keycode_return)
{
    if (keysyms_per_keycode_return) *keysyms_per_keycode_return = 1;
    int count = keycode_count > 0 ? keycode_count : 1;
    KeySym *mapping = (KeySym *)calloc(count, sizeof(KeySym));
    return mapping;
}

int XChangeKeyboardMapping(Display *display, int first_keycode,
    int keysyms_per_keycode, KeySym *keysyms, int num_codes)
{
    return 0;
}

/* ── Cursor ── */

Cursor XCreateFontCursor(Display *display, unsigned int shape) {
    return 1;
}

int XDefineCursor(Display *display, Window w, Cursor cursor) { return 0; }
int XUndefineCursor(Display *display, Window w) { return 0; }
int XFreeCursor(Display *display, Cursor cursor) { return 0; }

/* ── Hints ── */

void XSetWMNormalHints(Display *display, Window w, XSizeHints *hints) {
    /* no-op */
}

int XSetTransientForHint(Display *display, Window w, Window prop_window) {
    return 0;
}

Status XGetTransientForHint(Display *display, Window w, Window *prop_window_return) {
    if (prop_window_return) *prop_window_return = None;
    return 0;
}

XSizeHints *XAllocSizeHints(void) {
    return (XSizeHints *)calloc(1, sizeof(XSizeHints));
}

XWMHints *XAllocWMHints(void) {
    return (XWMHints *)calloc(1, sizeof(XWMHints));
}

XClassHint *XAllocClassHints(void) {
    return (XClassHint *)calloc(1, sizeof(XClassHint));
}

void XSetWMHints(Display *display, Window w, XWMHints *wm_hints) {
    /* no-op */
}

XWMHints *XGetWMHints(Display *display, Window w) {
    XWMHints *hints = XAllocWMHints();
    if (hints) {
        hints->flags = InputHint | StateHint;
        hints->input = True;
        hints->initial_state = NormalState;
    }
    return hints;
}

void XSetClassHint(Display *display, Window w, XClassHint *class_hints) {
    /* no-op */
}

Status XGetClassHint(Display *display, Window w, XClassHint *class_hints_return) {
    if (class_hints_return) {
        class_hints_return->res_name = NULL;
        class_hints_return->res_class = NULL;
    }
    return 0;
}

/* ── Clipboard / Selection ── */

int XSetSelectionOwner(Display *display, Atom selection, Window owner, Time time) {
    return 0;
}

Window XGetSelectionOwner(Display *display, Atom selection) {
    return None;
}

int XConvertSelection(Display *display, Atom selection, Atom target,
    Atom property, Window requestor, Time time)
{
    return 0;
}

/* ── Threading ── */

Status XInitThreads(void) { return 1; }
void XLockDisplay(Display *display) { }
void XUnlockDisplay(Display *display) { }

/* ── Locale ── */

Bool XSupportsLocale(void) { return True; }

char *XSetLocaleModifiers(const char *modifier_list) {
    static char empty[] = "";
    return empty;
}

/* ── Extensions ── */

char **XListExtensions(Display *display, int *nextensions_return) {
    /* Advertise GLX so Wine's x11drv knows GL is available */
    char **list = (char **)malloc(sizeof(char *) * 2);
    if (list) {
        list[0] = (char *)malloc(4);
        if (list[0]) strcpy(list[0], "GLX");
        list[1] = NULL; /* null-terminate for XFreeExtensionList */
        if (nextensions_return) *nextensions_return = 1;
    } else {
        if (nextensions_return) *nextensions_return = 0;
    }
    return list;
}

int XFreeExtensionList(char **list) {
    if (list) {
        for (int i = 0; list[i]; i++) free(list[i]);
        free(list);
    }
    return 0;
}

Bool XQueryExtension(Display *display, const char *name,
    int *major_opcode_return, int *first_event_return, int *first_error_return)
{
    if (name && strcmp(name, "GLX") == 0) {
        if (major_opcode_return) *major_opcode_return = 151;
        if (first_event_return) *first_event_return = 0;
        if (first_error_return) *first_error_return = 0;
        return True;
    }
    if (major_opcode_return) *major_opcode_return = 0;
    if (first_event_return) *first_event_return = 0;
    if (first_error_return) *first_error_return = 0;
    return False;
}

/* ── Connection / Display info functions ── */

int XConnectionNumber(Display *display) {
    if (display) return display->fd;
    return -1;
}

char *XDisplayString(Display *display) {
    if (display) return display->vendor;
    return "X11Stub";
}

unsigned long XLastKnownRequestProcessed(Display *display) {
    return 0;
}

unsigned long XNextRequest(Display *display) {
    return 1;
}

char *XServerVendor(Display *display) {
    return "X11Stub";
}

int XProtocolVersion(Display *display) {
    return 11;
}

int XProtocolRevision(Display *display) {
    return 0;
}

int XVendorRelease(Display *display) {
    return 0;
}

int XImageByteOrder(Display *display) {
    return 0; /* LSBFirst */
}

int XBitmapBitOrder(Display *display) {
    return 0; /* LSBFirst */
}

int XBitmapUnit(Display *display) {
    return 32;
}

int XBitmapPad(Display *display) {
    return 32;
}

int XDisplayCells(Display *display, int screen_number) {
    return 256;
}

int XDisplayPlanes(Display *display, int screen_number) {
    return 24;
}

int XDisplayWidthMM(Display *display, int screen_number) {
    _ensure_init();
    return _screen_width * 340 / 1280;
}

int XDisplayHeightMM(Display *display, int screen_number) {
    _ensure_init();
    return _screen_height * 190 / 720;
}

unsigned long XBlackPixel(Display *display, int screen_number) {
    return 0x000000;
}

unsigned long XWhitePixel(Display *display, int screen_number) {
    return 0xFFFFFF;
}

/* ── Window background / border ── */

int XSetWindowBackground(Display *display, Window w, unsigned long background_pixel) {
    return 0;
}

int XSetWindowBorder(Display *display, Window w, unsigned long border_pixel) {
    return 0;
}

int XSetWindowBorderWidth(Display *display, Window w, unsigned int width) {
    TrackedWindow *tw = _find_window(w);
    if (tw) tw->border_width = width;
    return 0;
}

int XClearWindow(Display *display, Window w) {
    return 0;
}

int XClearArea(Display *display, Window w, int x, int y,
    unsigned int width, unsigned int height, Bool exposures)
{
    if (exposures) {
        TrackedWindow *tw = _find_window(w);
        if (tw) {
            XEvent ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = Expose;
            ev.xexpose.display = display;
            ev.xexpose.window = w;
            ev.xexpose.x = x;
            ev.xexpose.y = y;
            ev.xexpose.width = width > 0 ? width : (tw->width - x);
            ev.xexpose.height = height > 0 ? height : (tw->height - y);
            ev.xexpose.count = 0;
            _push_event(&ev);
        }
    }
    return 0;
}

int XSetCloseDownMode(Display *display, int close_mode) {
    return 0;
}

int XBell(Display *display, int percent) {
    return 0;
}

/* ── Text property helpers ── */

Status XStringListToTextProperty(char **list, int count, XTextProperty *text_prop_return) {
    if (!text_prop_return || !list || count <= 0) return 0;
    size_t total = 0;
    for (int i = 0; i < count; i++) {
        total += list[i] ? strlen(list[i]) : 0;
        if (i < count - 1) total++;
    }
    unsigned char *buf = (unsigned char *)malloc(total + 1);
    if (!buf) return 0;
    size_t pos = 0;
    for (int i = 0; i < count; i++) {
        if (list[i]) {
            size_t len = strlen(list[i]);
            memcpy(buf + pos, list[i], len);
            pos += len;
        }
        if (i < count - 1) buf[pos++] = '\0';
    }
    buf[pos] = '\0';
    text_prop_return->value = buf;
    text_prop_return->encoding = XA_STRING;
    text_prop_return->format = 8;
    text_prop_return->nitems = total;
    return 1;
}

Status XTextPropertyToStringList(XTextProperty *text_prop, char ***list_return, int *count_return) {
    if (!text_prop || !list_return || !count_return) return 0;
    if (!text_prop->value || text_prop->nitems == 0) {
        *list_return = NULL;
        *count_return = 0;
        return 1;
    }
    int count = 1;
    for (unsigned long i = 0; i < text_prop->nitems; i++) {
        if (text_prop->value[i] == '\0') count++;
    }
    char **list = (char **)malloc(sizeof(char *) * (count + 1));
    if (!list) return 0;
    const char *start = (const char *)text_prop->value;
    int idx = 0;
    for (int i = 0; i < count && idx < count; i++) {
        size_t len = strlen(start);
        list[idx] = (char *)malloc(len + 1);
        if (list[idx]) strcpy(list[idx], start);
        start += len + 1;
        idx++;
    }
    list[idx] = NULL;
    *list_return = list;
    *count_return = idx;
    return 1;
}

void XFreeStringList(char **list) {
    if (list) {
        for (int i = 0; list[i]; i++) free(list[i]);
        free(list);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * Font / Pixmap / Image stubs (needed by GL4ES's glXUseXFont)
 * ══════════════════════════════════════════════════════════════════════════ */

static XCharStruct _stub_char_bounds = { 0, 8, 8, 12, 0, 0 };
static XFontStruct _stub_font_struct = {
    .ext_data = NULL,
    .fid = 1,
    .direction = 0,
    .min_char_or_byte2 = 32,
    .max_char_or_byte2 = 126,
    .min_byte1 = 0,
    .max_byte1 = 0,
    .all_chars_exist = True,
    .default_char = 32,
    .n_properties = 0,
    .properties = NULL,
    .min_bounds = { 0, 8, 8, 12, 0, 0 },
    .max_bounds = { 0, 8, 8, 12, 0, 0 },
    .per_char = NULL,
    .ascent = 12,
    .descent = 4,
};

XFontStruct *XQueryFont(Display *display, XID font_ID) {
    return &_stub_font_struct;
}

XFontStruct *XLoadQueryFont(Display *display, const char *name) {
    return &_stub_font_struct;
}

int XFreeFont(Display *display, XFontStruct *font_struct) {
    return 0;
}

int XTextWidth(XFontStruct *font_struct, const char *string, int count) {
    return count * 8;
}

int XFreeFontInfo(char **names, XFontStruct *free_info, int actual_count) {
    return 0;
}

Pixmap XCreatePixmap(Display *display, Drawable d,
    unsigned int width, unsigned int height, unsigned int depth)
{
    static Pixmap next_pixmap = 200;
    return next_pixmap++;
}

int XFreePixmap(Display *display, Pixmap pixmap) { return 0; }

int XSetForeground(Display *display, GC gc, unsigned long foreground) {
    return 0;
}

int XFillRectangle(Display *display, Drawable d, GC gc,
    int x, int y, unsigned int width, unsigned int height)
{
    return 0;
}

int XDrawString16(Display *display, Drawable d, GC gc,
    int x, int y, const XChar2b *string, int length)
{
    return 0;
}

/* Stub XImage helpers */
static unsigned long _stub_get_pixel(XImage *image, int x, int y) {
    return 0;
}

static int _stub_put_pixel(XImage *image, int x, int y, unsigned long pixel) {
    return 0;
}

static int _stub_destroy_image(XImage *image) {
    if (image && image->data) {
        free(image->data);
        image->data = NULL;
    }
    free(image);
    return 0;
}

XImage *XGetImage(Display *display, Drawable d,
    int x, int y, unsigned int width, unsigned int height,
    unsigned long plane_mask, int format)
{
    XImage *img = (XImage *)calloc(1, sizeof(XImage));
    if (!img) return NULL;
    img->width = width;
    img->height = height;
    img->depth = 1;
    img->format = format;
    img->bytes_per_line = (width + 7) / 8;
    img->bits_per_pixel = 1;
    img->data = (char *)calloc(img->bytes_per_line * height, 1);
    img->f.get_pixel = _stub_get_pixel;
    img->f.put_pixel = _stub_put_pixel;
    img->f.destroy_image = _stub_destroy_image;
    return img;
}

static struct _XGC { int dummy; } _stub_gc_storage = { 0 };

GC XCreateGC(Display *display, Drawable d,
    unsigned long valuemask, XGCValues *values)
{
    return (GC)&_stub_gc_storage;
}

int XFreeGC(Display *display, GC gc) { return 0; }

int XPutImage(Display *display, Drawable d, GC gc,
    XImage *image, int src_x, int src_y, int dest_x, int dest_y,
    unsigned int width, unsigned int height)
{
    return 0;
}

XImage *XCreateImage(Display *display, Visual *visual,
    unsigned int depth, int format, int offset,
    char *data, unsigned int width, unsigned int height,
    int bitmap_pad, int bytes_per_line)
{
    XImage *img = (XImage *)calloc(1, sizeof(XImage));
    if (!img) return NULL;
    img->width = width;
    img->height = height;
    img->xoffset = offset;
    img->format = format;
    img->depth = depth;
    img->bitmap_pad = bitmap_pad;
    img->bits_per_pixel = (depth <= 8) ? 8 : ((depth <= 16) ? 16 : 32);
    if (bytes_per_line > 0) {
        img->bytes_per_line = bytes_per_line;
    } else {
        int bpl = (width * img->bits_per_pixel + 7) / 8;
        int pad = bitmap_pad / 8;
        if (pad > 0) bpl = ((bpl + pad - 1) / pad) * pad;
        img->bytes_per_line = bpl;
    }
    img->data = data;
    img->red_mask   = (visual && visual->red_mask)   ? visual->red_mask   : 0x00FF0000;
    img->green_mask = (visual && visual->green_mask) ? visual->green_mask : 0x0000FF00;
    img->blue_mask  = (visual && visual->blue_mask)  ? visual->blue_mask  : 0x000000FF;
    img->f.get_pixel = _stub_get_pixel;
    img->f.put_pixel = _stub_put_pixel;
    img->f.destroy_image = _stub_destroy_image;
    return img;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Xrandr stubs (gl4es may query screen resolution)
 * ══════════════════════════════════════════════════════════════════════════ */

static XRRScreenSize _stub_screen_size;
static XRRScreenConfiguration _stub_xrr_config;
static int _xrr_inited = 0;

static void _ensure_xrr_init(void) {
    if (_xrr_inited) return;
    _xrr_inited = 1;
    _ensure_init();
    _stub_screen_size.width = _screen_width;
    _stub_screen_size.height = _screen_height;
    _stub_screen_size.mwidth = _screen_width * 340 / 1280;
    _stub_screen_size.mheight = _screen_height * 190 / 720;
}

XRRScreenConfiguration *XRRGetScreenInfo(Display *dpy, Window window) {
    _ensure_xrr_init();
    return &_stub_xrr_config;
}

void XRRFreeScreenConfigInfo(XRRScreenConfiguration *config) { }

XRRScreenSize *XRRConfigSizes(XRRScreenConfiguration *config, int *nsizes) {
    _ensure_xrr_init();
    if (nsizes) *nsizes = 1;
    return &_stub_screen_size;
}

SizeID XRRConfigCurrentConfiguration(XRRScreenConfiguration *config, Rotation *rotation) {
    if (rotation) *rotation = 1;
    return 0;
}

short XRRConfigCurrentRate(XRRScreenConfiguration *config) {
    return 60;
}

Status XRRSetScreenConfigAndRate(Display *dpy, XRRScreenConfiguration *config,
    Drawable draw, int size_index, Rotation rotation, short rate, Time timestamp)
{
    return 0;
}
