#include "../include/Screen.h"

#include <sys/ioctl.h>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/fb.h>

using namespace vid;

// Screen::Error

Screen::Error::Error(Screen::Error::ErrorValue value) noexcept : value(value) { }

Screen::Error::operator int() const noexcept { return value; }

// Screen

int interruptedIoctl(int fd, unsigned long request, void* argp) {
	int returnValue;
	do { returnValue = ioctl(fd, request, argp); }
	while (returnValue == -1 && errno == EINTR);
	return returnValue;
}

// NOTE: Commented some of these out, see header.
//uint32_t Screen::virtualWidth() const noexcept { return variableInfo.xres_virtual; }
//uint32_t Screen::virtualHeight() const noexcept { return variableInfo.yres_virtual; }
uint32_t Screen::width() const noexcept { return variableInfo.xres; }
uint32_t Screen::height() const noexcept { return variableInfo.yres; }
//uint32_t Screen::xOffset() const noexcept { return variableInfo.xoffset; }
//uint32_t Screen::yOffset() const noexcept { return variableInfo.yoffset; }

Screen::Error Screen::open() {
	if (fd != -1) { return Error::not_closed; }
	fd = ::open("/dev/fb0", O_RDWR | O_NONBLOCK);
	if (fd == -1) { return Error::file_open_failed; }
	return Error::none;
}

Screen::Error Screen::init() {
	if (initialized) { return Error::not_freed; }
	if (interruptedIoctl(fd, FBIOGET_VSCREENINFO, &variableInfo) == -1) { return Error::device_variable_info_unavailable; }
	frameSize = variableInfo.xres * variableInfo.yres * variableInfo.bits_per_pixel / 8;
	frame = mmap(nullptr, frameSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (frame == MAP_FAILED) { return Error::mmap_failed; }
	initialized = true;
	return Error::none;
}

Screen::Error Screen::free() {
	if (!initialized) { return Error::already_freed; }
	if (munmap(frame, frameSize) == -1) { return Error::munmap_failed; }
	initialized = false;
	return Error::none;
}

Screen::Error Screen::close() {
	if (fd == -1) { return Error::already_closed; }
	Error err = free(); if (err != Error::none && err != Error::already_freed) { return err; }
	if (::close(fd) == -1) { fd = -1; return Error::file_close_failed; }
	fd = -1;
	return Error::none;
}

Screen::~Screen() { close(); }
