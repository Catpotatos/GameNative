/*
 * Minimal Xlib type definitions for gl4es cross-compilation on Android/Bionic.
 * Only the types and functions needed by gl4es's GLX implementation are defined.
 * Based on the X11 public API (MIT/X11 License).
 */
#ifndef _XLIB_H_
#define _XLIB_H_

#include <X11/X.h>
#include <stddef.h>

/* Forward declarations */
typedef struct _XDisplay Display;
typedef struct _XGC *GC;
typedef struct _XExtData XExtData;

typedef struct {
    XExtData *ext_data;
    VisualID visualid;
    int class;
    unsigned long red_mask, green_mask, blue_mask;
    int bits_per_rgb;
    int map_entries;
} Visual;

typedef struct {
    int depth;
    int nvisuals;
    Visual *visuals;
} Depth;

typedef struct {
    XExtData *ext_data;
    Display *display;
    Window root;
    int width, height;
    int mwidth, mheight;
    int ndepths;
    Depth *depths;
    int root_depth;
    Visual *root_visual;
    GC default_gc;
    Colormap cmap;
    unsigned long white_pixel;
    unsigned long black_pixel;
    int max_maps, min_maps;
    int backing_store;
    Bool save_unders;
    long root_input_mask;
} Screen;

typedef struct _XDisplay {
    XExtData *ext_data;
    int fd;
    int default_screen;
    int nscreens;
    Screen *screens;
    int proto_major_version;
    int proto_minor_version;
    char *vendor;
    /* Remaining fields omitted — gl4es only accesses the above through macros */
} Display;

typedef struct {
    Visual *visual;
    VisualID visualid;
    int screen;
    int depth;
    int class;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    int colormap_size;
    int bits_per_rgb;
} XVisualInfo;

typedef struct {
    int x, y;
    int width, height;
    int border_width;
    int depth;
    Visual *visual;
    Window root;
    int class;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    Bool save_under;
    Colormap colormap;
    Bool map_installed;
    int map_state;
    long all_event_masks;
    long your_event_mask;
    long do_not_propagate_mask;
    Bool override_redirect;
    Screen *screen;
} XWindowAttributes;

typedef struct {
    unsigned long background_pixmap;
    unsigned long background_pixel;
    unsigned long border_pixmap;
    unsigned long border_pixel;
    int bit_gravity;
    int win_gravity;
    int backing_store;
    unsigned long backing_planes;
    unsigned long backing_pixel;
    Bool save_under;
    long event_mask;
    long do_not_propagate_mask;
    Bool override_redirect;
    Colormap colormap;
    Cursor cursor;
} XSetWindowAttributes;

/* XWindowChanges — used by XConfigureWindow */
typedef struct {
    int x, y;
    int width, height;
    int border_width;
    Window sibling;
    int stack_mode;
} XWindowChanges;

/* ── Event structures ── */

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int x, y;
    int width, height;
    int count;
} XExposeEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window event;
    Window window;
    int x, y;
    int width, height;
    int border_width;
    Window above;
    Bool override_redirect;
} XConfigureEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window event;
    Window window;
    Bool override_redirect;
} XMapEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window event;
    Window window;
    Bool from_configure;
} XUnmapEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Atom message_type;
    int format;
    union {
        char b[20];
        short s[10];
        long l[5];
    } data;
} XClientMessageEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    int mode;
    int detail;
} XFocusChangeEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    unsigned int keycode;
    Bool same_screen;
} XKeyEvent;
typedef XKeyEvent XKeyPressedEvent;
typedef XKeyEvent XKeyReleasedEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    unsigned int button;
    Bool same_screen;
} XButtonEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Window root;
    Window subwindow;
    Time time;
    int x, y;
    int x_root, y_root;
    unsigned int state;
    char is_hint;
    Bool same_screen;
} XMotionEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Atom atom;
    Time time;
    int state;
} XPropertyEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window window;
    Atom selection;
    Time time;
} XSelectionClearEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window owner;
    Window requestor;
    Atom selection;
    Atom target;
    Atom property;
    Time time;
} XSelectionRequestEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window requestor;
    Atom selection;
    Atom target;
    Atom property;
    Time time;
} XSelectionEvent;

typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    Window event;
    Window window;
} XDestroyWindowEvent;

typedef union _XEvent {
    int type;
    XExposeEvent xexpose;
    XConfigureEvent xconfigure;
    XMapEvent xmap;
    XUnmapEvent xunmap;
    XClientMessageEvent xclient;
    XFocusChangeEvent xfocus;
    XKeyEvent xkey;
    XButtonEvent xbutton;
    XMotionEvent xmotion;
    XPropertyEvent xproperty;
    XSelectionClearEvent xselectionclear;
    XSelectionRequestEvent xselectionrequest;
    XSelectionEvent xselection;
    XDestroyWindowEvent xdestroywindow;
    long pad[24];
} XEvent;

/* ── Error handling ── */

typedef struct {
    int type;
    Display *display;
    XID resourceid;
    unsigned long serial;
    unsigned char error_code;
    unsigned char request_code;
    unsigned char minor_code;
} XErrorEvent;

typedef int (*XErrorHandler)(Display *, XErrorEvent *);
typedef int (*XIOErrorHandler)(Display *);

/* ── Hints / Properties ── */

typedef struct {
    long flags;
    int x, y;
    int width, height;
    int min_width, min_height;
    int max_width, max_height;
    int width_inc, height_inc;
    struct { int x; int y; } min_aspect, max_aspect;
    int base_width, base_height;
    int win_gravity;
} XSizeHints;

typedef struct {
    unsigned char *value;
    Atom encoding;
    int format;
    unsigned long nitems;
} XTextProperty;

/* XImage structure — needed by GL4ES's glXUseXFont / utils.c */
typedef struct _XImage {
    int width, height;
    int xoffset;
    int format;
    char *data;
    int byte_order;
    int bitmap_unit;
    int bitmap_bit_order;
    int bitmap_pad;
    int depth;
    int bytes_per_line;
    int bits_per_pixel;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    /* function pointers (stubs) */
    struct funcs {
        struct _XImage *(*create_image)();
        int (*destroy_image)(struct _XImage *);
        unsigned long (*get_pixel)(struct _XImage *, int, int);
        int (*put_pixel)(struct _XImage *, int, int, unsigned long);
        struct _XImage *(*sub_image)();
        int (*add_pixel)(struct _XImage *, long);
    } f;
} XImage;

/* Font-related types — needed by GL4ES's glXUseXFont */
typedef struct {
    short lbearing;
    short rbearing;
    short width;
    short ascent;
    short descent;
    unsigned short attributes;
} XCharStruct;

typedef struct {
    unsigned char byte1;
    unsigned char byte2;
} XChar2b;

typedef struct {
    XExtData *ext_data;
    Font fid;
    unsigned direction;
    unsigned min_char_or_byte2;
    unsigned max_char_or_byte2;
    unsigned min_byte1;
    unsigned max_byte1;
    Bool all_chars_exist;
    unsigned default_char;
    int n_properties;
    void *properties;  /* XFontProp* */
    XCharStruct min_bounds;
    XCharStruct max_bounds;
    XCharStruct *per_char;
    int ascent;
    int descent;
} XFontStruct;

/* GC values — needed by GL4ES utils.c */
typedef struct {
    int function;
    unsigned long plane_mask;
    unsigned long foreground;
    unsigned long background;
    int line_width;
    int line_style;
    int cap_style;
    int join_style;
    int fill_style;
    int fill_rule;
    int arc_mode;
    Pixmap tile;
    Pixmap stipple;
    int ts_x_origin;
    int ts_y_origin;
    Font font;
    int subwindow_mode;
    Bool graphics_exposures;
    int clip_x_origin;
    int clip_y_origin;
    Pixmap clip_mask;
    int dash_offset;
    char dashes;
} XGCValues;

/* XGetPixel macro — accesses the XImage function table */
#define XGetPixel(ximage, x, y) \
    ((*((ximage)->f.get_pixel))((ximage), (x), (y)))
