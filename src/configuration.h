#ifndef __WIT_CONFIGURATION_H__
#define __WIT_CONFIGURATION_H__

#include <stdint.h>

/* this structure is passed to wit_display_create(). According bits in this
 * structure are object created or not
 *
 * example:
 *
 * struct wit_configuration conf = {0};
 * conf.globals |= CONF_SEAT;
 * conf.resources |= CONF_SEAT | CONF_POINTER | CONF_TOUCH
 *
 * wit_display_create(&conf);
 *
 * This code will result in creating global seat and
 * consequently, when bind is called, resources for seat, pointer and touch.
 * But not even when bind for keyboard will be called, resource will *not* be created.
 *
 * When NULL to wit_display_create() is passed, then default configuration is
 * used.
 * Default configuration is:
 * 	globals = CONF_SEAT
 *      resources = CONF_ALL
 * 	options = 0
 */
struct wit_config {
	uint32_t globals;	/* bitmap of globals */
	uint32_t resources;	/* bitmap of resources */
	uint32_t options;	/* versatile bitmap */
};

enum {
	CONF_SEAT 	= 1,
	CONF_POINTER 	= 1 << 1,
	CONF_KEYBOARD 	= 1 << 2,
	CONF_TOUCH 	= 1 << 3,
	/* FREE */

	CONF_ALL 	= ~((uint32_t) 0)
};

const struct wit_config wit_default_config = {
	CONF_SEAT,
	CONF_ALL,
	0
};

#endif /* __WIT_CONFIGURATION_H__ */
