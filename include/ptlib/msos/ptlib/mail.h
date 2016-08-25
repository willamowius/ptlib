/*
 * mail.h
 *
 * Electronic mail class.
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

///////////////////////////////////////////////////////////////////////////////
// PMail

  protected:
    DWORD    sessionId;
    DWORD    lastError;
    unsigned hUserInterface;

    PBoolean LogOnCommonInterface(const char * username,
                                  const char * password, const char * service);

#if P_HAS_CMC
    class CMCDLL : public PDynaLink
    {
      PCLASSINFO(CMCDLL, PDynaLink)
      public:
        CMCDLL();

        CMC_return_code (FAR PASCAL *logon)(
            CMC_string              service,
            CMC_string              user,
            CMC_string              password,
            CMC_enum                character_set,
            CMC_ui_id               ui_id,
            CMC_uint16              caller_cmc_version,
            CMC_flags               logon_flags,
            CMC_session_id FAR      *session,
            CMC_extension FAR       *logon_extensions
        );
        CMC_return_code (FAR PASCAL *logoff)(
            CMC_session_id          session,
            CMC_ui_id               ui_id,
            CMC_flags               logoff_flags,
            CMC_extension FAR       *logoff_extensions
        );
        CMC_return_code (FAR PASCAL *free_buf)(
            CMC_buffer              memory
        );
        CMC_return_code (FAR PASCAL *query_configuration)(
            CMC_session_id session,
            CMC_enum item,
            CMC_buffer                    reference,
            CMC_extension FAR *config_extensions
        );
        CMC_return_code (FAR PASCAL *look_up)(
            CMC_session_id          session,
            CMC_recipient FAR       *recipient_in,
            CMC_flags               look_up_flags,
            CMC_ui_id               ui_id,
            CMC_uint32 FAR          *count,
            CMC_recipient FAR * FAR *recipient_out,
            CMC_extension FAR       *look_up_extensions
        );
        CMC_return_code (FAR PASCAL *list)(
            CMC_session_id          session,
            CMC_string              message_type,
            CMC_flags               list_flags,
            CMC_message_reference   *seed,
            CMC_uint32 FAR          *count,
            CMC_ui_id               ui_id,
            CMC_message_summary FAR * FAR *result,
            CMC_extension FAR       *list_extensions
        );
        CMC_return_code (FAR PASCAL *send)(
            CMC_session_id          session,
            CMC_message FAR         *message,
            CMC_flags               send_flags,
            CMC_ui_id               ui_id,
            CMC_extension FAR       *send_extensions
        );
        CMC_return_code (FAR PASCAL *read)(
            CMC_session_id          session,
            CMC_message_reference   *message_reference,
            CMC_flags               read_flags,
            CMC_message FAR * FAR   *message,
            CMC_ui_id               ui_id,
            CMC_extension FAR       *read_extensions
        );
        CMC_return_code (FAR PASCAL *act_on)(
            CMC_session_id          session,
            CMC_message_reference   *message_reference,
            CMC_enum                operation,
            CMC_flags               act_on_flags,
            CMC_ui_id               ui_id,
            CMC_extension FAR       *act_on_extensions
        );
    };
    CMCDLL cmc;
#endif
#if P_HAS_MAPI
    class MAPIDLL : public PDynaLink
    {
      PCLASSINFO(MAPIDLL, PDynaLink)
      public:
        MAPIDLL();

        ULONG (FAR PASCAL *Logon)(HWND, LPCSTR, LPCSTR, FLAGS, ULONG, LPLHANDLE);
        ULONG (FAR PASCAL *Logoff)(LHANDLE, HWND, FLAGS, ULONG);
        ULONG (FAR PASCAL *SendMail)(LHANDLE, HWND, lpMapiMessage, FLAGS, ULONG);
        ULONG (FAR PASCAL *SendDocuments)(HWND, LPSTR, LPSTR, LPSTR, ULONG);
        ULONG (FAR PASCAL *FindNext)(LHANDLE, HWND, LPCSTR, LPCSTR, FLAGS, ULONG, LPSTR);
        ULONG (FAR PASCAL *ReadMail)(LHANDLE, HWND, LPCSTR, FLAGS, ULONG, lpMapiMessage FAR *);
        ULONG (FAR PASCAL *SaveMail)(LHANDLE, HWND, lpMapiMessage, FLAGS, ULONG, LPSTR);
        ULONG (FAR PASCAL *DeleteMail)(LHANDLE, HWND, LPCSTR, FLAGS, ULONG);
        ULONG (FAR PASCAL *FreeBuffer)(LPVOID);
        ULONG (FAR PASCAL *Address)(LHANDLE, HWND, LPSTR, ULONG, LPSTR, ULONG, lpMapiRecipDesc, FLAGS, ULONG, LPULONG, lpMapiRecipDesc FAR *);
        ULONG (FAR PASCAL *Details)(LHANDLE, HWND,lpMapiRecipDesc, FLAGS, ULONG);
        ULONG (FAR PASCAL *ResolveName)(LHANDLE, HWND, LPCSTR, FLAGS, ULONG, lpMapiRecipDesc FAR *);
    };
    MAPIDLL mapi;
#endif

// End Of File ///////////////////////////////////////////////////////////////
