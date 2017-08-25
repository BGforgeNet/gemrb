/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TextArea.h"

#include "win32def.h"

#include "Interface.h"
#include "Variables.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {
	
TextArea::SpanSelector::SpanSelector(TextArea& ta, const std::vector<const String*>& opts, bool numbered)
: TextContainer(Region(), ta.ftext, ta.palettes[PALETTE_SELECTED]), ta(ta)
{
	selectedSpan = NULL;
	hoverSpan = NULL;

	size = opts.size();
	
	Size s = ta.Dimensions();
	s.h = 0; // will dynamically size itself
	SetFrameSize(s);
	
	Size flexFrame(-1, 0); // flex frame for hanging indent after optnum
	Region r = Frame();
	for (size_t i = 0; i < opts.size(); i++) {
		// FIXME: wrong frame
		TextContainer* selOption = new OptSpan(r, ta.ftext, ta.palettes[PALETTE_OPTIONS]);
		if (numbered) {
			wchar_t optNum[6];
			swprintf(optNum, sizeof(optNum)/sizeof(optNum[0]), L"%d. - ", i+1);
			// TODO: as per the original PALETTE_SELECTED should be updated to the PC color (same color their name is rendered in)
			// but that should probably actually be done by the dialog handler, not here.
			selOption->AppendContent(new TextSpan(optNum, NULL, ta.palettes[PALETTE_SELECTED]));
		}
		selOption->AppendContent(new TextSpan(*opts[i], NULL, NULL, &flexFrame));
		
		AppendText(L"test");
		AddSubviewInFrontOfView(selOption);
		if (EventMgr::TouchInputEnabled) {
			// now add a newline for keeping the options spaced out (for touch screens)
			AppendText(L"\n");
		}
	}
	
	MakeSelection(0);
}

void TextArea::SpanSelector::ClearHover()
{
	if (hoverSpan) {
		if (hoverSpan == selectedSpan) {
			hoverSpan->SetPalette(ta.palettes[PALETTE_SELECTED]);
		} else {
			// reset the old hover span
			hoverSpan->SetPalette(ta.palettes[PALETTE_OPTIONS]);
		}
		hoverSpan = NULL;
	}
}

void TextArea::SpanSelector::MakeSelection(size_t idx)
{
	TextContainer* optspan = TextAtIndex(idx);

	if (selectedSpan && selectedSpan != optspan) {
		// reset the previous selection
		selectedSpan->SetPalette(ta.palettes[PALETTE_OPTIONS]);
		MarkDirty();
	}
	selectedSpan = optspan;
	selectedSpan->SetPalette(ta.palettes[PALETTE_SELECTED]);
}
	
TextContainer* TextArea::SpanSelector::TextAtPoint(const Point& p)
{
	// container only has text, so...
	return static_cast<TextContainer*>(SubviewAt(p, true, false));
}
	
TextContainer* TextArea::SpanSelector::TextAtIndex(size_t idx)
{
	std::list<View*>::iterator it = subViews.begin();
	std::advance(it, idx);
	return static_cast<TextContainer*>(*it);
}

void TextArea::SpanSelector::OnMouseOver(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());
	TextContainer* span = TextAtPoint(p);
	
	if (hoverSpan || span)
		MarkDirty();
	
	ClearHover();
	if (span) {
		hoverSpan = span;
		hoverSpan->SetPalette(ta.palettes[PALETTE_HOVER]);
	}
}
	
void TextArea::SpanSelector::OnMouseUp(const MouseEvent& me, unsigned short /*Mod*/)
{
	Point p = ConvertPointFromScreen(me.Pos());
	TextContainer* span = TextAtPoint(p);
	
	if (span) {
		std::list<View*>::iterator it = subViews.begin();
		unsigned int idx = 0;
		while (*it++ != span) { ++idx; };
		
		ta.UpdateState(idx + 1);
	}
}
	
void TextArea::SpanSelector::OnMouseLeave(const MouseEvent& /*me*/, const DragOp*)
{
	ClearHover();
}

TextArea::TextArea(const Region& frame, Font* text)
	: Control(frame), scrollview(Region(Point(), Dimensions())), ftext(text), palettes()
{
	palettes[PALETTE_NORMAL] = text->GetPalette();
	finit = ftext;
	Init();
}

