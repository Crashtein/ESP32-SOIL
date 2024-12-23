#ifndef MENU_MANAGER_H
#define MENU_MANAGER_H

#include <functional>
#include <string>
#include <vector>
#include <memory>

// Typy callbacków
using MenuCallback = std::function<void()>;
using MenuNavigateCallback = std::function<void(const char*)>;

// Struktura reprezentująca pole menu
struct MenuItem {
    std::string name;                       // Nazwa pola menu
    MenuCallback callback;                  // Callback wywoływany po wybraniu opcji
    MenuNavigateCallback onNavigateCallback; // Callback wywoływany przy nawigacji
    bool checked;                           // Informacja, czy opcja została zaznaczona
    std::vector<std::shared_ptr<MenuItem>> subMenu; // Podmenu (kolejny poziom)

    MenuItem(const std::string& name,
             MenuCallback callback = nullptr,
             MenuNavigateCallback onNavigate = nullptr,
             bool checked = false)
        : name(name), callback(callback), onNavigateCallback(onNavigate), checked(checked), subMenu() {}
};

class MenuManager {
public:
    // Singleton - uzyskanie instancji
    static MenuManager& getInstance();

    // Ustawienie początkowego menu głównego
    void setMainMenu(const std::vector<std::shared_ptr<MenuItem>>& menu);

    // Nawigacja w menu
    void navigateNext();
    void navigatePrevious();
    void navigateInto();
    void navigateBack();

    // Wywołanie aktualnego elementu
    void selectCurrent();

    // Pobieranie informacji o aktualnym elemencie
    std::string getCurrentItemName() const;

    // Pobieranie pełnej ścieżki do aktualnej pozycji
    std::string getCurrentPath() const;

    // Wypisywanie struktury menu ze stanem zaznaczenia
    void printMenu() const;

    // Funkcja zwracająca numer elementu w głównym menu
    size_t getCurrentItemIndex() const;

    // Funkcja zwracająca numer elementu w podmenu
    std::pair<size_t, size_t> getCurrentItemIndexInSubmenu() const;

private:
    MenuManager(); // Prywatny konstruktor singletonu
    MenuManager(const MenuManager&) = delete;
    MenuManager& operator=(const MenuManager&) = delete;

    // Funkcja pomocnicza do wywołania callbacku nawigacyjnego
    void triggerNavigateCallback();

    // Funkcja pomocnicza do rekurencyjnego wypisania struktury menu
    void printMenuRecursive(const std::vector<std::shared_ptr<MenuItem>>& menu, int depth) const;

    // Dane wewnętrzne
    std::vector<std::shared_ptr<MenuItem>> mainMenu;   // Główne menu
    std::vector<std::shared_ptr<MenuItem>>* currentMenu; // Wskaźnik na aktualne menu
    size_t currentIndex;                              // Aktualny indeks w menu

    std::vector<std::vector<std::shared_ptr<MenuItem>>*> menuStack; // Stos z poprzednimi poziomami
    std::vector<size_t> indexStack;                                // Stos indeksów w menu nadrzędnym
};

#endif // MENU_MANAGER_H
