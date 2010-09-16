#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_keysyms.h>

#include <X11/keysym.h>

xcb_screen_t *s;
xcb_connection_t *c;

xcb_window_t m[1024];
int wn=0;
int wc=0;

void system2(char *s)
{
	if (fork() == 0) {
		if (fork() == 0) {
			setsid();
			dup2(open("/dev/null",O_RDONLY,0),0);
			system(s);
		}
		exit(0);
	}
}


int managed(xcb_window_t w) {
	int i;
	for(i=0;i<wn;i++) {
		if(m[i]==w) { return i; }
	}
	return -1;
}

int is_classname(xcb_window_t w, char *class, char *name) {
	xcb_get_property_cookie_t cookie;
	xcb_get_wm_class_reply_t ch;
	cookie = xcb_get_wm_class_unchecked(c,w);
        if(xcb_get_wm_class_reply(c,cookie,&ch,NULL)) {
		if(  (class && strcmp(class,ch.class_name)!=0)
		  || (name && strcmp(name,ch.instance_name)!=0)) {
			xcb_get_wm_class_reply_wipe(&ch);
			return 0;
		}
		xcb_get_wm_class_reply_wipe(&ch);
		return 1;
	}
	return 0;
}

void classname(xcb_window_t w) {
	xcb_get_property_cookie_t cookie;
	xcb_get_wm_class_reply_t ch;
	cookie = xcb_get_wm_class_unchecked(c,w);
        if(xcb_get_wm_class_reply(c,cookie,&ch,NULL)) {
		printf("%x %s %s\n",w,ch.class_name,ch.instance_name);
		xcb_get_wm_class_reply_wipe(&ch);
	}
}

void printclassname(xcb_window_t w) {
	xcb_get_property_cookie_t cookie;
	xcb_get_wm_class_reply_t ch;
	cookie = xcb_get_wm_class_unchecked(c,w);
        if(xcb_get_wm_class_reply(c,cookie,&ch,NULL)) {
		printf("%x %s %s\n",w,ch.class_name,ch.instance_name);
		xcb_get_wm_class_reply_wipe(&ch);
	}
}

int is_managed(xcb_window_t w) { return managed(w)!=-1; }

void manage(xcb_window_t w) {
	if(is_managed(w)) { return; }
	m[wn++]=w;

	//XSelectInput(dpy,w,EnterWindowMask | FocusChangeMask | PropertyChangeMask | StructureNotifyMask);
        //const uint32_t v[1] = {XCB_EVENT_MASK_STRUCTURE_NOTIFY};
        //xcb_change_window_attributes(c,w,XCB_CW_EVENT_MASK,v);

	printf("MANaging %u\n",wn); printclassname(w);
}

void unmanage(xcb_window_t w) {
	int i=managed(w);
	if(i<0) return;
	m[i]=m[--wn];
	printf("unmanaging %u ",i); printclassname(w);
}

void setnormal(xcb_window_t w) {
	uint32_t data[] = {XCB_WM_STATE_NORMAL, XCB_NONE};
	xcb_change_property(c,XCB_PROP_MODE_REPLACE,w,
			xcb_atom_get(c, "WM_STATE"),xcb_atom_get(c, "WM_STATE"), 
                        32, 2, data);
}

void better_size(xcb_window_t w,uint32_t sz[4]) {
	if(is_classname(w,"Pidgin",0)) {
		sz[0]=1600-256;
		sz[2]=256;
	}
}


void resize(xcb_window_t w) {
	uint32_t v[]={256,64,1024,760};
	better_size(w,v);
	xcb_configure_window(c,w,XCB_CONFIG_WINDOW_X|XCB_CONFIG_WINDOW_Y
				|XCB_CONFIG_WINDOW_WIDTH|XCB_CONFIG_WINDOW_HEIGHT,
				v);
}

