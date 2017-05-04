#pragma once
/*
 * This is a bucket of samples, is is 'n' samples long.
 * And it represents collecting them at a rate 'r' per
 * second. So a total of n / r seconds worth of signal.
 */
typedef struct {
	float	*data;
	int		n;	/* number of samples */
	int		r;	/* sample rate in Hz */
} sample_buffer;

float cos_basis(float, float);
float sin_basis(float, float);
void add_cos(sample_buffer *, float, float);
void add_triangle(sample_buffer *, float, float);
void add_square(sample_buffer *, float, float);

