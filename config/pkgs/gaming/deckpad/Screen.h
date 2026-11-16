#pragma once

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>

/* One black, full-screen X11 window that can show a grey picture in its
 * middle. Xlib's event union and its macros are kept on this side; Screen.btrc
 * decides what to show and when. */
typedef struct DeckScreenStorage {
	Display* display;
	Window window;
	GC context;
	XImage* notices[4];
	int count;
} *DeckScreenHandle;

static inline int deckScreenHexDigit(char ch) {
	if (ch >= '0' && ch <= '9') { return ch - '0'; }
	if (ch >= 'a' && ch <= 'f') { return 10 + (ch - 'a'); }
	if (ch >= 'A' && ch <= 'F') { return 10 + (ch - 'A'); }
	return -1;
}

/* NULL when there is no display to open. */
static inline DeckScreenHandle deckScreenOpen(const char* title) {
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) { return NULL; }
	DeckScreenHandle screen = calloc(1, sizeof(*screen));
	if (screen == NULL) { XCloseDisplay(display); return NULL; }
	int number = DefaultScreen(display);
	screen->display = display;
	screen->window = XCreateSimpleWindow(display, RootWindow(display, number), 0, 0, (unsigned int)DisplayWidth(display, number), (unsigned int)DisplayHeight(display, number), 0, 0, BlackPixel(display, number));
	XStoreName(display, screen->window, title);
	XClassHint name = { (char*)"deckpad-screen", (char*)"deckpad-screen" };
	XSetClassHint(display, screen->window, &name);
	Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
	XChangeProperty(display, screen->window, XInternAtom(display, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (unsigned char*)&fullscreen, 1);
	XSelectInput(display, screen->window, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
	/* No pointer over a dark screen. */
	char nothing = 0;
	Pixmap empty = XCreateBitmapFromData(display, screen->window, &nothing, 1, 1);
	XColor black;
	memset(&black, 0, sizeof(black));
	XDefineCursor(display, screen->window, XCreatePixmapCursor(display, empty, empty, &black, &black, 0, 0));
	screen->context = XCreateGC(display, screen->window, 0, NULL);
	XMapRaised(display, screen->window);
	XFlush(display);
	return screen;
}

/* Keep a picture of width x height grey levels encoded as two hex digits per
 * pixel. Its number for deckScreenPaint, or -1 when it cannot be shown here. */
static inline int deckScreenAddNoticeHex(DeckScreenHandle screen, const char* hex, int width, int height) {
	if (screen == NULL || screen->display == NULL || hex == NULL) { return -1; }
	int number = DefaultScreen(screen->display);
	if (screen->count >= 4 || width <= 0 || height <= 0 || DefaultDepth(screen->display, number) < 24) { return -1; }
	size_t count = (size_t)width * (size_t)height;
	if (strlen(hex) < count * 2) { return -1; }
	char* pixels = malloc(count * 4);
	if (pixels == NULL) { return -1; }
	for (size_t index = 0; index < count; index++) {
		int hi = deckScreenHexDigit(hex[index * 2]);
		int lo = deckScreenHexDigit(hex[index * 2 + 1]);
		if (hi < 0 || lo < 0) { free(pixels); return -1; }
		char level = (char)((hi << 4) | lo);
		pixels[index * 4] = pixels[index * 4 + 1] = pixels[index * 4 + 2] = level;
		pixels[index * 4 + 3] = 0;
	}
	XImage* image = XCreateImage(screen->display, DefaultVisual(screen->display, number), (unsigned int)DefaultDepth(screen->display, number), ZPixmap, 0, pixels, (unsigned int)width, (unsigned int)height, 32, 0);
	if (image == NULL) { free(pixels); return -1; }
	screen->notices[screen->count] = image;
	return screen->count++;
}

/* Take what the server sent. Bit 1: the screen was touched or a key pressed;
 * bit 2: the window needs painting again. */
static inline int deckScreenEvents(DeckScreenHandle screen) {
	if (screen == NULL || screen->display == NULL) { return 0; }
	int seen = 0;
	while (XPending(screen->display) > 0) {
		XEvent event;
		XNextEvent(screen->display, &event);
		if (event.type == ButtonPress || event.type == KeyPress) { seen |= 1; }
		else if (event.type == Expose || event.type == ConfigureNotify) { seen |= 2; }
	}
	return seen;
}

/* Black, with the numbered notice in the middle; any other number is black alone. */
static inline void deckScreenPaint(DeckScreenHandle screen, int notice) {
	if (screen == NULL || screen->display == NULL) { return; }
	XClearWindow(screen->display, screen->window);
	if (notice >= 0 && notice < screen->count) {
		XImage* image = screen->notices[notice];
		XWindowAttributes attributes;
		if (XGetWindowAttributes(screen->display, screen->window, &attributes) != 0) {
			XPutImage(screen->display, screen->window, screen->context, image, 0, 0, (attributes.width - image->width) / 2, (attributes.height - image->height) / 2, (unsigned int)image->width, (unsigned int)image->height);
		}
	}
	XFlush(screen->display);
}

/* The connection to wait on for the next event. */
static inline int deckScreenDescriptor(DeckScreenHandle screen) {
	return screen != NULL && screen->display != NULL ? ConnectionNumber(screen->display) : -1;
}

static inline void deckScreenShutdown(DeckScreenHandle screen) {
	if (screen != NULL && screen->display != NULL) {
		XCloseDisplay(screen->display);
		screen->display = NULL;
	}
}

static inline void deckScreenClose(DeckScreenHandle screen) {
	if (screen == NULL) { return; }
	deckScreenShutdown(screen);
	free(screen);
}
