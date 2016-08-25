/*
 * args.h
 *
 * Program Argument Parsing class
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_ARGLIST_H
#define PTLIB_ARGLIST_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/** This class allows the parsing of a set of program arguments. This translates
   the standard argc/argv style variables passed into the main() function into
   a set of options (preceded by a '-' character) and parameters.
*/
class PArgList : public PObject
{
  PCLASSINFO(PArgList, PObject);

  public:
  /**@name Construction */
  //@{
    /** Create an argument list.
        An argument list is created given the standard arguments and a
       specification for options. The program arguments are parsed from this
       into options and parameters.

       The specification string consists of case significant letters for each
       option. If the letter is followed by the ':' character then the option
       has an associated string. This string must be in the argument or in the
       next argument.
     */
    PArgList(
      const char * theArgPtr = NULL,        ///< A string constituting the arguments 
      const char * argumentSpecPtr = NULL,  ///< The specification C string for argument options. See description for details.
      PBoolean optionsBeforeParams = true       ///< Parse options only before parameters 
    );
    /** Create an argument list. */
    PArgList(
      const PString & theArgStr,             ///< A string constituting the arguments 
      const char * argumentSpecPtr = NULL,   ///< The specification C string for argument options. See description for details.
      PBoolean optionsBeforeParams = true        ///< Parse options only before parameters 
    );
    /** Create an argument list. */
    PArgList(
      const PString & theArgStr,             ///< A string constituting the arguments 
      const PString & argumentSpecStr,       ///< The specification string for argument options. See description for details.
      PBoolean optionsBeforeParams = true        ///< Parse options only before parameters 
    );
    /** Create an argument list. */
    PArgList(
      int theArgc,                           ///< Count of argument strings in theArgv 
      char ** theArgv,                       ///< An array of strings constituting the arguments 
      const char * argumentSpecPtr = NULL,   ///< The specification C string for argument options. See description for details.
      PBoolean optionsBeforeParams = true        ///< Parse options only before parameters 
    );
    /** Create an argument list. */
    PArgList(
      int theArgc,                           ///< Count of argument strings in theArgv 
      char ** theArgv,                       ///< An array of strings constituting the arguments 
      const PString & argumentSpecStr,       ///< The specification string for argument options. See description for details.
      PBoolean optionsBeforeParams = true        ///< Parse options only before parameters 
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Output the string to the specified stream.
     */
    virtual void PrintOn(
      ostream & strm  ///< I/O stream to output to.
    ) const;

    /**Input the string from the specified stream. This will read all
       characters until a end of line is reached, then parsing the arguments.
     */
    virtual void ReadFrom(
      istream & strm  ///< I/O stream to input from. 
    );
  //@}

  /**@name Setting & Parsing */
  //@{
    /** Set the internal copy of the program arguments.
    */
    void SetArgs(
      const PString & theArgStr ///< A string constituting the arguments 
    );
    /** Set the internal copy of the program arguments. */
    void SetArgs(
      int theArgc,     ///< Count of argument strings in theArgv 
      char ** theArgv  ///< An array of strings constituting the arguments 
    );
    /** Set the internal copy of the program arguments. */
    void SetArgs(
      const PStringArray & theArgs ///< A string array constituting the arguments
    );

    /** Parse the arguments.
       Parse the standard C program arguments into an argument of options and
       parameters. Consecutive calls with <code>optionsBeforeParams</code> set
       to true will parse out different options and parameters. If SetArgs()
       function is called then the Parse() function will restart from the
       beginning of the argument list.

       The specification string consists of case significant letters for each
       option. If the letter is followed by a '-' character then a long name
       version of the option is present. This is terminated either by a '.' or
       a ':' character. If the single letter or long name is followed by the
       ':' character then the option has may have an associated string. This
       string must be within the argument or in the next argument. If a single
       letter option is followed by a ';' character, then the option may have
       an associated string but this MUST follow the letter immediately, if
       it is present at all.

       For example, "ab:c" allows for "-a -b arg -barg -c" and
       "a-an-arg.b-option:c;" allows for "-a --an-arg --option arg -c -copt".

       @return true if there is at least one parameter after parsing.
     */
    virtual PBoolean Parse(
      const char * theArgumentSpec,    ///< The specification string for argument options. See description for details.
      PBoolean optionsBeforeParams = true  ///< Parse options only before parameters
    );
    /** Parse the arguments. */
    virtual PBoolean Parse(
      const PString & theArgumentStr,  ///< The specification string for argument options. See description for details.       
      PBoolean optionsBeforeParams = true  ///< Parse options only before parameters
    );
  //@}

  /**@name Getting parsed arguments */
  //@{
    /** Get the count of the number of times the option was specified on the
       command line.

       @return option repeat count.
     */
    virtual PINDEX GetOptionCount(
      char optionChar        ///< Character letter code for the option 
    ) const;
    /** Get the count of option */
    virtual PINDEX GetOptionCount(
      const char * optionStr ///< String code for the option 
    ) const;
    /** Get the count of option */
    virtual PINDEX GetOptionCount(
      const PString & optionName ///< String code for the option 
    ) const;

    /** Get if option present.
      Determines whether the option was specified on the command line.

       @return true if the option was present.
     */
    PBoolean HasOption(
      char optionChar             ///< Character letter code for the option 
    ) const;
    /** Get if option present. */
    PBoolean HasOption(
      const char * optionStr     ///< String letter code for the option 
    ) const;
    /** Get if option present. */
    PBoolean HasOption(
      const PString & optionName ///<  String code for the option 
    ) const;

    /** Get option string.
       Gets the string associated with an option e.g. -ofile or -o file
       would return the string "file". An option may have an associated string
       if it had a ':' character folowing it in the specification string passed
       to the Parse() function.

       @return the options associated string.
     */
    virtual PString GetOptionString(
      char optionChar,          ///< Character letter code for the option 
      const char * dflt = NULL  ///< Default value of the option string 
    ) const;
    /** Get option string. */
    virtual PString GetOptionString(
      const char * optionStr,   ///< String letter code for the option 
      const char * dflt = NULL  ///<Default value of the option string 
    ) const;
    /** Get option string. */
    virtual PString GetOptionString(
      const PString & optionName, ///< String code for the option 
      const char * dflt = NULL    ///< Default value of the option string 
    ) const;

    /** Get the argument count.
       Get the number of parameters that may be obtained via the
       <code>GetParameter()</code> function. Note that this does not include options
       and option strings.

       @return count of parameters.
     */
    PINDEX GetCount() const;

    /** Get the parameters that were parsed in the argument list.

       @return array of parameter strings at the specified index range.
     */
    PStringArray GetParameters(
      PINDEX first = 0,
      PINDEX last = P_MAX_INDEX
    ) const;

    /** Get the parameter that was parsed in the argument list.

       @return parameter string at the specified index.
     */
    PString GetParameter(
      PINDEX num   ///< Number of the parameter to retrieve. 
    ) const;

    /** Get the parameter that was parsed in the argument list. The argument
       list object can thus be treated as an "array" of parameters.

       @return parameter string at the specified index.
     */
    PString operator[](
      PINDEX num   ///< Number of the parameter to retrieve. 
    ) const;

    /** Shift the parameters by the specified amount. This allows the parameters
       to be parsed at the same position in the argument list "array".
     */
    void Shift(
      int sh ///< Number of parameters to shift forward through list 
    );

    /** Shift the parameters by the specified amount. This allows the parameters
       to be parsed at the same position in the argument list "array".
     */
    PArgList & operator<<(
      int sh ///< Number of parameters to shift forward through list 
    );

    /** Shift the parameters by the specified amount. This allows the parameters
       to be parsed at the same position in the argument list "array".
     */
    PArgList & operator>>(
      int sh ///< Number of parameters to shift backward through list 
    );
  //@}

  /**@name Errors */
  //@{
    /** This function is called when access to illegal parameter index is made
       in the GetParameter function. The default behaviour is to output a
       message to the standard <code>PError</code> stream.
     */
    virtual void IllegalArgumentIndex(
      PINDEX idx ///< Number of the parameter that was accessed. 
    ) const;

    /** This function is called when an unknown option was specified on the
       command line. The default behaviour is to output a message to the
       standard <code>PError</code> stream.
     */
    virtual void UnknownOption(
      const PString & option   ///< Option that was illegally placed on command line. 
    ) const;

    /** This function is called when an option that requires an associated
       string was specified on the command line but no associated string was
       provided. The default behaviour is to output a message to the standard
       <code>PError</code> stream.
     */
    virtual void MissingArgument(
      const PString & option  ///< Option for which the associated string was missing. 
    ) const;
  //@}

  protected:
    /// The original program arguments.
    PStringArray argumentArray;
    /// The specification letters for options
    PString      optionLetters;
    /// The specification strings for options
    PStringArray optionNames;
    /// The count of the number of times an option appeared in the command line.
    PIntArray    optionCount;
    /// The array of associated strings to options.
    PStringArray optionString;
    /// The index of each .
    PIntArray    parameterIndex;
    /// Shift count for the parameters in the argument list.
    int          shift;

  private:
    PBoolean ParseOption(PINDEX idx, PINDEX offset, PINDEX & arg, const PIntArray & canHaveOptionString);
    PINDEX GetOptionCountByIndex(PINDEX idx) const;
    PString GetOptionStringByIndex(PINDEX idx, const char * dflt) const;
    int m_argsParsed;
};


#ifdef P_CONFIG_FILE

/**This class parse command line arguments with the ability to override them
   from a PConfig file/registry.
  */
class PConfigArgs : public PArgList
{
    PCLASSINFO(PConfigArgs, PArgList);
  public:
  /**@name Construction */
  //@{
    PConfigArgs(
      const PArgList & args   ///< Raw argument list.
    );
  //@}

