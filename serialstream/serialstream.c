#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include "serialstream.h"

int serialstreamAddCallbackPair( struct serialstream_struct *ss, const char *tag, void (*datacallback)(char *)) {
	int sizeCb = sizeof(struct ss_callback_pair);
	struct ss_callback_pair *pCb; //pointer for callback pair allocation

	pCb = calloc(ss->numCallbacks+1, sizeCb); //allocate memory for an additional pair
	if(pCb == NULL )
		return 0;

	// copy in old callback pairs
	if(ss->callbacks != NULL) memcpy(pCb, ss->callbacks, sizeCb*ss->numCallbacks);

	// free up the unused memory from the old pairs
	free(ss->callbacks);

	// point the structure member to the new pair array
	ss->callbacks = pCb;
	if(ss->callbacks==NULL)
		return 0;

	// iterate through the pairs looking for duplicates
	for(int i = 0; i < ss->numCallbacks; i++)
		if( strcmp( ss->callbacks[i].tag, tag ) == 0 )
			return -1; //dupe found

	// add the newest callback
	ss->callbacks[ss->numCallbacks].tag = tag;
	ss->callbacks[ss->numCallbacks++].dataCallback = datacallback;

	return 1;

}

int serialStreamProcessChar( struct serialstream_struct *ss ){
	int buffLen = strlen(ss->szDataBuff);

	// truncate the buffer if we're not matching anything in the appropriate size
	if( buffLen >= SERIALBUFFLEN - 1 ) {
		ss->szDataBuff[0] = '\0';
		buffLen = 0;
	}
	// append the incoming character
	ss->szDataBuff[buffLen++] = ss->cIn;
	ss->szDataBuff[buffLen] = '\0';

	// find start handshake
	char *streamStart = strstr(ss->szDataBuff, ss->framePairs.pszFrameStart);
	if(streamStart == NULL)
		return -1;
	streamStart += strlen(ss->framePairs.pszFrameStart);

	// find end handshake
	char *streamEnd = strstr( streamStart, ss->framePairs.pszFrameEnd);
	if(streamEnd == NULL)
		return -1;

	// generate a new char array (allocate memory, copy) to add to the frame queue
	int dataStreamLen = (streamEnd - streamStart);
	char *dataStream = calloc(dataStreamLen + 1, sizeof(char));
	strncpy(dataStream, streamStart, dataStreamLen);

	// sanity check the frame queue, allocate more memory as necessary
	if( ss->pszFrames == NULL || ss->numFrames == 0 )
		ss->pszFrames = calloc(1, sizeof(char **));
	else
		ss->pszFrames = realloc(ss->pszFrames, (ss->numFrames+1)*sizeof(char **));

	// drop the new frame onto the queue for the next callback processing runthrough
	ss->pszFrames[ss->numFrames++] = dataStream;
	ss->szDataBuff[0] = '\0';
	buffLen = 0;

	return 1;

}

void runSerialStreamCallbackQueue( struct serialstream_struct *ss ) {
	// sanity check to see if we have any frames waiting in the queue
	if(ss->pszFrames == NULL || ss->numFrames == 0) return;

	// ok, apparently we do. start iterating through them and looking for associated callbacks
	for(int f=0; f < ss->numFrames; f++) {
		if(ss->pszFrames[f] == NULL) continue;

		// parse the callback identifier token
		char *pTok = strtok(ss->pszFrames[f],ss->framePairs.cDelimiter);
		char *pszParam;

		// while we're actually seeing tokens
		while(pTok != NULL) {
			// iterate through the callbacks stack and look for a matching indentifier
			for(int i=0; i < ss->numCallbacks; i++) {
				if( strcmp(pTok, ss->callbacks[i].tag) == 0 ) {
					// sweet, an identifier was found.
					// if this token pair hasn't already invoked something, we need to parse the parameter to pass
					if( pszParam == NULL) pszParam = strtok( NULL, ss->framePairs.cDelimiter);

					// invoke the callback function using function pointers.
					(*(ss->callbacks[i].dataCallback))(pszParam);

					break;
				}
			}
			// our job with that token pair is done. let's move on to the next
			pTok = strtok( NULL, ss->framePairs.cDelimiter);
			pszParam = NULL;
		}
		// we're done with this whole frame. Don't leak memory, give it back to the stack!
		free(ss->pszFrames[f]);
		ss->pszFrames[f] = NULL;
	}

	// now would be a bad time for serial data to interrupt us and add a new frame.
	// if it did, we'd lose that frame forever. disable interrupts so that can't happen
	cli();
	// free all the frame pointer pointers (oh boy!) and set the count to 0
	free(ss->pszFrames);
	ss->numFrames = 0;
	ss->pszFrames=NULL;
	// ok, now we can get interrupted again
	sei();
}
