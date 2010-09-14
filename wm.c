/* TinyWM is written by Nick Welch <mack@incise.org>, 2005.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void dumpevent(XEvent *);

Display *dpy;

Window win[1024];
int winn=0;

void put_in_place(Window w)
{
	XWindowChanges wc={.x=10,.y=10,.width=300,.height=300,.border_width=0};
	XConfigureWindow(dpy,w,CWX|CWY|CWWidth|CWHeight|CWBorderWidth,&wc);

        XConfigureRequestEvent  cr;

        bzero(&cr, sizeof cr);
        cr.type = ConfigureRequest;
        cr.display = dpy;
        cr.parent = w;
        cr.window = w;
        cr.x = 10;
        cr.y = 10;
        cr.width = 300;
        cr.height = 300;
        cr.border_width = 0;

        XSendEvent(dpy, w, False, StructureNotifyMask, (XEvent *)&cr);

	printf("put in place\n");
}

void put_in_place_again(Window w) {
        XConfigureEvent         ce;

        ce.type = ConfigureNotify;
        ce.display = dpy;
        ce.event = w;
        ce.window = w;
        ce.x = 10;
        ce.y = 10;
        ce.width = 300;
        ce.height = 300;
        ce.border_width = 0;
        ce.above = None;
        ce.override_redirect = False;
        XSendEvent(dpy, w, False, StructureNotifyMask, (XEvent *)&ce);

	printf("put in place again\n");
}


int main(int argc, char *argv[])
{
    Window root;
    XWindowAttributes attr;
    XButtonEvent start;
    XEvent ev;

    if(!(dpy = XOpenDisplay(0x0))) return 1;

    root = DefaultRootWindow(dpy);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F1")), Mod4Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("F2")), Mod4Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XStringToKeysym("Return")), Mod4Mask, root,
            True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);

    printf("restarted\n");

    XSelectInput(dpy, root, SubstructureRedirectMask);
    XSync(dpy, False);


    for(;;)
    {
        XNextEvent(dpy, &ev);
	dumpevent(&ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None)
		if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("F1"))) {
            		XRaiseWindow(dpy, ev.xkey.subwindow);
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("Return"))) {
            		system("gnome-terminal");
		} else {
			execl(argv[0],argv[0],NULL);
		}
        else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            XGrabPointer(dpy, ev.xbutton.subwindow, True,
                    PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
                    GrabModeAsync, None, None, CurrentTime);
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        } else if(ev.type == ConfigureRequest) {
		int i=0; for(i=0;i<winn;i++) {
			if(ev.xconfigurerequest.window==win[i]) {
				put_in_place_again(ev.xconfigurerequest.window);
			}
		}
		if(i==winn) {
			put_in_place(ev.xconfigurerequest.window);
			win[winn++]=ev.xconfigurerequest.window;
		}
        } else if(ev.type == MapRequest) {
		XMapRaised(dpy,ev.xmaprequest.window);
	} else if(ev.type == MotionNotify)
        {
            int xdiff, ydiff;
            while(XCheckTypedEvent(dpy, MotionNotify, &ev));
            xdiff = ev.xbutton.x_root - start.x_root;
            ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, ev.xmotion.window,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if(ev.type == ButtonRelease)
            XUngrabPointer(dpy, CurrentTime);
    }
}
