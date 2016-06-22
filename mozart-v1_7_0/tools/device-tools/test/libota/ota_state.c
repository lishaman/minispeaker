/*
	Get state for whether at OTA state.
 */

#include <stdio.h>

#include "ota_interface.h"

int main(int argc, char **argv)
{
	int state = mozart_ota_get_state();

	return state ? 1 : 0;
}
