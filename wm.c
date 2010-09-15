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

Window firefox=0,terminal=0,pidgin=0;

Window win[1024];
int winn=0;

int on_error(Display *d, XErrorEvent *e) {
	printf("ERROR\n");
	return 0;
}

void ignore_errors() { XSetErrorHandler(on_error); }
void restore_errors() { XSetErrorHandler(NULL); }

void closewindow() {
        XClientMessageEvent     cm;

        bzero(&cm, sizeof cm);
        cm.type = ClientMessage;
	int back;
	XGetInputFocus(dpy,&cm.window,&back);
        cm.message_type = XInternAtom(dpy, "WM_PROTOCOLS", False);
        cm.format = 32;
        cm.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
        cm.data.l[1] = CurrentTime;
        XSendEvent(dpy, cm.window, False, 0L, (XEvent *)&cm);
}

int is_role(Window w, char *role) {
	XTextProperty p;
	XGetTextProperty(dpy,w, &p, XInternAtom(dpy,"WM_WINDOW_ROLE",False));
	if(!p.value) return 0;
	printf("ROLE: %s\n", p.value);
	int ret=(strcmp(role,(char*)p.value));
	XFree(p.value);
	return ret;
}

void better_place(Window w,XWindowChanges *wc) {
	XClassHint h;
	XGetClassHint(dpy,w,&h);
	printf("name: %s class %s\n",h.res_name,h.res_class);

	if(strcmp(h.res_class,"Pidgin")==0) {
		wc->x=1600-256;
		wc->width=256;

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

void takeover(Window w);

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

int is_name(Window w,char *name) {
	XClassHint h;
	int ret=0;
	if(XGetClassHint(dpy,w,&h)) {
		ret=(strcmp(name,h.res_name)==0);
		printf("name %s\n",h.res_name);
		XFree(h.res_name); XFree(h.res_class);
	}
	return ret;
}

int is_class(Window w,char *class) {
	XClassHint h;
	int ret=0;
	if(XGetClassHint(dpy,w,&h)) {
		ret=(strcmp(class,h.res_class)==0);
		printf("class %s\n",h.res_class);
		XFree(h.res_name); XFree(h.res_class);
	}
	return ret;
}

int is_viewable(Window w) {
	XWindowAttributes a;
	XGetWindowAttributes(dpy,w,&a);
	return a.map_state == IsViewable;
}

void takeover(Window w) {
	printf("\ntakeover %x\n",(unsigned int)w);
       	XSelectInput(dpy,w,EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);
	if(is_class(w,"Firefox")&&is_viewable(w)) { firefox=w; printf("found firefox\n"); }
	else if(is_name(w,"gnome-terminal")&&is_viewable(w)) { terminal=w; printf("found terminal\n"); }
	else if(is_class(w,"Pidgin")&&is_viewable(w)) { pidgin=w; printf("found pidgin\n"); }
}

void takeover_existing() {
	Window r;
	Window *w;
	unsigned int n;
	XQueryTree(dpy,DefaultRootWindow(dpy),&r,&r,&w,&n);
	int i;
	for(i=0;i<n;i++) { if(is_viewable(w[i])) takeover(w[i]); }
	XFree(w);
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

    takeover_existing();

    ignore_errors();
    for(;;)
    {
	XSync(dpy,False);
        XNextEvent(dpy, &ev);
	//dumpevent(&ev);
        if(ev.type == KeyPress && ev.xkey.subwindow != None) {
		if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("F1"))) {
            		XRaiseWindow(dpy, ev.xkey.subwindow);
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("Return"))) {
            		system("gnome-terminal");
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("p"))) {
            		system("dmenu_run&");
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("x"))) {
            		closewindow();
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("2"))) {
			if(firefox) {
				XSetInputFocus(dpy,firefox,RevertToParent,CurrentTime);
				XMapRaised(dpy,firefox);
			} else {
				system("pidof firefox-bin || firefox&");
			}
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("1"))) {
			if(terminal) {
				XSetInputFocus(dpy,terminal,RevertToParent,CurrentTime);
				XMapRaised(dpy,terminal);
			} else {
				system("pidof gnome-terminal || gnome-terminal&");
			}
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("0"))) {
			if(pidgin) {
				XSetInputFocus(dpy,pidgin,RevertToParent,CurrentTime);
				XMapRaised(dpy,pidgin);
			} else {
				system("pidof pidgin || pidgin&");
			}
		} else if(ev.xkey.keycode==XKeysymToKeycode(dpy, XStringToKeysym("r"))) {
			execl(argv[0],argv[0],NULL);
		}
		} else if(ev.type == ButtonPress && ev.xbutton.subwindow != None)
		{
			XSetInputFocus(dpy,ev.xbutton.subwindow,RevertToParent,CurrentTime);
			XMapRaised(dpy,ev.xbutton.subwindow);
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
			takeover(ev.xmaprequest.window);
			XSetInputFocus(dpy,ev.xmaprequest.window,RevertToParent,CurrentTime);
		} else if(ev.type == UnmapNotify) {
			if(ev.xunmap.window==firefox) firefox=0;
			if(ev.xunmap.window==terminal) terminal=0;
			if(ev.xunmap.window==pidgin) pidgin=0;
			takeover_existing();
		} else if(ev.type == EnterNotify) {
			//printf("enternotify %x\n",(unsigned int)ev.xcrossing.window);
			//XSetInputFocus(dpy, ev.xcrossing.window, RevertToParent, CurrentTime);
			//printf("ok\n");
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
