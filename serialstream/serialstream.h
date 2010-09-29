#ifndef SERIALSTREAM_H_
#define SERIALSTREAM_H_

struct ss_callback_pair {
	char *tag;
	void (*dataCallback)(struct serialstream_struct *);
};

struct ss_framing {
	char *pszFrameStart;
	char *pszFrameEnd;
};

struct serialstream_struct {
	char *pszDataBuff;
	char cIn;
	struct ss_framing framePairs;
	int	numCallbacks;
	struct ss_callback_pair *callbacks;
};

#endif
