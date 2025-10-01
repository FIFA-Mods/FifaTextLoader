#include "plugin.h"
#include "TextFileTable.h"
#include <map>

using namespace plugin;
using namespace std;
using namespace filesystem;

uintptr_t gSetLanguage = 0, gGetString = 0, gStringAssign = 0, gEnforceCase = 0, gGetHash = 0;

map<unsigned int, string>& GetTranslationEntries() {
    static map<unsigned int, string> entries;
    return entries;
}

void LoadTranslationEntries(char const *languageCode) {
    GetTranslationEntries().clear();
    for (const auto& p : recursive_directory_iterator(L"plugins\\")) {
        if (ToLower(p.path().extension().string()) == ".tr") {
            TextFileTable file;
            if (file.Read(p.path(), L'\t')) {
                bool canAddEntries = true;
                for (size_t r = 0; r < file.NumRows(); r++) {
                    auto const &row = file.Row(r);
                    auto numColumns = row.size();
                    if (numColumns >= 1) {
                        auto& col1 = row[0];
                        if (!col1.empty() && col1[0] != ';') {
                            if (col1[0] == '[') {
                                auto bcPos = col1.find(']', 1);
                                if (bcPos != std::string::npos) {
                                    std::string currTranslationLanguage = col1.substr(1, bcPos - 1);
                                    Trim(currTranslationLanguage);
                                    if (currTranslationLanguage.empty())
                                        canAddEntries = true;
                                    else {
                                        canAddEntries = false;
                                        currTranslationLanguage = ToLower(currTranslationLanguage);
                                        auto languages = Split(currTranslationLanguage, ',', true, true, false);
                                        for (auto const& l : languages) {
                                            if (l == languageCode) {
                                                canAddEntries = true;
                                                break;
                                            }
                                        }
                                    }
                                    continue;
                                }
                            }
                            if (canAddEntries) {
                                auto hash = CallAndReturnDynGlobal<unsigned int>(gGetHash, col1.c_str());
                                GetTranslationEntries()[hash] = (numColumns >= 2) ? row[1] : string();
                            }
                        }
                    }
                }
            }
        }
    }
}

int METHOD LocalizationInterface_SetLanguage(void *t, DUMMY_ARG, const char *languageCode, const char *countryCode) {
    int result = CallMethodAndReturnDynGlobal<int>(gSetLanguage, t, languageCode, countryCode);
    LoadTranslationEntries(languageCode);
    return result;
}

int METHOD LocalizationInterface_GetString(void *t, DUMMY_ARG, void *pDstStringUTF8, unsigned int uKey, unsigned int eCase) {
    auto it = GetTranslationEntries().find(uKey);
    if (it != GetTranslationEntries().end()) {
        CallMethodDynGlobal(gStringAssign, pDstStringUTF8, (*it).second.c_str());
        CallMethodDynGlobal(gEnforceCase, t, pDstStringUTF8, eCase);
        return 1;
    }
    return CallMethodAndReturnDynGlobal<int>(gGetString, t, pDstStringUTF8, uKey, eCase);
}

class FifaTextLoader {
public:
    FifaTextLoader() {
        if (!CheckPluginName(Magic<'T','e','x','t','L','o','a','d','e','r','.','a','s','i'>()))
            return;
        auto v = FIFA::GetAppVersion();
        switch (v.id()) {
        case ID_FIFA13_1700_RLD:
            gGetString = patch::RedirectCall(0x1BA3B9D, LocalizationInterface_GetString);
            gSetLanguage = patch::RedirectCall(0x60D972, LocalizationInterface_SetLanguage);
            gStringAssign = 0x493420;
            gEnforceCase = 0x1BA33C0;
            gGetHash = 0x484380;
            break;
        case ID_FIFA13_1800:
            gGetString = patch::RedirectCall(0x1B9F2CD, LocalizationInterface_GetString);
            gSetLanguage = patch::RedirectCall(0x6089F2, LocalizationInterface_SetLanguage);
            gStringAssign = 0x48E480;
            gEnforceCase = 0x1B9EAF0;
            gGetHash = 0x47F3D0;
            break;
        }
    }
} fifaTextLoader;
