#ifndef VERSION_H
#define VERSION_H

#define SCANTOOL_VERSION          1
#define SCANTOOL_SUB_VERSION      13
#define SCANTOOL_WIP_VERSION      0
#define SCANTOOL_VERSION_STR      "1.13"
#define SCANTOOL_VERSION_EX_STR   "1.13"
#define SCANTOOL_YEAR_STR         "2006"
#define SCANTOOL_DATE             20060522    /* yyyymmdd */

#ifdef ALLEGRO_WINDOWS
   #define SCANTOOL_PLATFORM_STR   "Windows"
#else
   #define SCANTOOL_PLATFORM_STR   "DOS"
#endif

#endif
