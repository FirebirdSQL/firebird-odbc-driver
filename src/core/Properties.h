#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

namespace IscDbcLibrary {

class Properties  
{
public:
	virtual const char * findValue (const char *name, const char *defaultValue) = 0;
	virtual void putValue (const char *name, const char *value) = 0;
	virtual void putValue (const char *name, int nameLength, const char *value, int valueLength) = 0;
	virtual int	 getCount () = 0;
	virtual const char *getName (int index) = 0;
	virtual const char *getValue (int index) = 0;
	virtual void release() = 0;
};

}; // end namespace IscDbcLibrary

#endif
