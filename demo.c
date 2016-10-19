#include <stdio.h>
#include <unistd.h>
#include "ctlr/ctlr.h"

void demo_event_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			printf("[%s] button %d\n",
			       e->button.pressed ? " X " : "   ",
			       e->id);
			break;
		case CTLR_EVENT_ENCODER:
			printf("[%s] encoder %d\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       e->id);
			break;
		case CTLR_EVENT_GRID:
			if(e->grid.flags & CTLR_EVENT_GRID_BUTTON) {
				pressed = e->grid.pressed ? " X " : "   ";
			} else {
				pressed = "---";
			}
			printf("[%s] grid %d", pressed, e->grid.pos);
			if(e->grid.flags & CTLR_EVENT_GRID_PRESSURE)
				printf(", pressure %1.3f", e->grid.pressure);
			printf("\n");
			break;
		default:
			break;
		};
	}
}

int main()
{
	int dev_id = CTLR_DEV_SIMPLE;
	void *userdata = 0x0;
	void *future = 0x0;
	struct ctlr_dev_t *dev = ctlr_dev_connect(dev_id,
						  demo_event_func,
						  userdata,
						  future);

	uint32_t i = 8;
	while(i > 0)
	{
		ctlr_dev_poll(dev);
		i--;
	}

	uint32_t light_id = 30;
	//uint32_t light_status = UINT32_MAX;
	const uint32_t BLINK  = (1 << 31);
	const uint32_t BRIGHT = (0x7F << 24);
	uint32_t light_status = BLINK | BRIGHT | (0xFF << 16) | (0x0 << 8) | (0x0);
	sleep(1);
	ctlr_dev_light_set(dev, 31, light_status);
	ctlr_dev_light_set(dev, 31+4, light_status);
	ctlr_dev_light_set(dev, 31+8, light_status);

	for(int i = 0; i < 255; i++) {
		ctlr_dev_light_set(dev, 9, light_status);
		usleep(200*50);
		printf("%d\n", i);
	}

	printf("done - sleeping 1\n");
	sleep(1);
	ctlr_dev_light_set(dev, light_id, light_status * 0);
	sleep(1);
	ctlr_dev_disconnect(dev);
	return 0;
}