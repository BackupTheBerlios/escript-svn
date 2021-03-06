// Exception.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "ExtObject.h"

namespace EScript {

/*! [Exception] ---|> [ExtObject] ---|> [Object]  */
class Exception : public ExtObject {
		ES_PROVIDES_TYPE_NAME(Exception)
	public:
		static Type* typeObject;
		static void init(EScript::Namespace & globals);
		// ----

		Exception(const std::string & msg,int line=0,Type * type=NULL);
		virtual ~Exception();

		void setMessage(const std::string & newMessage)		{	msg=newMessage;	}
		const std::string & getMessage()const				{	return msg;	}

		int getLine()const									{	return line;	}
		void setLine(int newLine)							{	line=newLine;	}

		const std::string & getStackInfo()const				{	return stackInfo;	}
		void setStackInfo(const std::string & s)			{	stackInfo=s;	}

		void setFilename(const std::string & filename)		{   filenameId=stringToIdentifierId(filename);  }
		void setFilenameId(identifierId _filenameId)		{   filenameId=_filenameId;  }
		std::string getFilename()const                  	{   return identifierIdToString(filenameId);    }
		identifierId getFilenameId()const                  	{   return filenameId;    }

		/// ---|> [Object]
		virtual Object * clone()const;
		virtual std::string toString()const;

	protected:
		std::string msg;
		std::string stackInfo;
		int line;
		identifierId filenameId;
};

}

#endif // EXCEPTION_H
