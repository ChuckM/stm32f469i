#include <stdio.h>
#include <stdint.h>
#include "../util/util.h"

int
main(void)
{
	printf("Hello world\n");
	while (1) {
		touch_event *e = get_touch(1);
		printf("[X, Y] => [%d, %d]\n", e->tp[0].x, e->tp[0].y);
	}
}
