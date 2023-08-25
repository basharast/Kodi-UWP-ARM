/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "AddonString.h"
#include "Alternative.h"
#include "ListItem.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"

#include <string>
#include <vector>

namespace XBMCAddon
{
namespace xbmcgui
{
constexpr int INPUT_ALPHANUM{0};
constexpr int INPUT_NUMERIC{1};
constexpr int INPUT_DATE{2};
constexpr int INPUT_TIME{3};
constexpr int INPUT_IPADDRESS{4};
constexpr int INPUT_PASSWORD{5};

constexpr int PASSWORD_VERIFY{1};
constexpr int ALPHANUM_HIDE_INPUT{2};

    ///
    /// \defgroup python_Dialog Dialog
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief **Kodi's dialog class**
    ///
    /// The graphical control element dialog box (also called dialogue box or
    /// just dialog) is a small window that communicates information to the user
    /// and prompts them for a response.
    ///
    class Dialog : public AddonClass
    {
    public:

      inline Dialog() = default;
      ~Dialog() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().yesno(heading, message, [nolabel, yeslabel, autoclose]) }
      /// **Yes / no dialog**
      ///
      /// The Yes / No dialog can be used to inform the user about questions and
      /// get the answer.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param message        string or unicode - message text.
      /// @param nolabel        [opt] label to put on the no button.
      /// @param yeslabel       [opt] label to put on the yes button.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      /// @return Returns True if 'Yes' was pressed, else False.
      ///
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v13 Added new option **autoclose**.
      /// @python_v19 Renamed option **line1** to **message**.
      /// @python_v19 Removed option **line2**.
      /// @python_v19 Removed option **line3**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.yesno('Kodi', 'Do you want to exit this script?')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      yesno(...);
#else
      bool yesno(const String& heading, const String& message,
                 const String& nolabel = emptyString,
                 const String& yeslabel = emptyString,
                 int autoclose = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().yesnocustom(heading, message, customlabel, [nolabel, yeslabel, autoclose]) }
      /// **Yes / no / custom dialog**
      ///
      /// The YesNoCustom dialog can be used to inform the user about questions and
      /// get the answer. The dialog provides a third button appart from yes and no.
      /// Button labels are fully customizable.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param message        string or unicode - message text.
      /// @param customlabel    string or unicode - label to put on the custom button.
      /// @param nolabel        [opt] label to put on the no button.
      /// @param yeslabel       [opt] label to put on the yes button.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      /// @return Returns the integer value for the selected button (-1:cancelled, 0:no, 1:yes, 2:custom)
      ///
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.yesnocustom('Kodi', 'Question?', 'Maybe')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      yesnocustom(...);
#else
      int yesnocustom(const String& heading,
                      const String& message,
                      const String& customlabel,
                      const String& nolabel = emptyString,
                      const String& yeslabel = emptyString,
                      int autoclose = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().info(listitem) }
      /// **Info dialog**
      ///
      /// Show the corresponding info dialog for a given listitem
      ///
      /// @param listitem       xbmcgui.ListItem - ListItem to show info for.
      /// @return Returns whether the dialog opened successfully.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.info(listitem)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      info(...);
#else
      bool info(const ListItem* item);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().select(heading, list[, autoclose, preselect, useDetails]) }
      /// **Select dialog**
      ///
      /// Show of a dialog to select of an entry as a key
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param list           list of strings / xbmcgui.ListItems - list of items shown in dialog.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      /// @param preselect      [opt] integer - index of preselected item. (default=no preselected item)
      /// @param useDetails     [opt] bool - use detailed list instead of a compact list. (default=false)
      /// @return Returns the position of the highlighted item as an integer.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v17 **preselect** option added.
      /// @python_v17 Added new option **useDetails**.
      /// @python_v17 Allow listitems for parameter **list**
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      select(...);
#else
      int select(const String& heading, const std::vector<Alternative<String, const ListItem* > > & list, int autoclose=0, int preselect=-1, bool useDetails=false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().contextmenu(list) }
      /// Show a context menu.
      ///
      /// @param list           string list - list of items.
      /// @return               the position of the highlighted item as an integer
      ///                       (-1 if cancelled).
      ///
      ///
      ///--------------------------------------------------------------------------
      /// @python_v17 New function added
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.contextmenu(['Option #1', 'Option #2', 'Option #3'])
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      contextmenu(...);
#else
      int contextmenu(const std::vector<String>& list);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().multiselect(heading, options[, autoclose, preselect, useDetails]) }
      /// Show a multi-select dialog.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param options        list of strings / xbmcgui.ListItems - options to choose from.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog.
      ///                       (default=do not autoclose)
      /// @param preselect      [opt] list of int - indexes of items to preselect
      ///                       in list (default: do not preselect any item)
      /// @param useDetails     [opt] bool - use detailed list instead of a compact list. (default=false)
      /// @return               Returns the selected items as a list of indices,
      ///                       or None if cancelled.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v16 New function added.
      /// @python_v17 Added new option **preselect**.
      /// @python_v17 Added new option **useDetails**.
      /// @python_v17 Allow listitems for parameter **options**
      ///
      /// **Example:**
      /// @code{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.multiselect("Choose something", ["Foo", "Bar", "Baz"], preselect=[1,2])
      /// ..
      /// @endcode
      ///
      multiselect(...);
