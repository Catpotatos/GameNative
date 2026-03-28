/*
 * Minimal Xrandr stub header for gl4es cross-compilation.
 * gl4es may reference Xrandr for resolution queries; we provide no-op stubs.
 */
#ifndef _XRANDR_H_
#define _XRANDR_H_

#include <X11/Xlib.h>

typedef XID RROutput;
typedef XID RRCrtc;
typedef XID RRMode;
typedef unsigned short Rotation;
typedef unsigned short SizeID;
typedef unsigned short SubpixelOrder;
typedef unsigned short Connection;

typedef struct {
    int width, height;
    int mwidth, mheight;
} XRRScreenSize;

typedef struct _XRRScreenConfiguration XRRScreenConfiguration;

extern XRRScreenConfiguration *XRRGetScreenInfo(Display *dpy, Window window);
extern void XRRFreeScreenConfigInfo(XRRScreenConfiguration *config);
extern XRRScreenSize *XRRConfigSizes(XRRScreenConfiguration *config, int *nsizes);
extern SizeID XRRConfigCurrentConfiguration(XRRScreenConfiguration *config, Rotation *rotation);
extern short XRRConfigCurrentRate(XRRScreenConfiguration *config);
extern Status XRRSetScreenConfigAndRate(Display *dpy, XRRScreenConfiguration *config,
    Drawable draw, int size_index, Rotation rotation, short rate, Time timestamp);

#endif /* _XRANDR_H_ */

