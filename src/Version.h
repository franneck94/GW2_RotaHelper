#ifndef VERSION_H
#define VERSION_H

#define MAJOR 0
#define MINOR 6
#define BUILD 0
#define REVISION 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define VERSION_STRING TOSTRING(MAJOR) "." TOSTRING(MINOR) "." TOSTRING(BUILD)

#endif // VERSION_H