#define XDestroyImage(ximage) \
    ((*((ximage)->f.destroy_image))((ximage)))

/* ── Function declarations ── */

/* Core display/window functions */
extern Display *XOpenDisplay(const char *display_name);
extern int XCloseDisplay(Display *display);
extern int XFree(void *data);
extern int XSync(Display *display, Bool discard);
extern int XFlush(Display *display);
extern Window XCreateWindow(Display *display, Window parent,
    int x, int y, unsigned int width, unsigned int height,
    unsigned int border_width, int depth, unsigned int class,
    Visual *visual, unsigned long valuemask,
    XSetWindowAttributes *attributes);
extern int XDestroyWindow(Display *display, Window w);
extern int XMapWindow(Display *display, Window w);
extern int XUnmapWindow(Display *display, Window w);
extern Status XGetWindowAttributes(Display *display, Window w,
    XWindowAttributes *window_attributes_return);
extern XVisualInfo *XGetVisualInfo(Display *display, long vinfo_mask,
    XVisualInfo *vinfo_template, int *nitems_return);
extern int XDefaultScreen(Display *display);
extern int XDefaultDepth(Display *display, int screen);
extern Colormap XCreateColormap(Display *display, Window w,
    Visual *visual, int alloc);
extern int XFreeColormap(Display *display, Colormap colormap);
extern int XPending(Display *display);
extern int XNextEvent(Display *display, XEvent *event_return);
extern int XMoveResizeWindow(Display *display, Window w,
    int x, int y, unsigned int width, unsigned int height);
extern Status XMatchVisualInfo(Display *display, int screen,
    int depth, int class, XVisualInfo *vinfo_return);
extern int XScreenCount(Display *display);
extern void XSetWMNormalHints(Display *display, Window w, XSizeHints *hints);

/* Error handling (proper signatures) */
extern XErrorHandler XSetErrorHandler(XErrorHandler handler);
extern XIOErrorHandler XSetIOErrorHandler(XIOErrorHandler handler);

/* Atom / Property functions */
extern Atom XInternAtom(Display *display, const char *atom_name, Bool only_if_exists);
extern char *XGetAtomName(Display *display, Atom atom);
extern int XChangeProperty(Display *display, Window w, Atom property,
    Atom type, int format, int mode, const unsigned char *data, int nelements);
extern int XGetWindowProperty(Display *display, Window w, Atom property,
    long long_offset, long long_length, Bool delete,
    Atom req_type, Atom *actual_type_return, int *actual_format_return,
    unsigned long *nitems_return, unsigned long *bytes_after_return,
    unsigned char **prop_return);
extern int XDeleteProperty(Display *display, Window w, Atom property);

/* Event handling */
extern int XSelectInput(Display *display, Window w, long event_mask);
extern Status XSendEvent(Display *display, Window w, Bool propagate,
    long event_mask, XEvent *event_send);
extern int XEventsQueued(Display *display, int mode);

/* Window configuration */
extern int XConfigureWindow(Display *display, Window w,
    unsigned int value_mask, XWindowChanges *changes);
extern int XReparentWindow(Display *display, Window w, Window parent, int x, int y);
extern Status XQueryTree(Display *display, Window w,
    Window *root_return, Window *parent_return,
    Window **children_return, unsigned int *nchildren_return);

/* Window geometry */
extern Status XGetGeometry(Display *display, Drawable d,
    Window *root_return, int *x_return, int *y_return,
    unsigned int *width_return, unsigned int *height_return,
    unsigned int *border_width_return, unsigned int *depth_return);
extern Bool XTranslateCoordinates(Display *display,
    Window src_w, Window dest_w, int src_x, int src_y,
    int *dest_x_return, int *dest_y_return, Window *child_return);

/* Window naming */
extern int XStoreName(Display *display, Window w, const char *window_name);
extern int XSetIconName(Display *display, Window w, const char *icon_name);

/* Input focus */
extern int XSetInputFocus(Display *display, Window focus, int revert_to, Time time);
extern int XGetInputFocus(Display *display, Window *focus_return, int *revert_to_return);

