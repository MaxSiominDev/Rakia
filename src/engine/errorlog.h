#ifndef ERRORLOG_H
#define ERRORLOG_H

// on Windows, redirects stderr to a log file in the user's temp folder, since a GUI-subsystem exe has no
// visible console; a no-op elsewhere, where stderr already prints normally
void errorlog_init(void);
// on Windows, if initialization failed, shows a message box with the tail of the log and its path; a
// no-op elsewhere
void errorlog_report_failure(void);

#endif
