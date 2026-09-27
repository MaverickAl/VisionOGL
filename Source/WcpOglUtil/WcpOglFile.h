#pragma once
#include "WcpOglPreComp.h"
#include "WcpOglUtility.h"
#include "WcpOglAssert.h"
#include <iostream>
#include <fstream>

namespace wcpogl
{
	void AppendDirectory( std::string &fileName, const char* directory );

	void ReadCommentBlock( std::istream &is );

	bool IsFileReadable( const std::string &fileName );

	template <class VariableType>
	void LoadVariable( std::istream &fileIn, VariableType &out )
	{
		// Check for comment blocks
		char ch;
		// Read all comment blocks
		while(true)
		{
			fileIn >> ch;
			if( ch == '/' )
			{
				assert( fileIn.get() == '*' );
				wcpogl::ReadCommentBlock( fileIn );
			}
			else
			{
				fileIn.putback( ch );
				break;
			}
		}

		// We're past the comment blocks, read the variable
		fileIn >> out;		
	}


	template <class VariableType>
	void LoadVariables(std::istream &fileIn, std::vector<VariableType> &variables)
	{
		char ch = '\n';
		VariableType variable;

		// Read in and ignore \n's, comments etc
		while( ch != '(' )
		{
			if(	!(fileIn >> ch) )
			{
				assert(false);
				return;
			}
		}
		assert(ch == '(' );
		//assume atleast one param
		fileIn >> variable;
		variables.push_back(variable);
		for(int i=1; ch!=')' ; i++)
		{
			fileIn >> ch;
			if(ch==')')
				break;
			// It wasn't the end of variables so put the character back
			fileIn.putback( ch );
			if ( !(fileIn >> variable) )
			{
				break;
			}
			variables.push_back(variable);
		}
	}

	template<class VariableType>
	bool LoadToken( std::istream &is, const std::string &tokenName, VariableType& out )
	{
		std::string		attribute;
		while( is >> attribute )
		{
			if( attribute == tokenName )
			{
				is >> out;
				return true;				
			}
		}
		// Reached the end of the file before loading
		assert(false);
		return false;
	}

	inline bool LoadQuotedString( std::istream &is, std::string &out )
	{
		char c;
		is >> c;
		assert( c == '"' );
		getline(is, out, '\"');

		return true;
	}

	// Find the number of a given character within a string
	inline uint MatchingsCharsInString( const std::string &searchString, char searchVal )
	{
		uint occurences = 0;
		for( uint i = 0; i < searchString.size(); i++ )
		{
			if ( searchString[i] == searchVal )
			{
				occurences++;
			}
		}
		return occurences;
	}

	// Count the number of unique variables in a string, separated by whitespace
	inline uint VarsInString( const std::string &searchString )
	{
		bool lastChWhitespace = true;
		uint varCount = 0;
		for( uint i = 0; i < searchString.size(); i++ )
		{
			if ( searchString[i] != ' ' && searchString[i] != '\t' )
			{
				if( lastChWhitespace == true )
				{
					lastChWhitespace = false;
					varCount++;
				}
			}
			else
			{
				lastChWhitespace = true;
			}
		}
		return varCount;

	}
}