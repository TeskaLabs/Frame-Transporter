#include "../_ft_internal.h"

///

bool ft_pool_init(struct ft_pool * this)
{
	assert(this != NULL);
	this->zones = NULL;

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN");

	this->alloc_fnct = ft_pool_alloc_default;

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");

	return true;
}


void ft_pool_fini(struct ft_pool * this)
{
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN");

	while(this->zones != NULL)
	{
		struct ft_poolzone * zone = this->zones;
		this->zones = zone->next;

		_ft_poolzone_del(zone);
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");

	//TODO: Unregister heartbeat
}


struct ft_frame * ft_pool_borrow_real_(struct ft_pool * this, uint64_t frame_type, const char * file, unsigned int line)
{
	assert(this != NULL);
	struct ft_frame * frame =  NULL;
	struct ft_poolzone ** last_zone_next = &this->zones;

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN ft:%08llx %s:%u", (unsigned long long) frame_type, file, line);

	// Borrow from existing zone
	for (struct ft_poolzone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		frame = _ft_poolzone_borrow(zone, frame_type, file, line);
		if (frame != NULL)
		{
			FT_TRACE(FT_TRACE_ID_MEMPOOL, "END f:%p", frame);
			return frame;
		}

		last_zone_next = &zone->next;
	}

	*last_zone_next = this->alloc_fnct(this);
	if (*last_zone_next == NULL)
	{
		FT_WARN("Frame pool ran out of memory");
		FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN out-out-mem");
		return NULL;
	}

	frame = _ft_poolzone_borrow(*last_zone_next, frame_type, file, line);
	if (frame == NULL)
	{
		FT_WARN("Frame pool ran out of memory (2)");
		FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN out-out-mem 2");
		return NULL;
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END f:%p", frame);
	return frame;
}


void ft_pool_set_alloc(struct ft_pool * this, ft_pool_alloc_fnct alloc_fnct)
{
	if (alloc_fnct == NULL) this->alloc_fnct = ft_pool_alloc_default;
	else this->alloc_fnct = alloc_fnct;
}


void ft_pool_heartbeat(struct ft_pool * this, ev_tstamp now)
{
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN fa:%zd zc:%zd", ft_pool_count_available_frames(this), ft_pool_count_zones(this));

	// Iterate via zones and find free-able ones with no used frames ...
	struct ft_poolzone ** last_zone_next = &this->zones;
	struct ft_poolzone * zone = this->zones;
	while (zone != NULL)
	{
		if ((!zone->flags.freeable) || (zone->frames_used > 0))
		{
			last_zone_next = &zone->next;
			zone = zone->next;
			continue;
		}

		if (zone->flags.free_on_hb)
		{
			zone->free_at = now + ft_config.fpool_zone_free_timeout;
			zone->flags.free_on_hb = false;

			last_zone_next = &zone->next;
			zone = zone->next;
			continue;
		}

		if (zone->free_at >= now)
		{
			last_zone_next = &zone->next;
			zone = zone->next;
			continue;			
		}

		// Unchain the zone
		struct ft_poolzone * zone_free = zone;
		zone = zone->next;
		*last_zone_next = zone;

		// Delete zone
		_ft_poolzone_del(zone_free);
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");
}


size_t ft_pool_count_available_frames(struct ft_pool * this)
{
	assert(this != NULL);
	size_t count = 0;

	for (struct ft_poolzone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		count += zone->frames_total - zone->frames_used;
	}

	return count;
}

size_t ft_pool_count_zones(struct ft_pool * this)
{
	assert(this != NULL);
	size_t count = 0;

	for (struct ft_poolzone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		count += 1;
	}

	return count;
}