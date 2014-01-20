#include "local_grid_file.h"
#include "fs_logger.h"
#include "fs_options.h"
#include "utils.h"

#include <cerrno>
#include <cstring>

#include <mongo/client/gridfs.h>
#include <mongo/client/connpool.h>

using namespace mongo;
using namespace mgridfs;
using namespace std;

LocalGridFile::LocalGridFile()
	: _size(0), _capacity(0), _readOnly(false), _dirty(false), _filename("") {
}

LocalGridFile::LocalGridFile(const string& filename)
	: _size(0), _capacity(0), _readOnly(false), _dirty(false), _filename(filename) {
}

LocalGridFile::~LocalGridFile() {
}

LocalMemoryGridFile::LocalMemoryGridFile()
	: _chunkSize(globalFSOptions._memChunkSize) {
}

LocalMemoryGridFile::LocalMemoryGridFile(const string& filename)
	: LocalGridFile(filename), _chunkSize(globalFSOptions._memChunkSize) {
}

LocalMemoryGridFile::~LocalMemoryGridFile() {
	if (_dirty) {
		//TODO: Call flush and log warning for unflushed dirty delete happing on file object
		warn() << "Flushing file data called on in-memory file delete {filname: " << _filename 
			<< ", size: " << _size << ", capacity: " << _capacity << ", dirty: " << _dirty
			<< ", readOnly: " << _readOnly << ", chunkSize: " << _chunkSize
			<< ", chunks: " << _chunks.size() << "}. "
			<< "This should have happened on file close rather than memory object deletion."
			<< endl;
		flush();
	}

	if (_chunks.size()) {
		// If chunks contain data elements, delete those memory blocks as well
		for (vector<char*>::iterator pIt = _chunks.begin(); pIt != _chunks.end(); ++pIt) {
			delete[] (*pIt);
		}
	}
}

void LocalMemoryGridFile::setDirty(bool flag) {
	if (!_readOnly) {
		_dirty = flag;
	}
}

bool LocalMemoryGridFile::setCapacity(size_t capacity) {
	trace() << " -> LocalMemoryGridFile::setCapacity {file: " << _filename << ", capacity: {old: " << _capacity
		<< ", new: " << capacity << "} }" << endl;
	//TODO: make setCapacity to see what all it affects
	return false;
}

bool LocalMemoryGridFile::setSize(size_t size) {
	trace() << " -> LocalMemoryGridFile::setSize {file: " << _filename << ", size: {old: " << _size
		<< ", new: " << size << "} }" << endl;

	//TODO: Make setSize smarter on when-all it can change size and in which direction
	if (size <= _size) {
		// The file is being truncated to smaller / equal size
		_size = size;
		_dirty = true;
		return true;
	}

	// The file is being expanded to larger size
	if (size < _capacity) {
		// Size requested is within the allocated capacity, so nothing extra
		// to do
		_size = size;
		_dirty = true;
		return true;
	}

	// Capacity will need to grow beyond current capacity
	if (size >= globalFSOptions._maxMemFileSize) {
		error() << "Memory size requested is beyond max capacity for in-memory files {filename: "
			<< _filename << ", size: " << _size << ", requested-size: " << size 
			<< "}, will not increase the capacity." << endl;
		return false;
	}

	size_t currentChunks = (_capacity + _chunkSize - 1) / _chunkSize;
	size_t newChunks = (size + _chunkSize - 1) / _chunkSize;
	if (currentChunks > newChunks) {
		error() << "Something is wrong with capapcity / chunk calculations {filename: " << _filename 
			<< ", capacity: " << _capacity << ", size: {old: " << _size << ", old: " << size 
			<< "}, chunks: {old: " << currentChunks << ", new: " << newChunks << "}, chunkSize: " 
			<< _chunkSize << "}, will not increase capacity." << endl;
		return false;
	}

	size_t diffChunks = newChunks - currentChunks;
	for (size_t i = 0; i < diffChunks; ++i) {
		char* tempData = new (nothrow) char[_chunkSize];
		if (!tempData) {
			error() << "Failed to allocate memory {filename: " << _filename << ", chunkSize: " << _chunkSize
				<< ", diffChunks: " << diffChunks << ", AllocatedChunks: " << i 
				<< "}, will not continue further." << endl;
			return false;
		}

		_chunks.push_back(tempData);
		_capacity += _chunkSize;
		_size = (_capacity < size) ? _capacity : size;
		_dirty = true;
		trace() << "Added chunk {total: " << _chunks.size() << ", capacity: " << _capacity
			<< ", size: " << _size << ", requested-size: " << size << "}" << endl;
	}
	
	return true;
}

