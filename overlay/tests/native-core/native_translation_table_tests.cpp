/*
 * P710 native snapshot update: tests/native-core/native_translation_table_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Translation-table shape, null-entry, and format-argument regression checks.
// This translation unit includes the original tables once and adds no game data.
#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "../../i18n/LanguageStrings.cpp"

// Keep the English binding contract strict. Other languages can have existing
// outer-extent differences, which the runtime baseline records below.
#define X(NAME)                                                             \
    static_assert(std::is_same_v<decltype(::NAME),                           \
                                 std::decay_t<decltype(lang_en::NAME)>>,    \
                  "English table binding shape differs: " #NAME);
#include "../../i18n/text.def"
#undef X

#define P710_ENGLISH_TABLE_MATCHES_ENUM(NAME, COUNT)                         \
    static_assert(std::size(lang_en::NAME) == COUNT,                         \
                  "English table size differs from its enum: " #NAME       \
                  " vs " #COUNT);

// These six original tables and enums already disagree. Pin both values at
// compile time until their source contracts are repaired separately.
#define P710_ENGLISH_ENUM_DEBT(NAME, COUNT, TABLE_SIZE, ENUM_SIZE)             \
    static_assert(std::size(lang_en::NAME) == TABLE_SIZE && COUNT == ENUM_SIZE, \
                  "English table/enum baseline changed: " #NAME);
P710_ENGLISH_ENUM_DEBT(AimAlumniText, TEXT_NUM_AIM_ALUMNI, 6, 5)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimBottomMenuText, TEXT_NUM_AIM_MENU)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimFiText, TEXT_NUM_AIM_FI)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimHistoryText, TEXT_NUM_AIM_HISTORY)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimLinkText, TEXM_NUM_AIM_LINK)
P710_ENGLISH_ENUM_DEBT(AimMemberText, TEXT_NUM_AIM_MEMBER_TEXT, 4, 1)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimPopUpText, TEXT_NUM_AIM_MEMBER_POPUP)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimScreenText, TEXT_NUM_AIM_SCREEN)
P710_ENGLISH_TABLE_MATCHES_ENUM(AimSortText, TEXT_NUM_AIM_SORT)
P710_ENGLISH_TABLE_MATCHES_ENUM(BobbyRFilter, TEXT_NUM_BOBBYR_FILTER)
P710_ENGLISH_TABLE_MATCHES_ENUM(BobbyROrderFormText, TEXT_NUM_BOBBYR_MAILORDER)
P710_ENGLISH_TABLE_MATCHES_ENUM(BobbyRText, TEXT_NUM_BOBBYR_GUNS)
P710_ENGLISH_TABLE_MATCHES_ENUM(BobbyRaysFrontText, TEXT_NUM_BOBBYR)
P710_ENGLISH_TABLE_MATCHES_ENUM(BrokenLinkText, TEXT_NUM_BROKEN_LINK)
P710_ENGLISH_TABLE_MATCHES_ENUM(CharacterInfo, TEXT_NUM_AIM_MEMBER_CHARINFO)
P710_ENGLISH_TABLE_MATCHES_ENUM(InsContractText, TEXT_NUM_INS_CONTRACT)
P710_ENGLISH_TABLE_MATCHES_ENUM(InsInfoText, TEXT_NUM_INS_INFO)
P710_ENGLISH_ENUM_DEBT(LargeTacticalStr, TEXT_NUM_LARGESTR, 4, 3)
P710_ENGLISH_TABLE_MATCHES_ENUM(MercAccountText, TEXT_NUM_MERC_ACCOUNT)
P710_ENGLISH_TABLE_MATCHES_ENUM(MercHomePageText, TEXT_NUM_MERC)
P710_ENGLISH_TABLE_MATCHES_ENUM(MercInfo, TEXT_NUM_MERC_FILES)
P710_ENGLISH_TABLE_MATCHES_ENUM(MercNoAccountText, TEXT_NUM_MERC_NO_ACC)
P710_ENGLISH_TABLE_MATCHES_ENUM(Message, TEXT_NUM_STR_MESSAGE)
P710_ENGLISH_TABLE_MATCHES_ENUM(New113Message, TEXT_NUM_MSG113)
P710_ENGLISH_TABLE_MATCHES_ENUM(NewInvMessage, TEXT_NUM_NIV)
P710_ENGLISH_TABLE_MATCHES_ENUM(SKI_Text, TEXT_NUM_SKI_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(SkiMessageBoxText, TEXT_NUM_SKI_MBOX_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(TacticalStr, TEXT_NUM_TACTICAL_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(VideoConfercingText, TEXT_NUM_AIM_MEMBER_VCONF)
P710_ENGLISH_TABLE_MATCHES_ENUM(gMoneyStatsDesc, TEXT_NUM_MONEY_DESC)
P710_ENGLISH_TABLE_MATCHES_ENUM(gpGameClockString, TEXT_NUM_GAMECLOCK)
P710_ENGLISH_TABLE_MATCHES_ENUM(gpStrategicString, TEXT_NUM_STRATEGIC_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzBobbyRShipmentText, TEXT_NUM_BOBBYR_SHIPMENT)
P710_ENGLISH_ENUM_DEBT(gzGIOScreenText, TEXT_NUM_GIO_TEXT, 69, 70)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzHelpScreenText, TEXT_NUM_HLP)
P710_ENGLISH_ENUM_DEBT(gzLaptopHelpText, TEXT_NUM_LAPTOP_BN_BOOKMARK_TEXT, 27, 16)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzMPCScreenText, TEXT_NUM_MPC_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzMPHScreenText, TEXT_NUM_MPH_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzMPJScreenText, TEXT_NUM_MPJ_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzMPSScreenText, TEXT_NUM_MPS_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzMoneyWithdrawMessageText, TEXT_NUM_MONEY_WITHDRAW)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzSkiAtmText, TEXT_NUM_SKI_ATM_MODE_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(gzTooltipStrings, TEXT_NUM_STR_TT)
P710_ENGLISH_TABLE_MATCHES_ENUM(pAntiHackerString, TEXT_NUM_ANTIHACKERSTR)
P710_ENGLISH_TABLE_MATCHES_ENUM(pExitingSectorHelpText, TEXT_NUM_EXIT_GUI)
P710_ENGLISH_TABLE_MATCHES_ENUM(pHelicopterEtaStrings, TEXT_NUM_STR_HELI_ETA)
P710_ENGLISH_TABLE_MATCHES_ENUM(pHelicopterRepairRefuelStrings, TEXT_NUM_STR_HELI_REPAIRS)
P710_ENGLISH_TABLE_MATCHES_ENUM(pMessageStrings, TEXT_NUM_MSG)
P710_ENGLISH_TABLE_MATCHES_ENUM(pTraitSkillsDenialStrings, TEXT_SKILL_DENIAL_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(sFloristCards, TEXT_NUM_FLORIST_CARDS)
P710_ENGLISH_TABLE_MATCHES_ENUM(sFloristGalleryText, TEXT_NUM_FLORIST_GALLERY)
P710_ENGLISH_TABLE_MATCHES_ENUM(sFloristText, TEXT_NUM_FLORIST)
P710_ENGLISH_TABLE_MATCHES_ENUM(sFuneralString, TEXT_NUM_FUNERAL)
P710_ENGLISH_TABLE_MATCHES_ENUM(sOrderFormText, TEXT_NUM_FLORIST_ORDER)
P710_ENGLISH_TABLE_MATCHES_ENUM(szCampaignHistoryDetail, TEXT_CAMPAIGNHISTORY_DETAIL_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(szCampaignHistoryWebSite, TEXT_CAMPAIGNHISTORY_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(szCorpseTextStr, TEXT_NUM_CORPSE_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szCovertTextStr, TEXT_NUM_COVERT_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szFoodTextStr, TEXT_NUM_FOOD_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szInventoryArmTextStr, TEXT_NUM_INV_ARM_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szMTATextStr, TEXT_NUM_MTA_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szMercCompareWebSite, TEXT_MERCCOMPARE_MAX)
P710_ENGLISH_ENUM_DEBT(szMilitiaWebSite, TEXT_MILITIAWEBSITE_MAX, 2, 6)
P710_ENGLISH_TABLE_MATCHES_ENUM(szPMCWebSite, TEXT_PMC_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(szPersonalityDisplayText, PERSONALITYTEXT_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(szPrisonerTextStr, TEXT_NUM_PRISONER_STR)
P710_ENGLISH_TABLE_MATCHES_ENUM(szWHOWebSite, TEXT_WHO_MAX)
P710_ENGLISH_TABLE_MATCHES_ENUM(zOptionsText, TEXT_NUM_OPT_TEXT)
P710_ENGLISH_TABLE_MATCHES_ENUM(zSaveLoadText, TEXT_NUM_SLG_TEXT)
#undef P710_ENGLISH_TABLE_MATCHES_ENUM
#undef P710_ENGLISH_ENUM_DEBT

namespace
{
struct Entry
{
    std::string name;
    const CHAR16* text;
};

using Entries = std::vector<Entry>;
using Signature = std::vector<std::wstring>;

struct Table
{
    std::string name;
    std::string shape;
    Entries entries;
};
using Tables = std::vector<Table>;

template <typename T>
std::string tableShape()
{
    if constexpr (std::is_array_v<T>)
        return "[" + std::to_string(std::extent_v<T>) + "]" +
               tableShape<std::remove_extent_t<T>>();
    else if constexpr (std::is_same_v<std::remove_cv_t<T>, CHAR16>)
        return "CHAR16";
    else
    {
        static_assert(std::is_convertible_v<T, const CHAR16*>,
                      "Unsupported translation entry type");
        return "STR16";
    }
}

template <typename T>
void collect(Entries& entries, const std::string& name, const T& table)
{
    if constexpr (std::is_array_v<T>)
    {
        if constexpr (std::is_same_v<
                          std::remove_cv_t<std::remove_extent_t<T>>, CHAR16>)
        {
            entries.push_back({name, table});
        }
        else
        {
            for (std::size_t index = 0; index < std::extent_v<T>; ++index)
            {
                collect(entries, name + "[" + std::to_string(index) + "]",
                        table[index]);
            }
        }
    }
    else if constexpr (std::is_convertible_v<T, const CHAR16*>)
    {
        entries.push_back({name, table});
    }
}

template <typename T>
void collectTable(Tables& tables, const char* name, const T& table)
{
    Table result{name, tableShape<T>(), {}};
    collect(result.entries, name, table);
    tables.push_back(std::move(result));
}

#define P710_ENTRIES_FUNCTION(CODE)                                           \
    Tables CODE##_entries()                                                  \
    {                                                                         \
        Tables entries;

P710_ENTRIES_FUNCTION(en)
#define X(NAME) collectTable(entries, #NAME, lang_en::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(de)
#define X(NAME) collectTable(entries, #NAME, lang_de::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(ru)
#define X(NAME) collectTable(entries, #NAME, lang_ru::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(nl)
#define X(NAME) collectTable(entries, #NAME, lang_nl::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(pl)
#define X(NAME) collectTable(entries, #NAME, lang_pl::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(fr)
#define X(NAME) collectTable(entries, #NAME, lang_fr::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(it)
#define X(NAME) collectTable(entries, #NAME, lang_it::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
P710_ENTRIES_FUNCTION(zh)
#define X(NAME) collectTable(entries, #NAME, lang_zh::NAME);
#include "../../i18n/text.def"
#undef X
    return entries;
}
#undef P710_ENTRIES_FUNCTION

Signature formatArguments(const CHAR16* text)
{
    Signature arguments;
    while (*text)
    {
        if (*text++ != L'%')
            continue;
        if (*text == L'%')
        {
            ++text;
            continue;
        }

        while (*text && std::wcschr(L"-+ #0", *text))
            ++text;
        while (*text && std::iswdigit(*text))
            ++text;
        if (*text == L'*')
        {
            arguments.emplace_back(L"*");
            ++text;
        }
        if (*text == L'.')
        {
            ++text;
            while (*text && std::iswdigit(*text))
                ++text;
            if (*text == L'*')
            {
                arguments.emplace_back(L"*");
                ++text;
            }
        }

        std::wstring token;
        if (*text == L'h' || *text == L'l')
        {
            const CHAR16 first = *text++;
            token += first;
            if (*text == first)
                token += *text++;
        }
        else if (*text && std::wcschr(L"Ljzt", *text))
        {
            token += *text++;
        }
        else if (*text == L'I')
        {
            token += *text++;
            while (*text && std::iswdigit(*text))
                token += *text++;
        }

        if (*text)
        {
            token += *text == L'i' ? L'd' : *text;
            ++text;
        }
        else
        {
            token += L"<incomplete>";
        }
        arguments.push_back(token);
    }
    return arguments;
}

std::string signatureText(const Signature& signature)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < signature.size(); ++index)
    {
        if (index)
            output << ',';
        for (const wchar_t character : signature[index])
        {
            if (character >= 32 && character <= 126)
                output << static_cast<char>(character);
            else
                output << "\\u" << std::hex << static_cast<unsigned int>(character)
                       << std::dec;
        }
    }
    return output.str();
}

const std::set<std::string> plainTextTables = {
    "FactDescText",
    "gzMiscItemStatsFasthelp[32]",
    "szUDBAdvStatsExplanationsTooltipText",
    "szUDBGenArmorStatsExplanationsTooltipText",
};

bool isPlainText(const std::string& name)
{
    return plainTextTables.count(name) != 0 ||
           plainTextTables.count(name.substr(0, name.find('['))) != 0;
}

int failures = 0;
int checks = 0;
constexpr int diagnosticLimit = 100;

void fail(const std::string& message)
{
    ++failures;
    if (failures <= diagnosticLimit)
        std::cerr << message << '\n';
}

void require(bool value, const std::string& message)
{
    ++checks;
    if (!value)
        fail(message);
}

// Each key records one existing shape defect. Both shapes are pinned so that
// additions, further drift, and obsolete baseline entries all fail the test.
using ShapeBaseline = std::map<std::string, std::pair<std::string, std::string>>;
const ShapeBaseline knownShapeDifferences = {
    {"de::WeaponType", {"[16001][30]CHAR16", "[9][30]CHAR16"}},
    {"de::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"de::pBookMarkStrings", {"[18]STR16", "[17]STR16"}},
    {"de::gzIMPDisabilityTraitText", {"[14]STR16", "[13]STR16"}},
    {"de::gzFacilityErrorMessage", {"[34]STR16", "[32]STR16"}},
    {"de::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"de::szMercCompareWebSite", {"[31]STR16", "[30]STR16"}},
    {"de::szMercCompareEventText", {"[40]STR16", "[39]STR16"}},
    {"de::szIndividualMilitiaBattleReportText", {"[12]STR16", "[11]STR16"}},
    {"de::TacticalStr", {"[213][80]CHAR16", "[212][80]CHAR16"}},
    {"ru::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"ru::pBookMarkStrings", {"[18]STR16", "[17]STR16"}},
    {"ru::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"nl::WeaponType", {"[16001][30]CHAR16", "[9][30]CHAR16"}},
    {"nl::pPersonnelScreenStrings", {"[26]STR16", "[25]STR16"}},
    {"nl::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"nl::pBookMarkStrings", {"[18]STR16", "[17]STR16"}},
    {"nl::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"nl::szMercCompareWebSite", {"[31]STR16", "[30]STR16"}},
    {"nl::szMercCompareEventText", {"[40]STR16", "[39]STR16"}},
    {"nl::szIndividualMilitiaBattleReportText", {"[12]STR16", "[11]STR16"}},
    {"nl::TacticalStr", {"[213][80]CHAR16", "[212][80]CHAR16"}},
    {"pl::pEditModeShutdownText", {"[1]STR16", "[2]STR16"}},
    {"pl::TeamTurnString", {"[10][255]CHAR16", "[9][255]CHAR16"}},
    {"pl::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"pl::pBookMarkStrings", {"[18]STR16", "[17]STR16"}},
    {"pl::sEnemyTauntsFireGun", {"[15]STR16", "[13]STR16"}},
    {"pl::sEnemyTauntsAlert", {"[3]STR16", "[2]STR16"}},
    {"pl::gzFacilityAssignmentStrings", {"[42]STR16", "[41]STR16"}},
    {"pl::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"pl::szMercCompareWebSite", {"[31]STR16", "[30]STR16"}},
    {"pl::szMercCompareEventText", {"[40]STR16", "[39]STR16"}},
    {"pl::szIndividualMilitiaBattleReportText", {"[12]STR16", "[11]STR16"}},
    {"pl::TacticalStr", {"[213][80]CHAR16", "[212][80]CHAR16"}},
    {"fr::WeaponType", {"[16001][30]CHAR16", "[9][30]CHAR16"}},
    {"fr::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"fr::pBookMarkStrings", {"[18]STR16", "[17]STR16"}},
    {"fr::szUDBGenArmorStatsExplanationsTooltipText", {"[5]STR16", "[3]STR16"}},
    {"fr::szUDBAdvStatsExplanationsTooltipText", {"[68]STR16", "[67]STR16"}},
    {"fr::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"fr::szMercCompareWebSite", {"[31]STR16", "[30]STR16"}},
    {"fr::szMercCompareEventText", {"[40]STR16", "[39]STR16"}},
    {"fr::szIndividualMilitiaBattleReportText", {"[12]STR16", "[11]STR16"}},
    {"fr::TacticalStr", {"[213][80]CHAR16", "[212][80]CHAR16"}},
    {"it::WeaponType", {"[16001][30]CHAR16", "[9][30]CHAR16"}},
    {"it::Message", {"[91][255]CHAR16", "[88][255]CHAR16"}},
    {"it::pWebPagesTitles", {"[60]STR16", "[57]STR16"}},
    {"it::pBookMarkStrings", {"[18]STR16", "[15]STR16"}},
    {"it::zMarksMapScreenText", {"[28]STR16", "[27]STR16"}},
    {"it::gzGIOScreenText", {"[69]STR16", "[70]STR16"}},
    {"it::gzLaptopHelpText", {"[27]STR16", "[26]STR16"}},
    {"it::szMercCompareWebSite", {"[31]STR16", "[30]STR16"}},
    {"it::szMercCompareEventText", {"[40]STR16", "[39]STR16"}},
    {"it::szIndividualMilitiaBattleReportText", {"[12]STR16", "[11]STR16"}},
    {"it::TacticalStr", {"[213][80]CHAR16", "[211][80]CHAR16"}},
    {"zh::szNationalityTextAdjective", {"[113]STR16", "[112]STR16"}},
};
std::set<std::string> observedShapeDifferences;

// Original translations also contain incompatible printf signatures. Keep each
// indexed entry explicit, with its exact English and translated signatures.
// This baseline records debt; it does not claim the strings are safe to format.
const ShapeBaseline knownFormatDifferences = {
    {"de::pMilitiaConfirmStrings[6]", {"", "s,s,d"}},
    {"de::gzFacilityErrorMessage[23]", {"s,s", "s,s,s,s,s,s,s"}},
    {"de::gzFacilityErrorMessage[25]", {"s,s", "s"}},
    {"de::gzFacilityErrorMessage[26]", {"s,s,s", "s,s"}},
    {"de::gzFacilityErrorMessage[27]", {"s", "s,s,s"}},
    {"de::gzFacilityErrorMessage[29]", {"s,s,s", "s,s"}},
    {"de::gzFacilityErrorMessage[30]", {"s,s", "s"}},
    {"de::gzFacilityErrorMessage[31]", {"s,s", "s,d"}},
    {"de::pMessageStrings[75]", {"s,s,d,s", "s,d,s,s"}},
    {"de::MPClientMessage[1]", {"S", ""}},
    {"de::gzIMPMajorTraitsHelpTextsRanger[4]", {"d", ""}},
    {"de::gzIMPMajorTraitsHelpTextsRanger[8]", {"d", ""}},
    {"de::gzIMPMajorTraitsHelpTextsGunslinger[8]", {"d", ""}},
    {"de::gzIMPMinorTraitsHelpTextsThrowing[8]", {"d", ""}},
    {"de::gzIMPMinorTraitsHelpTextsTeaching[3]", {"d", "zu"}},
    {"de::TacticalStr[74]", {"d,d,d,d", ""}},
    {"ru::szUDBAdvStatsTooltipText[65]", {"", "<incomplete>"}},
    {"pl::zNewTacticalMessages[19]", {"S,S", "S"}},
    {"pl::pMapErrorString[15]", {"s", ""}},
    {"pl::pMilitiaConfirmStrings[9]", {"s,d", "s,d,lo"}},
    {"pl::gzFacilityAssignmentStrings[4]", {"", "s"}},
    {"pl::gzFacilityAssignmentStrings[5]", {"s", ""}},
    {"pl::MPClientMessage[62]", {"S", "s"}},
    {"pl::gzIMPMajorTraitsHelpTextsAutoWeapons[6]", {"", "d,s"}},
    {"fr::pDisplaySelectionWindowGraphicalInformationText[0]", {"S,d,s,d,S", "S,d,s,S"}},
    {"fr::pDisplaySelectionWindowGraphicalInformationText[1]", {"S,d,d,S", "S,d,S"}},
    {"fr::pSkillTraitBeginIMPStrings[2]", {"d", ""}},
    {"fr::pSkillTraitBeginIMPStrings[3]", {"d,d,d", ""}},
    {"fr::szCovertTextStr[29]", {"s", ""}},
    {"fr::zSaveLoadText[14]", {"", "d"}},
    {"fr::MPClientMessage[74]", {"S,S,S", ""}},
    {"fr::gzIMPMajorTraitsHelpTextsRanger[4]", {"d", ""}},
    {"fr::gzIMPMajorTraitsHelpTextsGunslinger[8]", {"d", ""}},
    {"fr::TacticalStr[28]", {"s,d", "d,s"}},
    {"fr::TacticalStr[45]", {"s,d", "d,s"}},
    {"it::Message[67]", {"", "s"}},
    {"it::Message[68]", {"", "s"}},
    {"it::Message[69]", {"", "s"}},
    {"it::Message[77]", {"s", ""}},
    {"it::Message[78]", {"s", "s,s"}},
    {"it::Message[79]", {"s", "s,s,s"}},
    {"it::Message[80]", {"", "s,s"}},
    {"it::Message[81]", {"s,s", "s,s,s"}},
    {"it::Message[82]", {"s,s,s", ""}},
    {"it::Message[83]", {"s,s", ""}},
    {"it::Message[84]", {"s,s,s", "s,s"}},
    {"it::Message[85]", {"", "s,s"}},
    {"it::Message[86]", {"", "s,s"}},
    {"it::TacticalStr[28]", {"s,d", "d,s"}},
    {"it::TacticalStr[45]", {"s,d", "d,s"}},
    {"it::TacticalStr[112]", {"", "s"}},
    {"it::TacticalStr[113]", {"s", ""}},
    {"it::TacticalStr[131]", {"", "s"}},
    {"it::TacticalStr[132]", {"s", ""}},
    {"it::TacticalStr[133]", {"", "s"}},
    {"it::TacticalStr[134]", {"s", ""}},
    {"it::TacticalStr[136]", {"", "s"}},
    {"it::TacticalStr[137]", {"s", ""}},
    {"it::TacticalStr[141]", {"", "s"}},
    {"it::TacticalStr[143]", {"s", ""}},
    {"it::TacticalStr[146]", {"", "d,d,d,d"}},
    {"it::TacticalStr[147]", {"d,d,d,d", "s,s"}},
    {"it::TacticalStr[148]", {"s,s", ""}},
    {"it::TacticalStr[155]", {"", "d,d,d,d,s,d,s,d,s"}},
    {"it::TacticalStr[156]", {"d,d,d,d,s,d,s,d,s", ""}},
    {"it::TacticalStr[171]", {"", "s,s"}},
    {"it::TacticalStr[172]", {"s,s", ""}},
    {"it::TacticalStr[174]", {"", "d"}},
    {"it::TacticalStr[175]", {"d", ""}},
    {"it::TacticalStr[208]", {"", "s,s"}},
    {"zh::gzLateLocalizedString[58]", {"d,d", "d,\\uff05,d,\\uff05"}},
    {"zh::sSpecialCharacters[0]", {"<incomplete>", ""}},
    {"zh::gzMapInventorySortingMessage[5]", {"d,s", "s,d"}},
    {"zh::BobbyRText[32]", {"", "\\uff05"}},
    {"zh::gpStrategicString[69]", {"d,s", "s,d"}},
    {"zh::szBackgroundText_Value[0]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[1]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[2]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[3]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[4]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[5]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[6]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[7]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[8]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[9]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[10]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[11]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[12]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[13]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[14]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[15]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[16]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[17]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[18]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[19]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[20]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[21]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[22]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[23]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[24]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[25]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[26]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[27]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[28]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[29]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[30]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[31]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[32]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[33]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[34]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[35]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[36]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[37]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[38]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[39]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[40]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[41]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[42]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[43]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[44]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[45]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[47]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[48]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[49]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[50]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[51]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[55]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[56]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[57]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[58]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[59]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[60]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[61]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[62]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[63]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[64]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[65]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[67]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[68]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[72]", {"s,d", "s,d,\\uff05"}},
    {"zh::szBackgroundText_Value[73]", {"s,d", "s,d,\\uff05"}},
    {"zh::szCampaignHistoryDetail[22]", {"d,s,d", "s,d,d"}},
    {"zh::szCampaignStatsOperationPrefix[10]", {"s", "<incomplete>"}},
    {"zh::gzIMPMinorTraitsHelpTextsScouting[0]", {"d", "d,s"}},
    {"zh::gzIMPMinorTraitsHelpTextsScouting[1]", {"d", "d,s"}},
    {"zh::gzIMPMinorTraitsHelpTextsScouting[2]", {"d", "d,s"}},
};
std::set<std::string> observedFormatDifferences;

void checkLanguage(const char* code, const Tables& english,
                   const Tables& translation)
{
    require(translation.size() == english.size(),
            std::string(code) + ": table count differs from English");
    const std::size_t common = std::min(translation.size(), english.size());
    for (std::size_t index = 0; index < common; ++index)
    {
        const Table& expected = english[index];
        const Table& actual = translation[index];
        const std::string tableLabel = std::string(code) + "::" + actual.name;
        require(actual.name == expected.name, tableLabel + ": table order differs");
        if (actual.name != expected.name)
            continue;
        if (actual.shape != expected.shape)
        {
            observedShapeDifferences.insert(tableLabel);
            const auto baseline = knownShapeDifferences.find(tableLabel);
            require(baseline != knownShapeDifferences.end() &&
                        baseline->second.first == expected.shape &&
                        baseline->second.second == actual.shape,
                    tableLabel + ": shape " + actual.shape +
                        " differs from English " + expected.shape);
        }

        // Match indexed names within this table. A short or multidimensional
        // table must not move the checks for any subsequent table or row.
        std::map<std::string, const CHAR16*> expectedEntries;
        for (const Entry& entry : expected.entries)
            expectedEntries.emplace(entry.name, entry.text);
        for (const Entry& entry : actual.entries)
        {
            const std::string label = std::string(code) + "::" + entry.name;
            require(entry.text != nullptr, label + ": null entry");
            const auto source = expectedEntries.find(entry.name);
            if (source == expectedEntries.end() || !source->second ||
                !entry.text || isPlainText(entry.name))
                continue;
            const Signature expectedArguments = formatArguments(source->second);
            const Signature actualArguments = formatArguments(entry.text);
            if (actualArguments != expectedArguments)
            {
                observedFormatDifferences.insert(label);
                const auto baseline = knownFormatDifferences.find(label);
                const std::string expectedSignature = signatureText(expectedArguments);
                const std::string actualSignature = signatureText(actualArguments);
                require(baseline != knownFormatDifferences.end() &&
                            baseline->second.first == expectedSignature &&
                            baseline->second.second == actualSignature,
                        label + ": format signature " + actualSignature +
                            " differs from English " + expectedSignature);
            }
            else
            {
                require(true, label + ": compatible format signature");
            }
        }
    }
}

void checkScanner()
{
    require(formatArguments(L"%%") == Signature{},
            "escaped percent must not consume an argument");
    require(formatArguments(L"%i %d") == Signature{L"d", L"d"},
            "i and d conversions must compare as equivalent");
    require(formatArguments(L"%*.*ls") == Signature{L"*", L"*", L"ls"},
            "width, precision, and length arguments must retain order");
    require(formatArguments(L"%ld") != formatArguments(L"%d"),
            "integer length drift must be detected");
    require(formatArguments(L"%ls") != formatArguments(L"%hs"),
            "wide and narrow string drift must be detected");
    require(formatArguments(L"%d %ls") != formatArguments(L"%ls %d"),
            "argument reordering must be detected");
    require(signatureText(Signature{L"\uff05"}) == "\\uff05" &&
                signatureText(Signature{L"\uff05"}) !=
                    signatureText(Signature{L"\x0005"}),
            "non-ASCII format tokens must retain their complete code point");
}
}

int main()
{
    checkScanner();
    const Tables english = en_entries();
    checkLanguage("en", english, english);
    checkLanguage("de", english, de_entries());
    checkLanguage("ru", english, ru_entries());
    checkLanguage("nl", english, nl_entries());
    checkLanguage("pl", english, pl_entries());
    checkLanguage("fr", english, fr_entries());
    checkLanguage("it", english, it_entries());
    checkLanguage("zh", english, zh_entries());
    for (const auto& baseline : knownShapeDifferences)
        require(observedShapeDifferences.count(baseline.first) != 0,
                baseline.first + ": obsolete shape baseline entry");
    for (const auto& baseline : knownFormatDifferences)
        require(observedFormatDifferences.count(baseline.first) != 0,
                baseline.first + ": obsolete format baseline entry");
    if (failures)
    {
        if (failures > diagnosticLimit)
            std::cerr << (failures - diagnosticLimit)
                      << " more translation failures omitted\n";
        std::cerr << "FAIL " << failures << " of " << checks
                  << " translation checks\n";
        return 1;
    }
    std::cout << "PASS " << checks << " translation checks across "
              << english.size() << " English tables; "
              << observedShapeDifferences.size() << " known shape differences; "
              << observedFormatDifferences.size() << " known format differences\n";
    return 0;
}
