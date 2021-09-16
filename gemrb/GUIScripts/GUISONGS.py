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
#instead of credits, you can listen the songs of the game :)
import GemRB
import GameCheck

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0
MoviesTableName = ""
CreditsRef = ""
ColOffset = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable, MoviesTableName, ColOffset, CreditsRef

	MovieWindow = GemRB.LoadWindow(0, "GUIMOVIE")
	TextAreaControl = MovieWindow.GetControl(0)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)

	if GameCheck.IsBG1():
		MoviesTableName = "MUSIC"
		ColOffset = 1
		CreditsRef = "credits"
	elif GameCheck.IsBG2():
		MoviesTableName = "SONGLIST"
		ColOffset = 0
		CreditsRef = "endcrdit"
	MoviesTable = GemRB.LoadTable (MoviesTableName)
	TextAreaControl.SetColor (ColorWhitish, TA_COLOR_OPTIONS)
	TextAreaControl.SetOptions ([MoviesTable.GetValue (i, 0) for i in range(ColOffset, MoviesTable.GetRowCount())], "MovieIndex", 0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, lambda: PlayMusic(MovieWindow.GetVar("MovieIndex"), ColOffset))
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	MovieWindow.Focus()
	return
	
def PlayMusic(idx, col):
	t = MoviesTable.GetValue (idx + col, 1 - col)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie (CreditsRef, 1)
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Unload()
	if GameCheck.HasTOB():
		GemRB.SetNextScript ("Start2")
	else:
		GemRB.SetNextScript ("Start")
	return
