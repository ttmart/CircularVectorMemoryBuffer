// Pointers for each vector are organized this way:
	// 1) Begin vector-buffer location
	// 2) Location of *first byte* consumed by vector
	// 3) Location of first available byte after vector
	// 4) End vector-buffer location
	// 5) Bytes per datapoint

template<typename Pointer_type>
void CircularVectorMemoryBuffer<Pointer_type>::CircularVectorMemoryBuffer(bool(*writef)(Pointer_type, uint8_t), uint8_t(*readf)(uint8_t), Pointer_type first_available_byte)
{
	struct _CVMB { // Struct wrapper so we can create these functions

		static bool pmemwrite(Pointer_type write_reg_address, uint8_t val_to_write) {
			// Write func writef outputs bool and takes in a Pointer_type and unsigned 8 bit integer type input
			bool ret = (*writef)(write_reg_address, val_to_write);
			return ret
		}

		static uint8_t pmemread(Pointer_type read_reg_address) {
			// Read func readf outputs uint8_t and takes in a single Pointer_type
			uint8_t ret = (*readf)(read_reg_address);
			return ret
		}
	}

	_first_available_byte = first_available_byte; // Make first available byte available to other funcs
}

// This func reads the specified number of bytes and returns them in the datatype requested.
template<typename Pointer_type>
template<typename Datatype>
Datatype CircularVectorMemoryBuffer<Pointer_type>::_readDataPoint(Pointer_type address, Datatype datatype) { // datatype is the datatype that is expected from the output

	union converter { // Define union structure
		Datatype out_data; // Datatype is defined by input
		uint8_t in_data[sizeof(Datatype)]; // Takes in same number of bytes as size of Datatype
	}

	for (int i = 0; i < sizeof(datatype); i++) { // Loop through number of bytes in datatype (same as number of bytes per datpt in vector)
		converter.in_data[i] = _CVMB.pmemread(address + i); // Take in a byte at a time to converter from memory vector
	}
	return converter.out_data; // Convert individual bytes into Datatype via converter and return
}

// This func sets up the pointer block for the vectors, effectively initializing the vectors. It handles vector 1.
template<typename Pointer_type>
template<typename Num_vector_datapoints, typename Vector_datatype, typename... Remaining_vector_details>
bool CircularVectorMemoryBuffer<Pointer_type>::pointerCreate(Num_vector_datapoints num_vector_datapoints, Vector_datatype vector_datatype, Remaining_vector_details... remaining_vector_details)
{

	// Pointers for each vector are organized this way:
	// 1) Begin vector-buffer location
	// 2) Location of *first byte* consumed by vector
	// 3) Location of first available byte after vector
	// 4) End vector-buffer location
	// 5) Bytes per datapoint

	// Initialize output (true = success)
	bool op_success = true;

	// Find end of pointer block
	uint8_t num_pointers_per_vector = 5; // Begin, First, Last, End, Datapt Size
	uint8_t _num_vectors = sizeof...(remaining_vector_details) + 1; // uint8_t limits number of vectors to 255
	uint16_t pointer_block_size = sizeof(Pointer_type) * num_pointers_per_vector * num_vectors; // Find size of the pointer block 
	uint16_t end_pointer_block = pointer_block_size + _first_available_byte;

	// Find size of vector
	Pointer_type vector_size = num_vector_datapoints * sizeof(vector_datatype);

	// 1) Begin vector-buffer location
	op_success &= _writeDataPoint(_first_available_byte + 1, end_pointer_block + 1);

	// 2) Location of *first byte* consumed by vector
	op_success &= _writeDataPoint(_first_available_byte + 1 + sizeof(Pointer_type), end_pointer_block + 1);

	// 3) Location of first available byte after vector
	op_success &= _writeDataPoint(_first_available_byte + 1 + 2 * sizeof(Pointer_type), end_pointer_block + 1);

	// 4) End vector-buffer location
	op_success &= _writeDataPoint(_first_available_byte + 1 + 3 * sizeof(Pointer_type), vector_size + end_pointer_block);

	// 5) Bytes per datapoint
	op_success &= _writeDataPoint(_first_available_byte + 1 + 4 * sizeof(Pointer_type), sizeof(vector_datatype));

	// Find last byte consumed by this pointer
	Pointer_type last_ptr_consumed_byte = _first_available_byte + 4 * sizeof(Pointer_type) + sizeof(vector_datatype));

	// Last byte consumed by vector buffer
	Pointer_type last_vctr_consumed_byte = vector_size + end_pointer_block;

	op_success &= _pointerCreate(Pointer_type last_pointer_byte, remaining_vector_details...); // Return true if write operations were a success

	return op_success;
}

