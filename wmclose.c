#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_aux.h>

xcb_connection_t    *c;
xcb_screen_t        *s;

int main(int argc, char *argv[])
{
	c = xcb_connect(NULL,NULL);
	s = xcb_setup_roots_iterator( xcb_get_setup(c) ).data;

	int id=0;
	sscanf(argv[1],"%i",&id);
	if(id==0) return 1;
	printf("closing %u\n",id);

	xcb_client_message_event_t ev;
	memset(&ev,0,sizeof(ev));
        ev.response_type = XCB_CLIENT_MESSAGE;
        ev.window = id;
        ev.format = 32;
        ev.data.data32[1] = XCB_CURRENT_TIME;
        ev.type = xcb_atom_get(c,"WM_PROTOCOLS");
        ev.data.data32[0] = xcb_atom_get(c,"WM_DELETE_WINDOW");

        xcb_send_event(c,0,id,XCB_EVENT_MASK_NO_EVENT,(char *)&ev);
	xcb_flush(c);

	xcb_disconnect(c);
	return 0;
}