TextArea::TextArea(const Region& frame, Font* text, Font* caps,
				   Color textcolor, Color initcolor, Color lowtextcolor)
	: Control(frame), scrollview(Region(Point(), Dimensions())), ftext(text), palettes()
{
	palettes[PALETTE_NORMAL] = new Palette( textcolor, lowtextcolor );

	// quick font optimization (prevents creating unnecessary cap spans)
	finit = (caps != ftext) ? caps : ftext;

	// in case a bad or missing font was specified, use an obvious fallback
	if (!finit) {
		Log(ERROR, "TextArea", "Tried to use missing font, resorting to a fallback!");
		finit = core->GetTextFont();
		ftext = finit;
	}

	if (finit->Baseline < ftext->LineHeight) {
		// FIXME: initcolor is only used for *some* initial fonts
		// this is a hack to workaround the INITIALS font getting its palette set
		// do we have another (more sane) way to tell if a font needs this palette? (something in the BAM?)
		SetPalette(&initcolor, PALETTE_INITIALS);
	} else {
		palettes[PALETTE_INITIALS] = finit->GetPalette();
	}

	parser.ResetAttributes(text, palettes[PALETTE_NORMAL], finit, palettes[PALETTE_INITIALS]);
	Init();
}

void TextArea::Init()
{
	ControlType = IE_GUI_TEXTAREA;
	strncpy(VarName, "Selected", sizeof(VarName));

	selectOptions = NULL;
	textContainer = NULL;
	
	AddSubviewInFrontOfView(&scrollview);

	// initialize the Text containers
	ClearSelectOptions();
	ClearText();
	SetAnimPicture(NULL);
	
	scrollview.SetScrollIncrement(LineHeight());
}

void TextArea::DrawSelf(Region drawFrame, const Region& /*clip*/)
{
	if (AnimPicture) {
		// speaker portrait
		core->GetVideoDriver()->BlitSprite(AnimPicture, drawFrame.x, drawFrame.y);
	}
}

void TextArea::SizeChanged(const Size& /*oldSize*/)
{
	// TODO: subview resizing should be able to be handled generically by View
	Region r(Point(), Dimensions());
	scrollview.SetFrame(r);
}

void TextArea::SetAnimPicture(Sprite2D* pic)
{
	Control::SetAnimPicture(pic);
	
	assert(textContainer);
	Region tf = textContainer->Frame();
	if (AnimPicture) {
		// shrink and shift the container to accommodate the image
		tf.x += AnimPicture->Width;
		tf.w -= frame.x;
	} else {
		tf.x = 0;
		tf.w = Dimensions().w;
	}
	textContainer->SetFrame(tf);
}

ieDword TextArea::LineCount() const
{
	int rowHeight = LineHeight();
	if (rowHeight > 0)
		return (ContentHeight() + rowHeight - 1) / rowHeight; // round up
	else
		return 0;
}

void TextArea::UpdateScrollview()
{
	/*
	int textHeight = ContentHeight();
	Region nodeBounds;
	if (dialogBeginNode) {
		// possibly add some phony height to allow dialogBeginNode to the top when the scrollbar is at the bottom
		// add the height of a newline too so that there is a space
		nodeBounds = textContainer->BoundingBoxForContent(dialogBeginNode);
		Size selectFrame = selectOptions->Dimensions();
		// page = blank line + dialog node + blank line + select options
		int pageH = ftext->LineHeight + nodeBounds.h + selectFrame.h;
		if (pageH < frame.h) {
			// if the node isnt a full page by itself we need to fake it
			textHeight += frame.h - pageH;
		}
	}
	int rowHeight = GetRowHeight();
	int newRows = (textHeight + rowHeight - 1) / rowHeight; // round up
	if (newRows != rows) {
		UpdateRowCount(textHeight);
		ieWord visibleRows = (frame.h / GetRowHeight());
		ieWord sbMax = (rows > visibleRows) ? (rows - visibleRows) : 0;
		scrollbar->SetValueRange(0, sbMax);
	}

	if (flags&IE_GUI_TEXTAREA_AUTOSCROLL
		&& dialogBeginNode) {
		// now scroll dialogBeginNode to the top less a blank line
		ScrollToY(nodeBounds.y - ftext->LineHeight);
	}
	 */
}

