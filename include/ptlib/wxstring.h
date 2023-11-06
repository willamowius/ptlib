/*
 * wxstring.h
 *
 * Adapter class for WX Widgets strings.
 *
 * Portable Tools Library
 *
 * Copyright (c) 2009 Vox Lucida
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

#ifndef PTLIB_WXSTRING_H
#define PTLIB_WXSTRING_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

/**This class defines a class to bridge WX Widgets strings to PTLib strings.
 */
class PwxString : public wxString
{
  public:
    PwxString() { }
    PwxString(const wxString & str) : wxString(str) { }
    PwxString(const PString & str) : wxString((const char *)str, wxConvUTF8 ) { }
    PwxString(const PFilePath & fn) : wxString((const char *)fn, wxConvUTF8 ) { }
    PwxString(const char * str) : wxString(str, wxConvUTF8) { }
#ifdef OPAL_OPAL_MEDIAFMT_H
    PwxString(const OpalMediaFormat & fmt) : wxString((const char *)fmt.GetName(), wxConvUTF8) { }
#endif
#if wxUSE_UNICODE
    PwxString(const wchar_t * wstr) : wxString(wstr) { }
#endif

    inline PwxString & operator=(const char * str)     { *this = wxString(str, wxConvUTF8); return *this; }
#if wxUSE_UNICODE
    inline PwxString & operator=(const wchar_t * wstr) { wxString::operator=(wstr); return *this; }
#endif
    inline PwxString & operator=(const wxString & str) { wxString::operator=(str); return *this; }
    inline PwxString & operator=(const PString & str)  { *this = wxString((const char *)str, wxConvUTF8); return *this; }

    inline bool operator==(const char * other)            const { return IsSameAs(wxString(other, wxConvUTF8)); }
#if wxUSE_UNICODE
    inline bool operator==(const wchar_t * other)         const { return IsSameAs(other); }
#endif
    inline bool operator==(const wxString & other)        const { return IsSameAs(other); }
    inline bool operator==(const PString & other)         const { return IsSameAs(wxString((const char *)other, wxConvUTF8)); }
    inline bool operator==(const PwxString & other)       const { return IsSameAs(other); }
#ifdef OPAL_OPAL_MEDIAFMT_H
    inline bool operator==(const OpalMediaFormat & other) const { return IsSameAs(wxString((const char *)other.GetName(), wxConvUTF8)); }
#endif

    inline bool operator!=(const char * other)            const { return !IsSameAs(wxString(other, wxConvUTF8)); }
#if wxUSE_UNICODE
    inline bool operator!=(const wchar_t * other)         const { return !IsSameAs(other); }
#endif
    inline bool operator!=(const wxString & other)        const { return !IsSameAs(other); }
    inline bool operator!=(const PString & other)         const { return !IsSameAs(wxString((const char *)other, wxConvUTF8)); }
    inline bool operator!=(const PwxString & other)       const { return !IsSameAs(other); }
#ifdef OPAL_OPAL_MEDIAFMT_H
    inline bool operator!=(const OpalMediaFormat & other) const { return !IsSameAs(wxString((const char *)other.GetName(), wxConvUTF8)); }
#endif

#if wxUSE_UNICODE
    inline PString p_str() const { return ToUTF8().data(); }
    inline operator PString() const { return ToUTF8().data(); }
    inline operator PFilePath() const { return ToUTF8().data(); }
#if defined(PTLIB_PURL_H) && defined(P_URL)
    inline operator PURL() const { return ToUTF8().data(); }
#endif
#if defined(PTLIB_IPSOCKET_H)
    inline operator PIPSocket::Address() const { return PString(ToUTF8().data()); }
#endif
    inline friend ostream & operator<<(ostream & stream, const PwxString & string) { return stream << string.ToUTF8(); }
    inline friend wostream & operator<<(wostream & stream, const PwxString & string) { return stream << string.c_str(); }
#else
    inline PString p_str() const { return c_str(); }
    inline operator PString() const { return c_str(); }
    inline operator PFilePath() const { return c_str(); }
#if defined(PTLIB_PURL_H) && defined(P_URL)
    inline operator PURL() const { return c_str(); }
#endif
#if defined(PTLIB_IPSOCKET_H)
    inline operator PIPSocket::Address() const { return c_str(); }
#endif
    inline friend ostream & operator<<(ostream & stream, const PwxString & string) { return stream << string.c_str(); }
    inline friend wostream & operator<<(wostream & stream, const PwxString & string) { return stream << string.c_str(); }
#endif
};

__inline bool wxFromString(wxString & s1, PwxString * & s2) { *s2 = s1; return true; }
__inline wxString wxToString(const PwxString & str) { return str; }

#endif // PTLIB_WXSTRING_H


// End Of File ///////////////////////////////////////////////////////////////