bool LocalMemoryGridFile::setReadOnly() {
	trace() << " -> LocalMemoryGridFile::setReadonly {file: " << _filename << "}" << endl;
	if (_dirty) {
		return false;
	}

	_readOnly = true;
	return true;
}

bool LocalMemoryGridFile::setFilename(const string& filename) {
	trace() << " -> LocalMemoryGridFile::setFilename {old: " << _filename << ", new: " << filename << "}" << endl;
	//TODO: check for invalid instances
	if (!_filename.empty() || _dirty) {
		warn() << "setFilename called on local file object when filename already exists or "
			<< "is dirty {old: " << _filename << ", new: " << filename << ", isDirty: " << _dirty 
			<< "}, will not be updating filename." << endl;
		return false;
	}

	//TODO: Check flows that the handles and the LocalGridFS mappings are
	//being changed appropriately
	_filename = filename;
	return true;
}

int LocalMemoryGridFile::openRemote(int fileFlags) {
	trace() << " -> LocalMemoryGridFile::openRemote {fileFlags: " << fileFlags << "}" << endl;
	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile origGridFile = gridFS.findFile(BSON("filename" << _filename));

		if (!origGridFile.exists()) {
			dbc.done();
			error() << "Requested file not found for opening from remote {file: " << _filename << "}" << endl;
			return -EBADF;
		}

		if (origGridFile.getContentLength() > globalFSOptions._maxMemFileSize) {
			// Don't support opening files of size > MAX_MEMORY_FILE_CAPACITY in R/W mode
			error() << "Requested file length is beyond supported length for in-memory files {file: " 
				<< _filename << ", length: {requested: " << origGridFile.getContentLength()
				<< ", max-supported: " << globalFSOptions._maxMemFileSize << "} }" << endl;
			return -EROFS;
		}

		if (!initLocalBuffers(origGridFile)) {
			error() << "Failed to initialize local buffers {file: " << _filename << "}" << endl;
			return -EIO;
		}

		_dirty = false;
		dbc.done();
	} catch (DBException& e) {
		// Something failed in getting the file from GridFS
		error() << "Caught exception in getting remote file for flush {filename: " << _filename
			<< ", code: " << e.getCode() << ", what: " << e.what() << ", exception: " << e.toString() 
			<< "}" << endl;
		return -EIO;
	}

	return 0;
}

int LocalMemoryGridFile::write(const char *data, size_t len, off_t offset) {
	trace() << " -> LocalMemoryGridFile::write {len: " << len << ", offset: " << offset << "}" << endl;
	if (_readOnly) {
		debug() << "Encountered write call on a _readOnly file" << endl;
		return -EROFS;
	}

	if (len == 0) {
		return 0;
	}

	size_t updatedSize = (offset + len);
	if (_capacity > updatedSize) {
		// Nothing to be done, the size remains as it was before this 
		// Change size only if it is expanding
		_size = (updatedSize > _size) ? updatedSize : _size;
	} else if (!setSize(updatedSize)) {
		return -ENOMEM;
	}

	return _write(data, len, offset);
}

