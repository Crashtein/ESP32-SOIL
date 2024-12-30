#include "MenuManager.h"
#include <iostream>

MenuManager::MenuManager() : currentMenu(nullptr), currentIndex(0) {}

MenuManager &MenuManager::getInstance()
{
    static MenuManager instance;
    return instance;
}

void MenuManager::setMainMenu(const std::vector<std::shared_ptr<MenuItem>> &menu)
{
    mainMenu = menu;
    currentMenu = &mainMenu;
    currentIndex = 0;
    menuStack.clear();
    indexStack.clear();
    triggerNavigateCallback(); // Wywołaj callback dla pierwszego elementu
}

void MenuManager::navigateNext()
{
    if (currentMenu && !currentMenu->empty())
    {
        currentIndex = (currentIndex + 1) % currentMenu->size();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigatePrevious()
{
    if (currentMenu && !currentMenu->empty())
    {
        currentIndex = (currentIndex + currentMenu->size() - 1) % currentMenu->size();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigateInto()
{
    if (currentMenu && !(*currentMenu)[currentIndex]->subMenu.empty())
    {
        menuStack.push_back(currentMenu);
        indexStack.push_back(currentIndex);
        currentMenu = &(*currentMenu)[currentIndex]->subMenu;
        currentIndex = 0;
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::navigateBack()
{
    if (!menuStack.empty())
    {
        currentMenu = menuStack.back();
        currentIndex = indexStack.back();
        menuStack.pop_back();
        indexStack.pop_back();
        triggerNavigateCallback(); // Wywołaj callback nawigacyjny
    }
}

void MenuManager::selectCurrent()
{
    if (currentMenu && currentIndex < currentMenu->size())
    {
        auto &currentItem = (*currentMenu)[currentIndex];
        if (currentItem->checkable)
        {
            currentItem->checked = !currentItem->checked;
            triggerNavigateCallback();
        }
        if (currentItem->callback)
        {
            currentItem->callback();
        }
    }
}

String MenuManager::getCurrentItemName() const
{
    if (currentMenu && currentIndex < currentMenu->size())
    {
        return (*currentMenu)[currentIndex]->name;
    }
    return "";
}

String MenuManager::getCurrentPath() const
{
    if (!currentMenu)
        return "Menu not initialized";

    String path;
    for (size_t i = 0; i < menuStack.size(); ++i)
    {
        auto *subMenuStack = menuStack[i];
        if (indexStack[i] < subMenuStack->size())
        {
            path += subMenuStack->at(indexStack[i])->name + " > ";
        }
    }
    if (currentIndex < currentMenu->size())
    {
        path += currentMenu->at(currentIndex)->name;
    }
    else
    {
        path += "(Invalid Index)";
    }

    return path;
}

void MenuManager::triggerNavigateCallback()
{
    if (currentMenu && currentIndex < currentMenu->size() && (*currentMenu)[currentIndex]->onNavigateCallback)
    {
        // Wywołaj callback nawigacyjny dla bieżącego pola menu
        (*currentMenu)[currentIndex]->onNavigateCallback(currentIndex, getMenuItemsNamesAtParentLevel(), getMenuItemsCheckedAtParentLevel());
    }
}

void MenuManager::printMenu() const
{
    if (currentMenu)
    {
        outputDebugln("Menu structure:");
        printMenuRecursive(mainMenu, 0);
    }
    else
    {
        outputDebugln("Menu not initialized!");
    }
}

void MenuManager::printMenuRecursive(const std::vector<std::shared_ptr<MenuItem>> &menu, int depth) const
{
    int index = 0;
    for (const auto &item : menu)
    {
        outputDebugf("level: %d, index: %d , name: %s, checkable: %d, checked: %d", depth, index, item->name.c_str(), item->checkable, item->checked);
        outputDebug('\n');
        if (!item->subMenu.empty())
        {
            printMenuRecursive(item->subMenu, depth + 1);
        }
        index++;
    }
}

size_t MenuManager::getCurrentItemMainMenuIndex() const
{
    if (indexStack.size())
    {
        return indexStack[0];
    }
    return -1; // Wartość -1 oznacza brak zainicjalizowanego menu
}

size_t MenuManager::getCurrentItemIndex() const
{
    if (currentMenu)
    {
        return currentIndex;
    }
    return -1; // Wartość -1 oznacza brak zainicjalizowanego menu
}

std::pair<size_t, size_t> MenuManager::getCurrentItemIndexInSubmenu() const
{
    if (currentMenu)
    {
        if (menuStack.empty())
        {
            // Jeśli nie ma podmenu, zwracamy tylko indeks w głównym menu
            return {indexStack.back(), currentIndex};
        }
        else
        {
            // Jeśli jesteśmy w podmenu, zwracamy zarówno indeks w menu nadrzędnym, jak i w bieżącym podmenu
            return {indexStack.back(), currentIndex};
        }
    }
    return {-1, -1}; // Wartość -1 oznacza brak zainicjalizowanego menu
}

std::vector<String> MenuManager::getMenuItemsNamesAtParentLevel() const
{
    std::vector<String> items;
    for (const auto &item : *currentMenu)
    {
        items.push_back(item->name);
    }
    return items;
}

std::vector<bool> MenuManager::getMenuItemsCheckedAtParentLevel() const
{
    std::vector<bool> items;
    for (const auto &item : *currentMenu)
    {
        items.push_back(item->checked);
    }
    return items;
}

size_t MenuManager::getDepth() const
{
    return indexStack.size();
}