#include "util.h"

void strntr(char* str, size_t len, char a, char b) {
	while(len-- > 0) {
		if(*str == a) {
			*str = b;
		}
		str++;
	}
}