int LocalMemoryGridFile::_write(const char *data, size_t len, off_t offset) {
	// Assumes the appropriate space is available and theh chunks have been allocated
	// appropriately
	size_t whichChunk = (offset + 1) / _chunkSize;
	size_t offsetInChunk = offset % _chunkSize;
	size_t bytesWritten = 0;
	char* dest = NULL;
	while (bytesWritten < len) {
		// TODO: Check for the data / size of the file / offset consistency / chunk tracking etc.
		size_t bytesPending = len - bytesWritten;
		size_t n = ((bytesPending > (_chunkSize - offsetInChunk)) ? (_chunkSize - offsetInChunk) : bytesPending);

		trace() << "Writing to chunk {chunk: " << whichChunk << ", input-offset: " << bytesWritten 
			<< ", chunk-offset: " << offsetInChunk
			<< ", n: " << n << ", bytesPending: " << bytesPending << "}" << endl;

		dest = _chunks[whichChunk];
		memcpy(dest + offsetInChunk, (data + bytesWritten), n);

		bytesWritten += n;
		offsetInChunk = 0;
		++whichChunk;
	}

	_dirty = true;
	return len;
}

int LocalMemoryGridFile::read(char *data, size_t len, off_t offset) const {
	trace() << " -> LocalMemoryGridFile::read {len: " << len << ", offset: " << offset << "}" << endl;
	if (offset >= (off_t)_size) {
		return -EOF;
	}

	if (len == 0) {
		return 0;
	}

	// Read from the chunks of in-memory grid file to buffer
	unsigned int numChunks = (_size + 1) / _chunkSize;

	//TODO: Implement the file offset tracking for the file
	unsigned int activeChunkNum = offset / _chunkSize;
	size_t bytesRead = 0;
	while (bytesRead < len && activeChunkNum < numChunks) {
		int bytesToRead = 0;

		//TODO: Change the chunkLen to be correct length in case of last chunk
		int chunkLen = _chunkSize;
		const char* chunkData = _chunks[activeChunkNum];
		if (!chunkData) {
			warn() << "Encountered NULL chunk data while reading in-memory chunk {activeChunkNum: " << activeChunkNum 
				<< "}, will return IO error to the reader." << endl;
			return -EIO;
		}

		if (bytesRead) {
			// If we are in-between reading data, check on what data is left to be read
			bytesToRead = min((size_t)chunkLen, (len - bytesRead));
			memcpy(data + bytesRead, chunkData, bytesToRead);
		} else {
			// This is the first chunk we are reading and could be starting from in-between offset of the
			// chunk that was previously read partially
			bytesToRead = min((size_t)(chunkLen - (offset % _chunkSize)), (size_t)(len - bytesRead));
			memcpy(data + bytesRead, chunkData + (offset % _chunkSize), bytesToRead);
		}

		bytesRead += bytesToRead;
		++activeChunkNum;
	}

	return bytesRead;
}

