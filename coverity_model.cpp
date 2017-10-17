// This is the modeling file for Coverity to avoid false positives.
// This code is not supposed to be compiled into PTLib!

struct _ios_fields{
	int _precision;
};
class ios : public _ios_fields{
	
	int precision() const { return _precision; }
	int precision(int newp) {return _precision; }
	
};

#define PLDAP_ATTR_INIT(cls, typ, nam, val) cls::typ nam = val;
#define PAssertAlways(msg)	 __coverity_panic__();

