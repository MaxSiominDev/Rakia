#ifndef CHECK_H
#define CHECK_H

extern int failures;

void check(int condition, const char *what);
void check_close(float actual, float expected, float tolerance, const char *what);

#endif
