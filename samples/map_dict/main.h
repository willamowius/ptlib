/*
 * main.h
 *
 * PWLib application header file for MapDictionary
 *
 * Copyright 2009 Derek J Smithies
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _MapDictionary_MAIN_H
#define _MapDictionary_MAIN_H

#include <ptlib/pprocess.h>

#include <map>

/*! \mainpage  map_dictionary 

The purpose of this program is to find out which is faster:
\li STL based map
\li PTLib based dictionary

By default, this program creates the 200 instances of the structure Element.
Each instance of the Element class is given a random (and hopefully) unique key.
The key can exist either as a string, or as a number.

Each instance of the class Element has a key to the next instance.

After creation of an instance of the class Element, it is put into a
map (or PDictionary), keyed of the random key.

Then, the code looks through the map (or PDictionary) for each created
instance. Since the code knows the first key, it can get the key of
the next element via the current element. In this way, the code is
made to make N acceses to seemingly random parts of the map (or
PDictionary).

Results so far indicate that when the key is integer, STL map is
always faster - by a factor of four or more.

When the key is a PString, STL map is only faster when there are more
than 1000 elements in the map.
*/

/**This class is the core of the thing. It is placed in the structure
   (map or dictionary) being tested. There are hundreds/thousands of
   these created. Each instance holds the key to the next
   element. Since the key to the next element is random, any code that
   follows this ends up walking over the entire map/directory in a
   random fashion */
class Element : public PObject
{
  PCLASSINFO(Element, PObject);

  /*The string held in this class */
  PString contents;

  /**the string key to the next element */
  PString nextKey;

  /**the integer key to the next element */
  PINDEX  nextIntKey;

  /**the string key to get to this element */
  PString thisKey;

  /**the integer key to get to this element */
  PINDEX thisIntKey;
};


/**Defination of a STL based map, keyed of a PString */
typedef std::map<PString, Element *> ElementMap;

/**Defination of a PTLib based dictionary, keyed of a PString */
typedef PDictionary<PString, Element > ElementDict;

/**Defination of a STL based map, keyed of an integer */
typedef std::map<PINDEX, Element *> ElementIntMap;

/**Defination of a PTLib based dictionary, keyed of an integer */
typedef PDictionary<POrdinalKey, Element > ElementIntDict;


/**This is where all the activity happens. This class is launched on
   program startup, and does timing runs on the map and dictionaries
   to see which is faster */
class MapDictionary : public PProcess
{
  PCLASSINFO(MapDictionary, PProcess)

  public:
  /**Constructor */
    MapDictionary();

    /**Destructor */
    ~MapDictionary();

    /**Program execution starts here */
    void Main();

    /**Test the STL map, which uses a string index */
    void TestMap();

    /**Test the PTLib dictionary, which uses a string index */
    void TestDict();

    /**Test the STL map, which uses an integer index */
    void TestIntMap();

    /**Test the PTLib dictionary, which uses an integer index */
    void TestIntDict();


 protected:
    
    /**The number of times we go over all the elements */
  PINDEX loops;

  /**The number of elements to put in the map/dictionary */
  PINDEX size;

  /**The string key to the first element in the map/dictionary */
  PString firstKey;

  /**the integer key to the first element in the map/dictionary */
  PINDEX firstIntKey;
};




#endif  // _MapDictionary_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
