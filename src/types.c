#include <stdlib.h>

#include "types.h"

void string_free(struct String* obj) {
	
	free(obj->s);
	obj->slength = 0;
	
}