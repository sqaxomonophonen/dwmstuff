#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/select.h>

#include <X11/Xlib.h>

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

static void mysleep()
{
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	select(0, NULL, NULL, NULL, &timeout);
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

	int i = 0;
	for (;;i++) {
		char buf[SZ];
		char* bufptr = buf;
		size_t buf_remaining = SZ;

		char tmp[SZ];

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

			if (pct < 10 && i&1 && discharging) {
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

		mysleep();
	}

	XCloseDisplay(dpy);

	return 0;
}
