#include "i18n.hpp"

Language I18N::currentLang = Language::ENGLISH;

std::vector<Translation> I18N::texts = {
    // ── ENGLISH ──────────────────────────────────────────────────────────────
    { "CamNX", "MENU", "Filter Mode", "Black & White", "Night Vision", "Thermal", "Matrix", "Barbie", "Sepia", "Cyberpunk",
      "Camera", "Gallery", "Photo Share", "Language",
      "[A] Take Photo/Select", "[L/R] Change Filter", "[-] Exit", "[ZL/ZR] Switch Menu", "Resolution", "CONTROLS",
      "GALLERY", "No photos found", "[Y] Delete  [A] Select/Fullscreen", "Delete", "Back",
      "Delete this photo?", "Yes", "No", "",
      "PHOTO SHARE", "Connect to the address below or QR:", "",
      "",
      "[X] Refresh", "Language", ""
    },
    // ── TURKISH ───────────────────────────────────────────────────────────────
    { "CamNX", "MENÜ", "Filtre Modu", "Siyah Beyaz", "Gece Görüşü", "Termal", "Matrix", "Barbie", "Sepya", "Cyberpunk",
      "Kamera", "Galeri", "Fotoğraf Paylaşımı", "Dil",
      "[A] Fotoğraf Çek/Seç", "[L/R] Filtre Değiştirme", "[-] Çıkış", "[ZL/ZR] Menü Geçişi", "Çözünürlük", "Kontroller",
      "GALERİ", "Fotoğraf Bulunamadı", "[Y] Sil  [A] Seç/Büyült", "Sil", "Geri",
      "Bu fotoğraf silinsin mi?", "Evet", "Hayır", "",
      "FOTOĞRAF PAYLAŞIMI", "Aşağıdaki veya QR'daki adrese bağlanın", "",
      "",
      "[X] Yenile", "Dil", ""
    },
    // ── SPANISH ───────────────────────────────────────────────────────────────
    { "CamNX", "MENÚ", "Modo Filtro", "Blanco y Negro", "Visión Nocturna", "Térmico", "Matrix", "Barbie", "Sepia", "Cyberpunk",
      "Cámara", "Galería", "Compartir Fotos", "Idioma",
      "[A] Tomar Foto/Seleccionar", "[L/R] Cambiar Filtro", "[-] Salir", "[ZL/ZR] Cambiar Menú", "Resolución", "CONTROLES",
      "GALERÍA", "Foto no encontrada", "[Y] Borrar  [A] Selec/Pantalla Comp", "Borrar", "Atrás",
      "¿Borrar esta foto?", "Sí", "No", "",
      "COMPARTIR FOTOS", "Conéctese a la dirección a continuación:", "",
      "",
      "[X] Actualizar", "Idioma", ""
    },
    // ── FRENCH ────────────────────────────────────────────────────────────────
    { "CamNX", "MENU", "Mode Filtre", "Noir et Blanc", "Vision Nocturne", "Thermique", "Matrix", "Barbie", "Sépia", "Cyberpunk",
      "Caméra", "Galerie", "Partage Photo", "Langue",
      "[A] Prendre Photo/Sélection", "[L/R] Changer Filtre", "[-] Quitter", "[ZL/ZR] Changer Menu", "Résolution", "CONTRÔLES",
      "GALERIE", "Aucune photo", "[Y] Supprimer  [A] Sélection/Plein Écran", "Supprimer", "Retour",
      "Supprimer la photo?", "Oui", "Non", "",
      "PARTAGE PHOTO", "Connectez-vous à cette adresse:", "",
      "",
      "[X] Actualiser", "Langue", ""
    },
    // ── GERMAN ────────────────────────────────────────────────────────────────
    { "CamNX", "MENÜ", "Filtermodus", "Schwarz-Weiß", "Nachtsicht", "Wärmebild", "Matrix", "Barbie", "Sepia", "Cyberpunk",
      "Kamera", "Galerie", "Foto-Freigabe", "Sprache",
      "[A] Foto aufnehmen/Auswahl", "[L/R] Filter ändern", "[-] Beenden", "[ZL/ZR] Menü wechseln", "Auflösung", "STEUERUNG",
      "GALERIE", "Keine Fotos", "[Y] Löschen  [A] Wählen/Vollbild", "Löschen", "Zurück",
      "Foto löschen?", "Ja", "Nein", "",
      "FOTO-FREIGABE", "Verbinden Sie sich mit dieser Adresse:", "",
      "",
      "[X] Aktualisieren", "Sprache", ""
    },
    // ── ITALIAN ───────────────────────────────────────────────────────────────
    { "CamNX", "MENU", "Modo Filtro", "Bianco e Nero", "Visione Notturna", "Termico", "Matrix", "Barbie", "Seppia", "Cyberpunk",
      "Fotocamera", "Galleria", "Condivisione Foto", "Lingua",
      "[A] Scatta Foto/Seleziona", "[L/R] Cambia Filtro", "[-] Esci", "[ZL/ZR] Cambia Menu", "Risoluzione", "CONTROLLI",
      "GALLERIA", "Nessuna foto", "[Y] Elimina  [A] Seleziona/Schermo Int", "Elimina", "Indietro",
      "Eliminare foto?", "Sì", "No", "",
      "CONDIVISIONE FOTO", "Connettiti a questo indirizzo:", "",
      "",
      "[X] Aggiorna", "Lingua", ""
    },
    // ── PORTUGUESE ────────────────────────────────────────────────────────────
    { "CamNX", "MENU", "Modo Filtro", "Preto e Branco", "Visão Noturna", "Térmico", "Matrix", "Barbie", "Sépia", "Cyberpunk",
      "Câmera", "Galeria", "Partilha de Fotos", "Idioma",
      "[A] Tirar Foto/Selecionar", "[L/R] Mudar Filtro", "[-] Sair", "[ZL/ZR] Mudar Menu", "Resolução", "CONTROLES",
      "GALERIA", "Sem fotos", "[Y] Apagar  [A] Selec/Tela Cheia", "Apagar", "Voltar",
      "Apagar foto?", "Sim", "Não", "",
      "PARTILHA DE FOTOS", "Ligue-se a este endereço:", "",
      "",
      "[X] Atualizar", "Idioma", ""
    },
    // ── RUSSIAN ───────────────────────────────────────────────────────────────
    { "CamNX", "МЕНЮ", "Режим Фильтра", "Черно-Белый", "Ночное Видение", "Тепловизор", "Матрица", "Барби", "Сепия", "Киберпанк",
      "Камера", "Галерея", "Обмен Фото", "Язык",
      "[A] Сделать фото/Выбрать", "[L/R] Изменить фильтр", "[-] Выход", "[ZL/ZR] Переключить меню", "Разрешение", "УПРАВЛЕНИЕ",
      "ГАЛЕРЕЯ", "Нет фото", "[Y] Удалить  [A] Выбрать/Полный экран", "Удалить", "Назад",
      "Удалить фото?", "Да", "Нет", "",
      "ОБМЕН ФОТО", "Откройте этот адрес в браузере:", "",
      "",
      "[X] Обновить", "Язык", ""
    },
    // ── JAPANESE ─────────────────────────────────────────────────────────────
    { "CamNX", "メニュー", "フィルター", "白黒", "暗視", "サーマル", "マトリックス", "バービー", "セピア", "サイバーパンク",
      "カメラ", "ギャラリー", "写真共有", "言語",
      "[A] 写真を撮る/選択", "[L/R] フィルター変更", "[-] 終了", "[ZL/ZR] メニュー切り替え", "解像度", "コントロール",
      "ギャラリー", "写真がありません", "[Y] 削除  [A] 選択/全画面", "削除", "戻る",
      "この写真を削除しますか？", "はい", "いいえ", "",
      "写真共有", "ブラウザでこのアドレスを開いてください:", "",
      "",
      "[X] 更新", "言語", ""
    },
    // ── KOREAN ───────────────────────────────────────────────────────────────
    { "CamNX", "메뉴", "필터", "흑백", "야간 투시경", "열화상", "매트릭스", "바비", "세피아", "사이버펑크",
      "카메라", "갤러리", "사진 공유", "언어",
      "[A] 사진 찍기/선택", "[L/R] 필터 변경", "[-] 종료", "[ZL/ZR] 메뉴 전환", "해상도", "컨트롤",
      "갤러리", "사진 없음", "[Y] 삭제  [A] 선택/전체 화면", "삭제", "뒤로가기",
      "사진을 삭제하시겠습니까?", "예", "아니요", "",
      "사진 공유", "브라우저에서 이 주소를 여십시오:", "",
      "",
      "[X] 새로고침", "언어", ""
    }
};

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

