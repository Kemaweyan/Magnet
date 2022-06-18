#ifndef SHA1_H
#define SHA1_H

typedef struct SHA1Context {
	unsigned Message_Digest[5];

	unsigned Length_Low;
	unsigned Length_High;

	unsigned char Message_Block[64];
	int Message_Block_Index;

	int Computed;
	int Corrupted;
} SHA1Context;

void SHA1Reset(SHA1Context *context);
int SHA1Result(SHA1Context *context);
void SHA1Input(SHA1Context *context, const unsigned char *message_array, unsigned length);

#endif
