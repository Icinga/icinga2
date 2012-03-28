#include "i2-base.h"

using namespace icinga;

unsigned long Object::ActiveObjects;

Object::Object(void) {
	ActiveObjects++;
}

Object::~Object(void) {
	ActiveObjects--;
}