#else
      std::unique_ptr<std::vector<int> > multiselect(const String& heading, const std::vector<Alternative<String, const ListItem* > > & options, int autoclose=0, const std::vector<int>& preselect = std::vector<int>(), bool useDetails=false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().ok(heading, message) }
      /// **OK dialog**
      ///
      /// The functions permit the call of a dialog of information, a
      /// confirmation of the user by press from OK required.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param message        string or unicode - message text.
      /// @return Returns True if 'Ok' was pressed, else False.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v19 Renamed option **line1** to **message**.
      /// @python_v19 Removed option **line2**.
      /// @python_v19 Removed option **line3**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ok = dialog.ok('Kodi', 'There was an error.')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      ok(...);
#else
      bool ok(const String& heading, const String& message);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().textviewer(heading, text, usemono) }
      /// **TextViewer dialog**
      ///
      /// The text viewer dialog can be used to display descriptions, help texts
      /// or other larger texts.
      ///
      /// @param heading       string or unicode - dialog heading.
      /// @param text          string or unicode - text.
      /// @param usemono       [opt] bool - use monospace font
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v16 New function added.
      /// @python_v18 New optional param added **usemono**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// dialog.textviewer('Plot', 'Some movie plot.')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      textviewer(...);
#else
      void textviewer(const String& heading, const String& text, bool usemono = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browse(type, heading, shares[, mask, useThumbs, treatAsFolder, defaultt, enableMultiple]) }
      /// **Browser dialog**
      ///
      /// The function offer the possibility to select a file by the user of
      /// the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   0   | ShowAndGetDirectory             |
      /// |   1   | ShowAndGetFile                  |
      /// |   2   | ShowAndGetImage                 |
      /// |   3   | ShowAndGetWriteableDirectory    |
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml)
      /// | Param          | Name                                         |
      /// |:--------------:|:---------------------------------------------|
      /// |   "programs"   | list program addons
      /// |   "video"      | list video sources
      /// |   "music"      | list music sources
      /// |   "pictures"   | list picture sources
      /// |   "files"      | list file sources (added through filemanager)
      /// |   "games"      | list game sources
      /// |   "local"      | list local drives
      /// |   ""           | list local drives and network shares
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist.
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders.
      /// @param defaultt       [opt] string - default path or file.
      /// @param enableMultiple [opt] boolean - if True multiple file selection is enabled.
      ///
      /// @return If enableMultiple is False (default): returns filename and/or path as a string
      ///        to the location of the highlighted item, if user pressed 'Ok' or a masked item
      ///        was selected. Returns the default value if dialog was canceled.
      ///        If enableMultiple is True: returns tuple of marked filenames as a string
      ///        if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.\n\n
      ///        If type is 0 or 3 the enableMultiple parameter is ignore
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New option added to browse network and/or local drives.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browse(3, 'Kodi', 'files', '', False, False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browse(...);
#else
      Alternative<String, std::vector<String> > browse(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false,
                          bool treatAsFolder = false, const String& defaultt = emptyString,
                          bool enableMultiple = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browseSingle(type, heading, shares[, mask, useThumbs, treatAsFolder, defaultt]) }
      /// **Browse single dialog**
      ///
      /// The function offer the possibility to select a file by the user of
      /// the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   0   | ShowAndGetDirectory
      /// |   1   | ShowAndGetFile
      /// |   2   | ShowAndGetImage
      /// |   3   | ShowAndGetWriteableDirectory
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml)
      /// | Param          | Name                                         |
      /// |:--------------:|:---------------------------------------------|
      /// |   "programs"   | list program addons
      /// |   "video"      | list video sources
      /// |   "music"      | list music sources
      /// |   "pictures"   | list picture sources
      /// |   "files"      | list file sources (added through filemanager)
      /// |   "games"      | list game sources
      /// |   "local"      | list local drives
      /// |   ""           | list local drives and network shares
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders (default=false).
      /// @param defaultt       [opt] string - default path or file.
      ///
      /// @return Returns filename and/or path as a string to the location of the highlighted item,
      ///        if user pressed 'Ok' or a masked item was selected.
      ///        Returns the default value if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New option added to browse network and/or local drives.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browseSingle(3, 'Kodi', 'files', '', False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browseSingle(...);
