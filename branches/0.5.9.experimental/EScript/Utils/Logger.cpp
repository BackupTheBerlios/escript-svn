// Logger.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Logger.h"
#include <stdexcept>
#include <iostream>

namespace EScript{



// ------------------------------------------------
// LoggerGroup
void LoggerGroup::addLogger(const std::string & name,Logger * logger){
	if(logger==NULL)
		throw std::invalid_argument("addLogger(NULL)");
	loggerRegistry[name] = logger;
}

bool LoggerGroup::removeLogger(const std::string & name){
	return loggerRegistry.erase(name)>0;
}
void LoggerGroup::clearLoggers(){
	loggerRegistry.clear();
}
Logger * LoggerGroup::getLogger(const std::string & name){
	const loggerRegistry_t::iterator lbIt = loggerRegistry.lower_bound(name);
	if(lbIt!=loggerRegistry.end() && !(loggerRegistry.key_comp()(name, lbIt->first)) ){
		return lbIt->second.get();
	}
	return NULL;
}

//! ---|> Logger
void LoggerGroup::doLog(level_t l,const std::string & msg){
	for(loggerRegistry_t::iterator it=loggerRegistry.begin();it!=loggerRegistry.end();++it){
		it->second->log(l,msg);
	}
}


// ------------------------------------------------
// StdLogger


//! ---|> Logger
void StdLogger::doLog(level_t l,const std::string & msg){
	out << std::endl;
	switch(l){
		case DEBUG:{
			out << "Debug: ";
			break;
		}
		case INFO:{
			out << "Debug: ";
			break;
		}
		case WARNING:{
			out << "Warning: ";
			break;
		}
		case PEDANTIC_WARNING:{
			out << "Pedantic warning: ";
			break;
		}
		case ERROR:{
			out << "Error: ";
			break;
		}
		case FATAL:{
			out << "Fatal error: ";
			break;
		}
		case _ALL:
		case _NONE:
		default:{
			out << "Logging ("<<l<<"):";
		}
	}
	out << msg << std::endl;
}

}
