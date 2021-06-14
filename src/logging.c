#include "logging.h"
#include "basics.h"
#include "serial_communications_port.h"

#define BUF_SIZE 200

char buffer[BUF_SIZE];

static uint64_t write_prefix_to_buffer(sloc loc) {
  buffer[0] = '[';
  uint64_t written = 1 + strcpy_s(buffer + 1, loc.file, BUF_SIZE - 1);
  if (written >= BUF_SIZE)
    return strcpy_s(buffer, "[source location too long]: ", BUF_SIZE);

  buffer[written] = ':';
  written++;

  written += fmt_u64(loc.line, buffer + written, BUF_SIZE - written);
  if (written >= BUF_SIZE)
    return strcpy_s(buffer, "[source location too long]: ", BUF_SIZE);

  written += strcpy_s(buffer + written, "]: ", BUF_SIZE - written);
  if (written >= BUF_SIZE)
    return strcpy_s(buffer, "[source location too long]: ", BUF_SIZE);

  return written;
}

void logging__log(sloc loc, uint32_t count, any *args) {
  uint64_t written = write_prefix_to_buffer(loc);

  for (uint32_t i = 0; i < count; i++) {
    written += any__fmt(args[i], buffer + written, BUF_SIZE - written);
    if (written > BUF_SIZE) // TODO expand buffer instead of crashing
      panic();
  }

  for (uint32_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__log_fmt(sloc loc, const char *fmt, uint32_t count, any *args) {
  uint64_t written = write_prefix_to_buffer(loc);

  uint64_t format_count = 0;
  while (written < BUF_SIZE && *fmt) {
    if (*fmt != '%') {
      buffer[written] = *fmt;
      written++;
      fmt++;
      continue;
    }

    fmt++;
    if (*fmt == '%') {
      buffer[written] = '%';
      written++;
      fmt++;
      continue;
    }

    // TODO how should we handle this? It's definitely a bug, and this case is
    // the scary one we don't ever want to happen
    if (format_count == count)
      logging__panic(loc, "didn't pass enough arguments for format string");

    written +=
        any__fmt(args[format_count], buffer + written, BUF_SIZE - written);
    format_count++;
  }

  if (written > BUF_SIZE || *fmt) // TODO expand buffer instead of crashing
    logging__panic(loc, "output message too long");

  // TODO how should we handle this? It's a bug, but it's kinda fine
  if (format_count != count)
    logging__panic(loc, "passed too many arguments for format string");

  for (uint32_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');
}

void logging__panic(sloc loc, char *message) {
  uint64_t written = write_prefix_to_buffer(loc);
  written += strcpy_s(buffer + written, message, BUF_SIZE - written);

  for (uint32_t i = 0; i < written; i++)
    serial__write(buffer[i]);
  serial__write('\n');

  for (;;)
    asm("hlt");
}