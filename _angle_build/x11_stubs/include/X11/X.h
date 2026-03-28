/*
 * Minimal X11 type definitions for gl4es cross-compilation on Android/Bionic.
 * Only the types and constants needed by gl4es's GLX implementation are defined.
 * Based on the X11 public API (MIT/X11 License).
 */
#ifndef _X11_X_H_
#define _X11_X_H_

typedef unsigned long XID;
typedef unsigned long Mask;
typedef unsigned long Atom;
typedef unsigned long VisualID;
typedef unsigned long Time;

typedef XID Window;
typedef XID Drawable;
typedef XID Font;
typedef XID Pixmap;
typedef XID Cursor;
typedef XID Colormap;
typedef XID GContext;
typedef XID KeySym;

typedef unsigned char KeyCode;

/* ── Event types ── */
#define KeyPress            2
#define KeyRelease          3
#define ButtonPress         4
#define ButtonRelease       5
#define MotionNotify        6
#define EnterNotify         7
#define LeaveNotify         8
#define FocusIn             9
#define FocusOut            10
#define KeymapNotify        11
#define Expose              12
#define GraphicsExpose      13
#define NoExpose            14
#define VisibilityNotify    15
#define CreateNotify        16
#define DestroyNotify       17
#define UnmapNotify         18
#define MapNotify           19
#define MapRequest          20
#define ReparentNotify      21
#define ConfigureNotify     22
#define ConfigureRequest    23
#define GravityNotify       24
#define ResizeRequest       25
#define CirculateNotify     26
#define CirculateRequest    27
#define PropertyNotify      28
#define SelectionClear      29
#define SelectionRequest    30
#define SelectionNotify     31
#define ColormapNotify      32
#define ClientMessage       33
#define MappingNotify       34
#define LASTEvent           35

/* Input event masks */
#define NoEventMask                 0L
#define KeyPressMask                (1L<<0)
#define KeyReleaseMask              (1L<<1)
#define ButtonPressMask             (1L<<2)
#define ButtonReleaseMask           (1L<<3)
#define EnterWindowMask             (1L<<4)
#define LeaveWindowMask             (1L<<5)
#define PointerMotionMask           (1L<<6)
#define PointerMotionHintMask       (1L<<7)
#define Button1MotionMask           (1L<<8)
#define Button2MotionMask           (1L<<9)
#define Button3MotionMask           (1L<<10)
#define Button4MotionMask           (1L<<11)
#define Button5MotionMask           (1L<<12)
#define ButtonMotionMask            (1L<<13)
#define KeymapStateMask             (1L<<14)
#define ExposureMask                (1L<<15)
#define VisibilityChangeMask        (1L<<16)
#define StructureNotifyMask         (1L<<17)
#define ResizeRedirectMask          (1L<<18)
#define SubstructureNotifyMask      (1L<<19)
#define SubstructureRedirectMask    (1L<<20)
#define FocusChangeMask             (1L<<21)
#define PropertyChangeMask          (1L<<22)
#define ColormapChangeMask          (1L<<23)
#define OwnerGrabButtonMask         (1L<<24)

/* Window attributes / visual */
#define CWBackPixmap            (1L<<0)
#define CWBackPixel             (1L<<1)
#define CWBorderPixmap          (1L<<2)
#define CWBorderPixel           (1L<<3)
#define CWBitGravity            (1L<<4)
#define CWWinGravity            (1L<<5)
#define CWBackingStore          (1L<<6)
#define CWBackingPlanes         (1L<<7)
#define CWBackingPixel          (1L<<8)
#define CWOverrideRedirect      (1L<<9)
#define CWSaveUnder             (1L<<10)
#define CWEventMask             (1L<<11)
#define CWDontPropagate         (1L<<12)
#define CWColormap              (1L<<13)
#define CWCursor                (1L<<14)

/* XConfigureWindow value masks */
#define CWX                     (1<<0)
#define CWY                     (1<<1)
#define CWWidth                 (1<<2)
#define CWHeight                (1<<3)
#define CWBorderWidth           (1<<4)
#define CWSibling               (1<<5)
#define CWStackMode             (1<<6)

