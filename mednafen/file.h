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
	MDFNFILE(const char *path, const void *known_ext, const char *purpose = NULL);

	~MDFNFILE();

	bool Open(const char *path, const void *known_ext, const char *purpose = NULL, const bool suppress_notfound_pe = 0);

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

class PtrLengthPair
{
 public:

 inline PtrLengthPair(const void *new_data, const uint64_t new_length)
 {
  data = new_data;
  length = new_length;
 }

 ~PtrLengthPair() 
 { 

 } 

 inline const void *GetData(void) const
 {
  return(data);
 }

 inline uint64_t GetLength(void) const
 {
  return(length);
 }

 private:
 const void *data;
 uint64_t length;
};

#include <vector>

// These functions should be used for data like save states and non-volatile backup memory.
// Until(if, even) we add LoadFromFile functions, for reading the files these functions generate, just use gzopen(), gzread(), etc.
// "compress" is set to the zlib compression level.  0 disables compression entirely, and dumps the file without a gzip header or footer.
// (Note: There is a setting that will force compress to 0 in the internal DumpToFile logic, for hackers who don't want to ungzip save files.)

bool MDFN_DumpToFile(const char *filename, int compress, const void *data, const uint64_t length);
bool MDFN_DumpToFile(const char *filename, int compress, const std::vector<PtrLengthPair> &pearpairs);

#endif
