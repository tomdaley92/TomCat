/*

Thomas Daley
10/16/2016
AsyncStreamReader.cc

*/

#include "AsyncStreamReader.h"
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define DEBUG 0

DWORD WINAPI text_reader_thread(LPVOID data) {
	
	PStreamReaderData p_data = (PStreamReaderData) data;

	while(!feof(p_data->stream)) {

		char *sendbuf = (char *)malloc(sizeof(char) * DEFAULT_BUFLEN);
		memset(sendbuf, '\0', DEFAULT_BUFLEN);
		 
		if (fgets(sendbuf, DEFAULT_BUFLEN, p_data->stream) != NULL) {
			
			if (WaitForSingleObject(p_data->key, INFINITE) == WAIT_OBJECT_0) {
				p_data->q.push(sendbuf);
				p_data->s.push(strlen(sendbuf));
			}
			ReleaseMutex(p_data->key);
	    } 
	} 
	if (DEBUG) fprintf(stderr, "EOF\n");   

	if (WaitForSingleObject(p_data->key, INFINITE) == WAIT_OBJECT_0) {
		p_data->flag = 0;
	}
	ReleaseMutex(p_data->key); 
	if (DEBUG) fprintf(stderr, "Exited text_reader_thread gracefully.\n");
	ExitThread(0);
}

DWORD WINAPI binary_reader_thread(LPVOID data) {	
	
	PStreamReaderData p_data = (PStreamReaderData) data;

	while(!feof(p_data->stream)) {
		
		char *sendbuf = (char *)malloc(sizeof(char) * DEFAULT_BUFLEN);
		memset(sendbuf, '\0', DEFAULT_BUFLEN);
		int bytes_read = 0;		

		bytes_read = fread(sendbuf, 1, DEFAULT_BUFLEN, p_data->stream);
		if (bytes_read > 0) {
			
			if (WaitForSingleObject(p_data->key, INFINITE) == WAIT_OBJECT_0) {
				p_data->q.push(sendbuf);
				p_data->s.push(bytes_read);
			}
			ReleaseMutex(p_data->key);
	    } 
	}
	if (DEBUG) fprintf(stderr, "EOF\n"); 

	if (WaitForSingleObject(p_data->key, INFINITE) == WAIT_OBJECT_0) {
		p_data->flag = 0;
	}
	ReleaseMutex(p_data->key);      
	if (DEBUG) fprintf(stderr, "Exited binary_reader_thread gracefully.\n");

	ExitThread(0);
}

AsyncStreamReader::AsyncStreamReader() {
	/* Empty Constructor */
}

AsyncStreamReader::AsyncStreamReader(FILE *stream, int type) {
	
	data.stream = stream;
	if (data.stream == NULL) {
		fprintf(stderr, "AsyncStreamReader: stream is NULL.\n");
		exit(1);
	}

	/* There exists data until EOF is reached */
	data.flag = 1;

	data.key = CreateMutex(
		NULL,
		FALSE,
		NULL);

	if (data.key == NULL) {
		fprintf(stderr, "AsyncStreamReader: CreateMutex error.\n");
		exit(1);
	}

	if (type == TEXT_READER) {
		if (DEBUG) fprintf(stderr, "Creating text reader thread\n");
		T_HANDLE = CreateThread(
			NULL,
			0,
			text_reader_thread,
			&data,
			0,
			&T_ID);
	} else if (type == BINARY_READER) { 
		_setmode(fileno(stream), _O_BINARY);
		if (DEBUG) fprintf(stderr, "Creating binary reader thread\n");
		T_HANDLE = CreateThread(
			NULL,
			0,
			binary_reader_thread,
			&data,
			0,
			&T_ID);
	} else {
		fprintf(stderr, "AsyncStreamReader: Type is not recognized.\n");
		exit(1);
	}

   	if (T_HANDLE == NULL) {
		fprintf(stderr, "AsyncStreamReader: CreateThread error.\n");
		exit(1);
	}
}

AsyncStreamReader::~AsyncStreamReader() {
	
	if (DEBUG) fprintf(stderr, "AsyncStreamReader destuctor called.\n");
	
	if (T_HANDLE != NULL) {
		if (!CancelIo(T_HANDLE)){
	        if (DEBUG) fprintf(stderr, "Unable to cancel IO on AsyncStreamReader thread.\n");
	    }

		if (!TerminateThread(T_HANDLE, 0)){
	        if (DEBUG) fprintf(stderr, "Failed to terminate AsyncStreamReader thread.\n");
	    }
	    CloseHandle(T_HANDLE);
	}
}

int AsyncStreamReader::Read(char *dest) {
	/* NON-BLOCKING Read(): Returns the number of bytes read or -1 if eof is reached */
	int bytes_read = 0;
	int done_reading = -1;

	if (WaitForSingleObject(data.key, INFINITE) == WAIT_OBJECT_0) {

		if (data.q.size()) {

			bytes_read = data.s.front();
			memcpy(dest, data.q.front(), bytes_read);

			/* Cleanup */
			free(data.q.front());
			data.s.pop();
			data.q.pop();

		} else if (data.flag == 0 && data.q.size() == 0) {
			
			/* Signals EOF, done reading */
			return done_reading;
		}
		
	}
	ReleaseMutex(data.key);
	return bytes_read;
}