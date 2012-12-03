// derived from AVR App Note 135
#ifndef _AVR135_ICP_H_
#define _AVR135_ICP_H_	1

#include <stdint.h>

#define	ICP_ANALOG		0					/* assume analog data		*/

#define	ICP_BUFSIZE		16					/* queue only 1 element		*/
#if		ICP_ANALOG
#define	ICP_RX_QSIZE	ICP_BUFSIZE			/* same as BUFSIZE			*/
#else	/* ICP_DIGITAL */
#define	ICP_RX_QSIZE	(ICP_BUFSIZE+1)		/* 1 extra for queue management */
#endif

typedef uint16_t icp_sample_t;

void	icp_init(void);
icp_sample_t icp_rx(void);

#endif	/* AVR135_ICP_H_ */
