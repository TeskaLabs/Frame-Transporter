#ifndef FT_PUBSUB_H_
#define FT_PUBSUB_H_

// Implementation of Publish–subscribe pattern

// Well-known topics for ft_context.pubsub

extern const char * FT_PUBSUB_TOPIC_EXIT;
extern const char * FT_PUBSUB_TOPIC_HEARTBEAT;

//

struct ft_pubsub;
struct ft_subscriber;

typedef void (*ft_subscriber_cb)(struct ft_subscriber *, struct ft_pubsub * pubsub, const char * topic, void * data);

struct ft_subscriber
{
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

bool ft_subscriber_subscribe(struct ft_subscriber * , struct ft_pubsub * pubsub, const char * topic);
bool ft_subscriber_unsubscribe(struct ft_subscriber * , struct ft_pubsub * pubsub);

///

struct ft_pubsub
{
	struct ft_subscriber * subscribers;
};

bool ft_pubsub_init(struct ft_pubsub * );
void ft_pubsub_fini(struct ft_pubsub * );

bool ft_pubsub_publish(struct ft_pubsub * , const char * topic, void * data);

#endif // FT_PUBSUB_H_
