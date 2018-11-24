// NOTE: In all functions calling for vector number inputs, the vector number is zero-based (e.g. first vector is vector number 0)

#pragma once
#include "Arduino.h"


template<typename Pointer_type>
class CircularVectorMemoryBuffer{
public:

	void CircularVectorMemoryBuffer(bool(*writef)(Pointer_type, uint8_t), uint8_t(*readf)(uint8_t), Pointer_type first_available_byte);

	template<typename Num_vector_datapoints, typename Vector_datatype, typename... Remaining_vector_details>
	bool CircularVectorMemoryBuffer<Pointer_type>::pointerCreate(Num_vector_datapoints num_vector_datapoints, Vector_datatype vector_datatype, Remaining_vector_details... remaining_vector_details);

	template<typename Datatype>
	bool CircularVectorMemoryBuffer<Pointer_type>::append(Datatype datapoint_to_write, Pointer_type vector_num);

	template<typename Datatype>
	bool CircularVectorMemoryBuffer<Pointer_type>::prepend(Datatype datapoint_to_write, Pointer_type vector_num);

	Pointer_type CircularVectorMemoryBuffer<Pointer_type>::slotsAvailable(Pointer_type vector_num);

	Pointer_type CircularVectorMemoryBuffer<Pointer_type>::slotsConsumed(Pointer_type vector_num);

	template<typename Datatype>
	Datatype CircularVectorMemoryBuffer<Pointer_type>::pop(Pointer_type vector_num, Datatype expected_return_datatype, bool destructive = true);

	template<typename Datatype>
	Datatype CircularVectorMemoryBuffer<Pointer_type>::frontPop(Pointer_type vector_num, Datatype expected_return_datatype, bool destructive = true);

	template<typename Pointer_type>
	Pointer_type CircularVectorMemoryBuffer<Pointer_type>::datapointSize(Pointer_type vector_num);

	template<typename Pointer_type>
	bool CircularVectorMemoryBuffer<Pointer_type>::clearVector(Pointer_type vector_num);

	template<typename Pointer_type>
	uint8_t CircularVectorMemoryBuffer<Pointer_type>::numVectors();

	template<typename Pointer_type>
	Pointer_type CircularVectorMemoryBuffer<Pointer_type>::firstMemAddress();

	template<typename Pointer_type>
	Pointer_type CircularVectorMemoryBuffer<Pointer_type>::lastMemAddress();

private:

	Pointer_type _first_available_byte;

	uint8_t _num_vectors;
	
	template<typename Num_vector_datapoints, typename Vector_datatype, typename... Remaining_vector_details>
	bool CircularVectorMemoryBuffer<Pointer_type>::_pointerCreate(Pointer_type last_pointer_byte_addr, Pointer_type last_vector_byte_addr, Num_vector_datapoints num_vector_datapoints,
		Vector_datatype vector_datatype, Remaining_vector_details... remaining_vector_details);

	template<typename Num_vector_datapoints, typename Vector_datatype>
	bool CircularVectorMemoryBuffer<Pointer_type>::_pointerCreate(Pointer_type last_pointer_byte_addr, Pointer_type last_vector_byte_addr,
		Num_vector_datapoints num_vector_datapoints, Vector_datatype vector_datatype);

	template<typename Datatype>
	bool CircularVectorMemoryBuffer<Pointer_type>::_writeDataPoint(Pointer_type address, Datatype datapoint_to_write);

	template<typename Datatype>
	Datatype CircularVectorMemoryBuffer<Pointer_type>::_readDataPoint(Pointer_type address, Datatype datatype);

	template<typename Datatype>
	Pointer_type *CircularVectorMemoryBuffer<Pointer_type>::_vectorInfo(Pointer_type vector_num);
};

#include <CircularVectorMemoryBuffer.tpp> // see https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file