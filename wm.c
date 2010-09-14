/* TinyWM is written by Nick Welch <mack@incise.org>, 2005.
 *
 * This software is in the public domain
 * and is provided AS IS, with NO WARRANTY. */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>


#define MAX(a, b) ((a) > (b) ? (a) : (b))

void dumpevent(XEvent *);

Display *dpy;

Window win[1024];
int winn=0;

int on_error(Display *d, XErrorEvent *e) {
	printf("ERROR\n");
	return 0;
}

void better_place(Window w,XWindowChanges *wc) {
	XClassHint h;
	XGetClassHint(dpy,w,&h);

	printf("name: %s class %s\n",h.res_name,h.res_class);

	if(strcmp(h.res_class,"Pidgin")==0) {
		wc->x=1600-256;
		wc->width=256;

		XTextProperty p;
		XGetTextProperty(dpy,w, &p, XInternAtom(dpy,"WM_WINDOW_ROLE",False));
		printf("TYPE: %s\n", p.value);
		XFree(p.value);
	}


	XFree(h.res_name);
	XFree(h.res_class);
}


int is_transient(Window w) {
	Window trans;
	XGetTransientForHint(dpy,w,&trans);
	return trans;
}

void put_in_place_transient(Window w, XConfigureRequestEvent *e) {
	XWindowChanges wc={.x=e->x,.y=e->y,.width=e->width,.height=e->height,.border_width=0};
	XConfigureWindow(dpy,w,e->value_mask,&wc);
        XSendEvent(dpy, w, False, StructureNotifyMask, (XEvent *)e);

	printf("put in place transient\n");
}

void put_in_place_transient_again(Window w, XConfigureRequestEvent *e) {
	XWindowChanges wc={.x=e->x,.y=e->y,.width=e->width,.height=e->height,.border_width=0};
	XConfigureWindow(dpy,w,e->value_mask,&wc);
        XSendEvent(dpy, w, False, StructureNotifyMask, (XEvent *)e);

	printf("put in place again transient\n");
}

void put_in_place(Window w)
{
	XWindowChanges wc={.x=256,.y=64,.width=1024,.height=760,.border_width=0};
	better_place(w,&wc);
	XConfigureWindow(dpy,w,CWX|CWY|CWWidth|CWHeight|CWBorderWidth,&wc);

        XConfigureRequestEvent  cr;

        bzero(&cr, sizeof cr);
        cr.type = ConfigureRequest;
        cr.display = dpy;
        cr.parent = w;
        cr.window = w;
        cr.x = wc.x;
        cr.y = wc.y;
        cr.width = wc.width;
        cr.height = wc.height;
        cr.border_width = 0;

        XSendEvent(dpy, w, False, StructureNotifyMask, (XEvent *)&cr);

        XSelectInput(dpy,w,EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);

	printf("put in place\n");
}

void put_in_place_again(Window w) {
	XWindowChanges wc={.x=256,.y=64,.width=1024,.height=760,.border_width=0};
	better_place(w,&wc);
	XConfigureWindow(dpy,w,CWX|CWY|CWWidth|CWHeight|CWBorderWidth,&wc);

        XConfigureEvent         ce;

        ce.type = ConfigureNotify;
        ce.display = dpy;
        ce.event = w;
        ce.window = w;
        ce.x = wc.x;
        ce.y = wc.y;
        ce.width = wc.width;
        ce.height = wc.height;
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

    XGrabKey(dpy, AnyKey, Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabButton(dpy, 1, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod4Mask, root, True, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None);

    printf("restarted\n");

    XSelectInput(dpy, root, SubstructureRedirectMask);
    XSync(dpy, False);

    // XSetErrorHandler(on_error);

    for(;;)
    {
        XNextEvent(dpy, &ev);
	dumpevent(&ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
		if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("F1"))) {
            		XRaiseWindow(dpy, ev.xkey.subwindow);
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("Return"))) {
            		system("gnome-terminal");
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("p"))) {
            		system("dmenu_run&");
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("r"))) {
			execl(argv[0],argv[0],NULL);
		}
        } else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
        {
            XGrabPointer(dpy, ev.xbutton.subwindow, True,
                    PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
                    GrabModeAsync, None, None, CurrentTime);
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        } else if(ev.type == ConfigureRequest) {
		int i=0; for(i=0;i<winn;i++) {
			if(ev.xconfigurerequest.window==win[i]) {
				if(is_transient(ev.xconfigurerequest.window))
					put_in_place_transient_again(ev.xconfigurerequest.window,&ev.xconfigurerequest);
				else put_in_place_again(ev.xconfigurerequest.window);
				break;
			}
		}
		if(i==winn) {
			if(is_transient(ev.xconfigurerequest.window))
				put_in_place_transient(ev.xconfigurerequest.window,&ev.xconfigurerequest);
			else put_in_place(ev.xconfigurerequest.window);
			win[winn++]=ev.xconfigurerequest.window;
		}
        } else if(ev.type == MapRequest) {
		XMapRaised(dpy,ev.xmaprequest.window);
        } else if(ev.type == EnterNotify) {
		XSetInputFocus(dpy, ev.xcrossing.window, RevertToParent, CurrentTime);
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
