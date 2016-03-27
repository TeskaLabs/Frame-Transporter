#ifndef __LIBSCCMN_CONFIG_H__
#define __LIBSCCMN_CONFIG_H__

struct libsccmn_config
{
	bool initialized;
};

extern struct libsccmn_config libsccmn_config;

void libsccmn_config_init(void);

#endif // __LIBSCCMN_CONFIG_H__
