#ifndef VERSION_H
#define VERSION_H

#define MAJOR 0
#define MINOR 3
#define BUILD 0
#define REVISION 0

// Macro to convert a number to string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Macro to build version string
#define VERSION_STRING TOSTRING(MAJOR) "." TOSTRING(MINOR) "." TOSTRING(BUILD)

#endif // VERSION_H
