/*
 * P710 native snapshot update: sgp/NativeFontTranslation.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original glyph ordering from sgp/Font.cpp:1358-1862 at3394695.
// This project-owned adapter retains the original multilingual mapping;
// only allocation failure handling and unused-tail initialization are added.
#include "NativeFontTranslation.h"
#include "MemMan.h"
#include <cstring>
FontTranslationTable *CreateEnglishTransTable(	)
{
	FontTranslationTable *pTable = NULL;
	UINT16	*temp;

	pTable = (FontTranslationTable *)MemAlloc(sizeof(FontTranslationTable));
	if (!pTable) return nullptr;
	memset(pTable, 0, sizeof(FontTranslationTable) );


	pTable->usNumberOfSymbols = 255;

	pTable->DynamicArrayOf16BitValues = (UINT16 *)MemAlloc(pTable->usNumberOfSymbols * 2);
	if (!pTable->DynamicArrayOf16BitValues) { MemFree(pTable); return nullptr; }
	// Original allocates255 entries but initializes fewer: preserve explicit
	// glyph order and zero the unused tail rather than reading allocator bytes.
	memset(pTable->DynamicArrayOf16BitValues, 0, pTable->usNumberOfSymbols * sizeof(UINT16));
	temp = pTable->DynamicArrayOf16BitValues;

	*temp = 'A';
	temp++;
	*temp='B';
	temp++;
	*temp ='C';
	temp++;
	*temp ='D';
	temp++;
	*temp = 'E';
	temp++;
	*temp = 'F';
	temp++;
	*temp='G';
	temp++;
	*temp ='H';
	temp++;
	*temp ='I';
	temp++;
	*temp = 'J';
	temp++;
	*temp = 'K';
	temp++;
	*temp='L';
	temp++;
	*temp ='M';
	temp++;
	*temp ='N';
	temp++;
	*temp = 'O';
	temp++;
	*temp = 'P';
	temp++;
	*temp='Q';
	temp++;
	*temp ='R';
	temp++;
	*temp ='S';
	temp++;
	*temp = 'T';
	temp++;
	*temp ='U';
	temp++;
	*temp ='V';
	temp++;
	*temp = 'W';
	temp++;
	*temp = 'X';
	temp++;
	*temp='Y';
	temp++;
	*temp ='Z';
	temp++;
	*temp ='a';
	temp++;
	*temp = 'b';
	temp++;
	*temp ='c';
	temp++;
	*temp = 'd';
	temp++;
	*temp ='e';
	temp++;
	*temp = 'f';
	temp++;
	*temp ='g';
	temp++;
	*temp = 'h';
	temp++;
	*temp ='i';
	temp++;
	*temp = 'j';
	temp++;
	*temp ='k';
	temp++;
	*temp = 'l';
	temp++;
	*temp ='m';
	temp++;
	*temp = 'n';
	temp++;
	*temp ='o';
	temp++;
	*temp = 'p';
	temp++;
	*temp ='q';
	temp++;
	*temp = 'r';
	temp++;
	*temp ='s';
	temp++;
	*temp = 't';
	temp++;
	*temp ='u';
	temp++;
	*temp = 'v';
	temp++;
	*temp ='w';
	temp++;
	*temp = 'x';
	temp++;
	*temp ='y';
	temp++;
	*temp = 'z';
	temp++;
	*temp ='0';
	temp++;
	*temp = '1';
	temp++;
	*temp ='2';
	temp++;
	*temp = '3';
	temp++;
	*temp ='4';
	temp++;
	*temp = '5';
	temp++;
	*temp ='6';
	temp++;
	*temp = '7';
	temp++;
	*temp ='8';
	temp++;
	*temp = '9';
	temp++;
	*temp ='!';
	temp++;
	*temp = '@';
	temp++;
	*temp ='#';
	temp++;
	*temp = '$';
	temp++;
	*temp ='%';
	temp++;
	*temp = '^';
	temp++;
	*temp ='&';
	temp++;
	*temp = '*';
	temp++;
	*temp ='(';
	temp++;
	*temp = ')';
	temp++;
	*temp ='-';
	temp++;
	*temp = '_';
	temp++;
	*temp ='+';
	temp++;
	*temp = '=';
	temp++;
	*temp ='|';
	temp++;
	*temp = '\\';
	temp++;
	*temp ='{';
	temp++;
	*temp = '}';
	temp++;
	*temp = '[';
	temp++;
	*temp =']';
	temp++;
	*temp = ':';
	temp++;
	*temp =';';
	temp++;
	*temp = '"';
	temp++;
	*temp ='\'';
	temp++;
	*temp = '<';
	temp++;
	*temp = '>';
	temp++;
	*temp =',';
	temp++;
	*temp = '.';
	temp++;
	*temp ='?';
	temp++;
	*temp = '/';
	temp++;
	*temp = ' ';
	temp++;


	*temp = 196;
	temp++;
	*temp = 214;
	temp++;
	*temp = 220;
	temp++;
	*temp = 228;
	temp++;
	*temp = 246;
	temp++;
	*temp = 252;
	temp++;
	*temp = 223;
	temp++;


	*temp = 1040;
	temp++;
	*temp = 1041;
	temp++;
	*temp = 1042;
	temp++;
	*temp = 1043;
	temp++;
	*temp = 1044;
	temp++;
	*temp = 1045;
	temp++;
	*temp = 1046;
	temp++;
	*temp = 1047;
	temp++;
	*temp = 1048;
	temp++;
	*temp = 1049;
	temp++;
	*temp = 1050;
	temp++;
	*temp = 1051;
	temp++;
	*temp = 1052;
	temp++;
	*temp = 1053;
	temp++;
	*temp = 1054;
	temp++;
	*temp = 1055;
	temp++;
	*temp = 1056;
	temp++;
	*temp = 1057;
	temp++;
	*temp = 1058;
	temp++;
	*temp = 1059;
	temp++;
	*temp = 1060;
	temp++;
	*temp = 1061;
	temp++;
	*temp = 1062;
	temp++;
	*temp = 1063;
	temp++;
	*temp = 1064;
	temp++;
	*temp = 1065;
	temp++;
	*temp = 1066;
	temp++;
	*temp = 1067;
	temp++;
	*temp = 1068;
	temp++;
	*temp = 1069;
	temp++;
	*temp = 1070;
	temp++;
	*temp = 1071;
	temp++;
	*temp = 1072;
	temp++;
	*temp = 1073;
	temp++;
	*temp = 1074;
	temp++;
	*temp = 1075;
	temp++;
	*temp = 1076;
	temp++;
	*temp = 1077;
	temp++;
	*temp = 1078;
	temp++;
	*temp = 1079;
	temp++;
	*temp = 1080;
	temp++;
	*temp = 1081;
	temp++;
	*temp = 1082;
	temp++;
	*temp = 1083;
	temp++;
	*temp = 1084;
	temp++;
	*temp = 1085;
	temp++;
	*temp = 1086;
	temp++;
	*temp = 1087;
	temp++;
	*temp = 1088;
	temp++;
	*temp = 1089;
	temp++;
	*temp = 1090;
	temp++;
	*temp = 1091;
	temp++;
	*temp = 1092;
	temp++;
	*temp = 1093;
	temp++;
	*temp = 1094;
	temp++;
	*temp = 1095;
	temp++;
	*temp = 1096;
	temp++;
	*temp = 1097;
	temp++;
	*temp = 1098;
	temp++;
	*temp = 1099;
	temp++;
	*temp = 1100;
	temp++;
	*temp = 1101;
	temp++;
	*temp = 1102;
	temp++;
	*temp = 1103;
	temp++;


	*temp = 8470;
	temp++;
	*temp = 1025;
	temp++;
	*temp = 1030;
	temp++;
	*temp = 1038;
	temp++;
	*temp = 1105;
	temp++;
	*temp = 1110;
	temp++;
	*temp = 1118;
	temp++;
	*temp = 1168;
	temp++;
	*temp = 1028;
	temp++;
	*temp = 1031;
	temp++;
	*temp = 1169;
	temp++;
	*temp = 1108;
	temp++;
	*temp = 1111;
	temp++;


	*temp = 260;
	temp++;
	*temp = 262;
	temp++;
	*temp = 280;
	temp++;
	*temp = 321;
	temp++;
	*temp = 323;
	temp++;
	*temp = 211;
	temp++;
	*temp = 346;
	temp++;
	*temp = 379;
	temp++;
	*temp = 377;
	temp++;
	*temp = 261;
	temp++;
	*temp = 263;
	temp++;
	*temp = 281;
	temp++;
	*temp = 322;
	temp++;
	*temp = 324;
	temp++;
	*temp = 243;
	temp++;
	*temp = 347;
	temp++;
	*temp = 380;
	temp++;
	*temp = 378;
	temp++;


	*temp = 192;
	temp++;
	*temp = 194;
	temp++;
	*temp = 199;
	temp++;
	*temp = 200;
	temp++;
	*temp = 201;
	temp++;
	*temp = 202;
	temp++;
	*temp = 203;
	temp++;
	*temp = 206;
	temp++;
	*temp = 207;
	temp++;
	*temp = 212;
	temp++;
	*temp = 217;
	temp++;
	*temp = 219;
	temp++;
	*temp = 376;
	temp++;

	*temp = 224;
	temp++;
	*temp = 226;
	temp++;
	*temp = 231;
	temp++;
	*temp = 232;
	temp++;
	*temp = 233;
	temp++;
	*temp = 234;
	temp++;
	*temp = 235;
	temp++;
	*temp = 238;
	temp++;
	*temp = 239;
	temp++;
	*temp = 244;
	temp++;
	*temp = 249;
	temp++;
	*temp = 251;
	temp++;
	*temp = 255;
	temp++;

	*temp = 204;
	temp++;
	*temp = 236;
	temp++;
	*temp = 210;
	temp++;
	*temp = 242;
	temp++;


	*temp = 198;
	temp++;
	*temp = 338;
	temp++;
	*temp = 230;
	temp++;
	*temp = 339;
	temp++;

















	return pTable;
}

void DestroyNativeFontTranslationTable(FontTranslationTable* table) noexcept {
    if (!table) return;
    MemFree(table->DynamicArrayOf16BitValues);
    MemFree(table);
}