#else
      String browseSingle(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false,
                          bool treatAsFolder = false,
                          const String& defaultt = emptyString );
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browseMultiple(type, heading, shares[, mask, useThumbs, treatAsFolder, defaultt]) }
      /// **Browser dialog**
      ///
      /// The function offer the possibility to select multiple files by the
      /// user of the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   1   | ShowAndGetFile
      /// |   2   | ShowAndGetImage
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml)
      /// | Param          | Name                                         |
      /// |:--------------:|:---------------------------------------------|
      /// |   "programs"   | list program addons
      /// |   "video"      | list video sources
      /// |   "music"      | list music sources
      /// |   "pictures"   | list picture sources
      /// |   "files"      | list file sources (added through filemanager)
      /// |   "games"      | list game sources
      /// |   "local"      | list local drives
      /// |   ""           | list local drives and network shares
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders (default=false).
      /// @param defaultt       [opt] string - default path or file.
      /// @return Returns tuple of marked filenames as a string,"
      ///       if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v18 New option added to browse network and/or local drives.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browseMultiple(2, 'Kodi', 'files', '', False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browseMultiple(...);
#else
      std::vector<String> browseMultiple(int type, const String& heading, const String& shares,
                                         const String& mask = emptyString, bool useThumbs = false,
                                         bool treatAsFolder = false,
                                         const String& defaultt = emptyString );
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().numeric(type, heading[, defaultt, bHiddenInput]) }
      /// **Numeric dialog**
      ///
      /// The function have to be permitted by the user for the representation
      /// of a numeric keyboard around an input.
      ///
      /// @param type           integer - the type of numeric dialog.
      /// | Param | Name                     | Format                       |
      /// |:-----:|:-------------------------|:-----------------------------|
      /// |  0    | ShowAndGetNumber         | (default format: #)
      /// |  1    | ShowAndGetDate           | (default format: DD/MM/YYYY)
      /// |  2    | ShowAndGetTime           | (default format: HH:MM)
      /// |  3    | ShowAndGetIPAddress      | (default format: #.#.#.#)
      /// |  4    | ShowAndVerifyNewPassword | (default format: *)
      /// @param heading        string or unicode - dialog heading (will be ignored for type 4).
      /// @param defaultt       [opt] string - default value.
      /// @param bHiddenInput   [opt] bool - mask input (available for type 0).
      /// @return Returns the entered data as a string.
      ///         Returns the default value if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v19 New option added ShowAndVerifyNewPassword.
      /// @python_v19 Added new option **bHiddenInput**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// d = dialog.numeric(1, 'Enter date of birth')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      numeric(...);
#else
      String numeric(int type, const String& heading, const String& defaultt = emptyString, bool bHiddenInput = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().notification(heading, message[, icon, time, sound]) }
      /// Show a Notification alert.
      ///
      /// @param heading        string - dialog heading.
      /// @param message        string - dialog message.
      /// @param icon           [opt] string - icon to use. (default xbmcgui.NOTIFICATION_INFO)
      /// @param time           [opt] integer - time in milliseconds (default 5000)
      /// @param sound          [opt] bool - play notification sound (default True)
      ///
      /// Builtin Icons:
      ///   - xbmcgui.NOTIFICATION_INFO
      ///   - xbmcgui.NOTIFICATION_WARNING
      ///   - xbmcgui.NOTIFICATION_ERROR
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v13 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// dialog.notification('Movie Trailers', 'Finding Nemo download finished.', xbmcgui.NOTIFICATION_INFO, 5000)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      notification(...);
#else
      void notification(const String& heading, const String& message, const String& icon = emptyString, int time = 0, bool sound = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().input(heading[, defaultt, type, option, autoclose]) }
      /// Show an Input dialog.
      ///
      /// @param heading        string - dialog heading.
      /// @param defaultt       [opt] string - default value. (default=empty string)
      /// @param type           [opt] integer - the type of keyboard dialog. (default=xbmcgui.INPUT_ALPHANUM)
      /// | Parameter                        | Format                          |
      /// |---------------------------------:|:--------------------------------|
      /// | <tt>xbmcgui.INPUT_ALPHANUM</tt>  | (standard keyboard)
      /// | <tt>xbmcgui.INPUT_NUMERIC</tt>   | (format: #)
      /// | <tt>xbmcgui.INPUT_DATE</tt>      | (format: DD/MM/YYYY)
      /// | <tt>xbmcgui.INPUT_TIME</tt>      | (format: HH:MM)
      /// | <tt>xbmcgui.INPUT_IPADDRESS</tt> | (format: #.#.#.#)
      /// | <tt>xbmcgui.INPUT_PASSWORD</tt>  | (return md5 hash of input, input is masked)
      /// @param option         [opt] integer - option for the dialog. (see Options below)
      ///   - Password dialog:
      ///     - <tt>xbmcgui.PASSWORD_VERIFY</tt> (verifies an existing (default) md5 hashed password)
      ///   - Alphanum dialog:
      ///     - <tt>xbmcgui.ALPHANUM_HIDE_INPUT</tt> (masks input)
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      ///
      /// @return Returns the entered data as a string.
      ///         Returns an empty string if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v13 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// d = dialog.input('Enter secret code', type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      input(...);
