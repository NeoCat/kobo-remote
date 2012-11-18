#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/input.h>

struct __attribute__((packed)) bitmapFileHeader {
	unsigned char	bfType[2];
	unsigned int	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;
};

struct __attribute__((packed)) bitmapInfoHeader {
	unsigned int	biSize;
	unsigned int	biWidth;
	unsigned int	biHeight;
	unsigned short	biPlanes;
	unsigned short	biBitCount;
	unsigned int	biCompression;
	unsigned int	biSizeImage;
	unsigned int	biXPixPerMeter;
	unsigned int	biYPixPerMeter;
	unsigned int	biClrUsed;
	unsigned int	biCirImportant;
};

static int send_screen(void)
{
	static unsigned short *pix565;
	unsigned int w = 600, h = 800, x, y;
	int fhsize = sizeof(struct bitmapFileHeader);
	int ihsize = sizeof(struct bitmapInfoHeader);
	struct bitmapFileHeader fileHeader =
		{"BM", fhsize+ihsize+w*h*3, 0, 0, fhsize+ihsize};
	struct bitmapInfoHeader infoHeader =
		{ihsize, w, h, 1, 24, 0, w*h*3, 0, 0, 0, 0};
	int fd, ret;

	fd = open("/dev/fb0", O_RDONLY);
	pix565 = mmap(NULL, w*h*2, PROT_READ, MAP_PRIVATE, fd, 0);
	if (pix565 == MAP_FAILED) {
		perror("fb0");
		exit(1);
	}

	puts("HTTP/1.0 200 OK");
	puts("Cache-control: no-cache");
	puts("Content-Type: image/bmp");
	puts("");

	fwrite(&fileHeader, sizeof(fileHeader), 1, stdout);
	fwrite(&infoHeader, sizeof(infoHeader), 1, stdout);

	for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
			unsigned short val = pix565[(w-x-1)*h+(h-y-1)];
		unsigned short r5 = (val>>11) & 0x1F; // 5 bits
		unsigned short g6 = (val>> 5) & 0x3F; // 6 bits
		unsigned short b5 = (val    ) & 0x1F; // 5 bits

		printf("%c%c%c",
		       (b5<<3)+(b5>>2), (g6<<2)+(g6>>4), (r5<<3)+(r5>>2));
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
	if (ret < 0) {
		puts("event write failed");
		exit(1);
	}
	timeradd(&event->time, &inc, &event->time);
}

static void write_touch_event(int fd, int x, int y, int pressure, int touch)
{
	struct input_event event;

	gettimeofday(&event.time, NULL);
	write_event(fd, &event, EV_ABS, ABS_X, y);
	write_event(fd, &event, EV_ABS, ABS_Y, 600-x);
	write_event(fd, &event, EV_ABS, ABS_PRESSURE, pressure);
	write_event(fd, &event, EV_KEY, BTN_TOUCH, touch);
	write_event(fd, &event, EV_SYN, 0, 0);
}

static void tap_screen(int x, int y)
{
	int fd;

	puts("HTTP/1.0 200 OK");
	puts("Content-type: text/plain");
	puts("Cache-control: no-cache");
	puts("");

	if (x < 0 || x >= 600 || y < 0 || y >= 800) {
		puts("out of range\n");
		return;
	}

	fd = open("/dev/input/event1", O_RDWR);
	if (fd < 0) {
		puts("event open failed");
		return;
	}

	write_touch_event(fd, x, y, 100, 1);
	usleep(10000);
	write_touch_event(fd, x, y, 0, 0);
	puts("OK");
}


static void send_index(void)
{
	puts("HTTP/1.0 200 OK");
	puts("Content-type: text/html");
	puts("");
	puts("<html><head><title>Kobo remote</title></head><body style=\"margin:0;background-color:#888;\"><img src=\"screen\" onclick=\"document.getElementById('frm').src='tap/'+event.layerX+'/'+event.layerY;setTimeout('location.reload(true)',1000)\" align=\"left\"><a href=\"/\">Refresh</a><br><iframe id='frm' border=0 marginwidth=0 marginheight=0 height=24 width=80 frameborder=0></iframe></body></html>");
}

static void send_error(void)
{
	puts("HTTP/1.0 404 Not found");
	puts("Content-type: text/plain");
	puts("");
	puts("404 Not found");
}

int main()
{
	char buffer[256];
	int x, y;

	if (scanf("GET %255s", buffer) != 1)
		return 0;

	if (strcmp("/", buffer) == 0)
		send_index();
	else if (strcmp("/screen", buffer) == 0)
		send_screen();
	else if (sscanf(buffer, "/tap/%d/%d", &x, &y) == 2)
		tap_screen(x, y);
	else
		send_error();

	return 0;
}
