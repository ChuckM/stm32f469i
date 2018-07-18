/*
 * process mic data
 *
 * Ok, we've sampled the microphone(s) for some period of time now
 * we want to pull out "signal" if we can find it. 
 *
 * Things we know 
 *		- the samples are taken at 3Mhz
 *		- they are Pulse Density Modulated.
 *		- the bits arrived MSB first
 * Things we aren't sure of
 *		- are both channels there or just one?
 *		- can we do a simple low pass filter and win?
 *		- what counts as a low pass filter, can we just average them?
 *		- what happens if we decimate by 64 and then filter
 *		- what is the "resulting" sample frequency?
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>

/* cheap popcnt implementation */
uint8_t count[256] = {
	0,1,1,2,1,2,2,3,
	1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,
	2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,
	2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,
	2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,
	4,5,5,6,5,6,6,7,
	1,2,2,3,2,3,3,4,
	2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,
	4,5,5,6,5,6,6,7,
	2,3,3,4,3,4,4,5,
	3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,
	4,5,5,6,5,6,6,7,
	3,4,4,5,4,5,5,6,
	4,5,5,6,5,6,6,7,
	4,5,5,6,5,6,6,7,
	5,6,6,7,6,7,7,8
};

int
main(int argc, char *argv[])
{
	FILE		*data;
	int			fd;
	uint8_t		*res, *samples;
	int			samp_cnt;
	uint8_t		buf[256];
	int			i, j, k;

	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		printf("Failed to open file, did you pass a filename to the program?\n");
		exit(1);
	}
	/* EXP 1: Decimate by 256 and consider it one channel 
	 * Result:	This was a bit weird the resulting data
	 *			appears as a logrithmic decay line.
	 * EXP 2: Consder it 2 channels, alternating on 16 bit boundaries
	 */
	samples = malloc(50000);
	if (samples == NULL) {
		printf("no memory\n");
		exit(1);
	}
	samp_cnt = 0;
	res = samples;
	while (1) {

		i = read(fd, buf, 64);
		if (i < 64) {
			break;
		}
		*res = 0;
		for (j = 0; j < 64; j += 4) {
			*res += count[buf[j]];
			*res += count[buf[j+1]];
		}
		res++; samp_cnt++;
	}
	printf("Collected %d samples.\n", samp_cnt);
	close(fd);
	data = fopen("processed-samples.dat", "w");
	for (j = 0; j < samp_cnt; j++) {
		fprintf(data, "%d\n", *(samples + j));
	}
	fclose(data);
}
