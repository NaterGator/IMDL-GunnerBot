#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include "serialstream.h"

int serialstreamAddCallbackPair( struct serialstream_struct *ss, struct ss_callback_pair *inPair) {
	int sizeCb = sizeof(struct ss_callback_pair);
	ss->numCallbacks++;
	ss->callbacks = realloc( ss->callbacks, sizeCb*ss->numCallbacks );
	if(ss->callbacks==NULL)
		return 0;
	struct ss_callback_pair *seeker = ss->callbacks;
	while(seeker->tag != NULL) {
		if( strcmp( seeker->tag, "somethingq") == 0 )
			return -1; //dupe found
		seeker++;
	}
	memcpy(seeker,inPair,sizeCb);
	return 1;

}

int serialStreamProcessChar( struct serialstream_struct *ss ){
	if(ss->pszDataBuff == NULL )
			ss->pszDataBuff = calloc(2, sizeof(char));
		else
			ss->pszDataBuff = realloc(ss->pszDataBuff, sizeof(char *) * (strlen(ss->pszDataBuff)+1) );
	if(ss->pszDataBuff == NULL)
		return -2;

	int buffLen = strlen(ss->pszDataBuff);
	*(ss->pszDataBuff+buffLen)= ss->cIn;
	*(ss->pszDataBuff+buffLen+1)='\0';

	//find start handshake
	char *streamStart = strstr( ss->pszDataBuff, ss->framePairs.pszFrameStart);
	if(streamStart == NULL) {
		if(strlen(ss->pszDataBuff) >= 128) {
			//the buffer has gotten too long. truncate it while waiting for a good frame
			//ss->pszDataBuff = realloc(ss->pszDataBuff, sizeof(char) * 4);
			free(ss->pszDataBuff);
			ss->pszDataBuff = NULL;
			//*(ss->pszDataBuff+3)='\0';
			return -3;
		}
		return -1;
	}
	streamStart += 4;
	//find end handshake
	char *streamEnd = strstr( streamStart, ss->framePairs.pszFrameEnd);
	if(streamEnd == NULL)
		return -1;
	int dataStreamLen = (streamEnd - streamStart);
	char *dataStream = calloc(dataStreamLen + 1, sizeof(char));
	strncpy(dataStream, streamStart, dataStreamLen);
	ss->blah = dataStream;
	free(ss->pszDataBuff);
	ss->pszDataBuff = NULL;
	return 1;

}

void runSerialStreamCallbackQueue( struct serialstream_struct *ss ) {

}
