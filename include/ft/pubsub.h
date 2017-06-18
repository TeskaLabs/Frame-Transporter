#ifndef FT_PUBSUB_H_
#define FT_PUBSUB_H_

// Implementation of Publish–subscribe pattern

// Well-known topics for ft_context.pubsub

extern const char * FT_PUBSUB_TOPIC_EXIT;

enum ft_exit_phase
{
	FT_EXIT_PHASE_POLITE
	//TODO: More phases
};

struct ft_pubsub_message_exit
{
	enum ft_exit_phase exit_phase;
};


extern const char * FT_PUBSUB_TOPIC_HEARTBEAT;

struct ft_pubsub_message_heartbeat
{
	ev_tstamp now;
};

extern const char * FT_PUBSUB_TOPIC_POOL_LOWMEM;

struct ft_pubsub_message_pool_lowmem
{
	struct ft_pool * pool;
	bool inc;
	int frames_available;
};


//

struct ft_pubsub;
struct ft_subscriber;

typedef void (*ft_subscriber_cb)(struct ft_subscriber *, struct ft_pubsub * pubsub, const char * topic, void * data);

struct ft_subscriber
{
	struct ft_pubsub * pubsub;
	struct ft_subscriber * next; // Allows S-link chaining

	const char * topic;
	ft_subscriber_cb callback;

	struct
	{
		unsigned int called;
	} stats;

	void * data;
};

bool ft_subscriber_init(struct ft_subscriber * , ft_subscriber_cb callback);
void ft_subscriber_fini(struct ft_subscriber *);

// pubsub can be NULL to submit into a pubsub of the default context
bool ft_subscriber_subscribe(struct ft_subscriber * , struct ft_pubsub * pubsub, const char * topic);
bool ft_subscriber_unsubscribe(struct ft_subscriber * );

///

struct ft_pubsub
{
	struct ft_subscriber * subscribers;
};

bool ft_pubsub_init(struct ft_pubsub * );
void ft_pubsub_fini(struct ft_pubsub * );

// pubsub can be NULL to submit into a pubsub of the default context
bool ft_pubsub_publish(struct ft_pubsub * , const char * topic, void * data);

#endif // FT_PUBSUB_H_