/* Server grab */
extern int XGrabServer(Display *display);
extern int XUngrabServer(Display *display);

/* Pointer */
extern Bool XQueryPointer(Display *display, Window w,
    Window *root_return, Window *child_return,
    int *root_x_return, int *root_y_return,
    int *win_x_return, int *win_y_return, unsigned int *mask_return);
extern int XWarpPointer(Display *display, Window src_w, Window dest_w,
    int src_x, int src_y, unsigned int src_width, unsigned int src_height,
    int dest_x, int dest_y);
extern int XGrabPointer(Display *display, Window grab_window,
    Bool owner_events, unsigned int event_mask,
    int pointer_mode, int keyboard_mode,
    Window confine_to, Cursor cursor, Time time);
extern int XUngrabPointer(Display *display, Time time);

/* Keyboard */
extern int XGrabKeyboard(Display *display, Window grab_window,
    Bool owner_events, int pointer_mode, int keyboard_mode, Time time);
extern int XUngrabKeyboard(Display *display, Time time);

/* Cursor */
extern Cursor XCreateFontCursor(Display *display, unsigned int shape);
extern int XDefineCursor(Display *display, Window w, Cursor cursor);
extern int XUndefineCursor(Display *display, Window w);
extern int XFreeCursor(Display *display, Cursor cursor);

/* Font functions */
extern XFontStruct *XQueryFont(Display *display, XID font_ID);
extern int XFreeFontInfo(char **names, XFontStruct *free_info, int actual_count);

/* Pixmap/Image functions */
extern Pixmap XCreatePixmap(Display *display, Drawable d,
    unsigned int width, unsigned int height, unsigned int depth);
extern int XFreePixmap(Display *display, Pixmap pixmap);
extern int XSetForeground(Display *display, GC gc, unsigned long foreground);
extern int XFillRectangle(Display *display, Drawable d, GC gc,
    int x, int y, unsigned int width, unsigned int height);
extern int XDrawString16(Display *display, Drawable d, GC gc,
    int x, int y, const XChar2b *string, int length);
extern XImage *XGetImage(Display *display, Drawable d,
    int x, int y, unsigned int width, unsigned int height,
    unsigned long plane_mask, int format);
extern GC XCreateGC(Display *display, Drawable d,
    unsigned long valuemask, XGCValues *values);
extern int XFreeGC(Display *display, GC gc);
extern int XPutImage(Display *display, Drawable d, GC gc,
    XImage *image, int src_x, int src_y, int dest_x, int dest_y,
    unsigned int width, unsigned int height);
extern XImage *XCreateImage(Display *display, Visual *visual,
    unsigned int depth, int format, int offset,
    char *data, unsigned int width, unsigned int height,
    int bitmap_pad, int bytes_per_line);

/* Threading */
extern Status XInitThreads(void);
extern void XLockDisplay(Display *display);
extern void XUnlockDisplay(Display *display);

/* Locale */
extern Bool XSupportsLocale(void);
extern char *XSetLocaleModifiers(const char *modifier_list);

/* Transient / WM hints */
extern int XSetTransientForHint(Display *display, Window w, Window prop_window);

/* Simple window creation */
extern Window XCreateSimpleWindow(Display *display, Window parent,
    int x, int y, unsigned int width, unsigned int height,
    unsigned int border_width, unsigned long border, unsigned long background);

/* Window resize/move */
extern int XResizeWindow(Display *display, Window w,
    unsigned int width, unsigned int height);
extern int XMoveWindow(Display *display, Window w, int x, int y);
extern int XRaiseWindow(Display *display, Window w);
extern int XLowerWindow(Display *display, Window w);
extern int XMapRaised(Display *display, Window w);
extern int XMapSubwindows(Display *display, Window w);
extern int XUnmapSubwindows(Display *display, Window w);

/* Window naming */
extern Status XFetchName(Display *display, Window w, char **window_name_return);
extern Status XGetWMName(Display *display, Window w, XTextProperty *text_prop_return);
extern void XSetWMName(Display *display, Window w, XTextProperty *text_prop);
extern void XSetWMProperties(Display *display, Window w,
    XTextProperty *window_name, XTextProperty *icon_name,
    char **argv, int argc, XSizeHints *normal_hints,
    void *wm_hints, void *class_hints);
