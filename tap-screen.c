#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <linux/input.h>

int watch(int fd)
{
	struct input_event event;

	for (;;) {
		if (read(fd, &event, sizeof(event)) != sizeof(event))
			exit(1);
		printf("type:%s(%02x)\tcode=%d\tval=%d\n",
		       event.type == EV_KEY?"KEY":
		       event.type == EV_REL?"REL":
		       event.type == EV_ABS?"ABS":"???",
		       event.type, event.code, event.value);
	}
}

static void write_event(int fd, struct input_event *event,
		 __u16 type, __u16 code, __u32 value)
{
	int ret;
	struct timeval inc = {0, 8};

	event->type = type;
	event->code = code;
	event->value = value;
	ret = write(fd, event, sizeof(*event));
	if (ret < 0)
		err(errno, "write_event");
	timeradd(&event->time, &inc, &event->time);
}

static void write_touch_event(int fd, struct input_event *event,
			      int x, int y, int pressure, int touch)
{
	gettimeofday(&event->time, NULL);
	write_event(fd, event, EV_ABS, ABS_X, y);
	write_event(fd, event, EV_ABS, ABS_Y, 600-x);
	write_event(fd, event, EV_ABS, ABS_PRESSURE, pressure);
	write_event(fd, event, EV_KEY, BTN_TOUCH, touch);
	write_event(fd, event, EV_SYN, 0, 0);
}

int main(int argc, char *argv[])
{
	struct input_event event;
	const char *dev = "/dev/input/event1";
	int fd, x, y;

	fd = open(dev, O_RDWR);
	if (fd < 0)
		err(errno, dev);

	if (argc < 3) {
		fprintf(stderr, "usage: %s x y\n", argv[0]);
		fprintf(stderr, "entering watch mode\n");
		watch(fd);
	}
	x = atoi(argv[1]);
	y = atoi(argv[2]);
	if (x < 0 || x >= 600 || y < 0 || y >= 800) {
		fprintf(stderr, "out of range\n");
		exit(1);
	}


	write_touch_event(fd, &event, x, y, 100, 1);
	usleep(10000);
	write_touch_event(fd, &event, x, y, 0, 0);

	return 0;
}
