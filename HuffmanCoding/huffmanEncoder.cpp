/*
	Huffman encoding
*/

#include "huffmanEncoder.h"

#include "binarytree.h"
#include "bitstream.h"

#include <iostream>
#include <queue>
#include <sstream>
#include <chrono>

using namespace std;
using namespace std::chrono;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef BinaryTree<uint8_t> HuffmanTree;
typedef HuffmanTree::NodeId HuffmanNode;

struct SCharacter
{
	unsigned int frequency = 0;
	unsigned char charCode = 0;
	HuffmanNode treeNode = 0;
};

struct SHuffmanTreeHeader
{
	uint32_t bitcount = 0;
};

//Comparison predicate - sort elements so charstructs with the smallest frequency are at the top of the queue
struct CharacterCompare
{
	bool operator()(const SCharacter& left, const SCharacter& right) const
	{
		//default is operator <
		return (left.frequency > right.frequency);
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//functions

static int findchar(const HuffmanTree& tree, HuffmanNode startNode, uint32_t pattern, uint8_t& ch)
{
	HuffmanNode curnode = startNode;
	int depth = 0;

	for (int i = 0; i < 32; i++)
	{
		curnode = tree.getChildNode(curnode, (pattern & (1 << (31 - i))) != 0);
		depth++;

		if (tree.isNodeLeaf(curnode))
		{
			tree.getNodeValue(curnode, ch);
			break;
		}
	}

	return depth;
}

//Check if character exists in huffman tree
static bool findpattern(const HuffmanTree& tree, HuffmanNode node, uint8_t c, uint32_t& pattern, uint32_t& depth)
{
	//Character or node cannot be null
	if (!tree.isNode(node))
		return false;

	uint8_t ch = 0;
	HuffmanNode left = tree.getChildNodeLeft(node);
	HuffmanNode right = tree.getChildNodeRight(node);

	if (tree.isNodeLeaf(node))
	{
		tree.getNodeValue(node, ch);
		if (ch == c)
			return true;
	}

	//Search left branch recursively
	// 0
	if (findpattern(tree, left, c, pattern, depth))
	{
		//Shift pattern but do not insert a new bit (1010 -> 0101)
		pattern = (pattern >> 1);
		depth++;
		return true;
	}

	//Search right branch recursively
	// 1
	if (findpattern(tree, right, c, pattern, depth))
	{
		//Shift pattern and insert new bit at front of pattern (1010 -> 1101)
		pattern = (pattern >> 1) | (1 << 31);
		depth++;
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void serializeNode(const HuffmanTree& tree, HuffmanNode node, BitStream& stream)
{
	if (!tree.isNode(node))
		return;

	if (tree.isNodeLeaf(node))
	{
		stream.writebit(1);

		uint8_t value = 0;
		tree.getNodeValue(node, value);
		stream.write(value);
	}
	else
	{
		stream.writebit(0);
		serializeNode(tree, tree.getChildNodeLeft(node), stream);
		serializeNode(tree, tree.getChildNodeRight(node), stream);
	}
}

static HuffmanNode deserializeNode(HuffmanTree& tree, HuffmanNode node, BitStream& stream)
{
	BitStream::bit_t bit = 0;
	stream.readbit(bit);

	if (bit == 1)
	{
		BitStream::byte_t byte = 0;
		stream.read(byte);
		return tree.allocNode(byte);
	}
	else
	{
		HuffmanNode parent = tree.allocNode(0);
		
		tree.linkNodeLeft(parent, deserializeNode(tree, node, stream));
		tree.linkNodeRight(parent, deserializeNode(tree, node, stream));

		return parent;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool huffmanCompress(const string& text, ostream& encodedText)
{
	cout << "Beginning compression.\n";

	//Binary tree
	HuffmanTree tree;
	//Root node of binary tree
	HuffmanNode rootNode = 0;

	//Compressed stream
	BitStream bitstream(text.size() * BitStream::bytewidth);

	cout << "Building tree...\n";

	const uint32_t tableSize = numeric_limits<uint8_t>::max();

	//Table for tracking the frequency of characters
	SCharacter frequencyTable[tableSize] = {};

	//Fill character frequency table
	for (char curChar : text)
	{
		frequencyTable[(size_t)curChar].charCode = curChar;
		frequencyTable[(size_t)curChar].frequency++;
	}

	//Scan frequency table and sort characters into a character queue
	priority_queue<SCharacter, vector<SCharacter>, CharacterCompare> alphabetQueue;

	for (auto it = begin(frequencyTable); it != end(frequencyTable); it++)
	{
		if ((it->frequency != 0) && (it->charCode > 0))
		{
			SCharacter charStruct;
			charStruct.frequency = it->frequency;
			charStruct.charCode = it->charCode;
			charStruct.treeNode = tree.allocNode(charStruct.charCode);
			alphabetQueue.push(charStruct);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Build tree from character queue

	while (alphabetQueue.size() > 1)
	{
		//Read two elements from the character queue at a time

		//First element in queue
		SCharacter charStruct0(alphabetQueue.top());
		alphabetQueue.pop();

		//Second element in queue
		SCharacter charStruct1(alphabetQueue.top());
		alphabetQueue.pop();

		BinaryTree<SCharacter>::NodeId right = 0;
		BinaryTree<SCharacter>::NodeId left = 0;

		//Check if first queue element is greater than the second element
		//The right-hand node must take the value with the larger frequency
		if (charStruct0.frequency > charStruct1.frequency)
		{
			right = charStruct0.treeNode;
			left = charStruct1.treeNode;
		}
		else
		{
			right = charStruct1.treeNode;
			left = charStruct0.treeNode;
		}

		//Create parent char
		SCharacter parentStruct;
		parentStruct.charCode = 0;
		parentStruct.frequency = charStruct1.frequency + charStruct0.frequency;
		parentStruct.treeNode = tree.allocNode(0);

		//Link child nodes to new parent node
		tree.linkNodeLeft(parentStruct.treeNode, left);
		tree.linkNodeRight(parentStruct.treeNode, right);

		//Enqueue parent char
		alphabetQueue.push(parentStruct);

		//Update root node to the most recent parent node each iteration
		rootNode = parentStruct.treeNode;
	}

	cout << "Tree built.\n";

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Serialize huffman codes from tree

	serializeNode(tree, rootNode, bitstream);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Begin compression

	cout << "Encoding...\n";
	cout << "0% completed";

	size_t charidx = 0;

	//Initial time
	auto t0 = high_resolution_clock::now();

	for (char c : text)
	{
		uint32_t pattern = 0;
		uint32_t depth = 0;

		if (!findpattern(tree, rootNode, c, pattern, depth))
		{
			cerr << "No pattern could be found for char '" << c << "'\n";
			continue;
		}

		//Print encoding progress to console
		charidx++;
		if (((charidx * 100) % text.size()) == 0) //Only print every percentile as printing can be expensive
		{
			size_t perc = (charidx * 100) / text.size();

			//Print percentage complete and time
			cout << "\r";
			cout << perc << "% completed (" << duration_cast<milliseconds>(high_resolution_clock::now() - t0).count() << "ms): " << string((size_t)perc / 5, '|');
			cout.flush();
		}

		//Write bit pattern to stream
		bitstream.write(pattern, 0, depth);
	}

	cout << endl;
	cout << "Encoded.\n";

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Write data to stream

	//Header
	SHuffmanTreeHeader header;
	header.bitcount = (uint32_t)bitstream.getBitCount();

	//Get initial position of write pointer
	streampos encodedTextSize = encodedText.tellp();

	//Write header
	encodedText.write(reinterpret_cast<const char*>(&header), sizeof(SHuffmanTreeHeader));
	assert(encodedText.good());
	//Write encoded bitstream
	encodedText.write(reinterpret_cast<const char*>(bitstream.getBitBuffer()), bitstream.getByteCount());
	assert(encodedText.good());

	//Get size of written data by subtracting original position of write pointer from the initial position
	encodedTextSize = encodedText.tellp() - encodedTextSize;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	cout << "Text length: " << text.size() << "B\n";
	cout << "Compressed text length: " << encodedTextSize << "B\n";
	cout << "Compression ratio: " << (float)encodedTextSize / text.size() << endl;

	return true;
}

bool huffmanDecompress(istream& encodedText, ostream& decodedText)
{
	cout << "Beginning decompression.\n";

	//Read data header
	SHuffmanTreeHeader header;

	encodedText.read(reinterpret_cast<char*>(&header), sizeof(SHuffmanTreeHeader));
	assert(encodedText.good());

	size_t bytecount = header.bitcount / BitStream::bytewidth;
	if (header.bitcount % BitStream::bytewidth)
		bytecount++;

	vector<BitStream::byte_t> tempBitBuffer(bytecount);
	encodedText.read(reinterpret_cast<char*>(&tempBitBuffer[0]), bytecount);
	assert(encodedText.good());

	//Create encoded text bitstream
	BitStream bitstream((const BitStream::byte_t*)&tempBitBuffer[0], header.bitcount);

	cout << "Rebuilding tree...\n";

	HuffmanTree tree;
	HuffmanNode root = 1;
	deserializeNode(tree, 0, bitstream);

	HuffmanNode curnode = root;

	cout << "Tree rebuilt\n";
	cout << "Decoding...\n";
	cout << "0% completed";

	size_t initialRead = bitstream.getRead();				 //Initial position of the read pointer (in bits)
	size_t dataSize = header.bitcount - bitstream.getRead(); //Size of the encoded data				(in bits)

	auto t0 = high_resolution_clock::now();

	while ((header.bitcount - bitstream.getRead()) > 0)
	{
		BitStream::bit_t bit = 0;
		bitstream.readbit(bit);

		curnode = tree.getChildNode(curnode, bit);

		if (tree.isNodeLeaf(curnode))
		{
			uint8_t c = 0;
			tree.getNodeValue(curnode, c);
			decodedText << (char)c;


			curnode = root;
		}

		//Print decoding progress to console
		if ((((bitstream.getRead() - initialRead) * 100) % dataSize) == 0)
		{
			size_t perc = ((bitstream.getRead() - initialRead) * 100) / dataSize;

			cout << "\r";
			cout << perc << "% completed (" << duration_cast<milliseconds>(high_resolution_clock::now() - t0).count() << "ms): " << string((size_t)perc / 5, '|');
			cout.flush();
		}
	}

	cout << endl;
	cout << "Decoded.\n";

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////