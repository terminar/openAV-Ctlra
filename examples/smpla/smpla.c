#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define SMPLA_JACK
#include <jack/jack.h>

#include "ctlra.h"
#include "audio.h"

static volatile uint32_t done;

/* TODO: use LV2 audio IO in future - so static is no issue */
static jack_client_t* client;
static jack_port_t* outputPortL;
static jack_port_t* outputPortR;

#define MODE_GROUP   0
#define MODE_PADS    1
#define MODE_PATTERN 2

static struct smpla_t *s;


struct col_t
{
	uint8_t green;
	uint8_t blue;
	uint8_t red;
};
static const struct col_t grp_col[] = {
	{0x00, 0x92,  254},
	{0xf9, 0x21, 0x00},
	{0x00, 0xf0, 0x00},
	{0x00, 0x00, 0xf0},
	{0xf0, 0xf0, 0xb0},
	{0x3f, 0x3f, 0xf0},
	{0xf0, 0x00, 0xf0},
	{0x00, 0xef, 0xd5},
	/* dark */
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00},
};

uint32_t col_dim(const struct col_t *col, float bri)
{
	struct col_t c;
	c.red   = col->red   * bri;
	c.green = col->green * bri;
	c.blue  = col->blue  * bri;
	uint32_t intCol = *(uint32_t *)&c;
	return intCol;
}


struct mm_t
{
	/* 0 Group   : Selects active group
	 * 1 Pads    : Plays different notes
	 * 2 Pattern : Sequence a single note
	 */
	uint8_t mode;
	uint8_t mode_prev;
	uint8_t shift_pressed;

	/* GROUP */
	uint8_t grp_id; /* 0 - 7, selected group */

	/* transport */
	uint8_t playing; /* stopped if not */

	/* Pads */
	uint8_t pads_pressed[16];
	uint8_t pads_seq_play[16];

	/* Pattern */
	uint8_t pattern_pad_id; /* the pad that this pattern is playing */
};

void machine3_feedback_func(struct ctlra_dev_t *dev, void *d)
{
	/* group, pads, sequencer modes */
	struct smpla_t *s = d;

	struct mm_t *mm = s->controller_data;
	int static_mute = 0;

	const struct col_t *col = &grp_col[mm->grp_id];
	struct Sequencer *sequencer = s->sequencers[mm->pattern_pad_id];

	switch(mm->mode) {
	case MODE_PATTERN: {
		/* layout all steps */
		for(int i = 0; i < 16; i++) {
			int on = sequencer_get_step(sequencer, i);
			ctlra_dev_light_set(dev, 87 + i,
					    col_dim(col, on ? 0.4 : 0.0));
		}

		/* highlight current step */
		int step = sequencer_get_current_step( sequencer );
		int on = sequencer_get_step(sequencer, step % 16);
		//printf("step %d on %d\n", step, on);
		if(on) {
			ctlra_dev_light_set(dev, 87 + step,
					    col_dim(&grp_col[mm->grp_id], 1.0));
		}

		if(mm->shift_pressed) {
			printf("setting pattern %d\n", mm->pattern_pad_id);
			ctlra_dev_light_set(dev, 87 + mm->pattern_pad_id,
					    0xff0000ff);
		}
		break;
		}
	case MODE_GROUP:
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, 87 + i,
				col_dim(&grp_col[i], i == mm->grp_id ? 1.0 : 0.05));
		}
		break;
	case MODE_PADS:
		/* pressed pads (manually - hence overrides */
		for(int i = 0; i < 16; i++) {
			ctlra_dev_light_set(dev, 87 + i,
					    col_dim(&grp_col[mm->grp_id],
						    mm->pads_pressed[i] ? 1.0 : 0));
		}
		/* sequencer playbacks */
		for(int i = 0; i < 16; i++) {
			if(mm->pads_seq_play[i] && !static_mute)
				ctlra_dev_light_set(dev, 87 + i, 0xffffffff);
		}
		break;
	}

	ctlra_dev_light_set(dev, 57, static_mute ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 46, mm->mode == MODE_PADS ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 50, mm->mode == MODE_GROUP ? 0xffffffff : 0x3);
	ctlra_dev_light_set(dev, 49, mm->mode == MODE_PATTERN ? 0xffffffff : 0x3);

	ctlra_dev_light_flush(dev, 1);
}

