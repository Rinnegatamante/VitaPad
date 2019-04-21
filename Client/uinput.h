#ifndef __UINPUT_H__
#define __UINPUT_H__

int create_device();
int emit(int fd, int type, int code, int val);

#endif // __UINPUT_H__