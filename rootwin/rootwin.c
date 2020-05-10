#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/select.h>
#include <stdint.h>
#include <sys/time.h>
#include <assert.h>

#include <X11/Xlib.h>
#include <X11/XF86keysym.h>

#define SZ (1<<12)

#define BUFN(...) \
	{ \
		int _r = snprintf(bufptr, buf_remaining, __VA_ARGS__); \
		if (_r < 0) { \
			perror("snprintf"); \
			exit(1); \
		} \
		buf_remaining -= _r; \
		bufptr += _r; \
	}

#define SEPARATOR BUFN("  ••  ")

#define READTMP(path) \
	{ \
		FILE* f = fopen(path, "r"); \
		if (f == NULL) { \
			continue; \
		} else { \
			size_t r = fread(tmp, 1, SZ-1, f); \
			fclose(f); \
			if (r < 1) { \
				continue; \
			} else { \
				tmp[r-1] = 0; \
			} \
		} \
	}

#define PSFIX "/sys/class/power_supply/"

static int wait_fd_seconds(int fd, double seconds)
{
	struct timeval tv;
	fd_set in_fds;
	FD_ZERO(&in_fds);
	FD_SET(fd, &in_fds);
	tv.tv_sec = trunc(seconds);
	tv.tv_usec = (seconds - trunc(seconds)) * 1e6;
	return select(fd+1, &in_fds, 0, 0, &tv);
}

static int XNextEventTimeout(Display *display, XEvent *event, double seconds)
{
	if (XPending(display) || wait_fd_seconds(ConnectionNumber(display), seconds)) {
		XNextEvent(display, event);
		return 1;
	} else {
		return 0;
	}
}

static int wait_for_timer(double* wait_seconds, uint64_t* tick)
{
	const int ticks_per_second = 3;
	const uint64_t half_tick_usec = 1000000 / (ticks_per_second * 2);

	struct timeval tv;
	assert(gettimeofday(&tv, NULL) == 0);
	uint64_t current_tick = (tv.tv_sec * (ticks_per_second*2) + (tv.tv_usec / half_tick_usec) + 1) / 2;
	if (current_tick == *tick) {
		*wait_seconds = ((double)(half_tick_usec - (tv.tv_usec % half_tick_usec))) / 1e6;
		return 1;
	} else {
		*tick = current_tick;
		return 0;
	}
}

int main(int argc, char** argv)
{
	Display* dpy;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "%s: cannot open display.\n", argv[0]);
		return 1;
	}

	Window root = DefaultRootWindow(dpy);

	{
		const char* background = "#000810";
		XColor color;
		Colormap cm = DefaultColormap(dpy, DefaultScreen(dpy));
		if (!XParseColor(dpy, cm, background, &color)) {
			fprintf(stderr, "XParseColor failed for color \"%s\"\n", background);
			exit(1);
		}
		if (!XAllocColor(dpy, cm, &color)) {
			fprintf(stderr, "XAllocColor failed for color \"%s\"\n", background);
			exit(1);
		}
		XSetWindowBackground(dpy, root, color.pixel);
		XClearWindow(dpy, root);
	}

	XSelectInput(dpy, root, KeyPressMask);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_MonBrightnessDown), 0, root, False, GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_MonBrightnessUp), 0, root, False, GrabModeAsync, GrabModeAsync);

	uint64_t tick = 0;
	for (;;) {
		//printf("tick %ld\n", tick);
		char buf[SZ];
		char* bufptr = buf;
		size_t buf_remaining = SZ;

		char tmp[SZ];

		for (;;) {
			double wait_seconds;
			if (!wait_for_timer(&wait_seconds, &tick)) break;

			XEvent ev;
			if (XNextEventTimeout(dpy, &ev, wait_seconds)) {
				KeySym sym;
				double brightness_delta = 0;
				switch (ev.type) {
				case KeyPress:
					sym = XLookupKeysym(&ev.xkey, 0);
					if (sym == XF86XK_MonBrightnessDown) {
						brightness_delta = -1;
					} else if (sym == XF86XK_MonBrightnessUp) {
						brightness_delta = 1;
					}
					break;
				}

				if (brightness_delta != 0) {
					READTMP("/sys/class/backlight/intel_backlight/max_brightness");
					double max_brightness = strtod(tmp, NULL);

					const char* brightness_path = "/sys/class/backlight/intel_backlight/brightness";
					READTMP(brightness_path);
					double brightness = strtod(tmp, NULL);

					const double power = 2.4;
					const double step = 0.015;

					double new_brightness = pow(pow(brightness / max_brightness, 1/power) + brightness_delta*step, power) * max_brightness;
					if (new_brightness > max_brightness) new_brightness = max_brightness;
					if (!isfinite(new_brightness)) new_brightness = 0;
					if (new_brightness < 1) new_brightness = 1;

					FILE* f = fopen(brightness_path, "w");
					if (f != NULL) {
						//printf("%d -> %s\n", (int)new_brightness, brightness_path);
						fprintf(f, "%d\n", (int)new_brightness);
						fclose(f);
					} else {
						fprintf(stderr, "%s: could not write\n", brightness_path);
					}
				}
			}


		}


		{
			READTMP("/proc/loadavg");
			BUFN("%s", tmp);
		}

		SEPARATOR;

		{
			READTMP(PSFIX "BAT0/energy_now");
			double now = strtod(tmp, NULL);

			READTMP(PSFIX "BAT0/energy_full");
			double full = strtod(tmp, NULL);

			int pct = (int)round((now / full) * 100.0);

			READTMP(PSFIX "AC/online");
			int charging = strcmp(tmp, "1") == 0;
			int discharging = !charging;

			char sep;
			if (discharging) {
				sep = '-';
			} else if (charging && pct < 90) {
				sep = '+';
			} else {
				sep = ':';
			}

			if (pct < 10 && tick&1 && discharging) {
				BUFN("    ");
			} else {
				BUFN("B%c%d%%", sep, pct);
			}
		}

		SEPARATOR;

		{
			time_t t = time(NULL);
			struct tm* tm = localtime(&t);
			if (tm == NULL) {
				perror("localtime");
				exit(1);
			}

			strftime(tmp, sizeof(tmp), "%F %T", tm);

			BUFN("%s", tmp);
		}


		XStoreName(dpy, root, buf);
		XSync(dpy, False);
	}

	XCloseDisplay(dpy);

	return 0;
}