void machine3_event_func(struct ctlra_dev_t* dev,
                     uint32_t num_events,
                     struct ctlra_event_t** events,
                     void *userdata)
{
	struct smpla_t *s = userdata;
	struct mm_t *mm = s->controller_data;
	uint8_t msg[3] = {0};
	int static_mute = 0;

	for(uint32_t i = 0; i < num_events; i++) {
		struct ctlra_event_t *e = events[i];
		int pr = 0;
		switch(e->type) {
		case CTLRA_EVENT_BUTTON:
			msg[0] = e->button.pressed ? 0x90 : 0x80;
			msg[1] = 60 + e->button.id;
			msg[2] = e->button.pressed ? 0x70 : 0;
			(void)msg;
			pr = e->button.pressed;
			switch(e->button.id) {
			case  5: mm->shift_pressed = pr; break;
			case 24: mm->mode = MODE_PATTERN; break;
			case 21: mm->mode = MODE_PADS; break;
			case 33: if(pr) static_mute = !static_mute; break;
			case 41: if(pr) mm->playing = 1; break;
			case 43: if(pr) mm->playing = 0; break;
			case 26:
				if(pr) {
					mm->mode_prev = mm->mode;
					mm->mode = MODE_GROUP;
				} else {
					mm->mode = mm->mode_prev;
				}
				break;
			default:
				printf("button %d\n", e->button.id);
				break;
			}
			printf("button %d\n", e->button.id);
			break;

		case CTLRA_EVENT_ENCODER:
			break;

		case CTLRA_EVENT_SLIDER:
			break;

		case CTLRA_EVENT_GRID: {
			pr = e->grid.pressed;
			int p = ((3 - (e->grid.pos / 4)) * 4) + (e->grid.pos % 4);
			printf("pad %d, mode = %d\n", p, mm->mode);
			mm->pads_pressed[p] = pr;
			if(mm->mode == MODE_GROUP) {
				/* select new group here */
				if(e->grid.pressed && p < 8) {
					mm->grp_id = p;
					printf("new group %d\n", p);
				}
			} else if(mm->mode == MODE_PADS && pr) {
				/* trigger a sample now */

				struct smpla_sample_state_t d = {
					.pad_id = 0,
					/* actions:
					 * 0) stop (also stops recording)
					 * 1) start playback from frame_start, to frame_end
					 * 2) record:
					 */
					.action = 1,
					.sample_id = p,
					.frame_start = 0,
					.frame_end = -1,
				};
				smpla_to_rt_write(s, smpla_sample_state,
						  &d, sizeof(d));
			} else if(mm->mode == MODE_PATTERN && pr) {
				printf("pattern pr\n");
				if(mm->shift_pressed) {
					mm->pattern_pad_id = p;
				} else {
					sequencer_toggle_step(s->sequencers[mm->pattern_pad_id], p);
				}
			}


			} break;

#if 0
			pr = e->grid.pressed;
			if(mm->mode == MODE_PATTERN && pr) {
				if(mm->shift_pressed) {
					mm->pattern_pad_id = e->grid.pos;
					//printf("new pattern pad id %d\n", mm->pattern_pad_id);
				}
				else
					sequencer_toggle_step(sequencer, e->grid.pos);
			}
			mm->pads_pressed[e->grid.pos] = e->grid.pressed;
			break;
#endif

		default:
			break;
		};
	}

	ctlra_dev_light_flush(dev, 0);
}

