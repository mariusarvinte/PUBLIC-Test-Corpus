// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef LOGGER_H
#define LOGGER_H

int initialize_logger();
void log_info(const char *message);
void log_warning(const char *message);
void log_error(const char *message);
void finalize_logger();

#endif // LOGGER_H
