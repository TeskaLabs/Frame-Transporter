#include "all.h"

///

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int application_configure_handler(void * user, const char * section, const char * name, const char * value)
{
	bool ok;

	struct application * this = (struct application *)user;
	assert(this != NULL);

	if (false) {
	}

	else if (MATCH("http", "listen"))
	{
		ok = listen_extend_http(&this->listen, &this->context, value);
		if (!ok) return 1;
		return 0;
	}

	else if (MATCH("https", "listen"))
	{
		this->config.ssl_used = true;
		ok = listen_extend_https(&this->listen, &this->context, value);
		if (!ok) return 1;
		return 0;
	}

	FT_ERROR("Unknown configuration entry '%s'/'%s'", section, name);

	return 1;
}


bool application_configure(struct application * this)
{
	assert(this->config.initialized == false);

	static char config_realpath[PATH_MAX+1];

	if (this->config.config_file != NULL)
	{
		char *r = realpath(this->config.config_file, config_realpath);
		if (r == NULL)
		{
			FT_ERROR("Can't load '%s' (file not found)", this->config.config_file);
			return false;		
		}

		if (ini_parse(this->config.config_file, application_configure_handler, this) < 0)
		{
			FT_ERROR("Can't load '%s'", this->config.config_file);
			return false;
		}
	}

	this->config.initialized = true;
	return true;
}