void I18N::init() {
    FILE* f = fopen("sdmc:/switch/CamNX/lang.txt", "r");
    if (f) {
        int l = 0;
        if (fscanf(f, "%d", &l) == 1) {
            if (l >= 0 && l < (int)Language::NUM_LANGUAGES) {
                currentLang = (Language)l;
            }
        }
        fclose(f);
    }
}

void I18N::setLanguage(Language lang) {
    if (lang >= Language::ENGLISH && lang < Language::NUM_LANGUAGES) {
        currentLang = lang;
        
        // Ensure dir exists
        mkdir("sdmc:/switch", 0777);
        mkdir("sdmc:/switch/CamNX", 0777);
        
        FILE* f = fopen("sdmc:/switch/CamNX/lang.txt", "w");
        if (f) {
            fprintf(f, "%d", (int)lang);
            fclose(f);
        }
    }
}

Language I18N::getLanguage() { 
    return currentLang; 
}

const Translation& I18N::get() { 
    return texts[(int)currentLang]; 
}

std::string I18N::getLanguageName(Language lang) {
    switch (lang) {
        case Language::ENGLISH:    return "English";
        case Language::TURKISH:    return "Turkish";
        case Language::SPANISH:    return "Spanish";
        case Language::FRENCH:     return "French";
        case Language::GERMAN:     return "German";
        case Language::ITALIAN:    return "Italian";
        case Language::PORTUGUESE: return "Portuguese";
        case Language::RUSSIAN:    return "Russian";
        case Language::JAPANESE:   return "Japanese";
        case Language::KOREAN:     return "Korean";
        default:                   return "Unknown";
    }
}

std::string I18N::getNativeLanguageName(Language lang) {
    switch (lang) {
        case Language::ENGLISH:    return "English";
        case Language::TURKISH:    return "Türkçe";
        case Language::SPANISH:    return "Espanol";
        case Language::FRENCH:     return "Francais";
        case Language::GERMAN:     return "Deutsch";
        case Language::ITALIAN:    return "Italiano";
        case Language::PORTUGUESE: return "Portugues";
        case Language::RUSSIAN:    return "Russian";
        case Language::JAPANESE:   return "Japanese";
        case Language::KOREAN:     return "Korean";
        default:                   return "Unknown";
    }
}