int ignored_input_cb(uint8_t nbytes, uint8_t * buffer, void *ud)
{
	return 0;
}


void sighndlr(int signal)
{
	done = 1;
	printf("\n");
}

void machine3_remove_func(struct ctlra_dev_t *dev, int unexpected_removal,
		     void *userdata)
{
}

int accept_dev_func(struct ctlra_t *ctlra,
		    const struct ctlra_dev_info_t *info,
		    struct ctlra_dev_t *dev,
                    void *userdata)
{
	ctlra_dev_set_event_func(dev, machine3_event_func);
	ctlra_dev_set_feedback_func(dev, machine3_feedback_func);
	//ctlra_dev_set_screen_feedback_func(dev, machine3_screen_redraw_func);
	ctlra_dev_set_remove_func(dev, machine3_remove_func);
	ctlra_dev_set_callback_userdata(dev, userdata);

	/* mk3 */
	if(info->device_id == 0x1600) {
	}

	printf("smpla: accepting %s %s\n", info->vendor, info->device);
	return 1;
}

void seqEventCb(int frame, int note, int velocity, void* userdata )
{
	printf("%s: frame %d, note %d : vel %d\n", __func__, frame, note, velocity);

	int static_mute = 0;
	if(static_mute)
		return;

	struct smpla_t *s = userdata;
	struct mm_t *mm = s->controller_data;
	memset(mm->pads_seq_play, 0, sizeof(mm->pads_seq_play));
	mm->pads_seq_play[note] = velocity;

	struct smpla_sample_state_t d = {
		.pad_id = 0,
		.action = 1,
		.sample_id = note,
		.frame_start = 0,
		.frame_end = -1,
	};
	smpla_sample_state(s, &d);

}

int process(jack_nframes_t nframes, void* arg)
{
	struct smpla_t *s = arg;

	float* buf_l = (float*)jack_port_get_buffer (outputPortL, nframes);
	memset(buf_l, 0, nframes * sizeof(float));
	float* buf_r = (float*)jack_port_get_buffer (outputPortR, nframes);
	memset(buf_r, 0, nframes * sizeof(float));

	const float *ins[2] = {buf_l, buf_r};
	float *outs[2] = {buf_l, buf_r};

	int ret = smpla_process(s, nframes, ins, outs);
	(void)ret;

	return 0;
}


int main()
{
	signal(SIGINT, sighndlr);

#ifdef SMPLA_JACK
	/* setup JACK */
	client = jack_client_open("Smpla", JackNullOption, 0, 0 );
	int sr = jack_get_sample_rate(client);
#endif

	s = smpla_init(sr);

	for(int i = 0; i < 16; i++) {
		struct Sequencer *sequencer = sequencer_new(sr);
		sequencer_set_callback(sequencer, seqEventCb, s);
		sequencer_set_length(sequencer, sr * 0.8);
		sequencer_set_num_steps(sequencer, 16);
		sequencer_set_note(sequencer, i);
		s->sequencers[i] = sequencer;
	}

	struct mm_t *mm = calloc(1, sizeof(struct mm_t));
	mm->mode = MODE_PADS;
	s->controller_data = mm;

	struct ctlra_t *ctlra = ctlra_create(NULL);
	int num_devs = ctlra_probe(ctlra, accept_dev_func, s);
	printf("sequencer: Connected controllers %d\n", num_devs);

	s->ctlra = ctlra;

#ifdef SMPLA_JACK
	if(jack_set_process_callback(client, process, s)) {
		printf("error setting JACK callback\n");
	}
	outputPortL = jack_port_register(client, "output_l",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );
	outputPortR = jack_port_register(client, "output_r",
	                                JACK_DEFAULT_AUDIO_TYPE,
	                                JackPortIsOutput, 0 );
	jack_activate(client);
#endif

	uint32_t sleep = sr / 128;

	while(!done) {
		usleep(1000*1000);
	}

	ctlra_exit(ctlra);

	return 0;
}
