
#pragma once

#include "../../framework/includes.h"
#include "../../library/memory.h"

#define APOCFONT_ALIGN_LEFT	0
#define APOCFONT_ALIGN_CENTRE	1
#define APOCFONT_ALIGN_RIGHT	2

class ApocalypseFont
{

	private:
		static std::string FontCharacterSet;

		std::vector<ALLEGRO_BITMAP*> fontbitmaps;
		std::vector<int> fontwidths;


	public:
		ApocalypseFont( bool LargeFont );
		~ApocalypseFont();

		void DrawString( int X, int Y, std::string Text, int Alignment );

		void DumpCharset();
};
