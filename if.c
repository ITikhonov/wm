#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>

#include <iconv.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>

xcb_connection_t    *c;
xcb_screen_t        *s;
xcb_window_t         w;
xcb_gcontext_t       g,bg;

void text(int x,int y,xcb_char2b_t *t,int n) {
	xcb_image_text_16(c,n,w,g,x,y,t);
}

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
			break;
		}
	}
	printf("len %lu %p %p\n",o-buf,o,buf);
	return o-buf;
}

xcb_char2b_t *utf8dup(char *s, int *n) {
	xcb_char2b_t *u=malloc(*n*2);
	*n=utf8_to_ucs2((uint8_t*)s,*n,u,*n*2);
	return u;
}

void update() {
	xcb_clear_area(c,1,w,0,0,0,0);
	xcb_flush(c);
}

void draw() {
	DIR *d=opendir(".");
	struct dirent *f=0;
	int i=0;
	while((f=readdir(d))) {
		if(f->d_name[0]=='.') continue;
		uint8_t buf[256];
		int fd=open(f->d_name,O_RDONLY); int n=read(fd,buf,256); close(fd);
		if(n>0) {
			xcb_char2b_t buf2[256];
			text(8,i*18+12,buf2,utf8_to_ucs2(buf,n,buf2,256));
		}
		i++;
	}
	closedir(d);
	xcb_flush(c);
}

void button(xcb_generic_event_t *e0) {
	printf("button\n");
        //xcb_button_press_event_t *e=(xcb_button_press_event_t*)e0;
	//unsigned int no=((e->event_y-6)/18);
}

static void alarm_handler(int signum) {
	printf("alarm\n");
	alarm(5);
}

 
int main(int argc, char* argv[])
{
	if(argc>1) chdir(argv[1]);
	xcb_generic_event_t *e;

	c = xcb_connect(NULL,NULL);
	s = xcb_setup_roots_iterator( xcb_get_setup(c) ).data;

	xcb_font_t font=xcb_generate_id(c);
	char *fontname="-microsoft-verdana-medium-r-*-*-10-0-0-0-p-0-iso10646-1";
	xcb_open_font(c,font,strlen(fontname),fontname);

	g = xcb_generate_id(c);
	uint32_t gv[]={s->white_pixel,0x333333,font,0};
	xcb_create_gc(c,g,s->root,XCB_GC_FOREGROUND|XCB_GC_BACKGROUND|XCB_GC_FONT|XCB_GC_GRAPHICS_EXPOSURES,gv);

	bg = xcb_generate_id(c);
	uint32_t gv3[]={0x333333,0xffffff,font,0};
	xcb_create_gc(c,bg,s->root,XCB_GC_FOREGROUND|XCB_GC_BACKGROUND|XCB_GC_FONT|XCB_GC_GRAPHICS_EXPOSURES,gv3);

	w = xcb_generate_id(c);
	uint32_t mv[]={s->black_pixel,XCB_EVENT_MASK_EXPOSURE|XCB_EVENT_MASK_BUTTON_PRESS};
	xcb_create_window(c,s->root_depth,w,s->root,
		    10,10,100,100,1,XCB_WINDOW_CLASS_INPUT_OUTPUT,s->root_visual,
		    XCB_CW_BACK_PIXEL|XCB_CW_EVENT_MASK,mv);

	xcb_change_property(c,XCB_PROP_MODE_REPLACE,w,xcb_atom_get(c, "WM_CLASS"),XCB_ATOM_STRING,8,14,"iflist\0iflist");

        const uint32_t v[1] = {XCB_EVENT_MASK_PROPERTY_CHANGE};
        xcb_change_window_attributes(c,s->root,XCB_CW_EVENT_MASK,v);


	xcb_map_window(c, w);
	xcb_flush(c);

	update();

	int fd=xcb_get_file_descriptor(c);

	struct sigaction sa;
	sa.sa_handler=alarm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags=0;
	sigaction(SIGALRM,&sa,NULL);

	alarm(1);

	for(;;) {
		fd_set s;
		FD_ZERO(&s);	
		FD_SET(fd,&s);

		for(;;) {
			if(select(fd+1,&s,0,&s,0)==1) break;
			update();
		}


		while ((e=xcb_poll_for_event(c))) {
			switch (e->response_type&~0x80) {
			case XCB_EXPOSE:
				printf("expose\n");
				draw();
				break;
			case XCB_PROPERTY_NOTIFY: update(); break;
			case XCB_BUTTON_PRESS: button(e); break;
			}
			free(e);
		}
	}

	xcb_destroy_window(c,w);

	xcb_disconnect(c);
	return 0;
}

