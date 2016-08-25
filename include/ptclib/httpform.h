/*
 * httpform.h
 *
 * Forms management using HTTP User Interface.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef PTLIB_HTTPFORM_H
#define PTLIB_HTTPFORM_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#if P_HTTPFORMS

#include <ptclib/http.h>
#include <ptclib/html.h>


///////////////////////////////////////////////////////////////////////////////
// PHTTPField

/** This class is the abstract base class for fields in a <code>PHTTPForm</code>
   resource type.
 */
class PHTTPField : public PObject
{
  PCLASSINFO(PHTTPField, PObject)
  public:
    PHTTPField(
      const char * bname,  // base name (identifier) for the field.
      const char * title,  // Title text for field (defaults to name).
      const char * help    // Help text for the field.
    );
    // Create a new field in a HTTP form.

    /** Compare the fields using the field names.

       @return
       Comparison of the name fields of the two fields.
     */
    virtual Comparison Compare(
      const PObject & obj
    ) const;

    /** Get the identifier name of the field.

       @return
       String for field name.
     */
    const PCaselessString & GetName() const { return fullName; }

    /** Get the identifier name of the field.

       @return
       String for field name.
     */
    const PCaselessString & GetBaseName() const { return baseName; }

    /** Set the name for the field.
     */
    virtual void SetName(
      const PString & newName   // New name for field
    );

    /** Locate the field naem, recusing down for composite fields.

       @return
       Pointer to located field, or NULL if not found.
     */
    virtual const PHTTPField * LocateName(
      const PString & name    // Full field name to locate
    ) const;

    /** Get the title of the field.

       @return
       String for title placed next to the field.
     */
    const PString & GetTitle() const { return title; }

    /** Get the title of the field.

       @return
       String for title placed next to the field.
     */
    const PString & GetHelp() const { return help; }

    void SetHelp(
      const PString & text        // Help text.
    ) { help = text; }
    void SetHelp(
      const PString & hotLinkURL, // URL for link to help page.
      const PString & linkText    // Help text in the link.
    );
    void SetHelp(
      const PString & hotLinkURL, // URL for link to help page.
      const PString & imageURL,   // URL for image to be displayed in link.
      const PString & imageText   // Text in the link when image unavailable.
    );
    // Set the help text for the field.

    /** Create a new field of the same class as the current field.

       @return
       New field object instance.
     */
    virtual PHTTPField * NewField() const = 0;

    virtual void ExpandFieldNames(PString & text, PINDEX start, PINDEX & finish) const;
    // Splice expanded macro substitutions into text string

    /** Convert the field to HTML form tag for inclusion into the HTTP page.
     */
    virtual void GetHTMLTag(
      PHTML & html    // HTML to receive the fields HTML tag.
    ) const = 0;

    /** Convert the field input to HTML for inclusion into the HTTP page.
     */
    virtual PString GetHTMLInput(
      const PString & input // Source HTML text for input tag.
    ) const;

    /** Convert the field input to HTML for inclusion into the HTTP page.
     */
    virtual PString GetHTMLSelect(
      const PString & selection // Source HTML text for input tag.
    ) const;

    /** Convert the field to HTML for inclusion into the HTTP page.
     */
    virtual void GetHTMLHeading(
      PHTML & html    // HTML to receive the field info.
    ) const;

    /** Get the string value of the field.

       @return
       String for field value.
     */
    virtual PString GetValue(PBoolean dflt = false) const = 0;

    /** Set the value of the field.
     */
    virtual void SetValue(
      const PString & newValue   // New value for the field.
    ) = 0;

    /** Get the value of the PConfig to the sub-field. If the field is not
       composite then it always sets the value as for the non-indexed version.
     */
    virtual void LoadFromConfig(
      PConfig & cfg   // Configuration for value transfer.
    );

    /** Set the value of the sub-field into the PConfig. If the field is not
       composite then it always sets the value as for the non-indexed version.
     */
    virtual void SaveToConfig(
      PConfig & cfg   // Configuration for value transfer.
    ) const;

    /** Validate the new field value before <code>SetValue()</code> is called.

       @return
       PBoolean if the new field value is OK.
     */
    virtual PBoolean Validated(
      const PString & newVal, // Proposed new value for the field.
      PStringStream & msg     // Stream to take error HTML if value not valid.
    ) const;


    /** Retrieve all the names in the field and subfields.

       @return
       Array of strings for each subfield.
     */
    virtual void GetAllNames(PStringArray & names) const;

    /** Set the value of the field in a list of fields.
     */
    virtual void SetAllValues(
      const PStringToString & data   // New value for the field.
    );

