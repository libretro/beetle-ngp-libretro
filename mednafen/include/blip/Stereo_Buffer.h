
// Simple stereo Blip_Buffer for sound emulators whose oscillators output
// either on the left only, center, or right only.

// Blip_Buffer 0.3.0. Copyright (C) 2003-2004 Shay Green. GNU GPL license.

#ifndef STEREO_BUFFER_H
#define STEREO_BUFFER_H

#include "Blip_Buffer.h"

#define BUF_COUNT 3

class Stereo_Buffer {
public:
	Stereo_Buffer();
	~Stereo_Buffer();
	
	// Same as in Blip_Buffer (see Blip_Buffer.h)
	bool set_sample_rate( long, int msec );
	void clock_rate( long );
	void bass_freq( int );
	void clear();
	
	// Buffers to output synthesis to
	Blip_Buffer* left();
	Blip_Buffer* center();
	Blip_Buffer* right();
	
	// Same as in Blip_Buffer. For more efficient operation, pass false
	// for was_stereo if the left and right buffers had nothing added
	// to them for this frame.
	void end_frame( int32_t);
	
	// Output is stereo with channels interleved, left before right. Counts
	// are in samples, *not* pairs.
	long samples_avail() const;
	long read_samples( int16_t*, long );
	
private:
	// noncopyable
	Stereo_Buffer( const Stereo_Buffer& );
	Stereo_Buffer& operator = ( const Stereo_Buffer& );
	
	Blip_Buffer bufs [BUF_COUNT];
	bool stereo_added;
	bool was_stereo;
	
	void mix_stereo( int16_t*, long );
};

	inline Blip_Buffer* Stereo_Buffer::left() {
		return &bufs [1];
	}
	
	inline Blip_Buffer* Stereo_Buffer::center() {
		return &bufs [0];
	}
	
	inline Blip_Buffer* Stereo_Buffer::right() {
		return &bufs [2];
	}
	
	inline long Stereo_Buffer::samples_avail() const {
		return bufs [0].samples_avail();
	}

#endif