int LocalMemoryGridFile::flush() {
	trace() << " -> LocalMemoryGridFile::flush {file: " << _filename << "}" << endl;
	if (!_dirty) {
		// Since, there are no dirty chunks, this does not need a flush
		info() << "buffers are not dirty.. need not flush {filename: " << _filename << "}" << endl;
		return 0;
	}

	size_t bufferLen = 0;
	boost::shared_array<char> buffer = createFlushBuffer(bufferLen);
	if (!buffer.get() && bufferLen > 0) {
		// Failed to create flush buffer
		return -ENOMEM;
	}

	// Get the existing gridfile from GridFS to get metadata and delete the
	// file from the system
	try {
		ScopedDbConnection dbc(globalFSOptions._connectString);
		GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);
		GridFile origGridFile = gridFS.findFile(BSON("filename" << _filename));

		if (!origGridFile.exists()) {
			dbc.done();
			warn() << "Requested file not found for flushing back data {file: " << _filename << "}" << endl;
			return -EBADF;
		}

		//TODO: Make checks for appropriate object correctness
		//i.e. do not update anything that is not a Regular File
		//Check what happens in case of a link

		gridFS.removeFile(_filename);
		trace() << "Removing the current file from GridFS {file: " << _filename << "}" << endl;
		//TODO: Check for remove status if that was successfull or not
		//TODO: Rather have an update along with active / passive flag for the
		//file

		try {
			GridFS gridFS(dbc.conn(), globalFSOptions._db, globalFSOptions._collPrefix);

			// Create an empty file to signify the file creation and open a local file for the same
			trace() << "Adding new file to GridFS {file: " << _filename << "}" << endl;
			BSONObj fileObj = gridFS.storeFile(buffer.get(), bufferLen, _filename);
			if (!fileObj.isValid()) {
				warn() << "Failed to save file object in data flush {file: " << _filename << "}" << std::endl;
				dbc.done();
				return -EBADF;
			}

			// Update the last updated date for the document
			BSONObj metadata = origGridFile.getMetadata();
			BSONElement fileObjId = fileObj.getField("_id");
			dbc->update(globalFSOptions._filesNS, BSON("_id" << fileObjId.OID()), 
					BSON("$set" << BSON(
								"uploadDate" << origGridFile.getUploadDate() 
								<< "metadata.type" << "file"
								<< "metadata.filename" << mgridfs::getPathBasename(_filename)
								<< "metadata.directory" << mgridfs::getPathDirname(_filename)
								<< "metadata.lastUpdated" << jsTime()
								<< "metadata.uid" << metadata["uid"]
								<< "metadata.gid" << metadata["gid"]
								<< "metadata.mode" << metadata["mode"]
							)
						)
					);
	} catch (DBException& e) {
			error() << "Caught exception in saving remote file in flush {code: " << e.getCode() << ", what: " << e.what()
				<< ", exception: " << e.toString() << "}" << endl;
			return -EIO;
		}

		dbc.done();
	} catch (DBException& e) {
		// Something failed in getting the file from GridFS
		error() << "Caught exception in getting remote file for flush {code: " << e.getCode() << ", what: " << e.what()
			<< ", exception: " << e.toString() << "}" << endl;
		return -EIO;
	}

	_dirty = false;
	debug() << "Completed flushing the file content to GridFS {file: " << _filename << "}" << endl;
	return 0;
}

boost::shared_array<char> LocalMemoryGridFile::createFlushBuffer(size_t& bufferLen) const {
	bufferLen = _size;
	boost::shared_array<char> tempBuffer;
	if (!bufferLen) {
		// If the buffer is 0 length, then return NULL with bufferLen = 0
		return tempBuffer;
	}

	tempBuffer.reset(new (nothrow) char[bufferLen]);
	if (!tempBuffer.get()) {
		// Failed to allocate memory, return 
		error() << "Failed to allocate temp buffer for full file {filename: " << _filename
			<< ", size: " << _size << "}, will return empty buffer without data." 
			<< endl;
		return tempBuffer;
	}

	// If the buffers are allocated, copy the data from memory chunks to
	// the temporary flush buffer
	size_t pending = _size;
	size_t offset = 0;
	for (size_t i = 0; i < _chunks.size() && pending > 0; ++i) {
		size_t bytesToCopy = (pending >= _chunkSize ? _chunkSize : pending);
		memcpy(tempBuffer.get() + offset, _chunks[i], bytesToCopy);
		pending -= bytesToCopy;
		offset += bytesToCopy;
	}

	return tempBuffer;
}

bool LocalMemoryGridFile::initLocalBuffers(GridFile& gridFile) {
	if (!setSize(gridFile.getContentLength())) {
		return false;
	}

	//TODO: Check for size compared to the content length that was specified for consistency

	off_t offset = 0;
	int chunkCount = gridFile.getNumChunks();
	for (int i = 0; i < chunkCount; ++i) {
		//TODO: Error checking or getChunk and related handling
		GridFSChunk chunk = gridFile.getChunk(i);
		int chunkLen = 0;
		const char* data = chunk.data(chunkLen);
		if (!data) {
			error() << "Failed to get data from expected chunk {file: " << _filename
				<< ", chunkOffset: " << i << ", totalChunks: " << chunkCount
				<< "}, will stop in-between and return error." << endl;
			return false;
		}
		_write(data, chunkLen, offset);
		offset += chunkLen;
	}

	return true;
}