  /**@name Overrides from class PArgList */
  //@{
    /** Get the count of the number of times the option was specified on the
       command line.

       @return option repeat count.
     */
    virtual PINDEX GetOptionCount(
      char optionChar  ///< Character letter code for the option
    ) const;
    /** Get the count of option */
    virtual PINDEX GetOptionCount(
      const char * optionStr ///< String code for the option
    ) const;
    /** Get the count of option */
    virtual PINDEX GetOptionCount(
      const PString & optionName ///< String code for the option
    ) const;

    /** Get option string.
       Gets the string associated with an option e.g. -ofile or -o file
       would return the string "file". An option may have an associated string
       if it had a ':' character folowing it in the specification string passed
       to the Parse() function.

       @return the options associated string.
     */
    virtual PString GetOptionString(
      char optionChar,          ///< Character letter code for the option 
      const char * dflt = NULL  ///< Default value of the option string 
    ) const;

    /** Get option string. */
    virtual PString GetOptionString(
      const char * optionStr,   ///< String letter code for the option 
      const char * dflt = NULL  ///< Default value of the option string 
    ) const;

    /** Get option string. */
    virtual PString GetOptionString(
      const PString & optionName, ///< String code for the option 
      const char * dflt = NULL    ///< Default value of the option string 
    ) const;
  //@}

  /**@name Overrides from class PArgList */
  //@{
    /**Save the current options to the PConfig.
       This function will check to see if the option name is present and if
       so, save to the PConfig all of the arguments present in the currently
       parsed list. Note that the optionName for saving is not saved to the
       PConfig itself as this would cause the data to be saved always!
      */
    void Save(
      const PString & optionName   ///< Option name for saving.
    );

    /**Set the PConfig section name for options.
      */
    void SetSectionName(
      const PString & section ///< New section name 
    ) { sectionName = section; }

    /**Get the PConfig section name for options.
      */
    const PString & GetSectionName() const { return sectionName; }

    /**Set the prefix for option negation.
       The default is "no-".
      */
    void SetNegationPrefix(
      const PString & prefix ///< New prefix string 
    ) { negationPrefix = prefix; }

    /**Get the prefix for option negation.
       The default is "no-".
      */
    const PString & GetNegationPrefix() const { return negationPrefix; }
  //@}


  protected:
    PString CharToString(char ch) const;
    PConfig config;
    PString sectionName;
    PString negationPrefix;
};

#endif // P_CONFIG_FILE


#endif // PTLIB_ARGLIST_H


// End Of File ///////////////////////////////////////////////////////////////