#else
      String input(const String& heading,
                   const String& defaultt = emptyString,
                   int type = INPUT_ALPHANUM,
                   int option = 0,
                   int autoclose = 0);
#endif

    private:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      // used by both yesno() and yesnocustom()
      int yesNoCustomInternal(const String& heading,
                              const String& message,
                              const String& nolabel,
                              const String& yeslabel,
                              const String& customlabel,
                              int autoclose);
#endif
    };
    //@}

    ///
    /// \defgroup python_DialogProgress DialogProgress
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Kodi's progress dialog class (Duh!)</b>
    ///
    ///
    class DialogProgress : public AddonClass
    {
      CGUIDialogProgress* dlg = nullptr;
      bool                open = false;

    protected:
      void deallocating() override;

    public:

      DialogProgress() = default;
      ~DialogProgress() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().create(heading[, message]) }
      /// Create and show a progress dialog.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param message        [opt] string or unicode - message text.
      ///
      /// @note Use update() to update lines and progressbar.
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v19 Renamed option **line1** to **message**.
      /// @python_v19 Removed option **line2**.
      /// @python_v19 Removed option **line3**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog = xbmcgui.DialogProgress()
      /// pDialog.create('Kodi', 'Initializing script...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      create(...);
#else
      void create(const String& heading, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().update(percent[, message]) }
      /// Updates the progress dialog.
      ///
      /// @param percent        integer - percent complete. (0:100)
      /// @param message        [opt] string or unicode - message text.
      ///
      ///
      ///
      ///------------------------------------------------------------------------
      /// @python_v19 Renamed option **line1** to **message**.
      /// @python_v19 Removed option **line2**.
      /// @python_v19 Removed option **line3**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.update(25, 'Importing modules...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      update(...);
#else
      void update(int percent, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().close() }
      /// Close the progress dialog.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close(...);
#else
      void close();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().iscanceled() }
      /// Checks progress is canceled.
      ///
      /// @return True if the user pressed cancel.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// if (pDialog.iscanceled()): return
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      iscanceled(...);
#else
      bool iscanceled();
#endif
    };

    //@}

    ///
    /// \defgroup python_DialogProgressBG DialogProgressBG
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Kodi's background progress dialog class</b>
    ///
    ///
    class DialogProgressBG : public AddonClass
    {
      CGUIDialogExtendedProgressBar* dlg = nullptr;
      CGUIDialogProgressBarHandle* handle = nullptr;
      bool open = false;

    protected:
      void deallocating() override;

    public:

      DialogProgressBG() = default;
      ~DialogProgressBG() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().create(heading[, message]) }
      /// Create and show a background progress dialog.
      ///
      /// @param heading     string or unicode - dialog heading.
      /// @param message     [opt] string or unicode - message text.
      ///
      /// @note 'heading' is used for the dialog's id. Use a unique heading.
      ///        Use  update() to update heading, message and progressbar.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog = xbmcgui.DialogProgressBG()
      /// pDialog.create('Movie Trailers', 'Downloading Monsters Inc... .')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      create(...);
#else
      void create(const String& heading, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().update([percent, heading, message]) }
      /// Updates the background progress dialog.
      ///
      /// @param percent     [opt] integer - percent complete. (0:100)
      /// @param heading     [opt] string or unicode - dialog heading.
      /// @param message     [opt] string or unicode - message text.
      ///
      /// @note To clear heading or message, you must pass a blank character.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.update(25, message='Downloading Finding Nemo ...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      update(...);
#else
      void update(int percent = 0, const String& heading = emptyString, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().close() }
      /// Close the background progress dialog
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close(...);
#else
      void close();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().isFinished() }
      /// Checks progress is finished
      ///
      /// @return True if the background dialog is active.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// if (pDialog.isFinished()): return
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      isFinished(...);
#else
      bool isFinished();
#endif
    };
    //@}

} // namespace xbmcgui
} // namespace XBMCAddon
