#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <iconv.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>

xcb_connection_t    *c;
xcb_screen_t        *s;
xcb_window_t         w;
xcb_gcontext_t       g;

void text(int x,int y,xcb_char2b_t *t,int n) {
	xcb_image_text_16(c,n,w,g,x,y,t);
}

xcb_window_t m[256];
xcb_char2b_t *names[256];
int namel[256];
int mn=0;

int utf8_to_ucs2(uint8_t *input, int l, xcb_char2b_t *buf, int m) {
	uint8_t *p=(uint8_t *)input,*e=p+l;
	xcb_char2b_t *o=buf;
	while(p<e) {
		if((*p&0x80)==0) {
			o->byte1=0;
			o->byte2=*p++;
			o++;
		} else if((*p&0xe0)==0xc0) {
			printf("%02x%02x -> ",p[0],p[1]);
			o->byte1=(p[0]>>2)&7;
			o->byte2=((p[0]&3)<<6) | (p[1]&0x3f);
			printf("%02x%02x\n",o->byte1,o->byte2);
			o++;
			p+=2;
		} else {
			printf("UNKNOWN CHARACTER\n");
		}
	}
	printf("len %lu %p %p\n",o-buf,o,buf);
	return o-buf;
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

char *getname(xcb_window_t w) {
        xcb_get_property_cookie_t cookie=xcb_get_property(c,0,w,WM_NAME,XCB_ATOM_STRING,0,256);
	xcb_get_property_reply_t *reply = xcb_get_property_reply(c,cookie,NULL);
	if(reply) {
		char *s=strndup(xcb_get_property_value(reply),xcb_get_property_value_length(reply));
		free(reply);
		return s;
	}
	return 0;
}

xcb_char2b_t *utf8dup(char *s, int *n) {
	xcb_char2b_t *u=malloc(*n*2);
	*n=utf8_to_ucs2((uint8_t*)s,*n,u,*n*2);
	return u;
}

xcb_char2b_t *getname_utf8(xcb_window_t w, int *rn) {
        xcb_get_property_cookie_t cookie=xcb_get_property(c,0,w,xcb_atom_get(c,"_NET_WM_NAME"),xcb_atom_get(c,"UTF8_STRING"),0,256);
	xcb_get_property_reply_t *reply = xcb_get_property_reply(c,cookie,NULL);
	if(reply) {
		char *s=xcb_get_property_value(reply);
		int n=xcb_get_property_value_length(reply);
		xcb_char2b_t *u=utf8dup(s,&n);
		*rn=n;
		free(reply);
		return u;
	}
	return 0;
}

int makename(xcb_window_t w) {
	if(is_classname(w,"Firefox",0)) {
		namel[mn]=7;
		names[mn]=utf8dup("firefox",&namel[mn]);
		return 1;
	}
	if(is_classname(w,0,"gnome-terminal")) {
		namel[mn]=8;
		names[mn]=utf8dup("terminal",&namel[mn]);
		return 1;
	}
	if(is_classname(w,"wmlist",0)) {
		return 0;
	}

	
	if((names[mn]=getname_utf8(w,&namel[mn]))) return 1;
	return 0;
}

void update() {
        xcb_get_property_cookie_t cookie;

        cookie=xcb_get_property(c,0,s->root,xcb_atom_get(c, "_NET_CLIENT_LIST"),XCB_ATOM_WINDOW,0,256);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(c,cookie,NULL);

	xcb_window_t *v=xcb_get_property_value(reply);
	int n=xcb_get_property_value_length(reply);

	while(mn) {
		mn--;
		printf("freeing %u\n",mn);
		free(names[mn]);
	}

	int i;
	for(i=0;i<n;i++) {
		if(makename(v[i])) {
			printf("add 0x%x as %u (%u)", v[i],mn,namel[mn]);
			int j; for(j=0;j<namel[mn];j++) {
				printf("%02x%02x ",names[mn][j].byte1,names[mn][j].byte2);
			}
			printf("\n");
			m[mn++]=v[i];
		}
	}
	printf("total: %u\n",mn);
	xcb_clear_area(c,1,w,0,0,0,0);
	xcb_flush(c);
}

void draw() {
	int i,y=16;
	for(i=0;i<mn;i++) {
		text(8,y,names[i],namel[i]);
		y+=16;
	}
	xcb_flush(c);
}
 
int main(void)
{
	xcb_generic_event_t *e;

	c = xcb_connect(NULL,NULL);
	s = xcb_setup_roots_iterator( xcb_get_setup(c) ).data;

	xcb_font_t font=xcb_generate_id(c);
	char *fontname="-microsoft-verdana-medium-r-*-*-10-0-0-0-p-0-iso10646-1";
	xcb_open_font(c,font,strlen(fontname),fontname);

	g = xcb_generate_id(c);
	uint32_t gv[]={s->black_pixel,s->white_pixel,font,0};
	xcb_create_gc(c,g,s->root,XCB_GC_FOREGROUND|XCB_GC_BACKGROUND|XCB_GC_FONT|XCB_GC_GRAPHICS_EXPOSURES,gv);

	w = xcb_generate_id(c);
	uint32_t mv[]={s->white_pixel,XCB_EVENT_MASK_EXPOSURE};
	xcb_create_window(c,s->root_depth,w,s->root,
		    10,10,100,100,1,XCB_WINDOW_CLASS_INPUT_OUTPUT,s->root_visual,
		    XCB_CW_BACK_PIXEL|XCB_CW_EVENT_MASK,mv);

	xcb_change_property(c,XCB_PROP_MODE_REPLACE,w,xcb_atom_get(c, "WM_CLASS"),XCB_ATOM_STRING,8,14,"wmlist\0wmlist");

        const uint32_t v[1] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
        xcb_change_window_attributes(c,s->root,XCB_CW_EVENT_MASK,v);


	xcb_map_window(c, w);
	xcb_flush(c);

	update();
	while ((e=xcb_wait_for_event(c))) {
		switch (e->response_type&~0x80) {
		case XCB_EXPOSE:
			printf("expose\n");
			draw();
			break;
		case XCB_PROPERTY_NOTIFY: update(); break;
		}
		free(e);
	}

	xcb_destroy_window(c,w);

	xcb_disconnect(c);
	return 0;
}

