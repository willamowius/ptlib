// This is the modeling file for Coverity to avoid false positives.
// This code is not supposed to be compiled into PTLib!

struct _ios_fields{
	int _precision;
};
class ios : public _ios_fields{
	
	int precision() const { return _precision; }
	int precision(int newp) {return _precision; }
	
};

enum PStandardAssertMessage {
  PLogicError,
  POutOfMemory,
  PNullPointerReference,
  PInvalidCast, 
  PInvalidArrayIndex,
  PInvalidArrayElement,
  PStackEmpty,
  PUnimplementedFunction,
  PInvalidParameter,
  POperatingSystemError,
  PChannelNotOpen,
  PUnsupportedFeature,
  PInvalidWindow,
  PMaxStandardAssertMessage
};

inline bool PAssertFuncInline(bool b, const char * file, int line, const char * className, PStandardAssertMessage msg)
{
	if (!b) {
		__coverity_panic__();
	}
	return b;
};

bool PAssertFunc(bool b, const char * file, int line, const char * className, const char * msg)
{
	if (!b) {
		__coverity_panic__();
	}
	return b;
};

void PAssertFunc(const char * file, int line, const char * className, const char * msg)
{
	__coverity_panic__();
};


#define PLDAP_ATTR_INIT(cls, typ, nam, val) ((cls)::(typ) (nam) = (val));

