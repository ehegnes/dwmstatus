/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

#define DELAY	5
#define GB	1073741824

#define BATT_PATH "/sys/class/power_supply/BAT0/"

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

char *
mktimes(char *fmt)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[1];

	if (getloadavg(avgs, 1) < 0)
		return smprintf("");

	return smprintf("%.2f", avgs[0]);
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	int battery;
	char status;
	FILE *fin;

	fin = fopen(BATT_PATH"capacity", "r");
	fscanf(fin, "%d\n", &battery);
	fclose(fin);
	fin = fopen(BATT_PATH"status", "r");
	fscanf(fin, "%c\n", &status);
	fclose(fin);

	return smprintf("%c %d%%", status, battery);
}

char *
freespace(void)
{
	struct statvfs vfs;

	if (statvfs("/", &vfs) < 0) {
		perror("statvfs");
		exit(1);
	}

	return smprintf("%.2f GB", (double)(vfs.f_bfree * vfs.f_bsize) / GB);
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

int
main(void)
{
	char *status;
	char *avgs;
	char *bat;
	char *space;
	char *time;
	char *temp0;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(DELAY)) {
		avgs = loadavg();
		bat = getbattery("/sys/class/power_supply/BAT0");
		time = mktimes("%m-%d %l:%M %p");
		temp0 = gettemperature("/sys/class/hwmon/hwmon0", "temp1_input");
		space = freespace();

		status = smprintf("%s | %s | %s | %s | %s",
				temp0, avgs, space, bat, time);
		setstatus(status);

		free(temp0);
		free(avgs);
		free(bat);
		free(space);
		free(time);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