    /** Validate the new field value in a list before <code>SetValue()</code> is called.

       @return
       PBoolean if the all the new field values are OK.
     */
    virtual PBoolean ValidateAll(
      const PStringToString & data, // Proposed new value for the field.
      PStringStream & msg     // Stream to take error HTML if value not valid.
    ) const;


    PBoolean NotYetInHTML() const { return notInHTML; }
    void SetInHTML() { notInHTML = false; }

  protected:
    PCaselessString baseName;
    PCaselessString fullName;
    PString title;
    PString help;
    PBoolean notInHTML;
};


PARRAY(PHTTPFields, PHTTPField);

class PHTTPCompositeField : public PHTTPField
{
  PCLASSINFO(PHTTPCompositeField, PHTTPField)
  public:
    PHTTPCompositeField(
      const char * name,          // Name (identifier) for the field.
      const char * title = NULL,  // Title text for field (defaults to name).
      const char * help = NULL,   // Help text for the field.
      bool includeHeaders = false // Make a sub-table and put headers on HTML fields.
    );

    virtual void SetName(
      const PString & name   // New name for field
    );

    virtual const PHTTPField * LocateName(
      const PString & name    // Full field name to locate
    ) const;

    virtual PHTTPField * NewField() const;

    virtual void ExpandFieldNames(PString & text, PINDEX start, PINDEX & finish) const;

    virtual void GetHTMLTag(
      PHTML & html    // HTML to receive the field info.
    ) const;

    virtual PString GetHTMLInput(
      const PString & input // Source HTML text for input tag.
    ) const;

    virtual void GetHTMLHeading(
      PHTML & html    // HTML to receive the field info.
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newValue   // New value for the field.
    );

    virtual void LoadFromConfig(
      PConfig & cfg   // Configuration for value transfer.
    );
    virtual void SaveToConfig(
      PConfig & cfg   // Configuration for value transfer.
    ) const;

    virtual void GetAllNames(PStringArray & names) const;
    virtual void SetAllValues(
      const PStringToString & data   // New value for the field.
    );

    virtual PBoolean ValidateAll(
      const PStringToString & data, // Proposed new value for the field.
      PStringStream & msg     // Stream to take error HTML if value not valid.
    ) const;


    /** Get the number of sub-fields in the composite field. Note that this is
       the total including any composite sub-fields, ie, it is the size of the
       whole tree of primitive fields.

       @return
       Returns field count.
     */
    virtual PINDEX GetSize() const;

    void Append(PHTTPField * fld);
    PHTTPField & operator[](PINDEX idx) const { return fields[idx]; }
    void RemoveAt(PINDEX idx) { fields.RemoveAt(idx); }
    void RemoveAll() { fields.RemoveAll(); }

  protected:
    PHTTPFields fields;
    bool        m_includeHeaders;
};


class PHTTPSubForm : public PHTTPCompositeField
{
  PCLASSINFO(PHTTPSubForm, PHTTPCompositeField)
  public:
    PHTTPSubForm(
      const PString & subFormName, // URL for the sub-form
      const char * name,           // Name (identifier) for the field.
      const char * title = NULL,   // Title text for field (defaults to name).
      PINDEX primaryField = 0,     // Pimary field whove value is in hot link
      PINDEX secondaryField = P_MAX_INDEX   // Seconary field next to hotlink
    );

  PHTTPField * NewField() const;
  void GetHTMLTag(PHTML & html) const;
  void GetHTMLHeading(PHTML & html) const;

  protected:
    PString subFormName;
    PINDEX primary;
    PINDEX secondary;
};


class PHTTPFieldArray : public PHTTPCompositeField
{
  PCLASSINFO(PHTTPFieldArray, PHTTPCompositeField)
  public:
    PHTTPFieldArray(
      PHTTPField * baseField,
      PBoolean ordered,
      PINDEX fixedSize = 0
    );

    ~PHTTPFieldArray();


    virtual PHTTPField * NewField() const;

    virtual void ExpandFieldNames(PString & text, PINDEX start, PINDEX & finish) const;

    virtual void GetHTMLTag(
      PHTML & html    // HTML to receive the field info.
    ) const;

    virtual void LoadFromConfig(
      PConfig & cfg   // Configuration for value transfer.
    );
    virtual void SaveToConfig(
      PConfig & cfg   // Configuration for value transfer.
    ) const;


    virtual void SetAllValues(
      const PStringToString & data   // New value for the field.
    );

    virtual PINDEX GetSize() const;
    void SetSize(PINDEX newSize);

    PStringArray GetStrings(
      PConfig & cfg   ///< Config file to get strings from
    );

    void SetStrings(
      PConfig & cfg,   ///< Config file to Set strings to
      const PStringArray & values ///< Strings to set
    );

  protected:
    void AddBlankField();
    void AddArrayControlBox(PHTML & html, PINDEX fld) const;
    void SetArrayFieldName(PINDEX idx) const;

