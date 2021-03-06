#ifndef __CCINTERFACE_H__
#define __CCINTERFACE_H__

#include "valuepair.hpp"

class CCInterface{
public:
	CCInterface(){}
	virtual ~CCInterface(){}
	virtual void run(int k)=0;
	virtual void run(int k, int para) {}
    virtual vector<ValuePair> getResults()=0;
};

#endif
