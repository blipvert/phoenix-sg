// <edit>
#include "linden_common.h"
#include "llimagemetadatareader.h"
LLJ2cParser::LLJ2cParser(U8* data,int data_size)
{
    	if(data && data_size)
	{
		mData.resize(data_size);
		memcpy(&(mData[0]), data, data_size);
		//std::copy(data,data+data_size,mData.begin());
	}
	mIter = mData.begin();
}

U8 LLJ2cParser::nextChar()
{
	U8 rtn = 0x00;
	if(mIter != mData.end())
	{
		rtn = (*mIter);
		mIter++;
	}
	return rtn;
}

std::vector<U8> LLJ2cParser::nextCharArray(int len)
{
	std::vector<U8> array;
	if(len > 0)
	{
		array.resize(len);
		for(S32 i = 0; i < len; i++)
		{
			array[i] = nextChar();
		}
	}
	return array;
}

std::vector<U8> LLJ2cParser::GetNextComment()
{
	std::vector<U8> content;
	while (mIter != mData.end())
	{
		U8 marker = nextChar();
		if (marker == 0xff)
		{
		    U8 marker_type = nextChar();
		    if (marker_type == 0x4f)
		    {
				continue;
		    }
		    if (marker_type == 0x90)
		    {
				//llinfos << "FOUND 0x90" << llendl;
				break; //return empty vector
		    }
		    
		    if (marker_type == 0x64)
		    {
				//llinfos << "FOUND 0x64 COMMENT SECTION" << llendl;
				S32 len = ((S32)nextChar())*256 + (S32)nextChar();
		   		if (len > 3) content = nextCharArray(len - 2);
				return content;
		    }
		}
	}
	content.clear(); //return empty vector by clear anything there
	return content;
}
std::map<std::string,std::string> LLImageMetaDataReader::ExtractKDUUploadComment(U8* data,int data_size)
{
	LLJ2cParser parser = LLJ2cParser(data,data_size);

	std::string decodedComment;

	std::map<std::string,std::string>  result;
	while(1)
	{
		std::vector<U8> comment = parser.GetNextComment();
		if (comment.empty())break; //exit loop
		if (comment[0] == 0x00 && comment.size() <= 130)
		{
			std::string fullComment(comment.begin()+2,comment.end());

			result["full"]=fullComment;
			//a=abbee3b5-fbe0-4cfa-8d15-323d6800448e&h=480&w=640&z=20081118204138&c=26961aff
			int pos = 0;//where we are in the string
			while(pos<fullComment.length())
			{
				int equalsPos = fullComment.find("=",pos);
				std::string infoType = fullComment.substr(pos,equalsPos-pos);
				if(infoType.length()>2 && infoType.length()<30)infoType="Other";
				pos=fullComment.find("&",pos);
				if(pos==(int)std::string::npos)pos=fullComment.length();
				pos++;
				std::string info = fullComment.substr(equalsPos+1,pos-2-equalsPos);
				//llinfos << "full comment was "<<fullComment.c_str()<<" and info is " << info.c_str() <<" and type is "<<infoType.c_str()<<" and pos is"<<pos<<llendl;
				result[infoType]=info;
			}
		}
	}

	return result;
}