// This is the recursive pointer creator (handles vectors 2 through N-1)
template<typename Pointer_type>
template<typename Num_vector_datapoints, typename Vector_datatype, typename... Remaining_vector_details>
bool CircularVectorMemoryBuffer<Pointer_type>::_pointerCreate(Pointer_type last_pointer_byte_addr, Pointer_type last_vector_byte_addr, Num_vector_datapoints num_vector_datapoints, 
															  Vector_datatype vector_datatype, Remaining_vector_details... remaining_vector_details)
{
	// Initialize output (true = success)
	bool op_success = true;
	
	// Find size of vector
	Pointer_type vector_size = num_vector_datapoints * sizeof(vector_datatype);

	// 1) Begin vector-buffer location
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1, last_vector_byte_addr + 1);

	// 2) Location of *first byte* consumed by vector
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + sizeof(Pointer_type), last_vector_byte_addr + 1);

	// 3) Location of first available byte after vector
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 2 * sizeof(Pointer_type), last_vector_byte_addr + 1);

	// 4) End vector-buffer location
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 3 * sizeof(Pointer_type), vector_size + last_vector_byte_addr);

	// 5) Bytes per datapoint
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 4 * sizeof(Pointer_type), sizeof(vector_datatype));

	// Find last byte consumed by this pointer
	Pointer_type last_ptr_consumed_byte = last_pointer_byte_addr + 4 * sizeof(Pointer_type) + sizeof(vector_datatype));

	// Last byte consumed by vector buffer
	Pointer_type last_vctr_consumed_byte = vector_size + end_pointer_block;

	op_success &= _pointerCreate(Pointer_type last_pointer_byte, remaining_vector_details...); // Return true if write operations were a success

	return op_success;
}

// This is the base case pointer creator (handles vector N)
template<typename Pointer_type>
template<typename Num_vector_datapoints, typename Vector_datatype>
bool CircularVectorMemoryBuffer<Pointer_type>::_pointerCreate(Pointer_type last_pointer_byte_addr, Pointer_type last_vector_byte_addr,
	Num_vector_datapoints num_vector_datapoints, Vector_datatype vector_datatype)
{
	// Initialize output (true = success)
	bool op_success = true;

	// Find size of vector
	Pointer_type vector_size = num_vector_datapoints * sizeof(vector_datatype);

	// 1) Begin vector-buffer location
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1, last_vector_byte_addr + 1);

	// 2) Location of *first byte* consumed by vector
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + sizeof(Pointer_type), last_vector_byte_addr + 1);

	// 3) Location of first available byte after vector
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 2 * sizeof(Pointer_type), last_vector_byte_addr + 1);

	// 4) End vector-buffer location
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 3 * sizeof(Pointer_type), vector_size + last_vector_byte_addr);

	// 5) Bytes per datapoint
	op_success &= _writeDataPoint(last_pointer_byte_addr + 1 + 4 * sizeof(Pointer_type), sizeof(vector_datatype));

	// Find last byte consumed by this pointer
	Pointer_type last_ptr_consumed_byte = last_pointer_byte_addr + 4 * sizeof(Pointer_type) + sizeof(vector_datatype));

	// Last byte consumed by vector buffer
	Pointer_type last_vctr_consumed_byte = vector_size + end_pointer_block;

	return op_success;
}

// This func breaks the input data into individual bytes which are then written to the specified location. It returns true if the operation was successful.
template<typename Pointer_type>
template<typename Datatype>
bool CircularVectorMemoryBuffer<Pointer_type>::_writeDataPoint(Pointer_type address, Datatype datapoint_to_write) {

	union converter { // Define union structure
		Datatype in_data; // Datatype is defined by input
		uint8_t out_data[sizeof(Datatype)]; // Send out same number of bytes as size of Datatype
	}

	bool op_success = true;

	for (int i = 0; i < sizeof(datatype); i++) {
		op_success &= _CVMB.pmemwrite(address, converter.out_data[i]);
	}
	return op_success;
}

