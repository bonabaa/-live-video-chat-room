/*
 * Generic USB HID Plugin LID for OPAL
 *
 * Copyright (C) 2006 Post Increment, All Rights Reserved
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
 * The Original Code is OPAL.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 19258 $
 * $Author: rjongbloed $
 * $Date: 2008-01-15 07:29:51 +0000 (Tue, 15 Jan 2008) $
 */


/*
The values 0-9, * and # are the usual DTMF numbers. The other values are:
  S = Start - go off hook
  E = End - go on hook
  c = clear entry
  h = hold
  m = mute microphnoe
  + = volume up
  - = volume down
  u,d,l,r = navigation keys
  p = phonebook
  a = add

  Note the first 14 codes are mandatory and MUST be supported.
*/
static const struct KeyInfo {
  char code;
  bool mandatory;
  const char * description;
} Keys[] = {
  { 'S', true,  "Start or Off Hook" },
  { 'E', true,  "End or On Hook" },
  { '1', true,  "1" },
  { '2', true,  "2" },
  { '3', true,  "3" },
  { '4', true,  "4" },
  { '5', true,  "5" },
  { '6', true,  "6" },
  { '7', true,  "7" },
  { '8', true,  "8" },
  { '9', true,  "9" },
  { '0', true,  "0" },
  { '*', true,  "*" },
  { '#', true,  "#" },
  { 'c', false, "Clear or Cancel" },
  { 'h', false, "Hold" },
  { 'm', false, "Mute" },
  { '+', false, "Volume up" },
  { '-', false, "Volume down" },
  { 'u', false, "Navigate up" },
  { 'd', false, "Navigate down" },
  { 'l', false, "Navigate left" },
  { 'r', false, "Navigate right" },
  { 'p', false, "Phonebook" },
  { 'a', false, "Add" }
};

#define NumKeys (sizeof(Keys)/sizeof(Keys[0]))

#define IdSeperatorChar  '-'
#define IdSeperatorStr   "-"
#define DevSeperatorChar '\\'
#define DevSeperatorStr  "\\"

static const char SectionFmt[] = "%04x" IdSeperatorStr "%04x";
static const char Description[] = "Description";
static const char AudioDevice[] = "AudioDevice";


///////////////////////////////////////////////////////////////////////////////
