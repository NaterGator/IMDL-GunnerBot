#ifndef SERIALSTREAM_H_
#define SERIALSTREAM_H_

struct serialstream_struct;

struct ss_callback_pair {
	char *tag;
	void (*dataCallback)(char *);
};

struct ss_framing {
	char *pszFrameStart;
	char *pszFrameEnd;
	char *cDelimiter;
};

struct serialstream_struct {
	char *pszDataBuff;
	char *pszFrame;
	char cIn;
	struct ss_framing framePairs;
	int	numCallbacks;
	struct ss_callback_pair *callbacks;
};

extern int serialstreamAddCallbackPair( struct serialstream_struct *ss, struct ss_callback_pair *inPair);
extern int serialStreamProcessChar( struct serialstream_struct *ss );
extern void runSerialStreamCallbackQueue( struct serialstream_struct *ss );


#endif