// This func appends data to end of circular buffer.
template<typename Pointer_type>
template<typename Datatype>
bool CircularVectorMemoryBuffer<Pointer_type>::append(Datatype datapoint_to_write, Pointer_type vector_num) {
	
	bool op_success = true; // Operation success flag
	bool buffer_overrun = false; // This is set to true if new data results in buffer returning to beginning
	Pointer_type new_after_vector_end; // Address of byte after vector end

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	// Find location of vector pointers
	Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);

	// Make sure data is right size
	if (dp_size != sizeof(Datatype)) {
		op_success = false; // False if datapoint is of wrong size
	}
	
	// Bytes to end of buffer
	Pointer_type bytes_to_buffer_end = 1 + (end_buffer - after_end_vector);

	// Check if datawrite will overrun buffer
	if (dp_size > bytes_to_buffer_end) {
		buffer_overrun = true;
	}

	if (buffer_overrun) { // If write results in overrun

		// Convert data into vector
		union converter { // Define union structure
			Datatype in_data; // Datatype is defined by input
			uint8_t out_data[sizeof(Datatype)]; // Takes in same number of bytes as size of Datatype
		}
		converter.in_data = datapoint_to_write;

		// Write first part (MSB) into end of buffer
		for (int i = 0; i < bytes_to_buffer_end; i++) {
			op_success &= _CVMB.pmemwrite(after_end_vector + i, converter.out_data[i]);
		}

		// Write second part(LSB) into beginning of buffer
		for (int i = bytes_to_buffer_end - 1; i < dp_size; i++) {
			op_success &= _CVMB.pmemwrite(begin_buffer + i - bytes_to_buffer_end + 1, converter.out_data[i]);
		}

		new_after_vector_end = begin_buffer + dp_size - bytes_to_buffer_end;

	}
	else { // If write doesn't result in overrun
		op_success &= _writeDataPoint(after_end_vector, datapoint_to_write);

		new_after_vector_end = after_end_vector + dp_size;
	}

	// New next available byte pointer
	op_success &= _writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 3, new_after_vector_end);

	// If new datapoint overwrote vector beginning datapoint, reset vector begin pointer
	if (new_after_vector_end > begin_vector && after_end_vector <= begin_vector) {
		op_success &= _writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 2, new_after_vector_end);
	}
}

// This func retrieves the pointer values for the specified vector.
template<typename Pointer_type>
template<typename Datatype>
Pointer_type *CircularVectorMemoryBuffer<Pointer_type>::_vectorInfo(Pointer_type vector_num) {

	// Find location of vector pointers
	Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);

	// Get first buffer location
	Pointer_type begin_vector_buffer = _readDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 1, Pointer_type vector_num);

	// Get first consumed vector byte
	Pointer_type first_consumed_address = _readDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 2, Pointer_type vector_num);

	// Read next available byte after vector
	Pointer_type last_consumed_address = _readDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 3, Pointer_type vector_num);

	// Read last buffer location
	Pointer_type end_vector_buffer = _readDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 4, Pointer_type vector_num);

	// Read datapoint size
	Pointer_type dp_size = _readDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 4, Pointer_type vector_num);

	// Create vector for output
	Pointer_type out_vec[5];

	out_vec[1] = begin_vector_buffer;
	out_vec[2] = first_consumed_address;
	out_vec[3] = last_consumed_address;
	out_vec[4] = end_vector_buffer;
	out_vec[5] = dp_size;
	
	return out_vec;
}

// This func prepends data to beginning of circular buffer, returnin true if operation was successful
template<typename Pointer_type>
template<typename Datatype>
bool CircularVectorMemoryBuffer<Pointer_type>::prepend(Datatype datapoint_to_write, Pointer_type vector_num) {

	bool op_success = true; // Operation success flag
	bool buffer_overrun = false; // This is set to true if new data results in buffer returning to beginning
	Pointer_type new_vector_begin; // Address of byte after vector end

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	// Find location of vector pointers
	Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);

	// Make sure data is right size
	if (dp_size != sizeof(datapoint_to_write)) {
		op_success = false; // False if datapoint is of wrong size
	}

	// Bytes to beginning of buffer
	Pointer_type bytes_to_buffer_begin = begin_vector - begin_buffer;

	// Check if datapoint overruns buffer
	if (dp_size > bytes_to_buffer_begin) {
		buffer_overrun = true;
	}

	if (buffer_overrun) { // If write results in overrun
		// Convert data into vector
		union converter { // Define union structure
			Datatype in_data; // Datatype is defined by input
			uint8_t out_data[sizeof(Datatype)]; // Takes in same number of bytes as size of Datatype
		}
		converter.in_data = datapoint_to_write;

		// Write first part (MSB) into end of buffer
		for (int i = 0; i < dp_size - bytes_to_buffer_begin; i++) {
			op_success &= _CVMB.pmemwrite(end_buffer - (dp_size - bytes_to_buffer_begin) + i, converter.out_data[i]);
		}

		// Write second part (LSB) into beginning of buffer
		for (int i = 0; i < bytes_to_buffer_begin; i++) {
			op_success &= _CVMB.pmemwrite(begin_buffer + i, converter.out_data[dp_size - bytes_to_buffer_begin - 1 + i]);
		}

		new_vector_begin = end_buffer - (dp_size - bytes_to_buffer_begin);
		
	}
	else { // If write doesn't result in overrun
		
		op_success &= _writeDataPoint(buffer_begin - dp_size, datapoint_to_write);

		new_vector_begin = buffer_begin - dp_size;
	}

	// New vector begin pointer
	op_success &= _CVMB.pmemwrite(first_pointer_first_byte + sizeof(Pointer_type) * 2, new_vector_begin);

	// If new datapoint overwrote vector ending datapoint, reset vector next available byte pointer (which is just the beginning byte)
	if (new_vector_begin < after_end_vector && begin_vector >= after_end_vector) {
		op_success &= _writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 3, new_vector_begin);
	}
}