/** Sets the Actual Text */
void TextArea::SetText(const String& text)
{
	ClearText();
	AppendText(text);
}

void TextArea::SetPalette(const Color* color, PALETTE_TYPE idx)
{
	assert(idx < PALETTE_TYPE_COUNT);
	if (color) {
		palettes[idx] = new Palette( *color, ColorBlack );
		palettes[idx]->release();
	} else if (idx > PALETTE_NORMAL) {
		// default to normal
		palettes[idx] = palettes[PALETTE_NORMAL];
	}
}

void TextArea::AppendText(const String& text)
{
	if (flags&IE_GUI_TEXTAREA_HISTORY) {
		int heightLimit = (ftext->LineHeight * 100); // 100 lines of content
		// start trimming content from the top until we are under the limit.
		Size frame = textContainer->Dimensions();
		int currHeight = frame.h;
		if (currHeight > heightLimit) {
			Region exclusion(Point(), Size(frame.w, currHeight - heightLimit));
			textContainer->DeleteContentsInRect(exclusion);
		}
	}

	size_t tagPos = text.find_first_of('[');
	if (tagPos != String::npos) {
		parser.ParseMarkupStringIntoContainer(text, *textContainer);
	} else if (text.length()) {
		if (finit != ftext) {
			// append cap spans
			size_t textpos = text.find_first_not_of(WHITESPACE_STRING);
			if (textpos != String::npos) {
				// first append the white space as its own span
				textContainer->AppendText(text.substr(0, textpos));

				// we must create and append this span here (instead of using AppendText),
				// because the original data files for the DC font specifies a line height of 13
				// that would cause overlap when the lines wrap beneath the DC if we didnt specify the correct size
				Size s = finit->GetGlyph(text[textpos]).size;
				if (s.h > ftext->LineHeight) {
					// pad this only if it is "real" (it is higher than the other text).
					// some text areas have a "cap" font assigned in the CHU that differs from ftext, but isnt meant to be a cap
					// see BG2 chargen
					s.w += 3;
				}
				TextSpan* dc = new TextSpan(text.substr(textpos, 1), finit, palettes[PALETTE_INITIALS], &s);
				textContainer->AppendContent(dc);
				textpos++;
				// FIXME: assuming we have more text!
				// FIXME: as this is currently implemented, the cap is *not* considered part of the word,
				// there is potential wrapping errors (BG2 char gen).
				// we could solve this by wrapping the cap and the letters remaining letters of the word into their own TextContainer
			} else {
				textpos = 0;
			}
			textContainer->AppendText(text.substr(textpos));
		} else {
			textContainer->AppendText(text);
		}
	}

	UpdateScrollview();
	if (flags&IE_GUI_TEXTAREA_AUTOSCROLL && !selectOptions)
	{
		// scroll to the bottom
		int bottom = ContentHeight() - frame.h;
		if (bottom > 0)
			ScrollToY(bottom, 500); // animated scroll
	}
	MarkDirty();
}
/*
int TextArea::InsertText(const char* text, int pos)
{
	// TODO: actually implement this
	AppendText(text);
	return pos;
}
*/
/** Key Press Event */
bool TextArea::OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/)
{
	if (flags & IE_GUI_TEXTAREA_EDITABLE) {
		if (Key.character) {
			MarkDirty();
			// TODO: implement this! currently does nothing
			size_t CurPos = 0, len = 0;
			switch (Key.keycode) {
				case GEM_HOME:
					CurPos = 0;
					break;
				case GEM_UP:
					break;
				case GEM_DOWN:
					break;
				case GEM_END:
					break;
				case GEM_LEFT:
					if (CurPos > 0) {
						CurPos--;
					} else {

					}
					break;
				case GEM_RIGHT:
					if (CurPos < len) {
						CurPos++;
					} else {

					}
					break;
				case GEM_DELETE:
					if (CurPos>=len) {
						break;
					}
					break;
				case GEM_BACKSP:
					if (CurPos != 0) {
						if (len<1) {
							break;
						}
						CurPos--;
					} else {

					}
					break;
				case GEM_RETURN:
					//add an empty line after CurLine
					// TODO: implement this
					//copy the text after the cursor into the new line

					//truncate the current line
					
					//move cursor to next line beginning
					CurPos=0;
					break;
			}

			PerformAction(Action::Change);
		}
		return true;
	}

	if (( Key.character < '1' ) || ( Key.character > '9' ))
		return false;

	MarkDirty();

	unsigned int lookupIdx = Key.character - '1';
	UpdateState(lookupIdx);
	return true;
}

