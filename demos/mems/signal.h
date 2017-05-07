#pragma once

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

/*
 * Some syntactic sugar to make this oft used code
 */
void set_minmax(sample_buffer *s, int ndx);
void clear_samples(sample_buffer *s);
sample_buffer *alloc_buf(int size);
void free_buf(sample_buffer *buf);
void add_cos(sample_buffer *, float, float);
void add_triangle(sample_buffer *, float, float);
void add_square(sample_buffer *, float, float);
void dft(sample_buffer *s, float min_freq, float max_freq, int bins, 
	sample_buffer *rx, sample_buffer *im, sample_buffer *mag);


