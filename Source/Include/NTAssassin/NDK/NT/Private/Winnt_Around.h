/* Include this file before and after winnt.h */

#ifndef _WINNT_

#define WIN32_NO_STATUS

#define PSID MS_PSID

#else

#undef WIN32_NO_STATUS

#undef PSID
typedef SID* PSID;

#endif
