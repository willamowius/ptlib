/*
 * html.cxx
 *
 * HTML classes.
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

#ifdef __GNUC__
#pragma implementation "html.h"
#endif

#include <ptlib.h>
#include <ptclib/html.h>


//////////////////////////////////////////////////////////////////////////////
// PHTML

PHTML::PHTML(ElementInSet initialState)
{
  memset(elementSet, 0, sizeof(elementSet));
  tableNestLevel = 0;
  initialElement = initialState;
  switch (initialState) {
    case NumElementsInSet :
      break;
    case InBody :
      Set(InBody);
      break;
    case InForm :
      Set(InBody);
      Set(InForm);
      break;
    default :
      PAssertAlways(PInvalidParameter);
  }
}


PHTML::PHTML(const char * cstr)
{
  memset(elementSet, 0, sizeof(elementSet));
  tableNestLevel = 0;
  initialElement = NumElementsInSet;
  ostream & this_stream = *this;
  this_stream << Title(cstr) << Body() << Heading(1) << cstr << Heading(1);
}


PHTML::PHTML(const PString & str)
{
  memset(elementSet, 0, sizeof(elementSet));
  tableNestLevel = 0;
  initialElement = NumElementsInSet;
  ostream & this_stream = *this;
  this_stream << Title(str) << Body() << Heading(1) << str << Heading(1);
}


PHTML::~PHTML()
{
#ifndef NDEBUG
  if (initialElement != NumElementsInSet) {
    Clr(initialElement);
    Clr(InBody);
  }
  for (PINDEX i = 0; i < PARRAYSIZE(elementSet); i++)
    PAssert(elementSet[i] == 0, psprintf("Failed to close element %u", i));
#endif
}


void PHTML::AssignContents(const PContainer & cont)
{
  PStringStream::AssignContents(cont);
  memset(elementSet, 0, sizeof(elementSet));
}


PBoolean PHTML::Is(ElementInSet elmt) const
{
  return (elementSet[elmt>>3]&(1<<(elmt&7))) != 0;
}


void PHTML::Set(ElementInSet elmt)
{
  elementSet[elmt>>3] |= (1<<(elmt&7));
}


void PHTML::Clr(ElementInSet elmt)
{
  elementSet[elmt>>3] &= ~(1<<(elmt&7));
}


void PHTML::Toggle(ElementInSet elmt)
{
  elementSet[elmt>>3] ^= (1<<(elmt&7));
}


void PHTML::Element::Output(PHTML & html) const
{
  PAssert(reqElement == NumElementsInSet || html.Is(reqElement),
                                                "HTML element out of context");

  if (crlf == BothCRLF || (crlf == OpenCRLF && !html.Is(inElement)))
    html << "\r\n";

  html << '<';
  if (html.Is(inElement))
    html << '/';
  html << name;

  AddAttr(html);

  if (attr != NULL)
    html << ' ' << attr;

  html << '>';
  if (crlf == BothCRLF || (crlf == CloseCRLF && html.Is(inElement)))
    html << "\r\n";

  if (inElement != NumElementsInSet)
    html.Toggle(inElement);
}


void PHTML::Element::AddAttr(PHTML &) const
{
}


PHTML::HTML::HTML(const char * attr)
  : Element("HTML", attr, InHTML, NumElementsInSet, BothCRLF)
{
}

PHTML::Head::Head()
  : Element("HEAD", NULL, InHead, NumElementsInSet, BothCRLF)
{
}

void PHTML::Head::Output(PHTML & html) const
{
  PAssert(!html.Is(InBody), "HTML element out of context");
  if (!html.Is(InHTML))
    html << HTML();
  Element::Output(html);
}


PHTML::Body::Body(const char * attr)
  : Element("BODY", attr, InBody, NumElementsInSet, BothCRLF)
{
}


void PHTML::Body::Output(PHTML & html) const
{
  if (!html.Is(InHTML))
    html << HTML();
  if (html.Is(InTitle))
    html << Title();
  if (html.Is(InHead))
    html << Head();
  Element::Output(html);
  if (!html.Is(InBody))
    html << HTML();
}


PHTML::Title::Title()
  : Element("TITLE", NULL, InTitle, InHead, CloseCRLF)
{
  titleString = NULL;
}

PHTML::Title::Title(const char * titleCStr)
  : Element("TITLE", NULL, InTitle, InHead, CloseCRLF)
{
  titleString = titleCStr;
}

PHTML::Title::Title(const PString & titleStr)
  : Element("TITLE", NULL, InTitle, InHead, CloseCRLF)
{
  titleString = titleStr;
}

void PHTML::Title::Output(PHTML & html) const
{
  PAssert(!html.Is(InBody), "HTML element out of context");
  if (!html.Is(InHead))
    html << Head();
  if (html.Is(InTitle)) {
    if (titleString != NULL)
      html << titleString;
    Element::Output(html);
  }
  else {
    Element::Output(html);
    if (titleString != NULL) {
      html << titleString;
      Element::Output(html);
    }
  }
}


PHTML::Banner::Banner(const char * attr)
  : Element("BANNER", attr, NumElementsInSet, InBody, BothCRLF)
{
}


PHTML::Division::Division(const char * attr)
  : Element("DIV", attr, InDivision, InBody, BothCRLF)
{
}


PHTML::Heading::Heading(int number,
                        int sequence,
                        int skip,
                        const char * attr)
  : Element("H", attr, InHeading, InBody, CloseCRLF)
{
  num = number;
  srcString = NULL;
  seqNum = sequence;
  skipSeq = skip;
}

PHTML::Heading::Heading(int number,
                        const char * image,
                        int sequence,
                        int skip,
                        const char * attr)
  : Element("H", attr, InHeading, InBody, CloseCRLF)
{
  num = number;
  srcString = image;
  seqNum = sequence;
  skipSeq = skip;
}

PHTML::Heading::Heading(int number,
                        const PString & imageStr,
                        int sequence,
                        int skip,
                        const char * attr)
  : Element("H", attr, InHeading, InBody, CloseCRLF)
{
  num = number;
  srcString = imageStr;
  seqNum = sequence;
  skipSeq = skip;
}

void PHTML::Heading::AddAttr(PHTML & html) const
{
  PAssert(num >= 1 && num <= 6, "Bad heading number");
  html << num;
  if (srcString != NULL)
    html << " SRC=\"" << srcString << '"';
  if (seqNum > 0)
    html << " SEQNUM=" << seqNum;
  if (skipSeq > 0)
    html << " SKIP=" << skipSeq;
}


PHTML::BreakLine::BreakLine(const char * attr)
  : Element("BR", attr, NumElementsInSet, InBody, CloseCRLF)
{
}


PHTML::Paragraph::Paragraph(const char * attr)
  : Element("P", attr, NumElementsInSet, InBody, OpenCRLF)
{
}


PHTML::PreFormat::PreFormat(int widthInChars, const char * attr)
  : Element("PRE", attr, InPreFormat, InBody, CloseCRLF)
{
  width = widthInChars;
}


void PHTML::PreFormat::AddAttr(PHTML & html) const
{
  if (width > 0)
    html << " WIDTH=" << width;
}


PHTML::HotLink::HotLink(const char * href, const char * attr)
  : Element("A", attr, InAnchor, InBody, NoCRLF)
{
  hrefString = href;
}

void PHTML::HotLink::AddAttr(PHTML & html) const
{
  if (hrefString != NULL && *hrefString != '\0')
    html << " HREF=\"" << hrefString << '"';
  else
    PAssert(html.Is(InAnchor), PInvalidParameter);
}


PHTML::Target::Target(const char * name, const char * attr)
  : Element("A", attr, NumElementsInSet, InBody, NoCRLF)
{
  nameString = name;
}

void PHTML::Target::AddAttr(PHTML & html) const
{
  if (nameString != NULL && *nameString != '\0')
    html << " NAME=\"" << nameString << '"';
}


PHTML::ImageElement::ImageElement(const char * n,
                                  const char * attr,
                                  ElementInSet elmt,
                                  ElementInSet req,
                                  OptionalCRLF c,
                                  const char * image)
  : Element(n, attr, elmt, req, c)
{
  srcString = image;
}


void PHTML::ImageElement::AddAttr(PHTML & html) const
{
  if (srcString != NULL)
    html << " SRC=\"" << srcString << '"';
}


PHTML::Image::Image(const char * src, int w, int h, const char * attr)
  : ImageElement("IMG", attr, NumElementsInSet, InBody, NoCRLF, src)
{
  altString = NULL;
  width = w;
  height = h;
}

PHTML::Image::Image(const char * src,
                    const char * alt,
                    int w, int h,
                    const char * attr)
  : ImageElement("IMG", attr, NumElementsInSet, InBody, NoCRLF, src)
{
  altString = alt;
  width = w;
  height = h;
}

void PHTML::Image::AddAttr(PHTML & html) const
{
  PAssert(srcString != NULL && *srcString != '\0', PInvalidParameter);
  if (altString != NULL)
    html << " ALT=\"" << altString << '"';
  if (width != 0)
    html << " WIDTH=" << width;
  if (height != 0)
    html << " HEIGHT=" << height;
  ImageElement::AddAttr(html);
}


PHTML::HRule::HRule(const char * image, const char * attr)
  : ImageElement("HR", attr, NumElementsInSet, InBody, BothCRLF, image)
{
}


PHTML::Note::Note(const char * image, const char * attr)
  : ImageElement("NOTE", attr, InNote, InBody, BothCRLF, image)
{
}


PHTML::Address::Address(const char * attr)
  : Element("ADDRESS", attr, InAddress, InBody, BothCRLF)
{
}


PHTML::BlockQuote::BlockQuote(const char * attr)
  : Element("BQ", attr, InBlockQuote, InBody, BothCRLF)
{
}


PHTML::Credit::Credit(const char * attr)
  : Element("CREDIT", attr, NumElementsInSet, InBlockQuote, OpenCRLF)
{
}

PHTML::SetTab::SetTab(const char * id, const char * attr)
  : Element("TAB", attr, NumElementsInSet, InBody, NoCRLF)
{
  ident = id;
}

void PHTML::SetTab::AddAttr(PHTML & html) const
{
  PAssert(ident != NULL && *ident != '\0', PInvalidParameter);
  html << " ID=" << ident;
}


PHTML::Tab::Tab(int indent, const char * attr)
  : Element("TAB", attr, NumElementsInSet, InBody, NoCRLF)
{
  ident = NULL;
  indentSize = indent;
}

PHTML::Tab::Tab(const char * id, const char * attr)
  : Element("TAB", attr, NumElementsInSet, InBody, NoCRLF)
{
  ident = id;
  indentSize = 0;
}

void PHTML::Tab::AddAttr(PHTML & html) const
{
  PAssert(indentSize!=0 || (ident!=NULL && *ident!='\0'), PInvalidParameter);
  if (indentSize > 0)
    html << " INDENT=" << indentSize;
  else
    html << " TO=" << ident;
}


PHTML::SimpleList::SimpleList(const char * attr)
  : Element("UL", attr, InList, InBody, BothCRLF)
{
}

void PHTML::SimpleList::AddAttr(PHTML & html) const
{
  html << " PLAIN";
}


PHTML::BulletList::BulletList(const char * attr)
  : Element("UL", attr, InList, InBody, BothCRLF)
{
}


PHTML::OrderedList::OrderedList(int seqNum, const char * attr)
  : Element("OL", attr, InList, InBody, BothCRLF)
{
  sequenceNum = seqNum;
}

void PHTML::OrderedList::AddAttr(PHTML & html) const
{
  if (sequenceNum > 0)
    html << " SEQNUM=" << sequenceNum;
  if (sequenceNum < 0)
    html << " CONTINUE";
}


PHTML::DefinitionList::DefinitionList(const char * attr)
  : Element("DL", attr, InList, InBody, BothCRLF)
{
}


PHTML::ListHeading::ListHeading(const char * attr)
  : Element("LH", attr, InListHeading, InList, CloseCRLF)
{
}

PHTML::ListItem::ListItem(int skip, const char * attr)
  : Element("LI", attr, NumElementsInSet, InList, OpenCRLF)
{
  skipSeq = skip;
}

void PHTML::ListItem::AddAttr(PHTML & html) const
{
  if (skipSeq > 0)
    html << " SKIP=" << skipSeq;
}


PHTML::DefinitionTerm::DefinitionTerm(const char * attr)
  : Element("DT", attr, NumElementsInSet, InList, NoCRLF)
{
}

void PHTML::DefinitionTerm::Output(PHTML & html) const
{
  PAssert(!html.Is(InDefinitionTerm), "HTML definition item missing");
  Element::Output(html);
  html.Set(InDefinitionTerm);
}


PHTML::DefinitionItem::DefinitionItem(const char * attr)
  : Element("DD", attr, NumElementsInSet, InList, NoCRLF)
{
}

void PHTML::DefinitionItem::Output(PHTML & html) const
{
  PAssert(html.Is(InDefinitionTerm), "HTML definition term missing");
  Element::Output(html);
  html.Clr(InDefinitionTerm);
}


PHTML::TableStart::TableStart(const char * attr)
  : Element("TABLE", attr, InTable, InBody, BothCRLF)
{
  borderFlag = PFalse;
}

PHTML::TableStart::TableStart(BorderCodes border, const char * attr)
  : Element("TABLE", attr, InTable, InBody, BothCRLF)
{
  borderFlag = border == Border;
}

void PHTML::TableStart::Output(PHTML & html) const
{
  if (html.tableNestLevel > 0)
    html.Clr(InTable);
  Element::Output(html);
}

void PHTML::TableStart::AddAttr(PHTML & html) const
{
  if (borderFlag)
    html << " BORDER";
  html.tableNestLevel++;
}


PHTML::TableEnd::TableEnd()
  : Element("TABLE", "", InTable, InBody, BothCRLF)
{
}

void PHTML::TableEnd::Output(PHTML & html) const
{
  PAssert(html.tableNestLevel > 0, "Table nesting error");
  Element::Output(html);
  html.tableNestLevel--;
  if (html.tableNestLevel > 0)
    html.Set(InTable);
}


PHTML::TableRow::TableRow(const char * attr)
  : Element("TR", attr, NumElementsInSet, InTable, OpenCRLF)
{
}


PHTML::TableHeader::TableHeader(const char * attr)
  : Element("TH", attr, NumElementsInSet, InTable, CloseCRLF)
{
}


PHTML::TableData::TableData(const char * attr)
  : Element("TD", attr, NumElementsInSet, InTable, NoCRLF)
{
}


PHTML::Form::Form(const char * method,
                  const char * action,
                  const char * mimeType,
                  const char * script)
  : Element("FORM", NULL, InForm, InBody, BothCRLF)
{
  methodString = method;
  actionString = action;
  mimeTypeString = mimeType;
  scriptString = script;
}

void PHTML::Form::AddAttr(PHTML & html) const
{
  if (methodString != NULL)
    html << " METHOD=" << methodString;
  if (actionString != NULL)
    html << " ACTION=\"" << actionString << '"';
  if (mimeTypeString != NULL)
    html << " ENCTYPE=\"" << mimeTypeString << '"';
  if (scriptString != NULL)
    html << " SCRIPT=\"" << scriptString << '"';
}


PHTML::FieldElement::FieldElement(const char * n,
                                  const char * attr,
                                  ElementInSet elmt,
                                  OptionalCRLF c,
                                  DisableCodes disabled)
  : Element(n, attr, elmt, InForm, c)
{
  disabledFlag = disabled == Disabled;
}

void PHTML::FieldElement::AddAttr(PHTML & html) const
{
  if (disabledFlag)
    html << " DISABLED";
}


PHTML::Select::Select(const char * fname, const char * attr)
  : FieldElement("SELECT", attr, InSelect, BothCRLF, Enabled)
{
  nameString = fname;
}

PHTML::Select::Select(const char * fname,
                      DisableCodes disabled,
                      const char * attr)
  : FieldElement("SELECT", attr, InSelect, BothCRLF, disabled)
{
  nameString = fname;
}

void PHTML::Select::AddAttr(PHTML & html) const
{
  if (!html.Is(InSelect)) {
    PAssert(nameString != NULL && *nameString != '\0', PInvalidParameter);
    html << " NAME=\"" << nameString << '"';
  }
  FieldElement::AddAttr(html);
}


PHTML::Option::Option(const char * attr)
  : FieldElement("OPTION", attr, NumElementsInSet, NoCRLF, Enabled)
{
  selectedFlag = PFalse;
}

PHTML::Option::Option(SelectionCodes select,
                      const char * attr)
  : FieldElement("OPTION", attr, NumElementsInSet, NoCRLF, Enabled)
{
  selectedFlag = select == Selected;
}

PHTML::Option::Option(DisableCodes disabled,
                      const char * attr)
  : FieldElement("OPTION", attr, NumElementsInSet, NoCRLF, disabled)
{
  selectedFlag = PFalse;
}

PHTML::Option::Option(SelectionCodes select,
                      DisableCodes disabled,
                      const char * attr)
  : FieldElement("OPTION", attr, NumElementsInSet, NoCRLF, disabled)
{
  selectedFlag = select == Selected;
}

void PHTML::Option::AddAttr(PHTML & html) const
{
  if (selectedFlag)
    html << " SELECTED";
  FieldElement::AddAttr(html);
}


PHTML::FormField::FormField(const char * n,
                            const char * attr,
                            ElementInSet elmt,
                            OptionalCRLF c,
                            DisableCodes disabled,
                            const char * fname)
  : FieldElement(n, attr, elmt, c, disabled)
{
  nameString = fname;
}

void PHTML::FormField::AddAttr(PHTML & html) const
{
  PAssert(nameString != NULL && *nameString != '\0', PInvalidParameter);
  html << " NAME=\"" << nameString << '"';
  FieldElement::AddAttr(html);
}


PHTML::TextArea::TextArea(const char * fname,
                          DisableCodes disabled,
                          const char * attr)
  : FormField("TEXTAREA", attr, InSelect, BothCRLF, disabled, fname)
{
  numRows = numCols = 0;
}

PHTML::TextArea::TextArea(const char * fname,
                          int rows, int cols,
                          DisableCodes disabled,
                          const char * attr)
  : FormField("TEXTAREA", attr, InSelect, BothCRLF, disabled, fname)
{
  numRows = rows;
  numCols = cols;
}

void PHTML::TextArea::AddAttr(PHTML & html) const
{
  if (numRows > 0)
    html << " ROWS=" << numRows;
  if (numCols > 0)
    html << " COLS=" << numCols;
  FormField::AddAttr(html);
}


PHTML::InputField::InputField(const char * type,
                              const char * fname,
                              DisableCodes disabled,
                              const char * attr)
  : FormField("INPUT", attr, NumElementsInSet, NoCRLF, disabled, fname)
{
  typeString = type;
}

void PHTML::InputField::AddAttr(PHTML & html) const
{
  PAssert(typeString != NULL && *typeString != '\0', PInvalidParameter);
  html << " TYPE=" << typeString;
  FormField::AddAttr(html);
}


PHTML::HiddenField::HiddenField(const char * fname,
                                const char * value,
                                const char * attr)
  : InputField("hidden", fname, Enabled, attr)
{
  valueString = value;
}

void PHTML::HiddenField::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  PAssert(valueString != NULL, PInvalidParameter);
  html << " VALUE=\"" << valueString << '"';
}


PHTML::InputText::InputText(const char * fname,
                            int size,
                            const char * init,
                            const char * attr)
  : InputField("text", fname, Enabled, attr)
{
  width = size;
  length = 0;
  value = init;
}

PHTML::InputText::InputText(const char * fname,
                            int size,
                            DisableCodes disabled,
                            const char * attr)
  : InputField("text", fname, disabled, attr)
{
  width = size;
  length = 0;
  value = NULL;
}

PHTML::InputText::InputText(const char * fname,
                            int size,
                            int maxLength,
                            DisableCodes disabled,
                            const char * attr)
  : InputField("text", fname, disabled, attr)
{
  width = size;
  length = maxLength;
  value = NULL;
}

PHTML::InputText::InputText(const char * fname,
                            int size,
                            const char * init,
                            int maxLength,
                            DisableCodes disabled,
                            const char * attr)
  : InputField("text", fname, disabled, attr)
{
  width = size;
  length = maxLength;
  value = init;
}

PHTML::InputText::InputText(const char * type,
                            const char * fname,
                            int size,
                            const char * init,
                            int maxLength,
                            DisableCodes disabled,
                            const char * attr)
  : InputField(type, fname, disabled, attr)
{
  width = size;
  length = maxLength;
  value = init;
}

void PHTML::InputText::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  html << " SIZE=" << width;
  if (length > 0)
    html << " MAXLENGTH=" << length;
  if (value != NULL)
    html << " VALUE=\"" << value << '"';
}


PHTML::InputPassword::InputPassword(const char * fname,
                                    int size,
                                    const char * init,
                                    const char * attr)
  : InputText("password", fname, size, init, 0, Enabled, attr)
{
}

PHTML::InputPassword::InputPassword(const char * fname,
                                    int size,
                                    DisableCodes disabled,
                                    const char * attr)
  : InputText("password", fname, size, NULL, 0, disabled, attr)
{
}

PHTML::InputPassword::InputPassword(const char * fname,
                                    int size,
                                    int maxLength,
                                    DisableCodes disabled,
                                    const char * attr)
  : InputText("password", fname, size, NULL, maxLength, disabled, attr)
{
}

PHTML::InputPassword::InputPassword(const char * fname,
                                    int size,
                                    const char * init,
                                    int maxLength,
                                    DisableCodes disabled,
                                    const char * attr)
  : InputText("password", fname, size, init, maxLength, disabled, attr)
{
}


PHTML::RadioButton::RadioButton(const char * fname,
                                const char * value,
                                const char * attr)
  : InputField("radio", fname, Enabled, attr)
{
  valueString = value;
  checkedFlag = PFalse;
}

PHTML::RadioButton::RadioButton(const char * fname,
                                const char * value,
                                DisableCodes disabled,
                                const char * attr)
  : InputField("radio", fname, disabled, attr)
{
  valueString = value;
  checkedFlag = PFalse;
}

PHTML::RadioButton::RadioButton(const char * fname,
                                const char * value,
                                CheckedCodes check,
                                DisableCodes disabled,
                                const char * attr)
  : InputField("radio", fname, disabled, attr)
{
  valueString = value;
  checkedFlag = check == Checked;
}

PHTML::RadioButton::RadioButton(const char * type,
                                const char * fname,
                                const char * value,
                                CheckedCodes check,
                                DisableCodes disabled,
                                const char * attr)
  : InputField(type, fname, disabled, attr)
{
  valueString = value;
  checkedFlag = check == Checked;
}

void PHTML::RadioButton::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  PAssert(valueString != NULL, PInvalidParameter);
  html << " VALUE=\"" << valueString << "\"";
  if (checkedFlag)
    html << " CHECKED";
}


PHTML::CheckBox::CheckBox(const char * fname, const char * attr)
  : RadioButton("checkbox", fname, "true", UnChecked, Enabled, attr)
{
}

PHTML::CheckBox::CheckBox(const char * fname,
                          DisableCodes disabled,
                          const char * attr)
  : RadioButton("checkbox", fname, "true", UnChecked, disabled, attr)
{
}

PHTML::CheckBox::CheckBox(const char * fname,
                          CheckedCodes check,
                          DisableCodes disabled,
                          const char * attr)
  : RadioButton("checkbox", fname, "true", check, disabled, attr)
{
}


PHTML::InputNumber::InputNumber(const char * fname,
                                int min, int max, int value,
                                DisableCodes disabled,
                                const char * attr)
  : InputField("number", fname, disabled, attr)
{
  Construct(min, max, value);
}

PHTML::InputNumber::InputNumber(const char * type,
                                const char * fname,
                                int min, int max,
                                int value,
                                DisableCodes disabled,
                                const char * attr)
  : InputField(type, fname, disabled, attr)
{
  Construct(min, max, value);
}

void PHTML::InputNumber::Construct(int min, int max, int value)
{
  PAssert(min <= max, PInvalidParameter);
  minValue = min;
  maxValue = max;
  if (value < min)
    initValue = min;
  else if (value > max)
    initValue = max;
  else
    initValue = value;
}

void PHTML::InputNumber::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  PINDEX max = PMAX(-minValue, maxValue);
  PINDEX width = 3;
  while (max > 10) {
    width++;
    max /= 10;
  }
  html << " SIZE=" << width
       << " MIN=" << minValue
       << " MAX=" << maxValue
       << " VALUE=\"" << initValue << "\"";
}


PHTML::InputRange::InputRange(const char * fname,
                              int min, int max, int value,
                              DisableCodes disabled,
                              const char * attr)
  : InputNumber("range", fname, min, max, value, disabled, attr)
{
}


PHTML::InputFile::InputFile(const char * fname,
                            const char * accept,
                            DisableCodes disabled,
                            const char * attr)
  : InputField("file", fname, disabled, attr)
{
  acceptString = accept;
}

void PHTML::InputFile::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  if (acceptString != NULL)
    html << " ACCEPT=\"" << acceptString << '"';
}


PHTML::InputImage::InputImage(const char * fname,
                              const char * src,
                              DisableCodes disabled,
                              const char * attr)
  : InputField("image", fname, disabled, attr)
{
  srcString = src;
}

PHTML::InputImage::InputImage(const char * type,
                              const char * fname,
                              const char * src,
                              DisableCodes disabled,
                              const char * attr)
  : InputField(type, fname, disabled, attr)
{
  srcString = src;
}

void PHTML::InputImage::AddAttr(PHTML & html) const
{
  InputField::AddAttr(html);
  if (srcString != NULL)
    html << " SRC=\"" << srcString << '"';
}


PHTML::InputScribble::InputScribble(const char * fname,
                                    const char * src,
                                    DisableCodes disabled,
                                    const char * attr)
  : InputImage("scribble", fname, src, disabled, attr)
{
}

PHTML::ResetButton::ResetButton(const char * title,
                                const char * fname,
                                const char * src,
                                DisableCodes disabled,
                                const char * attr)
  : InputImage("reset", fname != NULL ? fname : "reset", src, disabled, attr)
{
  titleString = title;
}

PHTML::ResetButton::ResetButton(const char * type,
                                const char * title,
                                const char * fname,
                                const char * src,
                                DisableCodes disabled,
                                const char * attr)
  : InputImage(type, fname, src, disabled, attr)
{
  titleString = title;
}

void PHTML::ResetButton::AddAttr(PHTML & html) const
{
  InputImage::AddAttr(html);
  if (titleString != NULL)
    html << " VALUE=\"" << titleString << '"';
}


PHTML::SubmitButton::SubmitButton(const char * title,
                                  const char * fname,
                                  const char * src,
                                  DisableCodes disabled,
                                  const char * attr)
  : ResetButton("submit",
                  title, fname != NULL ? fname : "submit", src, disabled, attr)
{
}

// End Of File ///////////////////////////////////////////////////////////////
