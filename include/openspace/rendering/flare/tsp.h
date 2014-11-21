/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014                                                                    *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __TSP_H__
#define __TSP_H__

#include <string>
#include <vector>
#include <list>

namespace openspace {
class TSP {
public:

	struct Header {
		unsigned int gridType_;
		unsigned int numOrigTimesteps_;
		unsigned int numTimesteps_;
		unsigned int xBrickDim_;
		unsigned int yBrickDim_;
		unsigned int zBrickDim_;
		unsigned int xNumBricks_;
		unsigned int yNumBricks_;
		unsigned int zNumBricks_;

	};

	enum NodeData {
		BRICK_INDEX = 0,
		CHILD_INDEX,
		SPATIAL_ERR,
		TEMPORAL_ERR,
		NUM_DATA
	};

	TSP(const std::string& filename);
	~TSP();

	// load performs readHeader, readCache, writeCache and construct 
	// in the correct sequence
	bool load();

	bool readHeader();
	bool readCache();
	bool writeCache();
	bool construct();

	bool calculateSpatialError();
	bool calculateTemporalError();

private:
	// Returns a list of the octree leaf nodes that a given input 
	// brick covers. If the input is already a leaf, the list will
	// only contain that one index.
	std::list<unsigned int> CoveredLeafBricks(unsigned int _brickIndex);

	// Returns a list of the BST leaf nodes that a given input brick
	// covers (at the same spatial subdivision level).
	std::list<unsigned int> CoveredBSTLeafBricks(unsigned int _brickIndex);

	// Return a list of eight children brick incices given a brick index
	std::list<unsigned int> ChildBricks(unsigned int _brickIndex);

	std::string _filename;
	std::streampos _dataOffset;

	// Holds the actual structure
	std::vector<int> data_;

	// Data from file
	Header _header;

	// Additional metadata
	unsigned int paddedBrickDim_;
	unsigned int numTotalNodes_;
	unsigned int numBSTLevels_;
	unsigned int numBSTNodes_;
	unsigned int numOTLevels_;
	unsigned int numOTNodes_;

	const unsigned int paddingWidth_ = 1;

	// Error stats
	float minSpatialError_;
	float maxSpatialError_;
	float medianSpatialError_;
	float minTemporalError_;
	float maxTemporalError_;
	float medianTemporalError_;

}; // class TSP
}  // namespace openspace

#endif