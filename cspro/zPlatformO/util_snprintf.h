#pragma once

#include <stdarg.h>

int ap_snprintf(wchar_t *buf, size_t len, const wchar_t *format,...);
int ap_vsnprintf(wchar_t *buf, size_t len, const wchar_t *format, va_list ap);
