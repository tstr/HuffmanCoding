/*
	Huffman Coding program
*/

#include <iostream>
#include <fstream>
#include <sstream>

#include "binarycalc.h"
#include "binarytree.h"
#include "bitstream.h"

#include "huffmanEncoder.h"

using namespace std;

/*
	* Parses command line arguments.
	* Possible arguments are listed below:

	* decompress a target
		--decompress
	* compress a target
		--compress
	* target file path
		--target [path]
	* output file path
		--output [path]
*/
bool parseArguments(const string& commandline, bool& compress, string& targetname, string& outputname);

//////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		cerr << "Invalid number of arguments\n";
		return 1;
	}

	string commandline;
	for (int i = 1; i < argc; i++)
	{
		commandline += argv[i];
		commandline += " ";
	}

	bool compress = false;
	string targetName;
	string outputName;

	if (!parseArguments(commandline, compress, targetName, outputName))
	{
		cerr << "Invalid arguments\n";
		return 1;
	}

	ios::open_mode outflags = ios::out;	//Write
	ios::open_mode targetflags = ios::in;	//Read

	//When in compression mode the output must be a binary stream
	//When in decompression mode the target must be a binary stream
	if (compress)
	{
		//Compression mode
		outflags |= ios::binary;
	}
	else
	{
		//Decompression mode
		targetflags |= ios::binary;
	}

	ofstream outputfile(outputName, outflags);
	ifstream targetfile(targetName, targetflags);

	//Check if file streams were opened correctly
	if (outputfile.fail())
	{
		cerr << "Unable able to open output file: \"" << outputName << "\"\n";
		return 1;
	}

	if (targetfile.fail())
	{
		cerr << "Unable to open target file: \"" << targetName << "\"\n";
		return 1;
	}
	
	//Compression mode
	if (compress)
	{
		stringstream targetstream;
		targetstream << targetfile.rdbuf();

		if (targetstream.fail())
		{
			cerr << "Unable to copy target stream\n";
			return 1;
		}

		if (!huffmanCompress(targetstream.str(), outputfile))
		{
			cerr << "An error occured during compression\n";
			return 1;
		}

		if (outputfile.fail())
		{
			cerr << "Unable to write encoded text to output\n";
			return 1;
		}
	}
	//Decompression mode
	else
	{
		if (!huffmanDecompress(targetfile, outputfile))
		{
			cerr << "An error occurred during decompression\n";
			return 1;
		}

		if (outputfile.fail())
		{
			cerr << "Unable to write decoded text to output\n";
			return 1;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

vector<string> tokenize(const string& str, const char* delim)
{
	vector<string> tokens;

	size_t prev_pos = 0;
	size_t pos = 0;

	while ((pos = str.find_first_of(delim, prev_pos)) != string::npos)
	{
		if (pos > prev_pos)
			tokens.push_back(str.substr(prev_pos, pos - prev_pos));

		prev_pos = pos + 1;
	}

	if (prev_pos< str.length())
		tokens.push_back(str.substr(prev_pos, string::npos));

	return tokens;
}

bool parseArguments(const string& _commandline, bool& compress, string& targetname, string& outputname)
{
	string commandline(_commandline);

	if (commandline.empty())
		return false;

	bool inquotes = false;

	for (char& c : commandline)
	{
		if (c == '\"')
			inquotes = !inquotes;

		if (inquotes)
		{
			if (c == ' ')
				c = '\?';
		}
	}

	vector<string> tokens(tokenize(commandline, "--"));

	for (string& arg : tokens)
	{
		//Trim whitespace from string
		while (arg.back() == ' ')
			arg.erase(arg.size() - 1);
		while (arg.front() == ' ')
			arg.erase(0, 1);

		string argType;		//Argument type eg. --target
		string argParam;	//Argument parameter eg. <targetfile>

		if (size_t splitpos = arg.find_first_of(' '))
		{
			if (splitpos == string::npos)
			{
				argType = arg;
			}
			else
			{
				argType = arg.substr(0, splitpos);
				argParam = arg.substr(splitpos + 1);
			}
		}
		
		if (argType == "compress")
		{
			compress = true;
		}
		else if (argType == "decompress")
		{
			compress = false;
		}
		else if (argType == "target")
		{
			if (argParam.empty())
			{
				cerr << "--target must have one parameter\n";
				continue;
			}

			//If path is surrounded by "" then ignore them
			targetname = argParam.substr(argParam.find_first_not_of('\"'), argParam.find_last_not_of('\"') + 1);

			for (char& c : targetname)
			{
				if (c == '\?')
					c = ' ';
			}
		}
		else if (argType == "output")
		{
			if (argParam.empty())
			{
				cerr << "--output must have one parameter\n";
				continue;
			}

			//If path is surrounded by "" then ignore them
			outputname = argParam.substr(argParam.find_first_not_of('\"'), argParam.find_last_not_of('\"') + 1);

			for (char& c : outputname)
			{
				if (c == '\?')
					c = ' ';
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////