    PHTTPField * baseField;
    PBoolean orderedArray;
    PBoolean canAddElements;
};


class PHTTPStringField : public PHTTPField
{
  PCLASSINFO(PHTTPStringField, PHTTPField)
  public:
    PHTTPStringField(
      const char * name,
      PINDEX size,
      const char * initVal = NULL,
      const char * help = NULL
    );
    PHTTPStringField(
      const char * name,
      const char * title,
      PINDEX size,
      const char * initVal = NULL,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );


  protected:
    PString value;
    PString initialValue;
    PINDEX size;
};


class PHTTPPasswordField : public PHTTPStringField
{
  PCLASSINFO(PHTTPPasswordField, PHTTPStringField)
  public:
    PHTTPPasswordField(
      const char * name,
      PINDEX size,
      const char * initVal = NULL,
      const char * help = NULL
    );
    PHTTPPasswordField(
      const char * name,
      const char * title,
      PINDEX size,
      const char * initVal = NULL,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );

    static PString Decrypt(const PString & pword);
};


class PHTTPDateField : public PHTTPStringField
{
  PCLASSINFO(PHTTPDateField, PHTTPStringField)
  public:
    PHTTPDateField(
      const char * name,
      const PTime & initVal = PTime(0),
      PTime::TimeFormat fmt = PTime::ShortDate
    );

    virtual PHTTPField * NewField() const;

    virtual void SetValue(
      const PString & newValue
    );

    virtual PBoolean Validated(
      const PString & newValue,
      PStringStream & msg
    ) const;

  protected:
    PTime::TimeFormat m_format;
};


class PHTTPIntegerField : public PHTTPField
{
  PCLASSINFO(PHTTPIntegerField, PHTTPField)
  public:
    PHTTPIntegerField(
      const char * name,
      int low, int high,
      int initVal = 0,
      const char * units = NULL,
      const char * help = NULL
    );
    PHTTPIntegerField(
      const char * name,
      const char * title,
      int low, int high,
      int initVal = 0,
      const char * units = NULL,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );

    virtual void LoadFromConfig(
      PConfig & cfg   ///< Configuration for value transfer.
    );
    virtual void SaveToConfig(
      PConfig & cfg   ///< Configuration for value transfer.
    ) const;

    virtual PBoolean Validated(
      const PString & newVal,
      PStringStream & msg
    ) const;


  protected:
    int low, high, value;
    int initialValue;
    PString units;
};


