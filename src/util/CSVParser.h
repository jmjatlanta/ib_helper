#pragma once
#include <string>
#include <fstream>

class CSVParser {
public:
	CSVParser(std::string line, std::string seperator) : line(line), seperator(seperator) {
		pos = seperator.length() * -1;
	}

   /***
    * In some cases, the field is fixed width, and not delimited
    * (i.e. the RA72)
    * @param fixedWidth the width of the field to grab
    * @returns the substring of fixedWidth characters starting at the current position
    */
   std::string retrieveNextElement(uint8_t fixedWidth) 
   {
      std::string retVal;
      size_t startPos = pos + seperator.length();
      if (startPos < line.size())
      {
          if (startPos + fixedWidth > line.size())
          {
            retVal = line.substr(startPos);
            pos = line.size();
          }
          else
          {
            retVal = line.substr(startPos, fixedWidth);
            pos += fixedWidth;
          }
      }
      return retVal;
   }

	/****
	 * Go through the line, retrieving the next element
	 */
	std::string retrieveNextElement() {
		std::string retVal = "";

		if (line.empty())
		{
			atEOL = true;
			return retVal;
		}

		if (pos > 0 && line.length() <= (size_t)pos)
		{
			atEOL = true;
			return retVal;
		}

		size_t end = line.find(seperator, pos + seperator.length());

		if (end == std::string::npos) {
			// we're at the end of the line
			retVal = line.substr(pos + seperator.length());
			pos = line.length();
			atEOL = true;
		} else {
			// we're not at the end of the line, another seperator found
			retVal = line.substr(pos + seperator.length(), end - (pos + seperator.length()));
			pos = end;
		}

		return retVal;
	}

	/****
	 * Retrieve the next element
	 * @param pos the position (0 based)
	 * @returns the string at that position
	 */
	std::string elementAt(int pos) {
		this->pos = seperator.length() * -1;
		for(int i = 0; i < pos; i++)
			retrieveNextElement();
		return retrieveNextElement();
	}

	uint16_t numElements()
	{
		this->pos = seperator.length() * -1;
		uint16_t counter = 0;
		atEOL = false;
		while (!atEOL)
		{
			retrieveNextElement();
			++counter;
		}
		return counter;
	}

	std::string line;
private:
	std::string seperator;
	long pos; // the current comma position
	bool atEOL;
};

/***
 * parses a CSV file
 */
class CSVFile
{
    public:
    CSVFile() {}
    CSVFile(const std::string& fileName) : fileName(fileName), in(std::ifstream(fileName.c_str()))
    {
    }
    bool eof()
    {
        if (!in.is_open())
            return true;
        return in.eof();
    }
    CSVParser next()
    {
        std::string line;
        std::getline(in, line);
        return CSVParser(line, ",");
    }
    std::string fileName;
    std::ifstream in;
};
