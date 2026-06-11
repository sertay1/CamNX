#pragma once
#include <string>
#include <vector>

enum class Language {
    ENGLISH,
    TURKISH,
    SPANISH,
    FRENCH,
    GERMAN,
    ITALIAN,
    PORTUGUESE,
    RUSSIAN,
    JAPANESE,
    KOREAN,
    NUM_LANGUAGES
};

struct Translation {
    std::string appName;
    std::string menuTitle;
    std::string filterMode;
    std::string filterBW;
    std::string filterNight;
    std::string filterThermal;
    std::string filterMatrix;
    std::string filterBarbie;
    std::string filterSepia;
    std::string filterCyberpunk;
    
    std::string navCamera;
    std::string navGallery;
    std::string navShare;
    std::string navLanguage;
    
    std::string ctrlPhoto;
    std::string ctrlFilter;
    std::string ctrlExit;
    std::string ctrlZL_ZR;
    std::string ctrlResolution;
    std::string ctrlTitle; // "CONTROLS"
    
    std::string galleryTitle;
    std::string galleryEmpty;
    std::string galleryControls; // "[Y] Delete  [A] Select/Fullscreen"
    std::string galleryDelete; // "Delete"
    std::string galleryBack; // "Back"
    std::string deletePrompt;
    std::string deleteYes;
    std::string deleteNo;
    
    std::string ctrlGallerySelect;
    std::string shareTitle;
    std::string shareDesc;
    std::string shareBack;
    std::string shareHint;
    std::string shareRefresh;
    
    std::string selectLang;
    std::string langHint;
};

class I18N {
public:
    static void         init();
    static void         setLanguage(Language lang);
    static Language     getLanguage();
    static const Translation& get();
    static std::string  getLanguageName(Language lang);
    static std::string  getNativeLanguageName(Language lang);

private:
    static Language currentLang;
    static std::vector<Translation> texts;
};
