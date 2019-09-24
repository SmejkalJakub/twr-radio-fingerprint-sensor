#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <bcl.h>
#include "bc_fingerprint.h"

void get_enroll_state(uint64_t *id, const char *topic, void *value, void *param);

#endif // _APPLICATION_H
