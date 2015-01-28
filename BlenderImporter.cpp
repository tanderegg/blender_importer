#include "BlenderImporter.h"

////////////////////////////////////////////////
// BlenderImporter implementation
// 
// A wrapper class for general purpose functions
// maybe should just be namespaced
////////////////////////////////////////////////
BlenderFile BlenderImporter::LoadBlendFile(std::string filename, BlenderImporterConfig config) {
	BlenderFile blenderFile(filename, config);
	blenderFile.Load();
	return blenderFile;
}

// Computes the length of a field based on it's string representation,
// i.e. *variable is a pointer, variable[5][10] is a 2 dimensional array
unsigned int BlenderImporter::ComputeFieldLength(std::string field_name, unsigned short length, size_t pointer_size) {
	// check if this is a pointer
	if (field_name.at(0) == '*' || field_name.at(0) == '(')
		return pointer_size;

	size_t pos = field_name.find("[");

	int arrayMult = 1;

	while (pos != std::string::npos) {
		// determine the number of array dimensions
		size_t end = field_name.find("]", pos);
		std::string arrayLength = field_name.substr(pos+1, end-pos-1);

		std::stringstream ss(arrayLength);

		long num;
		if((ss >> num).fail()) {
			assert(0 && "Error in number conversion");
		}

		arrayMult *= num;
		pos = field_name.find("[", end);
	}
	return arrayMult * length;
}