void configure_asis(xcb_generic_event_t *e0) {
	xcb_configure_request_event_t *e=(xcb_configure_request_event_t*)e0;
	uint16_t m = 0;
	uint32_t v[7];
	unsigned short i = 0;

	if(e->value_mask & XCB_CONFIG_WINDOW_X) { m |= XCB_CONFIG_WINDOW_X; v[i++] = e->x; }
	if(e->value_mask & XCB_CONFIG_WINDOW_Y) { m |= XCB_CONFIG_WINDOW_Y; v[i++] = e->y; }
	if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) { m |= XCB_CONFIG_WINDOW_WIDTH; v[i++] = e->width; }
	if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) { m |= XCB_CONFIG_WINDOW_HEIGHT; v[i++] = e->height; }
	if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) { m |= XCB_CONFIG_WINDOW_BORDER_WIDTH; v[i++] = e->border_width; }
	if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) { m |= XCB_CONFIG_WINDOW_SIBLING; v[i++] = e->sibling; }
	if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) { m |= XCB_CONFIG_WINDOW_STACK_MODE; v[i++] = e->stack_mode; }

	xcb_configure_window(c,e->window,m,v);
}

int is_transient(xcb_window_t w) {
	xcb_window_t t=0;
	xcb_get_wm_transient_for_reply(c,xcb_get_wm_transient_for(c,w),&t,NULL);
	return t;
}

void configure_request(xcb_generic_event_t *e0) {
	xcb_configure_request_event_t *e=(xcb_configure_request_event_t*)e0;
	printf("configure_request for 0x%x\n",e->window);

	printf("event: stack_mode %hhu, x %u, y %u, width %u, height %u, mask %04hx\n",
		e->stack_mode,e->x,e->y,e->width,e->height,e->value_mask);

	configure_asis(e0);
	if(!is_transient(e->window)) resize(e->window);
	xcb_aux_sync(c);
}

xcb_get_window_attributes_reply_t *getwinattr(xcb_window_t w) {
	xcb_get_window_attributes_cookie_t wa_c;
	xcb_get_window_attributes_reply_t *wa_r;
	wa_c = xcb_get_window_attributes_unchecked(c,w);
	if(!(wa_r = xcb_get_window_attributes_reply(c,wa_c,NULL))) return 0;
	return wa_r;
}

void show(xcb_window_t w) {
	printf("switching to (0x%x)\n",w);
	uint32_t v[1]={XCB_STACK_MODE_ABOVE};
	xcb_configure_window(c,w,XCB_CONFIG_WINDOW_STACK_MODE,v);
	xcb_set_input_focus(c,XCB_INPUT_FOCUS_PARENT,w,XCB_CURRENT_TIME);
	xcb_aux_sync(c);

}


void focus(xcb_generic_event_t *e0, int gain) {
	xcb_focus_in_event_t *e=(xcb_focus_in_event_t*)e0;
	printf("focus on 0x%x %s\n",e->event,gain?"given":"lost");
	if(gain) {
		uint32_t color[]={0xff00ff00};
		xcb_change_window_attributes(c,e->event,XCB_CW_BORDER_PIXEL, color);
		uint32_t b[]={1};
		xcb_configure_window(c,e->event,XCB_CONFIG_WINDOW_BORDER_WIDTH,b);
	} else {
		uint32_t color[]={0xffffffff};
		xcb_change_window_attributes(c,e->event,XCB_CW_BORDER_PIXEL, color);
		uint32_t b[]={1};
		xcb_configure_window(c,e->event,XCB_CONFIG_WINDOW_BORDER_WIDTH,b);
	}
	xcb_aux_sync(c);
}

void map_request(xcb_generic_event_t *e0) {
	printf("map_request\n");
	xcb_map_request_event_t *e=(xcb_map_request_event_t*)e0;
	xcb_get_window_attributes_reply_t *a=getwinattr(e->window);
	if(!a) return;
    	if(!a->override_redirect) {

		const uint32_t v[1] = {XCB_EVENT_MASK_FOCUS_CHANGE};
		xcb_change_window_attributes(c,e->window,XCB_CW_EVENT_MASK,v);
		manage(e->window);
		if(!is_transient(e->window)) resize(e->window);
		setnormal(e->window);
		xcb_map_window(c,e->window);
		show(e->window);
	}
	free(a);
}

void unmap_notify(xcb_generic_event_t *e0) {
	printf("unmap_notify\n");
	xcb_unmap_notify_event_t *e=(xcb_unmap_notify_event_t*)e0;
	unmanage(e->window);
}

