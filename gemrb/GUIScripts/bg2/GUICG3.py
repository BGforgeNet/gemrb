# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation, alignment (GUICG3)
import GemRB
import GUICommon
import CommonTables
from ie_stats import *
from GUIDefines import *

import CharGenCommon

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global MyChar
	
	MyChar = GemRB.GetVar ("Slot")
	Kit = GUICommon.GetKitIndex (MyChar)
	if Kit == 0:
		KitName = GUICommon.GetClassRowName (MyChar)
	else:
		#rowname is just a number, first value row what we need here
		KitName = CommonTables.KitList.GetValue(Kit, 0)

	AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	AlignmentWindow = GemRB.LoadWindow(3, "GUICG")
	CharGenCommon.PositionCharGenWin(AlignmentWindow)

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText (CommonTables.Aligns.GetValue (i,0))

		if AlignmentOk.GetValue(KitName, CommonTables.Aligns.GetValue (i, 4)) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, AlignmentPress)
			Button.SetVarAssoc("Alignment", i)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DoneButton.SetDisabled(True)
	AlignmentWindow.Focus()
	return

def AlignmentPress(btn, Alignment):
	TextAreaControl.SetText (CommonTables.Aligns.GetValue (Alignment, 1))
	DoneButton.SetDisabled(False)
	AlignmentWindow.SetVar ("Alignment", CommonTables.Aligns.GetValue (Alignment, 3))
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetNextScript("CharGen4")
	return

def NextPress():
	# save previous stats:
	#       alignment
	#       reputation
	#       alignment abilities
	Alignment = AlignmentWindow.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment)

	# use the alignment to apply starting reputation
	RepTable = GemRB.LoadTable ("repstart")
	AlignmentAbbrev = CommonTables.Aligns.FindValue (3, Alignment)
	Rep = RepTable.GetValue (AlignmentAbbrev, 0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, Rep)

	# set the party rep if this in the main char
	if MyChar == 1:
		GemRB.GameSetReputation (Rep)

	GemRB.SetNextScript("CharGen5") #appearance
	AlignmentWindow.Close()
	return
