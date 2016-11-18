/*
	Compression/decompression functions using the huffman encoding algorithm
*/

#pragma once

#include <string>
#include <ostream>

//Compresses a sequence of text using the huffman encoding algorithm and stores the encoded text
bool huffmanCompress(
	const std::string& text,
	std::ostream& encodedText
);

//Decompresses some encoded text and stores the decoded value
bool huffmanDecompress(
	std::istream& encodedText,
	std::ostream& text
);
