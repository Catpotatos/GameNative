/*
 * Minimal Xutil definitions for gl4es cross-compilation on Android/Bionic.
 */
#ifndef _XUTIL_H_
#define _XUTIL_H_

#include <X11/Xlib.h>

/* XVisualInfo is already defined in our Xlib.h */

/* Size hints flags */
#define PSize           (1L << 3)
#define PMinSize        (1L << 4)
#define PMaxSize        (1L << 5)
#define PPosition       (1L << 2)
#define USPosition      (1L << 0)
#define USSize          (1L << 1)
#define PResizeInc      (1L << 6)
#define PAspect         (1L << 7)
#define PBaseSize       (1L << 8)
#define PWinGravity     (1L << 9)

/* WM hints flags */
#define InputHint           (1L << 0)
#define StateHint           (1L << 1)
#define IconPixmapHint      (1L << 2)
#define IconWindowHint      (1L << 3)
#define IconPositionHint    (1L << 4)
#define IconMaskHint        (1L << 5)
#define WindowGroupHint     (1L << 6)
#define AllHints (InputHint|StateHint|IconPixmapHint|IconWindowHint| \
                  IconPositionHint|IconMaskHint|WindowGroupHint)

/* WM initial state */
#define WithdrawnState  0
#define NormalState     1
#define IconicState     3

typedef struct {
    long flags;
    Bool input;
    int initial_state;
    Pixmap icon_pixmap;
    Window icon_window;
    int icon_x, icon_y;
    Pixmap icon_mask;
    XID window_group;
} XWMHints;

typedef struct {
    char *res_name;
    char *res_class;
} XClassHint;

extern void XSetWMNormalHints(Display *display, Window w, XSizeHints *hints);
extern XSizeHints *XAllocSizeHints(void);
extern XWMHints *XAllocWMHints(void);
extern XClassHint *XAllocClassHints(void);
extern void XSetWMHints(Display *display, Window w, XWMHints *wm_hints);
extern XWMHints *XGetWMHints(Display *display, Window w);
extern void XSetClassHint(Display *display, Window w, XClassHint *class_hints);
extern Status XGetClassHint(Display *display, Window w, XClassHint *class_hints_return);

/* Standard colormap (Wine may reference) */
typedef struct {
    Colormap colormap;
    unsigned long red_max;
    unsigned long red_mult;
    unsigned long green_max;
    unsigned long green_mult;
    unsigned long blue_max;
    unsigned long blue_mult;
    unsigned long base_pixel;
    VisualID visualid;
    XID killid;
} XStandardColormap;

#endif /* _XUTIL_H_ */