extern void XSetWMIconName(Display *display, Window w, XTextProperty *text_prop);

/* Clipboard / Selection */
extern int XSetSelectionOwner(Display *display, Atom selection, Window owner, Time time);
extern Window XGetSelectionOwner(Display *display, Atom selection);
extern int XConvertSelection(Display *display, Atom selection, Atom target,
    Atom property, Window requestor, Time time);

/* Keyboard mapping */
extern KeyCode XKeysymToKeycode(Display *display, KeySym keysym);
extern KeySym XKeycodeToKeysym(Display *display, KeyCode keycode, int index);
extern int XLookupString(XKeyEvent *event_struct, char *buffer_return,
    int bytes_buffer, KeySym *keysym_return, void *status_in_out);
extern KeySym XLookupKeysym(XKeyEvent *key_event, int index);
extern void XDisplayKeycodes(Display *display, int *min_keycodes_return, int *max_keycodes_return);
extern KeySym *XGetKeyboardMapping(Display *display, KeyCode first_keycode,
    int keycode_count, int *keysyms_per_keycode_return);
extern int XChangeKeyboardMapping(Display *display, int first_keycode,
    int keysyms_per_keycode, KeySym *keysyms, int num_codes);

/* Extensions */
extern char **XListExtensions(Display *display, int *nextensions_return);
extern int XFreeExtensionList(char **list);
extern Bool XQueryExtension(Display *display, const char *name,
    int *major_opcode_return, int *first_event_return, int *first_error_return);

/* Connection / display info functions (complement macros) */
extern int XConnectionNumber(Display *display);
extern char *XDisplayString(Display *display);
extern unsigned long XLastKnownRequestProcessed(Display *display);
extern unsigned long XNextRequest(Display *display);
extern char *XServerVendor(Display *display);
extern int XProtocolVersion(Display *display);
extern int XProtocolRevision(Display *display);
extern int XVendorRelease(Display *display);
extern int XImageByteOrder(Display *display);
extern int XBitmapBitOrder(Display *display);
extern int XBitmapUnit(Display *display);
extern int XBitmapPad(Display *display);
extern int XDisplayCells(Display *display, int screen_number);
extern int XDisplayPlanes(Display *display, int screen_number);
extern int XDisplayWidthMM(Display *display, int screen_number);
extern int XDisplayHeightMM(Display *display, int screen_number);

/* Colormap */
extern int XAllocColor(Display *display, Colormap colormap, void *screen_in_out);
extern unsigned long XBlackPixel(Display *display, int screen_number);
extern unsigned long XWhitePixel(Display *display, int screen_number);

/* Font loading */
extern XFontStruct *XLoadQueryFont(Display *display, const char *name);
extern int XFreeFont(Display *display, XFontStruct *font_struct);
extern int XTextWidth(XFontStruct *font_struct, const char *string, int count);

/* Miscellaneous */
extern int XBell(Display *display, int percent);
extern int XSetCloseDownMode(Display *display, int close_mode);
extern Status XGetTransientForHint(Display *display, Window w, Window *prop_window_return);
extern int XSetWindowBackground(Display *display, Window w, unsigned long background_pixel);
extern int XSetWindowBorder(Display *display, Window w, unsigned long border_pixel);
extern int XSetWindowBorderWidth(Display *display, Window w, unsigned int width);
extern int XClearWindow(Display *display, Window w);
extern int XClearArea(Display *display, Window w, int x, int y,
    unsigned int width, unsigned int height, Bool exposures);
extern Atom *XListProperties(Display *display, Window w, int *num_prop_return);
extern Status XStringListToTextProperty(char **list, int count, XTextProperty *text_prop_return);
extern Status XTextPropertyToStringList(XTextProperty *text_prop, char ***list_return, int *count_return);
extern void XFreeStringList(char **list);

#endif /* _XLIB_H_ */

