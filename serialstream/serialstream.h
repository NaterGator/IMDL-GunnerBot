#ifndef SERIALSTREAM_H_
#define SERIALSTREAM_H_


// Set a default buffer length of 1024 bytes if the user doesn't specify.
#ifndef SERIALBUFFLEN
#define SERIALBUFFLEN 1024
#endif

/*
 * This is a callback pair struct.
 * The serialstreamAddCallbackPair() function generates them
 * and appends them to the config struct stack.
 */
struct ss_callback_pair {
	const char *tag;
	void (*dataCallback)(char *);
};

/*
 * This is the framing struct. The user is responsible for specifying and providing this.
 * pszFrameStart is what we match for a start frame. I use "$FB$" for frame beginning. Use what you like.
 * pszFrameEnd is what we match for an end frame. I use "$FE$", use what you like.
 * cDelimiter is the character that divides the token pairs into callbackTag and stringData.
 * I use "|", you use whatever you like. I suggest you make it something rare, because there is no escape sequence.
 */
struct ss_framing {
	const char *pszFrameStart;
	const char *pszFrameEnd;
	const char *cDelimiter;
};


/*
 * This is the configuration struct. It's used to track everything about the serial stream you're invoking methods on.
 * I only use this because I'm trying to adhere to quasi-objective-c. It makes it easier if you have multiple
 *  serial streams coming in to the same chip and you want to invoke common methods. If you only have one, it's a bit of a faff.
 *
 * szDataBuff is internally used to buffer incoming characters from the stream. It will be parsed automatically into frames of token pairs.
 * pszFrames is a pointer to frame pointers. It's automatically populated as the buffer fills up and is cleared.
 * cIn is the incoming character. You are responsible for setting this in the ISR (or wherever) and then invoking serialStreamProcessChar()
 * framePairs is what you set your ss_framing struct on.
 * numCallbacks is tracked internally, it represents the number of callbacks added.
 * numFrames is tracked internally, it represents the number of frames in the queue.
 * callbacks is a pointer to ss_callback_pair structs which wrap the callback data added with serialstreamAddCallbackPair()
 *
 */
struct serialstream_struct {
	char szDataBuff[SERIALBUFFLEN];
	char **pszFrames;
	char cIn;
	struct ss_framing framePairs;
	int	numCallbacks;
	int numFrames;
	struct ss_callback_pair *callbacks;
};

int serialstreamAddCallbackPair( struct serialstream_struct *ss, const char * tag, void (*datacallback)(char *));
extern int serialStreamProcessChar( struct serialstream_struct *ss );
extern void runSerialStreamCallbackQueue( struct serialstream_struct *ss );
extern void sendData(struct serialstream_struct *ss, char *pszData);


#define SRCALLBACK(name) void name(char *pszFrame)


#endif