// This func finds number of slots available for datapoints (NOTE: It cannot distinguish between an empty or full array, they both return 0 slots available)
template<typename Pointer_type>
Pointer_type CircularVectorMemoryBuffer<Pointer_type>::slotsAvailable(Pointer_type vector_num) {

	Pointer_type out;

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	if (begin_vector > after_end_vector) { // Vector overruns buffer
		out = begin_vector - after_end_vector; 
	}
	else if (begin_vector < after_end_vector) { // Vector is midbuffer
		out = (end_buffer - begin_buffer) - (after_end_vector - begin_vector);
	}
	else { // If the buffer is full or empty
		out = 0;
	}

	out /= dp_size; // Turn bytes into slots

	return out;
}

// This func finds the number of slots consumed by datapoints (NOTE: It cannot distinguish between an empty or full array, they both return 0 slots consumed)
template<typename Pointer_type>
Pointer_type CircularVectorMemoryBuffer<Pointer_type>::slotsConsumed(Pointer_type vector_num) {

	Pointer_type out;

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	if (begin_vector < after_end_vector) { // Vector is midbuffer
		out = after_end_vector - begin_vector;
	}
	else if (begin_vector < after_end_vector) { // 
		out = (end_buffer - begin_buffer) - (begin_vector - after_end_vector);
	}
	else { // If the buffer is full or empty
		out = 0;
	}

	out /= dp_size; // Turn bytes into slots

	return out
}

// This func performs a destructive or nondestructive read on the rearmost datapoint in the vector. NOTE: This function will still return data if the array is empty (you must be sure of array count yourself)
template<typename Pointer_type>
template<typename Datatype>
Datatype CircularVectorMemoryBuffer<Pointer_type>::pop(Pointer_type vector_num, Datatype expected_return_datatype, bool destructive = true) {

	Datatype return_data;

	if (destructive) { // If this is a destructive read
		Pointer_type new_after_end_addr; // Declare new after end address variable

		// Find location of vector pointers
		Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);
	}

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	if (after_end_vector - begin_buffer < dp_size) { // Datapoint wraps buffer

		union converter { // Define union structure
			Datatype out_data; // Datatype is defined by input
			uint8_t in_data[sizeof(Datatype)]; // Takes in same number of bytes as size of Datatype
		}

		// Gather datapoint in bytes
		for (int i = 0; i < dp_size - (after_end_vector - begin_buffer); i++) { // Loop rear (MSB) bytes 
			converter.in_data[i] = _CVMB.pmemread(end_buffer + 1 + i - (dp_size - (after_end_vector - begin_buffer)));
		}

		for (int i = 0; i < after_end_vector - begin_buffer; i++) { // Loop front (LSB) bytes
			converter.in_data[i + dp_size - (after_end_vector - begin_buffer)] = pmemread(begin_buffer + i)
		}

		return_data = converter.out_data;

		if (destructive) { // If this is a destructive read
			new_after_end_addr = end_buffer - (dp_size - (after_end_vector - begin_buffer)) + 1; // Set new vector end pointer loc
		}
	}
	else { // Datapoint is within buffer

		return_data = _readDataPoint(after_end_vector - dp_size, Datatype); // Read data

		if (destructive) { // If this is a destructive read
			new_after_end_addr = begin_vector; // Set new vector end pointer loc
		}
	}

	// See destructive input to see if we move the pointer or not (this performs either a destructive or non destructive read)
	// Instead of actively erasing the data, we just adjust our pointer (reduces reads on the data, although the limitation is still there on the pointers)
	if (destructive) {
		(void)_writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 3, new_after_end_addr);
	}

	return return_data;
}

