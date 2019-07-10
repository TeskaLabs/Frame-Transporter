#include "../_ft_internal.h"

///

static void ft_pool_on_heartbeat(struct ft_subscriber * subscriber, struct ft_pubsub * pubsub, const char * topic, void * data);

///

bool ft_pool_init(struct ft_pool * this)
{
	assert(this != NULL);
	this->zones = NULL;

	this->frames_total = 0;
	this->frames_used = 0;
	this->flags.last_zone = false;

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN");

	ft_subscriber_init(&this->heartbeat, ft_pool_on_heartbeat);
	this->heartbeat.data = this;

	ft_pool_set_alloc(this, ft_pool_alloc_default);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");

	return true;
}


void ft_pool_fini(struct ft_pool * this)
{
	assert(this != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN");

	ft_subscriber_fini(&this->heartbeat);

	while(this->zones != NULL)
	{
		struct ft_poolzone * zone = this->zones;
		this->zones = zone->next;

		_ft_poolzone_del(zone);
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");
}


void ft_pool_lowmem_pubsub_msg_(struct ft_pool * this, int frames_available, bool inc)
{
	static struct ft_pubsub_message_pool_lowmem ft_pool_borrow_real_msg_;
	ft_pool_borrow_real_msg_.pool = this;
	ft_pool_borrow_real_msg_.inc = inc;
	ft_pool_borrow_real_msg_.frames_available = frames_available;
	ft_pubsub_publish(NULL, FT_PUBSUB_TOPIC_POOL_LOWMEM, &ft_pool_borrow_real_msg_);
}


struct ft_frame * ft_pool_borrow_real_(struct ft_pool * this, uint64_t frame_type, const char * file, unsigned int line)
{
	if (this == NULL)
	{
		if (ft_context_default != NULL)
		{
			this = &ft_context_default->frame_pool;
		}
		if (this == NULL)
		{
			FT_WARN("Default context is not set!");
			return NULL;
		}		
	}

	struct ft_frame * frame =  NULL;
	struct ft_poolzone ** last_zone_next = &this->zones;

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN");

	// Borrow from existing zone
	for (struct ft_poolzone * zone = this->zones; zone != NULL; zone = zone->next)
	{
		frame = _ft_poolzone_borrow(zone, frame_type, file, line);
		if (frame != NULL)
		{
			FT_TRACE(FT_TRACE_ID_MEMPOOL, "END f:%p", frame);
			goto exit_with_frame;
		}

		last_zone_next = &zone->next;
	}

	if (this->flags.last_zone)
	{
		FT_WARN("Frame pool ran out of memory (1)");
		FT_TRACE(FT_TRACE_ID_MEMPOOL, "END out-out-mem last-zone");
		return NULL;
	}

	*last_zone_next = this->alloc_fnct(this);
	if (*last_zone_next == NULL)
	{
		FT_WARN("Frame pool ran out of memory (3)");
		FT_TRACE(FT_TRACE_ID_MEMPOOL, "END out-out-mem");
		return NULL;
	}

	frame = _ft_poolzone_borrow(*last_zone_next, frame_type, file, line);
	if (frame == NULL)
	{
		FT_WARN("Frame pool ran out of memory (2)");
		FT_TRACE(FT_TRACE_ID_MEMPOOL, "END out-out-mem 2");
		return NULL;
	}

exit_with_frame:
	assert(this->frames_used >= 0);
	this->frames_used += 1;

	if (this->flags.last_zone)
	{
		int frames_available = this->frames_total - this->frames_used;
		if (frames_available < FT_POOL_LOWMEM_LIMIT) ft_pool_lowmem_pubsub_msg_(this, frames_available, false);
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END f:%p (%d/%d)", frame, this->frames_used, this->frames_total);

	return frame;
}

void ft_frame_return(struct ft_frame * frame)
{
	int rc;

	assert(frame != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN f:%p ft:%x fls:%s:%u", frame, frame->type, frame->last_seen_by_file, frame->last_seen_by_line);

	assert(frame->type != FT_FRAME_TYPE_FREE);
	assert(frame->last_seen_by_file != NULL);
	assert(frame->last_seen_by_line > 0);

	struct ft_poolzone * zone = frame->zone;
	frame->type = FT_FRAME_TYPE_FREE;

	frame->next = zone->available_frames;
	zone->available_frames = frame;

	frame->last_seen_by_file = NULL;
	frame->last_seen_by_line = 0;

	zone->frames_used -= 1;
	zone->flags.free_on_hb = (frame->zone->frames_used == 0);

	// Erase a page
	if (zone->flags.erase_on_return)
		memset(frame->data, '\0', frame->capacity);

	// Unlock pages from the memory
	if (zone->flags.mlock_when_used)
	{
		rc = 0;
//		rc = munlock(frame->data, frame->capacity);
		if (rc != 0) FT_WARN_ERRNO(errno, "munlock in frame pool return");
	}

	// Advise that we will use it
	if (zone->flags.madvice_when_used)
	{
		rc = 0;
//		rc = posix_madvise(frame->data, frame->capacity, POSIX_MADV_DONTNEED);
		if (rc != 0) FT_WARN_ERRNO(errno, "posix_madvise in frame pool return");
	}

	struct ft_pool * pool = zone->pool;
	if (pool != NULL)
	{
		pool->frames_used -= 1;
		if (pool->flags.last_zone)
		{
			int frames_available = pool->frames_total - pool->frames_used;
			if (frames_available < FT_POOL_LOWMEM_LIMIT) ft_pool_lowmem_pubsub_msg_(pool, frames_available, true);
		}
	}

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "END");
}


void ft_pool_set_alloc(struct ft_pool * this, ft_pool_alloc_fnct alloc_fnct)
{
	if (alloc_fnct == NULL) this->alloc_fnct = ft_pool_alloc_default;
	else this->alloc_fnct = alloc_fnct;
}


static void ft_pool_on_heartbeat(struct ft_subscriber * subscriber, struct ft_pubsub * pubsub, const char * topic, void * data)
{
	struct ft_pool * this = subscriber->data;
	assert(this != NULL);

	struct ft_pubsub_message_heartbeat * msg = data;
	assert(msg != NULL);

	FT_TRACE(FT_TRACE_ID_MEMPOOL, "BEGIN fa:%ld zc:%ld", (long int)ft_pool_count_available_frames(this), (long int)ft_pool_count_zones(this));

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
			zone->free_at = msg->now + ft_config.fpool_zone_free_timeout;
			zone->flags.free_on_hb = false;

			last_zone_next = &zone->next;
			zone = zone->next;
			continue;
		}

		if (zone->free_at >= msg->now)
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