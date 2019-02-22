/*
 * videoenc.h
 *
 *  Created on: Feb 22, 2019
 *      Author: user003
 */

#ifndef VIDEOENC_H_
#define VIDEOENC_H_

#define USE_NEON_COPY 1


void neoncopy64byte(void *dst,	void *src, unsigned int sz);



#endif /* VIDEOENC_H_ */
