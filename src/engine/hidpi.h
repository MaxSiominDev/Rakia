#ifndef HIDPI_H
#define HIDPI_H

// needs the GLUT window and its context to exist; a no-op outside macOS
void hidpi_enable(void);
// pixels per point of the GLUT window
float hidpi_scale(void);

#endif