// This func performs a destructive or nondestructive read on the frontmost datapoint in the vector. NOTE: This function will still return data if the array is empty (you must be sure of array count yourself) 
template<typename Pointer_type>
template<typename Datatype>
Datatype CircularVectorMemoryBuffer<Pointer_type>::frontPop(Pointer_type vector_num, Datatype expected_return_datatype, bool destructive = true) {
	
	Datatype return_data;

	if (destructive) { // If this is a destructive read
		Pointer_type new_begin_addr; // Declare new begin address variable

		// Find location of vector pointers
		Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);
	}

	// Find location of vector pointers
	Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_buffer = *p[1];
	Pointer_type begin_vector = *p[2];
	Pointer_type after_end_vector = *p[3];
	Pointer_type end_buffer = *p[4];
	Pointer_type dp_size = *p[5];

	if (end_buffer - begin_vector < dp_size) { // If datapoint wraps
		
		union converter { // Define union structure
			Datatype out_data; // Datatype is defined by input
			uint8_t in_data[sizeof(Datatype)]; // Takes in same number of bytes as size of Datatype
		}

		// Gather data in bytes
		for (i = 0; i < (end_buffer - begin_vector) + 1; i++) { // Read rear (MSB) bytes
			converter.in_data[i] = _CVMB.pmemread(begin_vector + i);
		}

		for (i = 0; i < dp_size - ((end_buffer - begin_vector) + 1) + 1 ; i++) { // Read front (LSB) bytes
			converter.in_data[i + dp_size - ((end_buffer - begin_vector) + 1)] = _CVMB.pmemread(begin_buffer + i);
		}

		return_data = converter.out_data;

		if (destructive) { // If this is a destructive read
			new_begin_addr = begin_buffer + (dp_size - ((end_buffer - begin_vector) + 1)); // Set new vector begin pointer
		}
	}
	else { // If datapoint is within buffer		
		
		return_data = _readDataPoint(begin_vector, Datatype);

		if (destructive) { // If this is a destructive read
			new_begin_addr = begin_vector + dp_size; // Set new vector begin pointer
		}
	}

	// See destructive input to see if we move the pointer or not (this performs either a destructive or non destructive read)
	// Instead of actively erasing the data, we just adjust our pointer (reduces reads on the data, although the limitation is still there on the pointers)
	if (destructive) {
		(void)_writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 2, new_begin_addr);
	}

	return return_data;
}

// This func returns the size of a datapoint in the vector specified.
template<typename Pointer_type>
Pointer_type CircularVectorMemoryBuffer<Pointer_type>::datapointSize(Pointer_type vector_num) {

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type dp_size = *p[5];

	return dp_size;
}

// This func clears the specified vector.
template<typename Pointer_type>
bool CircularVectorMemoryBuffer<Pointer_type>::clearVector(Pointer_type vector_num) {

	bool out;

	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(vector_num);

	Pointer_type begin_vector = *p[2];

	// Find location of vector pointers
	Pointer_type first_pointer_first_byte = _first_available_byte + 1 + (vector_num * sizeof(Pointer_type) * 5);

	// Set after end vector pointer equal to begin vector pointer
	out = _writeDataPoint(first_pointer_first_byte + sizeof(Pointer_type) * 3, begin_vector);

	return out;
}

// This func returns the number of vectors in the circular vector memory.
template<typename Pointer_type>
uint8_t CircularVectorMemoryBuffer<Pointer_type>::numVectors() {
	return _num_vectors;
}

// This func returns the first address consumed by the entire block of memory dedicated to the circular vector memory.
template<typename Pointer_type>
Pointer_type CircularVectorMemoryBuffer<Pointer_type>::firstMemAddress() {
	return _first_available_byte;
}

// This func returns the last address consumed by the entire block of memory dedicated to the circuar vector memory.
template<typename Pointer_type>
Pointer_type CircularVectorMemoryBuffer<Pointer_type>::lastMemAddress() {
	
	// Get pointer info
	Pointer_type *pointers;
	p = _vectorInfo(_num_vectors - 1);

	Pointer_type end_buffer = *p[4]; // Last memory location of buffer of last vector

	return end_buffer
}
