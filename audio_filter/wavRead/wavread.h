/*
 * wavread.cpp
 *
 * Copyright 2013 Sachin Mousli <samousli@csd.auth.gr>
 * Copyright 2022 Bernd Porr
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef _WAV_READ
#define _WAV_READ

//left shift without losing bits
#define times 256

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cmath>
#include <cassert>

class WAVread {
public:

/*
 * Links explaining the RIFF-WAVE standard:
 *      http://www.topherlee.com/software/pcm-tut-wavformat.html
 *      https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 */
	struct WaveHeader {
		char chunkID[4];            // 1-4      "RIFF"
		int32_t chunkSize;          // 5-8
		char format[4];             // 9-12     "WAVE"
		char subchunkID[4];         // 13-16    "fmt\0"
		int32_t subchunkSize;       // 17-20
		uint16_t audioFormat;       // 21-22    PCM = 1
		uint16_t numChannels;       // 23-24
		int32_t sampleRate;         // 25-28
		int32_t bytesPerSecond;     // 29-32
		uint16_t blockAlign;        // 33-34
		uint16_t bitDepth;          // 35-36    24bit support only
		char dataID[4];             // 37-40    "data"
		int32_t dataSize;           // 41-44
	};
	
	struct StereoSample {
		int left;
		int right;
	};
	
	// open wav file and return buffer
	char* open(char fileName[]) {
		filePtr = fopen(fileName, "r");
		// if (filePtr == NULL) {
		// 	perror("Unable to open file");
		// 	return -1;
		// }
		int r = fread(&hdr, sizeof(hdr), 1, filePtr);
		// if (r < 0) return r;
		// if (hdr.bitDepth != 24) {
		// 	fprintf(stderr,"Only int24 is supported.\n");
		// 	return -1;
		// }

		buffer = (char*)malloc(864000 * 4); // 13 s
		//read into buffer
		int a = fread(buffer, 1, 864000 * 4, filePtr);
		return buffer;
	}

	int getFs() {
		return (int)(hdr.sampleRate);
	}
		
	void close() {
		fclose(filePtr);
	}
	
        /*
	 * Prints the wave header
	 */
	void printHeaderInfo() {
		char buf[5];
		printf("Header Info:\n");
		strncpy(buf, hdr.chunkID, 4);
		buf[4] = '\0';
		printf("    Chunk ID: %s\n",buf);
		printf("    Chunk Size: %d\n", hdr.chunkSize);
		strncpy(buf,hdr.format,4);
		buf[4] = '\0';
		printf("    Format: %s\n", buf);
		strncpy(buf,hdr.subchunkID,4);
		buf[4] = '\0';
		printf("    Sub-chunk ID: %s\n", buf);
		printf("    Sub-chunk Size: %d\n", hdr.subchunkSize);
		printf("    Audio Format: %d\n", hdr.audioFormat);
		printf("    Channel Count: %d\n", hdr.numChannels);
		printf("    Sample Rate: %d\n", hdr.sampleRate);
		printf("    Bytes per Second: %d\n", hdr.bytesPerSecond);
		printf("    Block alignment: %d\n", hdr.blockAlign);
		printf("    Bit depth: %d\n", hdr.bitDepth);
		strncpy(buf,hdr.dataID, 4);
		buf[4] = '\0';
		printf("    Data ID: %s\n", buf);
		printf("    Data Size: %d\n", hdr.dataSize);
	}
	
	bool hasSample() {
		return !feof(filePtr);
	}
	

	/*
	 * Riff is little endian, 24-bit data will be stored as int32, with the MSB of the
     * 24-bit data stored at the MSB of the int32, and typically the least
     * significant byte is 0x00.
	 */
	int getSample(bool realtime_test = true) {
		if(realtime_test){
			int i = (buffer[0]) + (buffer[1] << 8) + (buffer[2] << 16);
			int sample = (i * 256);
			sample = sample >> 8;
			buffer = buffer + 3;
			return sample;
		} else {
			int i;
			//read 3 bytes
			int r = fread(&i, 3, 1, filePtr);
			if (r < 0) return 0;
			// a trick to avoid range overflow, now it is real 24bit number
			int sample = (i * 256);
			sample = sample >> 8;
			//return floating number
			return sample;
		}
		return 0;
	}

	StereoSample getStereoSample() {
		if (2 != hdr.numChannels) throw "Not a stereo file.";
		StereoSample stereoSample;
		stereoSample.left = getSample();
		stereoSample.right = getSample();
		return stereoSample;
	}
	
private:
	int number = 0;
    FILE* filePtr = nullptr;
	WaveHeader hdr;
	unsigned char bytes[4];
	char *buffer;
};

#endif