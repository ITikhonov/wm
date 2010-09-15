#include <malloc.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

xcb_screen_t *s;
xcb_connection_t *c;

xcb_window_t m[1024];
int wn=0;
int wc=0;

int managed(xcb_window_t w) {
	int i;
	for(i=0;i<wn;i++) {
		if(m[i]==w) { return i; }
	}
	return -1;
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

	printf("managing "); printclassname(w);
}

void unmanage(xcb_window_t w) {
	int i=managed(w);
	if(i<0) return;
	m[i]=m[--wn];
	printf("unmanaging "); printclassname(w);
}

void configure_request_window(xcb_configure_request_event_t *e) {
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

void configure_request(xcb_generic_event_t *e0) {
	xcb_configure_request_event_t *e=(xcb_configure_request_event_t*)e0;

	if(is_managed(e->window)) {
		xcb_configure_notify_event_t ce;
		ce.response_type = XCB_CONFIGURE_NOTIFY;
		ce.event = e->window;
		ce.window = e->window;
		ce.x = e->x;
		ce.y = e->y;
		ce.width = e->width;
		ce.height = e->height;
		ce.border_width = 1;
		ce.above_sibling = XCB_NONE;
		ce.override_redirect = 0;
		xcb_send_event(c,0,e->window,XCB_EVENT_MASK_STRUCTURE_NOTIFY,(char*)&ce);
	    }
	    else configure_request_window(e);
}

xcb_get_window_attributes_reply_t *getwinattr(xcb_window_t w) {
	xcb_get_window_attributes_cookie_t wa_c;
	xcb_get_window_attributes_reply_t *wa_r;
	wa_c = xcb_get_window_attributes_unchecked(c,w);
	if(!(wa_r = xcb_get_window_attributes_reply(c,wa_c,NULL))) return 0;
	return wa_r;
}

void map_request(xcb_generic_event_t *e0) {
	xcb_map_request_event_t *e=(xcb_map_request_event_t*)e0;
	xcb_get_window_attributes_reply_t *a=getwinattr(e->window);
	if(!a) return;
    	if(!a->override_redirect)
		xcb_map_window(c,e->window);
	free(a);
}

void unmap_notify(xcb_generic_event_t *e0) {
	xcb_unmap_notify_event_t *e=(xcb_unmap_notify_event_t*)e0;
	unmanage(e->window);
}

void key_press(xcb_generic_event_t *e0) {
	//xcb_key_press_event_t *e=(xcb_key_press_event_t*)e0;
	printf("key: %u managed\n",wn);
	if(!wn) return;
	wc++; if(wc>=wn) {wc=0;}
	printf("switching to %u (0x%x)\n",wc,m[wc]);
	uint32_t v[1]={XCB_STACK_MODE_ABOVE};
	xcb_configure_window(c,m[wc],XCB_CONFIG_WINDOW_STACK_MODE,v);
	xcb_set_input_focus(c,XCB_INPUT_FOCUS_PARENT,m[wc],XCB_CURRENT_TIME);
	xcb_flush(c);

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

		manage(wins[i]);
	}
}


int main() {
	c=xcb_connect (NULL, NULL);
	s=xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	
        const uint32_t v[1] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT};
        xcb_change_window_attributes(c,s->root,XCB_CW_EVENT_MASK,v);

	xcb_grab_key (c,1,s->root,XCB_MOD_MASK_4,XCB_GRAB_ANY,XCB_GRAB_MODE_ASYNC,XCB_GRAB_MODE_ASYNC);
	xcb_flush(c);

	manage_existing();

	printf("loop\n");
	xcb_generic_event_t *e;
	while((e=xcb_wait_for_event(c))) {
		switch (e->response_type & ~0x80) {
		case XCB_CONFIGURE_REQUEST: configure_request(e); break;
		case XCB_MAP_REQUEST: configure_request(e); break;
		case XCB_UNMAP_NOTIFY: unmap_notify(e); break;
		case XCB_KEY_PRESS: key_press(e); break;
		}
		free(e);
	}

	return 0;
}