void destroy_notify(xcb_generic_event_t *e0) {
	printf("destroy\n");
	xcb_destroy_notify_event_t *e=(xcb_destroy_notify_event_t*)e0;
	unmanage(e->window);
}


void key_press(xcb_generic_event_t *e0) {
	xcb_key_press_event_t *e=(xcb_key_press_event_t*)e0;
	
	xcb_key_symbols_t *sm=xcb_key_symbols_alloc(c);
	xcb_keysym_t k=xcb_key_press_lookup_keysym (sm,e,0);

	printf("key %x on %u managed\n",k,wn);

	if(k==XK_p) {
		system2("dmenu_run&");
	} else if(k==XK_Tab) {
		if(!wn) return;
		wc++; if(wc>=wn) {wc=0;}
		show(m[wc]);
	}
}

void enter_notify(xcb_generic_event_t *e0) {
	xcb_enter_notify_event_t *e=(xcb_enter_notify_event_t*)e0;
	show(e->event);
}

void manage_existing() {
	xcb_query_tree_cookie_t cookie=xcb_query_tree_unchecked(c,s->root);
	xcb_query_tree_reply_t *r=xcb_query_tree_reply(c,cookie,NULL);
        xcb_window_t *wins=xcb_query_tree_children(r);
        int n=xcb_query_tree_children_length(r);

	int i;
	for(i=0;i<n;i++) {
		xcb_get_window_attributes_reply_t *a=getwinattr(wins[i]);
		if(!a) continue;
		if(a->override_redirect) continue;
		if(a->map_state!=XCB_MAP_STATE_VIEWABLE) continue;

		const uint32_t v[1] = {XCB_EVENT_MASK_FOCUS_CHANGE|XCB_EVENT_MASK_ENTER_WINDOW};
		xcb_change_window_attributes(c,wins[i],XCB_CW_EVENT_MASK,v);

		resize(wins[i]);
		setnormal(wins[i]);
		manage(wins[i]);
		printf("remapped\n");

		xcb_map_window(c,wins[i]);
	}

	xcb_aux_sync(c);
	wc=0;
	if(wn>0) { show(m[wc]); }
}

#define ERRORS_NBR              256
static xcb_event_handlers_t evenths;

/** Set the same handler for all errors */
void
xutil_error_handler_catch_all_set(xcb_event_handlers_t *evenths,
                                  xcb_generic_error_handler_t handler,
                                  void *data)
{
    int err_num;
    for(err_num = 0; err_num < ERRORS_NBR; err_num++)
        xcb_event_set_error_handler(evenths, err_num, handler, data);
}


static int
xerror(void *data, xcb_connection_t *c, xcb_generic_error_t *e)
{
	fprintf(stderr, "heterosis: fatal error: request code=%s, error code=%s\n",
		 xcb_event_get_request_label(e->major_code),
		 xcb_event_get_error_label(e->error_code));
	return 0;
}



int main() {
	c=xcb_connect (NULL, NULL);
	s=xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	
        const uint32_t v[1] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT};
        xcb_change_window_attributes(c,s->root,XCB_CW_EVENT_MASK,v);

	xcb_grab_key (c,1,s->root,XCB_MOD_MASK_4,XCB_GRAB_ANY,XCB_GRAB_MODE_ASYNC,XCB_GRAB_MODE_ASYNC);
	xcb_flush(c);

	xutil_error_handler_catch_all_set(&evenths, xerror, NULL);
	xcb_aux_sync(c);

	manage_existing();

	printf("loop\n");
	xcb_generic_event_t *e;
	while((e=xcb_wait_for_event(c))) {
		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_REQUEST: configure_request(e); break;
		case XCB_MAP_REQUEST: map_request(e); break;
		case XCB_UNMAP_NOTIFY: unmap_notify(e); break;
		case XCB_KEY_PRESS: key_press(e); break;
		case XCB_DESTROY_NOTIFY: destroy_notify(e); break;
		case XCB_FOCUS_IN: focus(e,1); break;
		case XCB_FOCUS_OUT: focus(e,0); break;
		case XCB_ENTER_NOTIFY: enter_notify(e); break;
		default:
			printf("unhandled %u (0x%x)\n",e->response_type & ~0x80,e->response_type & ~0x80);
		}
		free(e);
	}

	return 0;
}

