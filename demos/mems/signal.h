#pragma once
#include <stdint.h>
#include <string.h> /* for memset */

/* For the Cortex-4F samples are SP floating point */
typedef float sample_t;

/*
 * This is a bucket of samples, is is 'n' samples long.
 * And it represents collecting them at a rate 'r' per
 * second. So a total of n / r seconds worth of signal.
 */
typedef struct {
	sample_t	sample_min, sample_max;
	int		n;	/* number of samples */
	int		r;	/* sample rate in Hz */
	sample_t	*data;
} sample_buffer;

#define min(x, y)	((x < y) ? x : y)
#define max(x, y)	((x > y) ? x : y)

#define set_minmax(s, ndx)	{ s->sample_min = min(s->sample_min, s->data[ndx]); \
							  s->sample_max = max(s->sample_max, s->data[ndx]); }

#define reset_minmax(s)		s->sample_min = s->sample_max = 0

#define clear_samples(s)	memset(s->data, 0, sizeof(sample_t) * s->n)

/*
 * Some syntactic sugar to make this oft used code
 */
sample_buffer *alloc_buf(int size);
void free_buf(sample_buffer *buf);
void add_cos(sample_buffer *, float, float);
void add_triangle(sample_buffer *, float, float);
void add_square(sample_buffer *, float, float);
void dft(sample_buffer *s, float min_freq, float max_freq, int bins, 
	sample_buffer *rx, sample_buffer *im, sample_buffer *mag);