class PHTTPBooleanField : public PHTTPField
{
  PCLASSINFO(PHTTPBooleanField, PHTTPField)
  public:
    PHTTPBooleanField(
      const char * name,
      PBoolean initVal = false,
      const char * help = NULL
    );
    PHTTPBooleanField(
      const char * name,
      const char * title,
      PBoolean initVal = false,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetHTMLInput(
      const PString & input
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );

    virtual void LoadFromConfig(
      PConfig & cfg   ///< Configuration for value transfer.
    );
    virtual void SaveToConfig(
      PConfig & cfg   ///< Configuration for value transfer.
    ) const;


  protected:
    PBoolean value, initialValue;
};


class PHTTPRadioField : public PHTTPField
{
  PCLASSINFO(PHTTPRadioField, PHTTPField)
  public:
    PHTTPRadioField(
      const char * name,
      const PStringArray & valueArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      const PStringArray & valueArray,
      const PStringArray & titleArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      PINDEX count,
      const char * const * valueStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      PINDEX count,
      const char * const * valueStrings,
      const char * const * titleStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      const char * groupTitle,
      const PStringArray & valueArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      const char * groupTitle,
      const PStringArray & valueArray,
      const PStringArray & titleArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      const char * groupTitle,
      PINDEX count,
      const char * const * valueStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPRadioField(
      const char * name,
      const char * groupTitle,
      PINDEX count,
      const char * const * valueStrings,
      const char * const * titleStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetHTMLInput(
      const PString & input
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );


  protected:
    PStringArray values;
    PStringArray titles;
    PString value;
    PString initialValue;
};


class PHTTPSelectField : public PHTTPField
{
  PCLASSINFO(PHTTPSelectField, PHTTPField)
  public:
    PHTTPSelectField(
      const char * name,
      const PStringArray & valueArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPSelectField(
      const char * name,
      PINDEX count,
      const char * const * valueStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPSelectField(
      const char * name,
      const char * title,
      const PStringArray & valueArray,
      PINDEX initVal = 0,
      const char * help = NULL
    );
    PHTTPSelectField(
      const char * name,
      const char * title,
      PINDEX count,
      const char * const * valueStrings,
      PINDEX initVal = 0,
      const char * help = NULL
    );

    virtual PHTTPField * NewField() const;

    virtual void GetHTMLTag(
      PHTML & html    ///< HTML to receive the field info.
    ) const;

    virtual PString GetValue(PBoolean dflt = false) const;

    virtual void SetValue(
      const PString & newVal
    );


    PStringArray values;


  protected:
    PString value;
    PINDEX initialValue;
};


///////////////////////////////////////////////////////////////////////////////
// PHTTPForm

class PHTTPForm : public PHTTPString
{
  PCLASSINFO(PHTTPForm, PHTTPString)
  public:
    PHTTPForm(
      const PURL & url
    );
    PHTTPForm(
      const PURL & url,
      const PHTTPAuthority & auth
    );
    PHTTPForm(
      const PURL & url,
      const PString & html
    );
    PHTTPForm(
      const PURL & url,
      const PString & html,
      const PHTTPAuthority & auth
    );


    virtual void OnLoadedText(
      PHTTPRequest & request,    ///< Information on this request.
      PString & text             ///< Data used in reply.
    );
    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );


    PHTTPField * Add(
      PHTTPField * fld
    );
    void RemoveAllFields()
      { fields.RemoveAll(); fieldNames.RemoveAll(); }

    enum BuildOptions {
      CompleteHTML,
      InsertIntoForm,
      InsertIntoHTML
    };

    void BuildHTML(
      const char * heading
    );
    void BuildHTML(
      const PString & heading
    );
    void BuildHTML(
      PHTML & html,
      BuildOptions option = CompleteHTML
    );


  protected:
    PHTTPCompositeField fields;
    PStringSet fieldNames;
};


//////////////////////////////////////////////////////////////////////////////
// PHTTPConfig

class PHTTPConfig : public PHTTPForm
{
  PCLASSINFO(PHTTPConfig, PHTTPForm)
  public:
    PHTTPConfig(
      const PURL & url,
      const PString & section
    );
    PHTTPConfig(
      const PURL & url,
      const PString & section,
      const PHTTPAuthority & auth
    );
    PHTTPConfig(
      const PURL & url,
      const PString & section,
      const PString & html
    );
    PHTTPConfig(
      const PURL & url,
      const PString & section,
      const PString & html,
      const PHTTPAuthority & auth
    );

    virtual void OnLoadedText(
      PHTTPRequest & request,    ///< Information on this request.
      PString & text             ///< Data used in reply.
    );
    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );


    /** Load all of the values for the resource from the configuration.
     */
    void LoadFromConfig();

    /** Get the configuration file section that the page will alter.

       @return
       String for config file section.
     */
    const PString & GetConfigSection() const { return section; }

    void SetConfigSection(
      const PString & sect   ///< New section for the config page.
    ) { section = sect; }
    // Set the configuration file section.

    /** Add a field that will determine the name opf the secontion into which
       the other fields are to be added as keys. The section is not created and
       and error generated if the section already exists.
     */
    PHTTPField * AddSectionField(
      PHTTPField * sectionFld,     ///< Field to set as the section name
      const char * prefix = NULL,  ///< String to attach before the field value
      const char * suffix = NULL   ///< String to attach after the field value
    );

    /** Add fields to the HTTP form for adding a new key to the config file
       section.
     */
    void AddNewKeyFields(
      PHTTPField * keyFld,  ///< Field for the key to be added.
      PHTTPField * valFld   ///< Field for the value of the key yto be added.
    );


  protected:
    PString section;
    PString sectionPrefix;
    PString sectionSuffix;
    PHTTPField * sectionField;
    PHTTPField * keyField;
    PHTTPField * valField;

  private:
    void Construct();
};


//////////////////////////////////////////////////////////////////////////////
// PHTTPConfigSectionList

class PHTTPConfigSectionList : public PHTTPString
{
  PCLASSINFO(PHTTPConfigSectionList, PHTTPString)
  public:
    PHTTPConfigSectionList(
      const PURL & url,
      const PHTTPAuthority & auth,
      const PString & sectionPrefix,
      const PString & additionalValueName,
      const PURL & editSection,
      const PURL & newSection,
      const PString & newSectionTitle,
      PHTML & heading
    );

    virtual void OnLoadedText(
      PHTTPRequest & request,    ///< Information on this request.
      PString & text             ///< Data used in reply.
    );
    virtual PBoolean Post(
      PHTTPRequest & request,       ///< Information on this request.
      const PStringToString & data, ///< Variables in the POST data.
      PHTML & replyMessage          ///< Reply message for post.
    );

  protected:
    PString sectionPrefix;
    PString additionalValueName;
    PString newSectionLink;
    PString newSectionTitle;
    PString editSectionLink;
};


#endif // P_HTTPFORMS

#endif // PTLIB_HTTPFORM_H


// End Of File ///////////////////////////////////////////////////////////////
