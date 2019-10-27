#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __linux__

#define emit_abs(dev_fd, axis, value)          \
  if (emit(dev_fd, EV_ABS, axis, value) == -1) \
    die("emit_abs failed");

#define emit_button(dev_fd, btn, value)       \
  if (emit(dev_fd, EV_KEY, btn, value) == -1) \
    die("emit_btn failed");

#elif defined _WIN32

#define abs_report(xpad_abs, vita_abs) \
  xpad_abs = (vita_abs - 128) * 128;

#define button_report(buttons_report, vita_button, xpad_button) \
  if (vita_button)                                              \
    buttons_report |= xpad_button;

#endif

#endif // __MAIN_H__