#ifndef MDFN_FILE_H
#define MDFN_FILE_H

#include <stdint.h>
#include <string>

#define MDFNFILE_EC_NOTFOUND	1
#define MDFNFILE_EC_OTHER	2

class MDFNFILE
{
	public:

	MDFNFILE();
	// WIP constructors:
	MDFNFILE(const char *path);
	~MDFNFILE();

   bool Open(const char *path);

	bool Close(void);

	uint64_t fread(void *ptr, size_t size, size_t nmemb);
	int fseek(int64_t offset, int whence);

	int read32le(uint32_t *Bufo);
	int read16le(uint16_t *Bufo);

	char *fgets(char *s, int size);
   uint8_t *f_data;
   int64_t f_size;
   char *f_ext;

	private:

   int64_t location;

	bool MakeMemWrapAndClose(void *tz);
};

#endif
