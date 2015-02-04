// This file is part of e93.
//
// e93 is free software; you can redistribute it and/or modify
// it under the terms of the e93 LICENSE AGREEMENT.
//
// e93 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// e93 LICENSE AGREEMENT for more details.
//
// You should have received a copy of the e93 LICENSE AGREEMENT
// along with e93; see the file "LICENSE.TXT".


// global definitions and structures known by the editor

#define	VERSION	"1.3"
#define	EDITION	"r4"

#define		Max(a,b) 	((a)>(b)?(a):(b))
#define		Min(a,b) 	((a)<(b)?(a):(b))

typedef	int					INT32;				// 32 bits signed
typedef	unsigned int		UINT32;				// 32 bits unsigned
typedef	short int			INT16;				// 16 bits signed
typedef	unsigned short int	UINT16;				// 16 bits unsigned
typedef	signed char			INT8;				// 8 bits signed
typedef	unsigned char		UINT8;				// 8 bits unsigned


// we get the bool type from c++
// typedef	int					bool;
// #define	false	0
// #define	true	(!false)


// defines needed to handle text

#define	CHUNKSIZE				1024	// size in bytes of data chunks used while editing text

#define	MAXPATHNAMELENGTH		4096	// size in bytes maximum path name

#define	SELECTIONCHUNKELEMENTS	256		// number of SELECTIONELEMENTs in an array chunk
#define	UNDOCHUNKELEMENTS		256		// number of undo elements in an undo chunk