/* Visual classes */
#define StaticGray      0
#define GrayScale       1
#define StaticColor     2
#define PseudoColor     3
#define TrueColor       4
#define DirectColor     5

#define VisualNoMask            0x0
#define VisualIDMask            0x1
#define VisualScreenMask        0x2
#define VisualDepthMask         0x4
#define VisualClassMask         0x8
#define VisualRedMaskMask       0x10
#define VisualGreenMaskMask     0x20
#define VisualBlueMaskMask      0x40
#define VisualColormapSizeMask  0x80
#define VisualBitsPerRGBMask    0x100
#define VisualAllMask           0x1FF

#define AllocNone       0
#define AllocAll        1

#define InputOutput     1
#define InputOnly       2

#define None            0L
#define Bool            int
#define Status          int
#define True            1
#define False           0

#define Success         0

/* Image formats */
#define XYBitmap        0
#define XYPixmap        1
#define ZPixmap         2

/* Property modes */
#define PropModeReplace     0
#define PropModePrepend     1
#define PropModeAppend      2

/* GC value masks */
#define GCFunction      (1L<<0)
#define GCPlaneMask     (1L<<1)
#define GCForeground    (1L<<2)
#define GCBackground    (1L<<3)
#define GCLineWidth     (1L<<4)
#define GCLineStyle     (1L<<5)
#define GCCapStyle      (1L<<6)
#define GCJoinStyle     (1L<<7)
#define GCFont          (1L<<14)

/* Grab modes */
#define GrabModeSync    0
#define GrabModeAsync   1

/* Grab status */
#define GrabSuccess     0
#define AlreadyGrabbed  1
#define GrabInvalidTime 2
#define GrabNotViewable 3
#define GrabFrozen      4

/* Predefined atoms */
#define XA_PRIMARY      1
#define XA_SECONDARY    2
#define XA_ARC          3
#define XA_ATOM         4
#define XA_BITMAP       5
#define XA_CARDINAL     6
#define XA_COLORMAP     7
#define XA_CURSOR       8
#define XA_INTEGER      13
#define XA_PIXMAP       20
#define XA_STRING       31
#define XA_WINDOW       33
#define XA_WM_NAME      39
#define XA_WM_ICON_NAME 37
#define XA_WM_HINTS     35
#define XA_WM_CLASS     67
#define XA_WM_TRANSIENT_FOR 68
#define XA_WM_NORMAL_HINTS  40
#define XA_WM_SIZE_HINTS    41

/* Current time for grabs */
#define CurrentTime     0L

/* Revert-to for SetInputFocus */
#define RevertToNone        0
#define RevertToPointerRoot 1
#define RevertToParent      2

/* Stack mode for XConfigureWindow */
#define Above       0
#define Below       1
#define TopIf       2
#define BottomIf    3
#define Opposite    4

/* Macros for Display/Screen access */
#define RootWindow(dpy, scr)            (((dpy)->screens[(scr)]).root)
#define DefaultScreen(dpy)              ((dpy)->default_screen)
#define DefaultVisual(dpy, scr)         (((dpy)->screens[(scr)]).root_visual)
#define DefaultDepth(dpy, scr)          (((dpy)->screens[(scr)]).root_depth)
#define ScreenCount(dpy)                ((dpy)->nscreens)
#define ScreenOfDisplay(dpy, scr)       (&((dpy)->screens[(scr)]))
#define DefaultScreenOfDisplay(dpy)     ScreenOfDisplay(dpy, DefaultScreen(dpy))
#define WidthOfScreen(s)                ((s)->width)
#define HeightOfScreen(s)               ((s)->height)
#define DefaultRootWindow(dpy)          RootWindow(dpy, DefaultScreen(dpy))
#define DisplayWidth(dpy, scr)          (((dpy)->screens[(scr)]).width)
#define DisplayHeight(dpy, scr)         (((dpy)->screens[(scr)]).height)
#define BlackPixel(dpy, scr)            (((dpy)->screens[(scr)]).black_pixel)
#define WhitePixel(dpy, scr)            (((dpy)->screens[(scr)]).white_pixel)
#define ConnectionNumber(dpy)           ((dpy)->fd)
#define DisplayString(dpy)              ((dpy)->vendor)

#endif /* _X11_X_H_ */

