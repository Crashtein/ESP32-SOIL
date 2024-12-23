#include "MenuManager.h"
#include <iostream>

MenuManager::MenuManager() : currentMenu(nullptr), currentIndex(0) {}

MenuManager& MenuManager::getInstance() {
    static MenuManager instance;
    return instance;
}

void MenuManager::setMainMenu(const std::vector<std::shared_ptr<MenuItem>>& menu) {
    mainMenu = menu;
    currentMenu = &mainMenu;
    currentIndex = 0;
    menuStack.clear();
    indexStack.clear();
    triggerNavigateCallback(); // Wywołaj callback dla pierwszego elementu
}

void MenuManager::navigateNext() {
    if (currentMenu && !currentMenu->empty()) {
        currentIndex = (currentIndex + 1) % currentMenu->size();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigatePrevious() {
    if (currentMenu && !currentMenu->empty()) {
        currentIndex = (currentIndex + currentMenu->size() - 1) % currentMenu->size();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigateInto() {
    if (currentMenu && !(*currentMenu)[currentIndex]->subMenu.empty()) {
        menuStack.push_back(currentMenu);
        indexStack.push_back(currentIndex);
        currentMenu = &(*currentMenu)[currentIndex]->subMenu;
        currentIndex = 0;
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigateBack() {
    if (!menuStack.empty()) {
        currentMenu = menuStack.back();
        currentIndex = indexStack.back();
        menuStack.pop_back();
        indexStack.pop_back();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::selectCurrent() {
    if (currentMenu && currentIndex < currentMenu->size()) {
        auto& currentItem = (*currentMenu)[currentIndex];
        if (currentItem->callback) {
            currentItem->callback();
        }
        currentItem->checked = true; // Oznacz element jako wybrany
    }
}

std::string MenuManager::getCurrentItemName() const {
    if (currentMenu && currentIndex < currentMenu->size()) {
        return (*currentMenu)[currentIndex]->name;
    }
    return "";
}

std::string MenuManager::getCurrentPath() const {
    if (!currentMenu) return "Menu not initialized";

    std::string path;
    for (size_t i = 0; i < menuStack.size(); ++i) {
        auto *subMenuStack = menuStack[i];
        if (indexStack[i] < subMenuStack->size()) {
            path += subMenuStack->at(indexStack[i])->name + " > ";
        }
    }
    if (currentIndex < currentMenu->size()) {
        path += currentMenu->at(currentIndex)->name;
    } else {
        path += "(Invalid Index)";
    }

    return path;
}

void MenuManager::triggerNavigateCallback() {
    if (currentMenu && currentIndex < currentMenu->size() && (*currentMenu)[currentIndex]->onNavigateCallback) {
        // Wywołaj callback nawigacyjny dla bieżącego pola menu
        (*currentMenu)[currentIndex]->onNavigateCallback((*currentMenu)[currentIndex]->name.c_str());
    }
}

void MenuManager::printMenu() const {
    if (currentMenu) {
        std::cout << "Menu structure:\n";
        printMenuRecursive(mainMenu, 0);
    } else {
        std::cout << "Menu not initialized!\n";
    }
}

void MenuManager::printMenuRecursive(const std::vector<std::shared_ptr<MenuItem>>& menu, int depth) const {
    for (const auto& item : menu) {
        std::string prefix(depth * 2, ' '); // Indentacja
        std::cout << prefix << "- " << item->name;
        if (item->checked) {
            std::cout << " [Checked]";
        }
        std::cout << '\n';
        if (!item->subMenu.empty()) {
            printMenuRecursive(item->subMenu, depth + 1);
        }
    }
}

size_t MenuManager::getCurrentItemIndex() const {
    if (currentMenu) {
        return currentIndex;
    }
    return -1; // Wartość -1 oznacza brak zainicjalizowanego menu
}

std::pair<size_t, size_t> MenuManager::getCurrentItemIndexInSubmenu() const {
    if (currentMenu) {
        if (menuStack.empty()) {
            // Jeśli nie ma podmenu, zwracamy tylko indeks w głównym menu
            return {indexStack.back(), currentIndex};
        } else {
            // Jeśli jesteśmy w podmenu, zwracamy zarówno indeks w menu nadrzędnym, jak i w bieżącym podmenu
            return {indexStack.back(), currentIndex};
        }
    }
    return {-1, -1}; // Wartość -1 oznacza brak zainicjalizowanego menu
}