ieWord TextArea::LineHeight() const
{
	return ftext->LineHeight;
}

/** Will scroll y pixels over duration */
void TextArea::ScrollToY(int y, short lineduration)
{
	ieDword duration = lineduration * LineCount();
	scrollview.ScrollTo(Point(0, y), duration);
}

/** Mousewheel scroll */
/** This method is key to touchscreen scrolling */
void TextArea::OnMouseWheelScroll(const Point& delta)
{
	// the only time we should get this event is when AnimPicture is set
	// otherwise the scrollview would have recieved this
	assert(AnimPicture);
	scrollview.OnMouseWheelScroll(delta);
}

void TextArea::UpdateState(unsigned int optIdx)
{
	if (!VarName[0] || selectOptions == NULL || optIdx >= selectOptions->NumOpts()) {
		return;
	}
	if (!selectOptions) {
		// no selectable options present
		// set state to safe and return
		ClearSelectOptions();
		return;
	}

	// always run the TextAreaOnSelect handler even if the value hasnt changed
	// the *context* of the value can change (dialog) and the handler will want to know 
	SetValue( values[optIdx] );

	// this can be called from elsewhere (GUIScript), so we need to make sure we update the selected span
	selectOptions->MakeSelection(optIdx);

	PerformAction(Action::Select);
}

int TextArea::ContentHeight() const
{
	int cHeight = 0;
	cHeight += (textContainer) ? textContainer->Dimensions().h : 0;
	cHeight += (selectOptions) ? selectOptions->Dimensions().h : 0;
	return cHeight;
}

String TextArea::QueryText() const
{
	if (selectOptions) {
		return selectOptions->Selection()->Text();
	}
	return textContainer->Text();
}

void TextArea::ClearSelectOptions()
{
	values.clear();
	delete scrollview.RemoveSubview(selectOptions);
	dialogBeginNode = NULL;
	selectOptions = NULL;

	UpdateScrollview();
}

void TextArea::SetSelectOptions(const std::vector<SelectOption>& opts, bool numbered,
								const Color* color, const Color* hiColor, const Color* selColor)
{
	SetPalette(color, PALETTE_OPTIONS);
	SetPalette(hiColor, PALETTE_HOVER);
	SetPalette(selColor, PALETTE_SELECTED);

	ClearSelectOptions(); // deletes previous options
	
	ContentContainer::ContentList::const_reverse_iterator it = textContainer->Contents().rbegin();
	if (it != textContainer->Contents().rend()) {
		dialogBeginNode = *it; // need to get the last node *before* we append anything
		//selectOptions->AppendText(L"\n"); // always want a gap between text and select options for dialog
	}
	
	values.reserve(opts.size());
	std::vector<const String*> strings(opts.size());
	for (size_t i = 0; i < opts.size(); i++) {
		values[i] = opts[i].first;
		strings[i] = &(opts[i].second);
	}

	selectOptions = new SpanSelector(*this, strings, numbered);
	Point p(0, ContentHeight());
	selectOptions->SetFrameOrigin(p);
	scrollview.AddSubviewInFrontOfView(selectOptions);
	UpdateScrollview();
}

void TextArea::ClearText()
{
	delete scrollview.RemoveSubview(textContainer);

	parser.Reset(); // reset in case any tags were left open from before
	textContainer = new TextContainer(Region(Point(), Size(frame.w, 0)), ftext, palettes[PALETTE_NORMAL]);
	scrollview.AddSubviewInFrontOfView(textContainer);

	UpdateScrollview();
}

void TextArea::SetFocus()
{
	Control::SetFocus();
	if (IsFocused() && flags & IE_GUI_TEXTAREA_EDITABLE) {
		core->GetVideoDriver()->ShowSoftKeyboard();
	}
}

}
