#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "ctlr/ctlr.h"

void kontrol_z1_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			ctlr_dev_light_set(dev, e->button.id, UINT32_MAX);
			break;
		case CTLR_EVENT_ENCODER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			break;
		case CTLR_EVENT_SLIDER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			if(e->slider.id == 11) {
				uint32_t iter = (int)((e->slider.value+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlr_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlr_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlr_dev_light_set(dev, 1 + i, 0);
					ctlr_dev_light_set(dev, 8 + i, 0);
				}
			}

			break;
		case CTLR_EVENT_GRID:
			name = ctlr_dev_control_get_name(dev, e->button.id);
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

	ctlr_dev_light_flush(dev, 0);
}

void kontrol_x1_func(struct ctlr_dev_t* dev,
		     uint32_t num_events,
		     struct ctlr_event_t** events,
		     void *userdata)
{
	(void)dev;
	(void)userdata;
	for(uint32_t i = 0; i < num_events; i++) {
		char *pressed = 0;
		struct ctlr_event_t *e = events[i];
		const char *name = 0;
		switch(e->id) {
		case CTLR_EVENT_BUTTON:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] button %s (%d)\n",
			       e->button.pressed ? " X " : "   ",
			       name, e->button.id);
			ctlr_dev_light_set(dev, e->button.id, UINT32_MAX);
			break;
		case CTLR_EVENT_ENCODER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%s] encoder %s (%d)\n",
			       e->encoder.delta > 0 ? " ->" : "<- ",
			       name, e->button.id);
			break;
		case CTLR_EVENT_SLIDER:
			name = ctlr_dev_control_get_name(dev, e->button.id);
			printf("[%03d] slider %s (%d)\n",
			       (int)(e->slider.value * 100.f),
			       name, e->slider.id);
			if(e->slider.id == 11) {
				uint32_t iter = (int)((e->slider.value+0.05) * 7.f);
				for(i = 0; i < iter; i++) {
					ctlr_dev_light_set(dev, 1 + i, UINT32_MAX);
					ctlr_dev_light_set(dev, 8 + i, UINT32_MAX);
				}
				for(; i < 7.0; i++) {
					ctlr_dev_light_set(dev, 1 + i, 0);
					ctlr_dev_light_set(dev, 8 + i, 0);
				}
			}

			break;
		case CTLR_EVENT_GRID:
			name = ctlr_dev_control_get_name(dev, e->button.id);
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

	ctlr_dev_light_flush(dev, 0);
}


static volatile uint32_t done;

void sighndlr(int signal)
{
	done = 1;
}

int main()
{
	signal(SIGINT, sighndlr);

	struct ctlr_dev_t* dev_x1;
	struct ctlr_dev_t* dev_z1;
	//int dev_id = CTLR_DEV_SIMPLE;
	//int dev_id = CTLR_DEV_NI_KONTROL_Z1;
	//int dev_id = CTLR_DEV_NI_KONTROL_X1_MK2;
	//int dev_id = CTLR_DEV_NI_MASCHINE_MIKRO_MK2;
	void *userdata = 0x0;
	void *future = 0x0;
	dev_x1 = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_X1_MK2, kontrol_x1_func, userdata, future);
	if(!dev_x1)
		return -1;
	dev_z1 = ctlr_dev_connect(CTLR_DEV_NI_KONTROL_Z1, kontrol_z1_func, userdata, future);
	if(!dev_z1)
		return -1;

	printf("polling loop now..\n");
	while(!done) {
		ctlr_dev_poll(dev_x1);
		ctlr_dev_poll(dev_z1);
		usleep(100);
	}

	ctlr_dev_disconnect(dev_x1);
	ctlr_dev_disconnect(dev_z1);

	return 0;
}
